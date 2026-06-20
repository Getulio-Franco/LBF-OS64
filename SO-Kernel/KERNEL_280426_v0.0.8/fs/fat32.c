/**
 * ============================================================================
 * FAT32 CORE - INITIALIZATION AND GEOMETRY (V3.6 - MULTI-DEVICE ABSTRACTED)
 * ============================================================================
 */

#include "fs/fat32.h"
#include "drivers/video.h"
#include "util/string.h"
#include "../mem/heap.h"

// --- NOVA ABSTRAÇÃO ---
// Declaração da nossa função intermediária (pode vir de um storage.h no futuro)
extern int storage_read_sectors(uint8_t dev_id, uint32_t lba, uint32_t count, void* buffer);

extern void* pmm_alloc_block(void);
extern void  pmm_free_block(void* block);

// Para simplificar seu aprendizado, manteremos as globais. 
// No futuro, se quiser ler dois discos ao mesmo tempo, transformaremos isso em uma struct.
fat32_bpb_t disk_bpb;
uint32_t current_dir_cluster;
uint32_t fat_start_sector;
uint32_t data_start_sector;
uint32_t fat32_partition_offset = 0; 

static volatile int fat_lock = 0;

uint8_t fat32_current_dev_id = 0;

static void fat32_log(const char* msg) {
    terminal_print("[FAT32] ");
    terminal_print(msg);
    terminal_print("\n");
}

// Agora a função é dinâmica: você diz QUAL dispositivo quer montar!
int fat32_mount(uint8_t dev_id) {
    fat32_current_dev_id = dev_id;
    // Garante um endereço físico puro, limpo e alinhado em 4KB para o DMA
    uint8_t *sector_buffer = (uint8_t*)(uintptr_t)pmm_alloc_block();
    
    if (sector_buffer == 0) {
        fat32_log("ERRO: Falha ao alocar bloco fisico para o FAT32.");
        return 0;
    }
    
    while (__sync_lock_test_and_set(&fat_lock, 1));

    // 1: LER A MBR (Setor 0) usando a nova camada genérica
    // CORREÇÃO: Testamos se a função retornou 0 (Falso/Erro)
    if (storage_read_sectors(dev_id, 0, 1, sector_buffer) == 0) {
        fat32_log("ERRO: Falha ao ler a MBR do dispositivo informado.");
        __sync_lock_release(&fat_lock);
        pmm_free_block((void*)(uintptr_t)sector_buffer);
        return 0;
    }

    // Se o setor voltou zerado, a imagem ou a tabela está vazia
    if (sector_buffer[0] == 0x00 && sector_buffer[510] == 0x00) {
        fat32_log("ERRO: Setor MBR totalmente vazio.");
        __sync_lock_release(&fat_lock);
        pmm_free_block((void*)(uintptr_t)sector_buffer);
        return 0;
    }

    // Mapeia o ponteiro da partição na tabela MBR (Offset 446)
    mbr_partition_t* partition1 = (mbr_partition_t*)&sector_buffer[446];
    fat32_partition_offset = partition1->lba_start;

    // --- ESCUDO: Proteção contra LBA inválido ---
    if (fat32_partition_offset == 0xFFFFFFFF || fat32_partition_offset > 120000000) {
        fat32_log("AVISO: MBR Invalida/Incompativel. Usando fallback Setor 2048...");
        fat32_partition_offset = 2048; 
    } else if (fat32_partition_offset == 0) {
        fat32_log("AVISO: Particao inicia no setor 0? Usando fallback Setor 2048...");
        fat32_partition_offset = 2048; 
    }

    // 2: LER O VBR (Setor de Boot da Partição) de forma genérica
    // CORREÇÃO: Testamos se a função retornou 0 (Falso/Erro)
    if (storage_read_sectors(dev_id, fat32_partition_offset, 1, sector_buffer) == 0) {
        fat32_log("ERRO: O dispositivo de armazenamento nao respondeu no VBR.");
        __sync_lock_release(&fat_lock);
        pmm_free_block((void*)(uintptr_t)sector_buffer);
        return 0;
    }

    // Copia os dados do BPB salvando a geometria do disco aberto
    memcpy(&disk_bpb, sector_buffer, sizeof(fat32_bpb_t));

    // 3: ASSINATURA MÁGICA DO SETOR DE BOOT
    if (sector_buffer[510] != 0x55 || sector_buffer[511] != 0xAA) {
        fat32_log("ERRO: Assinatura FAT32 invalida no setor de boot.");
        __sync_lock_release(&fat_lock);
        pmm_free_block((void*)(uintptr_t)sector_buffer);
        return 0;
    }

    // 4: CÁLCULOS DOS SETORES DA ESTRUTURA
    uint32_t fat_size = (disk_bpb.fat_size_16 != 0) ? 
                         disk_bpb.fat_size_16 : disk_bpb.fat_size_32;

    fat_start_sector  = fat32_partition_offset + disk_bpb.reserved_sectors;
    data_start_sector = fat_start_sector + (disk_bpb.fat_count * fat_size);
    current_dir_cluster = disk_bpb.root_cluster;

    __sync_lock_release(&fat_lock);
    pmm_free_block((void*)(uintptr_t)sector_buffer); 
    
    if (dev_id == 0) {
        fat32_log("Montado com sucesso via SATA (Disco 0)!");
    } else {
        fat32_log("Montado com sucesso via USB (Disco 1)!");
    }

    return 1;
}
