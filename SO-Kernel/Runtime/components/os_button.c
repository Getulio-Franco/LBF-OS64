#include "os_button.h"

TOSControl* OS_CreateButton(TAppEnvironment* app, int x, int y, int w, int h, const char* caption) {
    if (!app) return NULL;

    // 1. Aloca e limpa a estrutura base unificada
    TOSControl* btn = (TOSControl*)malloc(sizeof(TOSControl));
    if (!btn) return NULL;
    memset(btn, 0, sizeof(TOSControl)); 

    // 2. Define as propriedades geométricas baseadas nos parâmetros passados
    btn->Type = TYPE_BUTTON; 
    btn->Left = x;
    btn->Top = y;
    btn->Width = w;          // <-- Agora usa o valor dinâmico enviado por você!
    btn->Height = h;         // <-- Agora usa o valor dinâmico enviado por você!
    btn->IsSelected = false;

    // 3. Registra o controle no App
    OS_RegisterControl(app, btn, "Button");

    // 4. Cria o componente visual no subsistema gráfico do Kernel
    btn->KernelHandle = (uint64_t)gui_create_button((TWinControl*)app->MainWindow, btn->Name, (char*)caption);
    
    if (btn->KernelHandle == 0) {
        free(btn);
        return NULL;
    }

    // 5. Sincroniza a geometria REAL e dinâmica com o objeto do Kernel
    gui_set_prop((void*)btn->KernelHandle, PROP_LEFT,   (uint64_t)btn->Left);
    gui_set_prop((void*)btn->KernelHandle, PROP_TOP,    (uint64_t)btn->Top);
    gui_set_prop((void*)btn->KernelHandle, PROP_WIDTH,  (uint64_t)btn->Width); // Aplica a largura correta
    gui_set_prop((void*)btn->KernelHandle, PROP_HEIGHT, (uint64_t)btn->Height); // Aplica a altura correta

    return btn;
}
