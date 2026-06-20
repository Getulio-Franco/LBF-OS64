/**
 * ============================================================================
 * FAT32 LOGIC - HEADER (V3.2 - SYNC)
 * ============================================================================
 * Descrição: Protótipos para conversão de endereços e manipulação da FAT.
 * ============================================================================
 */

#ifndef FS_FAT32_LOGIC_H
#define FS_FAT32_LOGIC_H

#include <stdint.h>
#include "fs/fat32.h"

/* --- VARIÁVEIS GLOBAIS (EXTERN) --- */
extern fat32_bpb_t disk_bpb;
extern uint32_t fat_start_sector;
extern uint32_t data_start_sector;
extern uint32_t current_dir_cluster;

/* --- FUNÇÕES DE ENDEREÇAMENTO --- */

/**
 * @brief Converte o número de um cluster para o endereço LBA físico.
 */
uint32_t fat32_cluster_to_lba(uint32_t cluster);

/* --- FUNÇÕES DE MANIPULAÇÃO DA TABELA FAT --- */

/**
 * @brief Consulta a FAT para obter o próximo cluster na corrente (Chain).
 * @return Próximo cluster, FAT32_EOF ou FAT32_BAD_CLUSTER em erro.
 */
uint32_t fat32_get_next_cluster(uint32_t current_cluster);

/**
 * @brief Atualiza uma entrada na FAT (Primária e Backup) para alocação.
 */
void fat32_set_cluster_entry(uint32_t cluster, uint32_t value);

/**
 * @brief Varre a FAT em busca do primeiro cluster livre (0x00000000).
 */
uint32_t fat32_find_free_cluster(void);

/* --- FUNÇÕES DE BUSCA E FORMATAÇÃO --- */

/**
 * @brief Busca uma entrada de diretório pelo nome no diretório atual.
 * @return 0 se encontrar (preenche out_entry), -1 se não encontrar.
 */
int fat32_find_entry(const char* name, fat32_directory_entry_t* out_entry);

/**
 * @brief Converte nome amigável para formato de disco 8.3 (ex: "test.c" -> "TEST    C  ").
 */
void fat32_to_83_filename(const char* input, char* output);

/**
 * @brief Converte formato de disco 8.3 para legível (ex: "TEST    C  " -> "test.c").
 */
void fat32_format_name_for_display(char* dest, unsigned char* src);

#endif /* FS_FAT32_LOGIC_H */
