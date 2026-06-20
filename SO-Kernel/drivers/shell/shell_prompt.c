/**
 * ============================================================================
 * SHELL PROMPT MANAGER - VESA/LFB EDITION
 * ============================================================================
 * Descrição: Gerencia o caminho atual (C:\) exibido no prompt da shell.
 * Localização: drivers/shell/shell_prompt.c
 * ============================================================================
 */

#include "shell_prompt.h"
#include "util/string.h"

// Buffer global para o caminho atual
static char current_path[256] = "C:\\";

/**
 * @brief Retorna o ponteiro para a string do caminho atual.
 */
char* get_current_path() { 
    return current_path; 
}

/**
 * @brief Atualiza o caminho da Shell baseado no argumento (cd ou cd..).
 */
void update_shell_path(const char* arg) {
    if (!arg) return;

    // --- Lógica para subir diretório (CD ..) ---
    if (strcmp(arg, "..") == 0) {
        int len = strlen(current_path);
        
        // Se já estiver na raiz "C:\", não faz nada
        if (len <= 3) return; 

        // Percorre de trás para frente para encontrar a última barra
        for (int i = len - 1; i >= 2; i--) {
            if (current_path[i] == '\\') {
                // Se for a barra logo após o "C:", mantemos a barra e fechamos a string
                if (i == 2) {
                    current_path[i + 1] = '\0';
                } else {
                    current_path[i] = '\0';
                }
                break;
            }
        }
    } 
    // --- Lógica para descer diretório (CD FOLDER) ---
    else {
        int len = strlen(current_path);

        // Se não estiver na raiz, precisamos adicionar uma barra de separação antes do nome
        if (len > 3 && current_path[len - 1] != '\\') {
            strcat(current_path, "\\");
        }
        
        // Concatena o novo diretório
        strcat(current_path, arg);

        // Padronização: Transforma o caminho em MAIÚSCULO (Estilo DOS clássico)
        for (int i = 0; current_path[i]; i++) {
            if (current_path[i] >= 'a' && current_path[i] <= 'z') {
                current_path[i] -= 32;
            }
        }
    }
}
