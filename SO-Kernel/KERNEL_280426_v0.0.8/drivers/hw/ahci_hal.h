#ifndef AHCI_HAL_H
#define AHCI_HAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Inicializa completamente o subsistema SATA/AHCI seguindo a ordem cronológica correta.
 * @return true se uma controladora foi configurada e há pelo menos um disco pronto para uso.
 */
bool ahci_hal_inicializar(void);

/**
 * @brief Função padronizada para ler setores do disco SATA (Interface para o FAT32 / VFS).
 * @param lba Setor lógico inicial.
 * @param count Quantidade de setores a ler.
 * @param buffer Ponteiro (virtual) de destino onde os dados serão armazenados.
 * @return 0 em caso de sucesso, ou um código de erro negativo se falhar.
 */
int ahci_hal_ler(uint64_t lba, uint32_t count, void* buffer);
//int ahci_hal_escrever(uint32_t lba, uint32_t count, uint8_t* buffer);

#endif // AHCI_HAL_H
