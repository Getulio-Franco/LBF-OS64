#ifndef RUNTIME_OS_FORM_H
#define RUNTIME_OS_FORM_H

#include "../controls.h"

// Estrutura para Janelas Assessórias / Diálogos / Modais em Tempo de Execução
typedef struct {
    uint64_t FormHandle;  // Handle de 64 bits associado à janela física no Kernel
    int Left;
    int Top;
    int Width;
    int Height;
    bool Modal;           // Define se bloqueia a interação com as outras janelas
} TOSForm;

// Instancia um novo formulário/janela independente no Ring 3
TOSForm* OS_CreateForm(const char* title, int x, int y, int width, int height);

// Destrói o formulário e libera os recursos alocados na Heap do App
void OS_DestroyForm(TOSForm* form);

#endif // RUNTIME_OS_FORM_H
