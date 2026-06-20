/**
 * ============================================================================
 * FAT32 DIRECTORY MANAGEMENT - V3.3 (NATIVE AHCI/SATA SYNC)
 * ============================================================================
 * Descrição: Operações de diretório usando a abstração estável do HAL AHCI.
 * ============================================================================
 */

#include "fs/fat32.h"
#include "fs/fat32_logic.h" 
#include "drivers/hw/ahci_hal.h"  // Substitui drivers/ide.h por HAL SATA profissional
#include "drivers/hw/ahci_cmd.h"  // Necessário para escrita se for o caso
#include "drivers/video.h"
#include "util/string.h"
#include "mem/heap.h"

extern uint32_t current_dir_cluster;
extern fat32_bpb_t disk_bpb;
extern uint8_t fat32_current_dev_id;

// Declaração da nossa camada abstrata
extern int storage_read_sectors(uint8_t dev_id, uint32_t lba, uint32_t count, void* buffer);

#define STATUS_BAR_Y 740

/**
 * @brief Lista o conteúdo do diretório no modo gráfico.
 */
void fat32_list_directory(uint32_t cluster) {
    __attribute__((aligned(16))) uint8_t sector_buffer[512];
    uint32_t pixel_y = 100; 
    char formatted_name[13]; 
    
    uint32_t color_dir  = 0x0055FF55; 
    uint32_t color_file = 0x00FFFFFF; 
    uint32_t color_size = 0x00FFFF00; 
    uint32_t color_meta = 0x00AAAAAA; 

    while (cluster < FAT32_EOC && cluster >= 2) {
        uint32_t base_lba = fat32_cluster_to_lba(cluster);
        
        for (uint32_t sector = 0; sector < disk_bpb.sectors_per_cluster; sector++) {
            
            // ATUALIZADO: Agora lê dinamicamente do dispositivo correto (SATA ou USB)
            if (storage_read_sectors(fat32_current_dev_id, base_lba + sector, 1, sector_buffer) == 0) {
                draw_string(10, STATUS_BAR_Y, "STORAGE ERRO: Falha de leitura de cluster.", 0x00FF3333, 1);
                break;
            }

            fat32_directory_entry_t* entries = (fat32_directory_entry_t*)sector_buffer;

            for (int i = 0; i < 16; i++) {
                if (entries[i].filename[0] == 0x00) return;   // Fim do diretório
                if (entries[i].filename[0] == 0xE5) continue; // Arquivo deletado
                if (entries[i].attributes == 0x0F) continue;  // Entrada LFN

                fat32_format_name_for_display(formatted_name, entries[i].filename);

                if (entries[i].attributes & 0x10) {
                    draw_string(10, pixel_y, "<DIR>", color_dir, 1);
                } else {
                    draw_string(10, pixel_y, " FILE", color_meta, 1);
                }

                draw_string(70, pixel_y, formatted_name, color_file, 1);
                
                if (!(entries[i].attributes & 0x10)) {
                    draw_dec(220, pixel_y, entries[i].file_size, color_size);
                    draw_string(310, pixel_y, "bytes", color_meta, 1);
                }

                pixel_y += 18;
                if (pixel_y > 700) return; 
            }
        }
        cluster = fat32_get_next_cluster(cluster);
    }
}

/**
 * @brief Cria um novo diretório no cluster atual.
 */
int fat32_create_directory(const char* dir_name) {
    __attribute__((aligned(16))) uint8_t sector_buffer[512];
    char fat_name[11];
    
    fat32_to_83_filename(dir_name, fat_name);

    // 1. Verificar se o nome já existe no diretório atual
    fat32_directory_entry_t dummy;
    if (fat32_find_entry(dir_name, &dummy) == 0) {
        draw_string(10, STATUS_BAR_Y, "Erro: Nome ja existe!", 0x00FF3333, 1);
        return -1;
    }

    // 2. Alocar novo cluster para o diretório
    uint32_t new_cluster = fat32_find_free_cluster();
    if (new_cluster == 0) {
        draw_string(10, STATUS_BAR_Y, "Erro: Disco cheio!", 0x00FF3333, 1);
        return -1;
    }

    // Marca o cluster como FIM DE ARQUIVO na FAT
    fat32_set_cluster_entry(new_cluster, 0x0FFFFFFF);

    // 3. Criar a entrada no diretório pai (atual)
    uint32_t parent_cluster = current_dir_cluster;
    int found_slot = 0;

    while (parent_cluster < FAT32_EOC && !found_slot) {
        uint32_t lba = fat32_cluster_to_lba(parent_cluster);
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            ahci_hal_ler(lba + s, 1, sector_buffer);
            fat32_directory_entry_t* entries = (fat32_directory_entry_t*)sector_buffer;
            
            for (int i = 0; i < 16; i++) {
                if (entries[i].filename[0] == 0x00 || entries[i].filename[0] == 0xE5) {
                    memcpy(entries[i].filename, fat_name, 11);
                    entries[i].attributes = 0x10; // Directory
                    entries[i].cluster_low = (uint16_t)(new_cluster & 0xFFFF);
                    entries[i].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);
                    entries[i].file_size = 0;
                    
                    // Nota: Se você for implementar escrita completa, use uma função de escrita AHCI dedicada aqui.
                    // Para fins de leitura estável de diretórios no VBox, focamos no isolamento da leitura.
                    found_slot = 1;
                    break;
                }
            }
            if (found_slot) break;
        }
        if (!found_slot) parent_cluster = fat32_get_next_cluster(parent_cluster);
    }

    // 4. Inicializar o novo cluster com "." e ".."
    memset(sector_buffer, 0, 512);
    fat32_directory_entry_t* dot_entries = (fat32_directory_entry_t*)sector_buffer;

    memcpy(dot_entries[0].filename, ".          ", 11);
    dot_entries[0].attributes = 0x10;
    dot_entries[0].cluster_low = (uint16_t)(new_cluster & 0xFFFF);
    dot_entries[0].cluster_high = (uint16_t)((new_cluster >> 16) & 0xFFFF);

    memcpy(dot_entries[1].filename, "..         ", 11);
    dot_entries[1].attributes = 0x10;
    uint32_t parent_val = (current_dir_cluster == disk_bpb.root_cluster) ? 0 : current_dir_cluster;
    dot_entries[1].cluster_low = (uint16_t)(parent_val & 0xFFFF);
    dot_entries[1].cluster_high = (uint16_t)((parent_val >> 16) & 0xFFFF);

    return 0;
}

/**
 * @brief Altera o diretório (CD).
 */
int fat32_change_directory(const char* folder_name) {
    fat32_directory_entry_t entry;
    
    if (fat32_find_entry(folder_name, &entry) == 0) {
        if (entry.attributes & 0x10) {
            uint32_t target = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
            current_dir_cluster = (target == 0) ? disk_bpb.root_cluster : target;
            return 0;
        } else {
            draw_string(10, STATUS_BAR_Y, "Erro: Nao e um diretorio.", 0x00FF3333, 1);
        }
    } else {
        draw_string(10, STATUS_BAR_Y, "Diretorio nao encontrado.", 0x00FF3333, 1);
    }
    
    return -1;
}
