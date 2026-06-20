#include "os_combobox.h"

// Função auxiliar interna para montar o texto formatado (Ex: "True (1/2)")
static void OS_ComboBox_UpdateKernelText(TOSComboBox* combo) {
    if (!combo || !combo->base.KernelHandle || combo->ItemCount == 0) return;

    char buffer_formatado[32];
    char tmp[16];

    memset(buffer_formatado, 0, sizeof(buffer_formatado));

    // Copia o texto do item atual (Ex: "False")
    strcpy(buffer_formatado, combo->Items[combo->ItemIndex]);
    strcat(buffer_formatado, " "); // Pequeno espaço de respiro

    // Converte e anexa o número do índice atual baseado em 1 (Ex: Item 1)
    memset(tmp, 0, sizeof(tmp));
    int_to_string(combo->ItemIndex + 1, tmp);
    strcat(buffer_formatado, tmp);

    // O caractere especial '-' que o driver gráfico sabe renderizar como divisor ou setinha
    strcat(buffer_formatado, "-");

    // Converte e anexa o total de itens
    memset(tmp, 0, sizeof(tmp));
    int_to_string(combo->ItemCount, tmp);
    strcat(buffer_formatado, tmp);

    // Envia a string final montada em Ring 3 para a propriedade de Caption do Kernel
    gui_set_prop((void*)combo->base.KernelHandle, PROP_CAPTION, (uintptr_t)buffer_formatado);
}

TOSComboBox* OS_CreateComboBox(TAppEnvironment* app, int x, int y, int w, int h) {
    if (!app) return NULL;

    // 1. Alocação e limpeza da estrutura em Ring 3
    TOSComboBox* combo = (TOSComboBox*)malloc(sizeof(TOSComboBox));
    if (!combo) return NULL;
    memset(combo, 0, sizeof(TOSComboBox));

    // 2. Configuração de Atributos Locais e Geometria
    combo->base.Type = TYPE_COMBOBOX; // Identificador unificado vindo do gui.h
    combo->base.Left = x;
    combo->base.Top = y;
    combo->base.Width = w;
    combo->base.Height = h;
    combo->ItemCount = 0;
    combo->ItemIndex = 0;
    combo->DroppedDown = false;

    // 3. Registro Centralizado no Runtime do App (Gera "ComboBox1", "ComboBox2"...)
    OS_RegisterControl(app, (TOSControl*)combo, "ComboBox");

    // 4. Criação do componente real no subsistema do Kernel usando os 2 argumentos limpos
    combo->base.KernelHandle = (uint64_t)gui_create_combobox((TWinControl*)app->MainWindow, combo->base.Name);
    if (combo->base.KernelHandle == 0) {
        free(combo);
        return NULL;
    }

    // 5. Sincronização inicial de propriedades via Syscall estável
    gui_set_prop((void*)combo->base.KernelHandle, PROP_LEFT,   (uint64_t)combo->base.Left);
    gui_set_prop((void*)combo->base.KernelHandle, PROP_TOP,    (uint64_t)combo->base.Top);
    gui_set_prop((void*)combo->base.KernelHandle, PROP_WIDTH,  (uint64_t)combo->base.Width);
    gui_set_prop((void*)combo->base.KernelHandle, PROP_HEIGHT, (uint64_t)combo->base.Height);

    return combo;
}

void OS_ComboBox_AddItem(TOSComboBox* combo, const char* texto) {
    if (!combo || !texto || combo->ItemCount >= 8) return;

    // Copia o texto de forma segura limitando a 15 caracteres + terminador nulo
    strncpy(combo->Items[combo->ItemCount], texto, 15);
    combo->Items[combo->ItemCount][15] = '\0';

    combo->ItemCount++;

    // Se for o primeiro item, força exibição imediata
    if (combo->ItemCount == 1) {
        combo->ItemIndex = 0;
        OS_ComboBox_UpdateKernelText(combo);
    }
}

void OS_ComboBox_Rotate(TOSComboBox* combo) {
    if (!combo || combo->ItemCount == 0) return;

    // Avança o carrossel (Se estourar o limite de itens inseridos, reinicia em 0)
    combo->ItemIndex = (combo->ItemIndex + 1) % combo->ItemCount;

    // Atualiza a visualização e os contadores no display
    OS_ComboBox_UpdateKernelText(combo);
}

char* OS_ComboBox_GetText(void* combo) {
    TOSControl* ctrl = (TOSControl*)combo;
    if (!ctrl || !ctrl->KernelHandle) return "";
    
    // O KernelHandle aponta direto para a estrutura TComboBox do Kernel
    TComboBox* cb = (TComboBox*)ctrl->KernelHandle;
    if (cb && cb->ItemCount > 0 && cb->ItemIndex >= 0) {
        return cb->Items[cb->ItemIndex];
    }
    return "";
}
