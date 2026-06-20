#include "storage.h"
#include "../video.h"
#include "../../util/string.h"
#include "../hw/ahci_hal.h"
#include "ehci_storage.h"

// Tabela global indexada contendo os dispositivos ativos no sistema
static block_device_t devices[STORAGE_MAX_DEVICES];

//extern int ahci_hal_ler(uint32_t lba, uint32_t count, uint8_t* buffer);
extern int ehci_storage_read_sectors(uint8_t addr, uint32_t lba, uint32_t count, void* buffer);

void storage_init(void) {
    for (int i = 0; i < STORAGE_MAX_DEVICES; i++) {
        devices[i].active = 0;
        devices[i].id = i;
        devices[i].name = "Vazio";
    }
    terminal_print("[STORAGE] Gerenciador unificado de discos online.\n");
}

int storage_register_device(uint8_t id, const char* name, uint32_t total_sectors, uint32_t block_size, storage_read_func_t read_f, storage_write_func_t write_f) {
    if (id >= STORAGE_MAX_DEVICES) return 0;
    
    char local_buf[32];
    
    devices[id].name = name;
    devices[id].total_sectors = total_sectors;
    devices[id].block_size = block_size;
    devices[id].read_sectors = read_f;
    devices[id].write_sectors = write_f;
    devices[id].active = 1;

    terminal_print("[STORAGE] Novo dispositivo registrado: [Disco ");
    int_to_string(id, local_buf); terminal_print(local_buf);
    terminal_print("] -> "); terminal_print(name);
    terminal_print("\n");
    
    return 1;
}

void storage_unregister_device(uint8_t id) {
    if (id < STORAGE_MAX_DEVICES) {
        char local_buf[32];
        
        devices[id].active = 0;
        devices[id].name = "Vazio";
        devices[id].read_sectors = NULL;
        devices[id].write_sectors = NULL;
        
        terminal_print("[STORAGE] Dispositivo ");
        int_to_string(id, local_buf); terminal_print(local_buf);
        terminal_print(" desconectado.\n");
    }
}

/*int storage_read_sectors(uint8_t dev_id, uint32_t lba, uint32_t count, void* buffer) {
    if (dev_id >= STORAGE_MAX_DEVICES || !devices[dev_id].active) {
        terminal_print("[STORAGE] ERRO: Leitura em disco inexistente ou inativo.\n");
        return 0; 
    }
    if (!devices[dev_id].read_sectors) return 0;
    return devices[dev_id].read_sectors(lba, count, buffer);
}*/

/*int storage_read_sectors(uint8_t dev_id, uint32_t lba, uint32_t count, void* buffer) {
    if (dev_id == 0) {
        // ahci_hal_ler retorna 0 no sucesso. 
        // Se for igual a 0, nós retornamos 1 (Sucesso para o FAT32).
        if (ahci_hal_ler(lba, count, buffer) == 0) {
            return 1; 
        } else {
            return 0; // Erro real de leitura no SATA
        }
    } 
    
    if (dev_id == 1) {
        // Certifique-se que o seu driver de USB retorne 1 no sucesso e 0 no erro!
        return usb_read_sectors(lba, count, buffer); 
    }

    return 0; // ID de dispositivo inválido
}*/

int storage_read_sectors(uint8_t dev_id, uint32_t lba, uint32_t count, void* buffer) {
    if (dev_id == 0) {
        // SATA: Inverte o retorno (0 vira 1 para Sucesso)
        return (ahci_hal_ler(lba, count, buffer) == 0) ? 1 : 0;
    }
    
    if (dev_id == 1) {
        // USB: Agora com o nome correto que o grep encontrou!
        // Passamos o endereço/ID do dispositivo (geralmente mapeado ou fixo no seu driver, aqui usamos 0 ou dev_id dependendo de como seu driver gerencia)
        // Se o seu ehci_storage_read_sectors já retorna 1 no sucesso, o return direto basta:
        return ehci_storage_read_sectors(dev_id, lba, count, buffer);
    }

    return 0; // ID inválido
}

int storage_write_sectors(uint8_t dev_id, uint32_t lba, uint32_t count, const void* buffer) {
    if (dev_id >= STORAGE_MAX_DEVICES || !devices[dev_id].active) {
        terminal_print("[STORAGE] ERRO: Escrita em disco inexistente ou inativo.\n");
        return 0; 
    }
    if (!devices[dev_id].write_sectors) return 0;
    return devices[dev_id].write_sectors(lba, count, buffer);
}
