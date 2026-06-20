#include "ahci_hal.h"
#include "ahci_pci.h"
#include "ahci_vmm.h"
#include "ahci_reset.h"
#include "ahci_mem.h"
#include "ahci_cmd.h"
#include "../../util/string.h"
#include "../../drivers/video.h"

// Estado global do driver encapsulado
static struct {
    volatile ahci_port_reg_t* porta_ativa;
    int numero_porta;
    bool inicializado;
} g_ahci_driver = {0, 0, false};

bool ahci_hal_inicializar(void) {
    terminal_print("AHCI_HAL: Iniciando sequencia de boot do driver SATA...\n");

    // Passo 1: Detectar via PCI
    ahci_pci_info_t pci = ahci_pci_detectar();
    if (!pci.encontrado) {
        terminal_print("AHCI_HAL: Abortado. Nenhuma controladora PCI encontrada.\n");
        return false;
    }

    // Passo 2: Mapear BAR5 no VMM com Cache Disable
    void* abar = ahci_vmm_mapear_abar(pci.bar5_phys);
    if (!abar) {
        terminal_print("AHCI_HAL: Abortado. Falha crítica no mapeamento VMM.\n");
        return false;
    }

    // Passo 3: Reset global da controladora e checagem de link elétrico
    volatile ahci_hba_reg_t* hba = (volatile ahci_hba_reg_t*)abar;
    int discos = ahci_reset_controladora(abar);
    if (discos <= 0) {
        terminal_print("AHCI_HAL: Abortado. Nenhum disco SATA respondeu ao COMRESET.\n");
        return false;
    }

    // Passo 4: Localizar a porta do disco e realizar o Rebase de Memória Alinhada
    for (int p = 0; p < 32; p++) {
        if ((hba->pi & (1 << p)) && (hba->ports[p].sig == 0x00000101)) {
            if (ahci_mem_configurar_porta(&hba->ports[p], p)) {
                g_ahci_driver.porta_ativa = &hba->ports[p];
                g_ahci_driver.numero_porta = p;
                g_ahci_driver.inicializado = true;
                
                terminal_print("AHCI_HAL: Driver SATA totalmente operacional e registrado!\n");
                return true;
            }
        }
    }

    return false;
}

int ahci_hal_ler(uint64_t lba, uint32_t count, void* buffer) {
    if (!g_ahci_driver.inicializado || !g_ahci_driver.porta_ativa) {
        return -1; // Driver não está pronto
    }

    if (count == 0 || buffer == 0) {
        return -2; // Parâmetros inválidos
    }

    // Estratégia de Bounce Buffer para Hardware Real / VirtualBox:
    // O buffer de destino do FAT32 pode não estar alinhado em 4KB na RAM física.
    // Para garantir o sucesso, fazemos o AHCI jogar os dados via DMA na nossa área 
    // estática garantida (AHCI_MEM_SAFE_BASE + 512KB) e depois copiamos via software para o buffer do usuário.
    uint64_t bounce_buffer_phys = AHCI_MEM_SAFE_BASE + 0x80000; // Deslocamento seguro de 512KB
    void* bounce_buffer_virt = (void*)(uintptr_t)bounce_buffer_phys;

    // Dispara a transferência física por DMA (Passo 5)
    bool sucesso = ahci_cmd_ler_setores(
        g_ahci_driver.porta_ativa, 
        g_ahci_driver.numero_porta, 
        lba, 
        count, 
        bounce_buffer_phys
    );

    if (!sucesso) {
        terminal_print("AHCI_HAL: Erro de E/S durante a leitura de setores.\n");
        return -3; // Falha na leitura de hardware
    }

    // Transfere com segurança os dados lidos do Bounce Buffer para o destino final
    memcpy(buffer, bounce_buffer_virt, count * 512);

    return 0; // Sucesso!
}

/*int ahci_hal_escrever(uint32_t lba, uint32_t count, uint8_t* buffer) {
    // TODO: Implementar envio de comandos FIS de escrita (Write DMA Ext)
    (void)lba; (void)count; (void)buffer;
    return 0;
}*/
