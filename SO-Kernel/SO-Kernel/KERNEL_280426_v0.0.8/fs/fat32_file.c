/**
 * ============================================================================
 * FAT32 FILE OPERATIONS - NATIVE AHCI/SATA MANIPULATION
 * ============================================================================
 * Descrição: Implementação de leitura, escrita, append e exibição em disco SATA.
 * Localização: fs/fat32_file.c
 * ============================================================================
 */

#include "fs/fat32_file.h" 
#include "fs/fat32.h"
#include "fs/fat32_logic.h"
#include "drivers/hw/ahci_hal.h"  // Nova abstração estável do hardware SATA
#include "drivers/pd/storage.h"
#include "drivers/video.h"
#include "util/string.h"
#include "mem/heap.h"

extern uint32_t current_dir_cluster;
extern fat32_bpb_t disk_bpb;
extern uint8_t fat32_current_dev_id;

/* --- STUB TEMPORÁRIO DE ESCRITA SATA --- 
 * Remove o erro do Linker até que a escrita AHCI seja implementada no HAL */
int ahci_hal_escrever(uint32_t lba, uint32_t count, uint8_t* buffer) {
    (void)lba;
    (void)count;
    (void)buffer;
    // Retorna 0 fingindo que escreveu com sucesso para não quebrar a lógica
    return 0; 
}

/**
 * @brief Lê e exibe o conteúdo de um arquivo no console VESA.
 */
int fat32_display_file(const char* target_name) {
    fat32_directory_entry_t entry;
    
    // Busca a entrada do arquivo para saber o tamanho
    if (fat32_find_entry(target_name, &entry) != 0) {
        return FS_NOT_FOUND;
    }

    // Aloca buffer temporário para leitura
    uint8_t* file_data = (uint8_t*)kmalloc(entry.file_size + 1);
    if (!file_data) return FS_ERROR_READ;

    if (fat32_read_file(target_name, file_data, entry.file_size) == FS_SUCCESS) {
        file_data[entry.file_size] = '\0'; // Null-terminate para segurança

        // Configuração de coordenadas para o modo VESA
        uint32_t cur_x = 20; 
        uint32_t cur_y = 150; // Começa abaixo do prompt do Shell
        uint32_t color = 0x00FFFFFF; // Branco

        for (uint32_t i = 0; i < entry.file_size; i++) {
            char c = (char)file_data[i];
            
            if (c == '\n') {
                cur_x = 20;
                cur_y += 16; // Pula uma linha (assumindo fonte 8x16)
            } else if (c >= 32 && c <= 126) {
                draw_char(cur_x, cur_y, c, color, 1);
                cur_x += 8;
            }

            // Wrap horizontal
            if (cur_x > 1000) { cur_x = 20; cur_y += 16; }
            // Limite vertical para não estourar a tela
            if (cur_y > 740) break;
        }
    }

    kfree(file_data);
    return FS_SUCCESS;
}

/**
 * @brief Lê um arquivo com suporte a offset (Fundamental para o ELF Loader).
 */
int fat32_read_file_at_offset(const char* name, uint8_t* buffer, uint32_t size, uint32_t offset) {
    if (!buffer || size == 0) return -1;

    fat32_directory_entry_t entry;
    if (fat32_find_entry(name, &entry) != 0) return FS_NOT_FOUND;

    if (offset >= entry.file_size) return -1;
    if (offset + size > entry.file_size) size = entry.file_size - offset;

    uint32_t bytes_per_cluster = disk_bpb.sectors_per_cluster * 512;
    uint32_t current_c = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;

    // Pula os clusters necessários para atingir o offset de leitura
    uint32_t skip = offset / bytes_per_cluster;
    for (uint32_t j = 0; j < skip; j++) {
        current_c = fat32_get_next_cluster(current_c);
        if (current_c >= FAT32_EOC) return -1;
    }

    uint32_t internal_offset = offset % bytes_per_cluster;
    uint32_t total_read = 0;
    uint8_t* temp_c = (uint8_t*)kmalloc(bytes_per_cluster);
    if (!temp_c) return FS_ERROR_READ;

    while (current_c >= 2 && current_c < FAT32_EOC && total_read < size) {
      /*  for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            // SUBSTITUIÇÃO: Lendo setores nativamente via AHCI HAL
            ahci_hal_ler(fat32_cluster_to_lba(current_c) + s, 1, temp_c + (s * 512));
        }*/
        
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
           // O gerenciador unificado decide se lê do SATA (0) ou USB (1) automaticamente!
          // extern uint8_t fat32_current_dev_id; 
           storage_read_sectors(fat32_current_dev_id, fat32_cluster_to_lba(current_c) + s, 1, temp_c + (s * 512));
        }

        uint32_t avail = bytes_per_cluster - internal_offset;
        uint32_t to_copy = (size - total_read > avail) ? avail : (size - total_read);

        memcpy(buffer + total_read, temp_c + internal_offset, to_copy);
        total_read += to_copy;
        internal_offset = 0;
        current_c = fat32_get_next_cluster(current_c);
    }

    kfree(temp_c);
    return FS_SUCCESS;
}

