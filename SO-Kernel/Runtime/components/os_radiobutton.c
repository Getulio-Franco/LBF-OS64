#include "os_radiobutton.h"

// Função externa do kernel já mapeada no seu ecossistema
extern void* gui_create_radio(void* parent, const char* name, const char* caption);

TOSRadioButton* OS_CreateRadioButton(TAppEnvironment* app, int x, int y, const char* caption) {
    if (!app) return NULL;

    // 1. Alocação e limpeza da estrutura estendida
    TOSRadioButton* rb = (TOSRadioButton*)malloc(sizeof(TOSRadioButton));
    if (!rb) return NULL;
    memset(rb, 0, sizeof(TOSRadioButton));

    // 2. Configuração de Atributos Locais e Geometria do Runtime
    rb->base.Type = TYPE_RADIOBUTTON; // Mapeado no seu gui.h
    rb->base.Left = x;
    rb->base.Top = y;
    rb->base.Width = 120;
    rb->base.Height = 20;
    rb->base.IsSelected = false;
    rb->Checked = false;
    rb->GroupIndex = 0; // Grupo padrão inicial

    // 3. Registro Centralizado no Runtime do App (Gera "RadioButton1", "RadioButton2"...)
    OS_RegisterControl(app, (TOSControl*)rb, "RadioButton");

    // 4. Criação no subsistema do Kernel usando a assinatura estável
    rb->base.KernelHandle = (uint64_t)gui_create_radio((TWinControl*)app->MainWindow, rb->base.Name, (char*)caption);
    
    if (rb->base.KernelHandle == 0) {
        free(rb);
        return NULL;
    }

    // 5. Sincronização de Geometria e Aparência via Syscall gui_set_prop
    gui_set_prop((void*)rb->base.KernelHandle, PROP_LEFT,   (uint64_t)rb->base.Left);
    gui_set_prop((void*)rb->base.KernelHandle, PROP_TOP,    (uint64_t)rb->base.Top);
    gui_set_prop((void*)rb->base.KernelHandle, PROP_WIDTH,  (uint64_t)rb->base.Width);
    gui_set_prop((void*)rb->base.KernelHandle, PROP_HEIGHT, (uint64_t)rb->base.Height);
    
    // Configura a cor de fundo padrão (Cinza/Padrão de Janela)
    gui_set_prop((void*)rb->base.KernelHandle, PROP_COLOR,  0xC0C0C0);

    return rb;
}

void OS_RadioButton_Select(TAppEnvironment* app, TOSRadioButton* target_rb) {
    if (!app || !target_rb || !target_rb->base.KernelHandle) return;

    // Se já estiver marcado, não precisa reprocessar o grupo
    if (target_rb->Checked) return;

    // Varre todos os controles registrados no App para limpar o grupo
    for (int i = 0; i < app->ControlCount; i++) {
        TOSControl* ctrl = app->Controls[i];

        if (ctrl && ctrl->Type == TYPE_RADIOBUTTON) {
            TOSRadioButton* current_rb = (TOSRadioButton*)ctrl;

            // Pertence ao mesmo grupo de exclusão mútua?
            if (current_rb->GroupIndex == target_rb->GroupIndex) {
                current_rb->Checked = false;
                
                // Se o seu gui.h tiver PROP_CHECKED/PROP_STATE definido no futuro, avisa o Kernel.
                // Caso contrário, o próprio Kernel gerencia o desmarcar visualmente.
                #ifdef PROP_CHECKED
                gui_set_prop((void*)current_rb->base.KernelHandle, PROP_CHECKED, 0);
                #endif
            }
        }
    }

    // Marca o botão alvo como ativo
    target_rb->Checked = true;

    #ifdef PROP_CHECKED
    gui_set_prop((void*)target_rb->base.KernelHandle, PROP_CHECKED, 1);
    #endif
}
