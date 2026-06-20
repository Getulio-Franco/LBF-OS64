#include "os_checkbox.h"

TOSCheckBox* OS_CreateCheckBox(TAppEnvironment* app, int x, int y, const char* caption) {
    if (!app) return NULL;

    // 1. Alocação e limpeza da memória da estrutura estendida
    TOSCheckBox* cb = (TOSCheckBox*)malloc(sizeof(TOSCheckBox));
    if (!cb) return NULL;
    memset(cb, 0, sizeof(TOSCheckBox));

    // 2. Configuração de Atributos Locais e Geometria do Runtime
    cb->base.Type = TYPE_CHECKBOX; 
    cb->base.Left = x;
    cb->base.Top = y;
    cb->base.Width = 140;          
    cb->base.Height = 20;
    cb->Checked = false;           // Estado padrão inicial (Falso/Desmarcado)
    cb->base.IsSelected = false;

    // 3. Registro Centralizado no Runtime do App
    OS_RegisterControl(app, (TOSControl*)cb, "CheckBox");

    // 4. Criação no subsistema do Kernel
    cb->base.KernelHandle = (uint64_t)gui_create_checkbox((TWinControl*)app->MainWindow, cb->base.Name, (char*)caption);
    
    if (cb->base.KernelHandle == 0) {
        free(cb);
        return NULL;
    }

    // 5. Sincronização inicial de Geometria via Syscall gui_set_prop
    gui_set_prop((void*)cb->base.KernelHandle, PROP_LEFT,   (uint64_t)cb->base.Left);
    gui_set_prop((void*)cb->base.KernelHandle, PROP_TOP,    (uint64_t)cb->base.Top);
    gui_set_prop((void*)cb->base.KernelHandle, PROP_WIDTH,  (uint64_t)cb->base.Width);
    gui_set_prop((void*)cb->base.KernelHandle, PROP_HEIGHT, (uint64_t)cb->base.Height);
    
    // NOTA: O Kernel já inicia o CheckBox como "false" por padrão ao criar!
    // Se no futuro você definir PROP_CHECKED no gui.h, basta descomentar a linha abaixo:
    // gui_set_prop((void*)cb->base.KernelHandle, PROP_CHECKED, (uint64_t)cb->Checked);

    return cb;
}

void OS_CheckBox_Toggle(TOSCheckBox* cb) {
    if (!cb || !cb->base.KernelHandle) return;

    // Inverte o estado booleano interno no Ring 3 (O App sabe se está marcado ou não)
    cb->Checked = !cb->Checked;

    // Se o seu Kernel precisar ser avisado da mudança visual, usamos a propriedade genérica de estado.
    // Caso o próprio kernel alterne o visual ao receber o clique do mouse, essa linha abaixo se torna opcional!
    #ifdef PROP_STATE
    gui_set_prop((void*)cb->base.KernelHandle, PROP_STATE, (uint64_t)cb->Checked);
    #endif
}
