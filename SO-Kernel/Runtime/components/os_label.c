#include "os_label.h"

TOSControl* OS_CreateLabel(TAppEnvironment* app, int x, int y, const char* caption) {
    if (!app) return NULL;

    // 1. Alocação e limpeza da estrutura base unificada
    TOSControl* lbl = (TOSControl*)malloc(sizeof(TOSControl));
    if (!lbl) return NULL;
    memset(lbl, 0, sizeof(TOSControl));

    // 2. Configuração de Atributos Locais e Geometria do Runtime
    lbl->Type = TYPE_LABEL; // Identificador nativo vindo do gui.h
    lbl->Left = x;
    lbl->Top = y;
    lbl->Width = 80;        // Largura padrão para evitar clipagem de textos comuns
    lbl->Height = 16;       // Altura ideal para a fonte padrão do Kernel
    lbl->IsSelected = false;

    // 3. Registro Centralizado no Ambiente do App (Gera "Label1", "Label2"...)
    OS_RegisterControl(app, lbl, "Label");

    // 4. Criação do componente no subsistema gráfico do Kernel
    // Passamos a janela principal (MainWindow) e o Name gerado automaticamente no passo 3
    lbl->KernelHandle = (uint64_t)gui_create_label((TWinControl*)app->MainWindow, lbl->Name, (char*)caption);
    
    if (lbl->KernelHandle == 0) {
        // Fallback de segurança caso a alocação de widget no Kernel falhe
        free(lbl);
        return NULL;
    }

    // 5. Sincronização de Geometria via Syscall gui_set_prop (Ponteiros limpos de 64 bits)
    gui_set_prop((void*)lbl->KernelHandle, PROP_LEFT,   (uint64_t)lbl->Left);
    gui_set_prop((void*)lbl->KernelHandle, PROP_TOP,    (uint64_t)lbl->Top);
    gui_set_prop((void*)lbl->KernelHandle, PROP_WIDTH,  (uint64_t)lbl->Width);
    gui_set_prop((void*)lbl->KernelHandle, PROP_HEIGHT, (uint64_t)lbl->Height);

    return lbl;
}
