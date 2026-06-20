/**
 * ============================================================================
 * FAT32 COPY - NATIVE AHCI/SATA IMPLEMENTATION
 * ============================================================================
 * Descrição: Operações de clonagem de arquivos integradas ao barramento SATA.
 * Localização: fs/fat32_copy.c
 * ============================================================================
 */

//#include "fs/fat32_xcopy.h"
#include "fs/fat32.h"
#include "fs/fat32_logic.h"
#include "fs/fat32_file.h"
#include "drivers/hw/ahci_hal.h"  // Nova abstração estável do hardware SATA
#include "drivers/video.h"        // Feedback visual na tela VESA
#include "util/string.h"
#include "mem/heap.h"

extern fat32_bpb_t disk_bpb;
extern uint32_t current_dir_cluster;

/**
 * @brief Busca metadados internamente (Função Auxiliar).
 */
static int fat32_get_entry_info(const char* name, uint32_t* cluster_out, uint32_t* size_out) {
    // Buffer alinhado a 16 bytes para segurança do barramento SATA
    __attribute__((aligned(16))) uint8_t sector_buffer[512];
    char fat_name[11];
    fat32_to_83_filename(name, fat_name);

    uint32_t dir_cluster = current_dir_cluster;
    while (dir_cluster >= 2 && dir_cluster < FAT32_EOC) {
        uint32_t lba = fat32_cluster_to_lba(dir_cluster);
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            // SUBSTITUIÇÃO: Leitura via HAL SATA nativo
            if (ahci_hal_ler(lba + s, 1, sector_buffer) != 0) return -1;
            
            fat32_directory_entry_t* entry = (fat32_directory_entry_t*)sector_buffer;
            for (int i = 0; i < 16; i++) {
                if (entry[i].filename[0] == 0x00) return -1;
                if (memcmp(entry[i].filename, fat_name, 11) == 0) {
                    *cluster_out = ((uint32_t)entry[i].cluster_high << 16) | entry[i].cluster_low;
                    *size_out = entry[i].file_size;
                    return 0;
                }
            }
        }
        dir_cluster = fat32_get_next_cluster(dir_cluster);
    }
    return -1;
}

/**
 * @brief Copia um arquivo (Função Principal).
 */
int fat32_copy_file(const char* source, const char* destination) {
    uint32_t start_cluster, file_size;
    
    // Feedback inicial usando as funções do seu terminal gráfico VESA
    terminal_print("Copiando: ");
    terminal_print(source);
    terminal_print(" -> ");
    terminal_print(destination);
    terminal_print("\n");

    if (fat32_get_entry_info(source, &start_cluster, &file_size) != 0) {
        terminal_print("Erro: Arquivo origem nao encontrado.\n");
        return FS_NOT_FOUND;
    }

    // Alocação dinâmica para acomodar o arquivo na memória temporariamente
    uint8_t* data_buffer = (uint8_t*)kmalloc(file_size);
    if (!data_buffer) {
        terminal_print("Erro: Memoria insuficiente (Heap Full).\n");
        return -1;
    }

    uint32_t total_read = 0;
    uint32_t current_c = start_cluster;

    // Loop de leitura de clusters via SATA
    while (current_c >= 2 && current_c < FAT32_EOC && total_read < file_size) {
        uint32_t lba = fat32_cluster_to_lba(current_c);
        
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            uint32_t remaining = file_size - total_read;
            if (remaining == 0) break;

            uint32_t to_copy = (remaining > 512) ? 512 : remaining;
            __attribute__((aligned(16))) uint8_t temp[512];
            
            // SUBSTITUIÇÃO: Leitura do bloco de dados via HAL SATA nativo
            if (ahci_hal_ler(lba + s, 1, temp) != 0) {
                kfree(data_buffer);
                terminal_print("Erro critico de leitura no disco SATA.\n");
                return FS_ERROR_READ;
            }
            
            memcpy(data_buffer + total_read, temp, to_copy);
            total_read += to_copy;
        }
        current_c = fat32_get_next_cluster(current_c);
    }

    // Gravação usando a função que portamos com o stub/hal estável
    int result = fat32_write_file(destination, data_buffer, file_size);

    if (result == FS_SUCCESS) {
        terminal_print("Copia concluida com sucesso.\n");
    } else {
        terminal_print("Erro ao gravar arquivo de destino.\n");
    }

    kfree(data_buffer);
    return result;
}
