#include "os_panel.h"
#include "../gui/gui.h"

TOSControl* OS_CreatePanel(TAppEnvironment* app, int x, int y, int w, int h) {
    if (!app) return NULL;

    // 1. Alocação e limpeza da estrutura estendida do Container em Ring 3
    TOSPanel* pnl = (TOSPanel*)malloc(sizeof(TOSPanel));
    if (!pnl) return NULL;
    memset(pnl, 0, sizeof(TOSPanel));

    // 2. Configuração de Atributos locais e Geometria de Runtime
    pnl->base.Type = TYPE_PANEL; // Valor 3 conforme a sua nova tabela!
    pnl->base.Left = x;
    pnl->base.Top = y;
    pnl->base.Width = w;
    pnl->base.Height = h;
    pnl->base.IsSelected = false;
    pnl->BevelWidth = 2;         // Espessura padrão do chanfro visual

    // 3. Registro Centralizado no Runtime do App (Gera "Panel1", "Panel2"...)
    OS_RegisterControl(app, (TOSControl*)pnl, "Panel");

    // 4. Criação no subsistema do Kernel (Mapeado de acordo com seu gui.h)
    pnl->base.KernelHandle = (uint64_t)gui_create_panel((TWinControl*)app->MainWindow, pnl->base.Name);
    
    if (pnl->base.KernelHandle == 0) {
        free(pnl);
        return NULL;
    }

    // 5. Sincronização de Geometria e Estado com o Kernel via Syscall gui_set_prop
    gui_set_prop((void*)pnl->base.KernelHandle, PROP_LEFT,   (uint64_t)pnl->base.Left);
    gui_set_prop((void*)pnl->base.KernelHandle, PROP_TOP,    (uint64_t)pnl->base.Top);
    gui_set_prop((void*)pnl->base.KernelHandle, PROP_WIDTH,  (uint64_t)pnl->base.Width);
    gui_set_prop((void*)pnl->base.KernelHandle, PROP_HEIGHT, (uint64_t)pnl->base.Height);
    
    // Injeta a cor cinza clássica (0xC0C0C0) diretamente no objeto do Kernel
    gui_set_prop((void*)pnl->base.KernelHandle, PROP_COLOR,  0xC0C0C0);

    // Se o seu gui.h possuir uma propriedade para chanfros (ex: PROP_BEVEL), aplique aqui:
    #ifdef PROP_BEVEL
    gui_set_prop((void*)pnl->base.KernelHandle, PROP_BEVEL,  (uint64_t)pnl->BevelWidth);
    #endif

    return (TOSControl*)pnl;
}

void OS_SetParent(TOSControl* control, TOSControl* new_parent) {
    if (!control || !control->KernelHandle || !new_parent || !new_parent->KernelHandle) return;

    // 1. Vincula o controle ao novo pai diretamente no subsistema do Kernel
    // O Kernel vai reordenar a árvore de desenho para renderizar este controle dentro do painel
    gui_add_to_parent((TWinControl*)new_parent->KernelHandle, (TControl*)control->KernelHandle);

    // 2. Se o seu TOSControl tiver um ponteiro ou ID guardando o pai no Ring 3, 
    // você pode atualizar aqui. Exemplo conceitual: control->ParentHandle = new_parent->KernelHandle;
}
