#include "os_memo.h"

TOSMemo* OS_CreateMemo(TAppEnvironment* app, int x, int y, int w, int h) {
    if (!app) return NULL;

    // 1. Alocação e limpeza da estrutura principal em Ring 3
    TOSMemo* memo = (TOSMemo*)malloc(sizeof(TOSMemo));
    if (!memo) return NULL;
    memset(memo, 0, sizeof(TOSMemo)); 

    // 2. Configuração de Atributos Locais e Geometria
    memo->base.Type = TYPE_MEMO; // Identificador nativo vindo do gui.h
    memo->base.Left = x;
    memo->base.Top = y;
    memo->base.Width = w;  
    memo->base.Height = h;  
    memo->base.IsSelected = false;
    
    // Inicialização do Buffer Dinâmico (Começa alocando 512 bytes na heap)
    memo->AllocatedSize = 512;
    memo->Buffer = (char*)malloc(memo->AllocatedSize);
    if (!memo->Buffer) {
        free(memo);
        return NULL;
    }
    memo->Buffer[0] = '\0';
    memo->TextLength = 0;
    memo->CursorX = 0;
    memo->CursorY = 0;

    // 3. Registro Centralizado no Runtime (Gera "Memo1", "Memo2"...)
    OS_RegisterControl(app, (TOSControl*)memo, "Memo");

    // 4. Criação e acoplamento no Subsistema Gráfico do Kernel via gui.h
    memo->base.KernelHandle = (uint64_t)gui_create_memo((TWinControl*)app->MainWindow, memo->base.Name);
    
    if (memo->base.KernelHandle == 0) {
        free(memo->Buffer);
        free(memo);
        return NULL;
    }

    // 5. Sincronização via propriedades dinâmicas direto para o Kernel
    gui_set_prop((void*)memo->base.KernelHandle, PROP_LEFT,   (uint64_t)memo->base.Left);
    gui_set_prop((void*)memo->base.KernelHandle, PROP_TOP,    (uint64_t)memo->base.Top);
    gui_set_prop((void*)memo->base.KernelHandle, PROP_WIDTH,  (uint64_t)memo->base.Width);
    gui_set_prop((void*)memo->base.KernelHandle, PROP_HEIGHT, (uint64_t)memo->base.Height);
    
    // Injeta fundo branco padrão direto na instância do Kernel
    gui_set_prop((void*)memo->base.KernelHandle, PROP_COLOR,  0xFFFFFF);

    return memo;
}

void OS_Memo_AddChar(TOSMemo* memo, char key) {
    if (!memo || !memo->base.KernelHandle) return;

    // 1. Tratamento do BACKSPACE (Apagar caractere)
    if (key == 8 || key == '\b') {
        if (memo->TextLength > 0) {
            memo->TextLength--;
            memo->Buffer[memo->TextLength] = '\0';
        }
    } 
    // 2. Tratamento de Caracteres Visíveis ou Quebra de Linha (Enter)
    else if ((key >= 32 && key <= 126) || key == '\n' || key == '\r') {
        // Normaliza o caractere de quebra de linha para o padrão UNIX (\n)
        char char_to_insert = (key == '\r') ? '\n' : key;

        // Se o buffer estiver prestes a estourar, dobra a capacidade via realocação dinâmica
        if (memo->TextLength + 2 >= memo->AllocatedSize) {
            int new_size = memo->AllocatedSize * 2;
            char* new_buffer = (char*)realloc(memo->Buffer, new_size);
            
            if (new_buffer) {
                memo->Buffer = new_buffer;
                memo->AllocatedSize = new_size;
            } else {
                return; // Proteção contra falta de memória (OOM) no S.O.
            }
        }

        // Insere o caractere e atualiza o terminador nulo
        memo->Buffer[memo->TextLength] = char_to_insert;
        memo->TextLength++;
        memo->Buffer[memo->TextLength] = '\0';
    }

    // 3. Atualiza a propriedade visual enviando o ponteiro de Ring 3 atualizado para o Kernel
    gui_set_prop((void*)memo->base.KernelHandle, PROP_CAPTION, (uintptr_t)memo->Buffer);
}

void OS_Memo_AddStr(TOSMemo* memo, const char* str) {
    if (!memo || !str) return;

    // Loop que percorre a string caractere por caractere até encontrar o terminador nulo '\0'
    while (*str != '\0') {
        OS_Memo_AddChar(memo, *str);
        str++; // Avança para o próximo caractere
    }
}

void OS_Memo_SetFocus(TOSMemo* memo) {
    if (!memo || !memo->base.KernelHandle) return;

    // Via syscall segura, ordena que o Kernel mude o g_focused_control para este Memo
    gui_set_prop((void*)memo->base.KernelHandle, PROP_SET_FOCUS, 1);
}

void OS_Memo_SetScroll(TOSMemo* memo, int value) {
    if (!memo || !memo->base.KernelHandle) return;
    
    if (value < 0) value = 0;
    memo->ScrollY = value;
    
    // Despacha para a Engine Gráfica atualizar o viewport interno
    gui_set_prop((void*)memo->base.KernelHandle, PROP_SCROLL_Y, (uint64_t)memo->ScrollY);
}