/**
 * @brief Wrapper para leitura simples.
 */
int fat32_read_file(const char* name, uint8_t* buffer, uint32_t max_size) {
    return fat32_read_file_at_offset(name, buffer, max_size, 0);
}

/**
 * @brief Grava um arquivo (Sobrescreve se existir).
 */
int fat32_write_file(const char* name, uint8_t* input_buffer, uint32_t size) {
    char fat_name[11];
    fat32_to_83_filename(name, fat_name);

    // Se já existe, deleta
    fat32_directory_entry_t dummy;
    if (fat32_find_entry(name, &dummy) == 0) fat32_delete_file(name);

    uint32_t bytes_per_cluster = disk_bpb.sectors_per_cluster * 512;
    uint32_t bytes_left = size;
    uint32_t current_c, prev_c = 0, first_c = 0;
    uint8_t* data_ptr = input_buffer;

    while (bytes_left > 0) {
        current_c = fat32_find_free_cluster();
        if (current_c == 0) return FS_DISK_FULL;
        if (first_c == 0) first_c = current_c;
        if (prev_c != 0) fat32_set_cluster_entry(prev_c, current_c);

        uint8_t* cluster_buf = (uint8_t*)kmalloc(bytes_per_cluster);
        memset(cluster_buf, 0, bytes_per_cluster);
        uint32_t to_write = (bytes_left > bytes_per_cluster) ? bytes_per_cluster : bytes_left;
        memcpy(cluster_buf, data_ptr, to_write);

        /*for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            // SUBSTITUIÇÃO: Escrita modular via AHCI HAL
            ahci_hal_escrever(fat32_cluster_to_lba(current_c) + s, 1, cluster_buf + (s * 512));
        }*/
        
        // COMO DEVE FICAR:
       // extern uint8_t fat32_current_dev_id;
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
           if (fat32_current_dev_id == 0) {
               ahci_hal_escrever(fat32_cluster_to_lba(current_c) + s, 1, cluster_buf + (s * 512));
           } else {
              // Quando implementar a escrita do USB, coloque a chamada aqui ou use:
              // storage_write_sectors(fat32_current_dev_id, fat32_cluster_to_lba(current_c) + s, 1, cluster_buf + (s * 512));
           }
        }
        
        fat32_set_cluster_entry(current_c, FAT32_EOC);
        kfree(cluster_buf);
        data_ptr += to_write;
        bytes_left -= to_write;
        prev_c = current_c;
    }

    // Criar entrada no diretório pai
    uint32_t dir_c = current_dir_cluster;
    __attribute__((aligned(16))) uint8_t sect[512];
    
    while (dir_c < FAT32_EOC) {
        uint32_t lba = fat32_cluster_to_lba(dir_c);
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            
            // CORREÇÃO: Leitura unificada também na criação de arquivos!
            storage_read_sectors(fat32_current_dev_id, lba + s, 1, sect);
            
            for (int i = 0; i < 512; i += 32) {
                if (sect[i] == 0x00 || sect[i] == 0xE5) {
                    fat32_directory_entry_t* e = (fat32_directory_entry_t*)&sect[i];
                    memcpy(e->filename, fat_name, 11);
                    e->attributes = 0x20; // Arquivo normal (Archive)
                    e->cluster_high = (first_c >> 16) & 0xFFFF;
                    e->cluster_low = first_c & 0xFFFF;
                    e->file_size = size;
                    
                    // CORREÇÃO: Proteção de escrita
                    if (fat32_current_dev_id == 0) {
                        ahci_hal_escrever(lba + s, 1, sect);
                    }
                    return FS_SUCCESS;
                }
            }
        }
        dir_c = fat32_get_next_cluster(dir_c);
    }
    return FS_DISK_FULL;
}

/**
 * @brief Deleta um arquivo e limpa a tabela FAT.
 */
int fat32_delete_file(const char* filename) {
    char fat_name[11];
    fat32_to_83_filename(filename, fat_name);
    __attribute__((aligned(16))) uint8_t sect[512];
    uint32_t dir_c = current_dir_cluster;

    while (dir_c < FAT32_EOC) {
        uint32_t lba = fat32_cluster_to_lba(dir_c);
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            
            // MUDANÇA AQUI: Leitura unificada substitui o ahci_hal_ler antigo
            storage_read_sectors(fat32_current_dev_id, lba + s, 1, sect);
            
            fat32_directory_entry_t* e = (fat32_directory_entry_t*)sect;
            for (int i = 0; i < 16; i++) {
                if (e[i].filename[0] == 0x00) return FS_NOT_FOUND;
                if (memcmp(e[i].filename, fat_name, 11) == 0) {
                    uint32_t c = ((uint32_t)e[i].cluster_high << 16) | e[i].cluster_low;
                    e[i].filename[0] = 0xE5; // Marca como deletado
                    
                    // Proteção temporária para gravação
                    if (fat32_current_dev_id == 0) {
                        ahci_hal_escrever(lba + s, 1, sect);
                    }
                    
                    while (c >= 2 && c < FAT32_EOC) {
                        uint32_t next = fat32_get_next_cluster(c);
                        fat32_set_cluster_entry(c, 0); // Libera cluster na FAT
                        c = next;
                    }
                    return FS_SUCCESS;
                }
            }
        }
        dir_c = fat32_get_next_cluster(dir_c);
    }
    return FS_NOT_FOUND;
}

