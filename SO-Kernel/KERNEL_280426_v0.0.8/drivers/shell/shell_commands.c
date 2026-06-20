/**
 * ============================================================================
 * SHELL COMMAND INTERPRETER - VESA/LFB EDITION (FIXED)
 * ============================================================================
 * Descrição: Processa e despacha os comandos do usuário.
 * Localização: drivers/shell/shell_commands.c
 * ============================================================================
 */

#include "shell_commands.h"
#include "shell_prompt.h"
#include "shell.h"         
#include "drivers/proc.h"  
#include "drivers/video.h" 
#include "drivers/syscall.h" 
#include "fs/fat32.h"        
#include "util/string.h"

/**
 * @brief Helper local para impressão na Shell.
 * Garante que mensagens de erro e ajuda usem o terminal gráfico.
 */
static void cmd_print(const char* str) {
    if (!str) return;
    while (*str) {
        terminal_putc(*str++);
    }
}

/**
 * @brief Interpreta e executa a linha de comando fornecida.
 */
void shell_execute_command(char* input_buffer) {
    
    // 1. Limpeza inicial de espaços em branco
    char* current_ptr = input_buffer;
    while (*current_ptr == ' ') {
        current_ptr++;
    }

    if (*current_ptr == '\0') {
        return;
    }

    // 2. Despacho de Comandos (Interface via Syscalls com 5 argumentos)
    
    // COMANDO: CLS / CLEAR (Limpa o Framebuffer)
    if (strcmp(current_ptr, "cls") == 0 || strcmp(current_ptr, "clear") == 0) {
        syscall_handler(SYS_CLEAR, 0, 0, 0, 0, 0);
    }

    // COMANDO: HELP
    else if (strcmp(current_ptr, "help") == 0) {
        cmd_print("Comandos Disponiveis (Modo VESA):\n");
        cmd_print("LS, CD, MKDIR, CAT, COPY, CP, RM, PS, KILL, EXEC, CLS\n");
    }
    
    // ========================================================================
    // NOVO COMANDO: TESTUSB (Dispara o Handshake USB Core via Syscall)
    // ========================================================================
  /*  else if (strcmp(current_ptr, "testusb") == 0) {
        cmd_print("Shell: Disparando requisicao de teste USB para o Ring 0...\n");
        // Despacha para o Kernel usando o padrão nativo de 5 argumentos do seu S.O.
        syscall_handler(SYS_TESTUSB, 0, 0, 0, 0, 0);
    }*/
    
    else if (strcmp(current_ptr, "testusb") == 0) {
        cmd_print("Shell: Disparando requisicao de teste USB para o Ring 0...\n");
        
        // Força os argumentos a irem como inteiros de 64 bits (uint64_t)
        syscall_handler((uint64_t)SYS_TESTUSB, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL);
    }
    
    else if (strcmp(current_ptr, "testinit") == 0) {
         cmd_print("Shell: Ativando interface Bulk-Only Transport (Passo 4)...\n");
    
         // Despacha para o Kernel usando o seu padrão nativo de 5 argumentos
         // Substitua SYS_TESTINIT pelo número correspondente na sua tabela de syscalls (ex: 65)
         syscall_handler(SYS_TESTINIT, 0, 0, 0, 0, 0);
    }
    
        else if (strcmp(current_ptr, "teststorage") == 0) {
         cmd_print("Shell: Ativando storage (Passo 5)...\n");
    
         // Despacha para o Kernel usando o seu padrão nativo de 5 argumentos
         // Substitua SYS_TESTINIT pelo número correspondente na sua tabela de syscalls (ex: 65)
         syscall_handler(SYS_TESTSTORAGE, 0, 0, 0, 0, 0);
    }

    // COMANDO: LS / DIR
    else if (strcmp(current_ptr, "ls") == 0 || strcmp(current_ptr, "dir") == 0) {
        syscall_handler(SYS_FATLS, 0, 0, 0, 0, 0);
    }

    // COMANDO: CAT (Exibir conteúdo de arquivo na tela gráfica)
    else if (strncmp(current_ptr, "cat ", 4) == 0 || strncmp(current_ptr, "type ", 5) == 0) {
        char* target_file = (current_ptr[0] == 'c') ? current_ptr + 4 : current_ptr + 5;
        while (*target_file == ' ') target_file++;
        
        if (*target_file != '\0') {
            syscall_handler(SYS_FATCAT, (uint64_t)target_file, 0, 0, 0, 0);
        }
    }

    // COMANDO: MKDIR
    else if (strncmp(current_ptr, "mkdir ", 6) == 0 || strncmp(current_ptr, "md ", 3) == 0) {
        char* new_dir = (current_ptr[1] == 'k') ? current_ptr + 6 : current_ptr + 3;
        while (*new_dir == ' ') new_dir++;
        
        // Passando os 5 argumentos necessários
        int status = (int)syscall_handler(SYS_MKDIR, (uint64_t)new_dir, 0, 0, 0, 0);
        if (status != 0) cmd_print("Erro: Nao foi possivel criar o diretorio.\n");
    }

    // COMANDO: CD (Navegação de diretórios)
    else if (strncmp(current_ptr, "cd ", 3) == 0 || strcmp(current_ptr, "cd..") == 0) {
        char* path_target;
        if (strcmp(current_ptr, "cd..") == 0) {
            path_target = "..";
        } else {
            path_target = current_ptr + 3;
            while (*path_target == ' ') path_target++;
        }
        
        int status = (int)syscall_handler(SYS_CHDIR, (uint64_t)path_target, 0, 0, 0, 0);
        if (status == 0) {
            update_shell_path(path_target);
        } else {
            cmd_print("Erro: Caminho nao encontrado no VFS.\n");
        }
    }

    // COMANDO: EXEC (Carregar programa ELF para Ring 3)
    else if (strncmp(current_ptr, "exec ", 5) == 0) {
        char* elf_path = current_ptr + 5;
        while (*elf_path == ' ') elf_path++;
        
        cmd_print("LFB: Carregando executavel...\n");
        // Chamada com 5 argumentos
        if (syscall_handler(SYS_EXEC, (uint64_t)elf_path, 0, 0, 0, 0) == (uint64_t)-1) {
            cmd_print("Erro: Falha ao carregar binario ELF.\n");
        }
    }

    // COMANDO: PS (Listar processos)
    else if (strcmp(current_ptr, "ps") == 0) {
        syscall_handler(SYS_PS, 0, 0, 0, 0, 0);
    }

    // COMANDO: KILL
    else if (strncmp(current_ptr, "kill ", 5) == 0) {
        uint64_t pid = atoi(current_ptr + 5);
        if (syscall_handler(SYS_KILL, pid, 0, 0, 0, 0) != 0) {
            cmd_print("Erro: Nao foi possivel finalizar o PID solicitado.\n");
        }
    }

    // COMANDO DESCONHECIDO
    else {
        cmd_print("Comando desconhecido: ");
        cmd_print(current_ptr);
        cmd_print("\nUtilize 'help' para a lista de comandos.\n");
    }

    // Pula uma linha para o próximo prompt
    cmd_print("\n");
}