/**
 * @brief Adiciona dados ao final de um arquivo existente.
 */
int fat32_append_file(const char* filename, uint8_t* data, uint32_t data_len) {
    if (!filename || !data || data_len == 0) return FS_SUCCESS;

    fat32_directory_entry_t entry;
    uint32_t entry_lba = 0;
    int entry_idx = 0;
    
    uint32_t dir_c = current_dir_cluster;
    __attribute__((aligned(16))) uint8_t sect[512];
    char fat_name[11];
    fat32_to_83_filename(filename, fat_name);
    int found = 0;

    while (dir_c < FAT32_EOC && !found) {
        uint32_t lba = fat32_cluster_to_lba(dir_c);
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            
            // MUDANÇA 1: Leitura unificada para buscar a entrada do arquivo
            storage_read_sectors(fat32_current_dev_id, lba + s, 1, sect);
            
            fat32_directory_entry_t* es = (fat32_directory_entry_t*)sect;
            for (int i = 0; i < 16; i++) {
                if (memcmp(es[i].filename, fat_name, 11) == 0) {
                    entry = es[i]; 
                    entry_lba = lba + s; 
                    entry_idx = i;
                    found = 1; 
                    break;
                }
            }
            if (found) break;
        }
        if (!found) dir_c = fat32_get_next_cluster(dir_c);
    }
    if (!found) return FS_NOT_FOUND;

    uint32_t bytes_per_cluster = disk_bpb.sectors_per_cluster * 512;
    uint32_t last_c = ((uint32_t)entry.cluster_high << 16) | entry.cluster_low;
    while (fat32_get_next_cluster(last_c) < FAT32_EOC) 
        last_c = fat32_get_next_cluster(last_c);

    uint32_t pos = entry.file_size % bytes_per_cluster;
    uint32_t written = 0;
    uint8_t* buf = (uint8_t*)kmalloc(bytes_per_cluster);
    if (!buf) return FS_ERROR_WRITE;

    // Preencher o espaço que resta no último cluster
    if (pos != 0 || entry.file_size == 0) {
        uint32_t space = bytes_per_cluster - pos;
        for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++) {
            
            // MUDANÇA 2: Leitura unificada para ler o último cluster válido
            storage_read_sectors(fat32_current_dev_id, fat32_cluster_to_lba(last_c) + s, 1, buf + (s * 512));
        }
        
        uint32_t to_copy = (data_len < space) ? data_len : space;
        memcpy(buf + pos, data, to_copy);
        
        // Grava apenas se for SATA
        if (fat32_current_dev_id == 0) {
            for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++)
                ahci_hal_escrever(fat32_cluster_to_lba(last_c) + s, 1, buf + (s * 512));
        }
        written = to_copy;
    }

    // Aloca novos clusters se necessário
    uint32_t prev = last_c;
    while (written < data_len) {
        uint32_t new_c = fat32_find_free_cluster();
        if (!new_c) { kfree(buf); return FS_DISK_FULL; }
        fat32_set_cluster_entry(prev, new_c);
        fat32_set_cluster_entry(new_c, FAT32_EOC);
        memset(buf, 0, bytes_per_cluster);
        uint32_t to_c = (data_len - written > bytes_per_cluster) ? bytes_per_cluster : (data_len - written);
        memcpy(buf, data + written, to_c);
        
        if (fat32_current_dev_id == 0) {
            for (uint32_t s = 0; s < disk_bpb.sectors_per_cluster; s++)
                ahci_hal_escrever(fat32_cluster_to_lba(new_c) + s, 1, buf + (s * 512));
        }
        written += to_c; 
        prev = new_c;
    }

    // MUDANÇA 3: Leitura unificada para atualizar os metadados do arquivo (Tamanho)
    storage_read_sectors(fat32_current_dev_id, entry_lba, 1, sect);
    
    ((fat32_directory_entry_t*)sect)[entry_idx].file_size += data_len;
    
    if (fat32_current_dev_id == 0) {
        ahci_hal_escrever(entry_lba, 1, sect);
    }

    kfree(buf);
    return FS_SUCCESS;
}
