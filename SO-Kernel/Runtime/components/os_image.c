#include "os_image.h"

TOSControl* OS_CreateImage(TAppEnvironment* app, int x, int y, int w, int h, const char* path) {
    if (!app) return NULL;

    // 1. Alocação e limpeza da estrutura base unificada (Substitui TImage/TVCLControl)
    TOSControl* img = (TOSControl*)malloc(sizeof(TOSControl));
    if (!img) return NULL;
    memset(img, 0, sizeof(TOSControl));

    // 2. Configuração de Atributos Locais e Geometria do Runtime
    img->Type = TYPE_IMAGE; // Identificador nativo vindo do gui.h
    img->Left = x;
    img->Top = y;
    img->Width = w;
    img->Height = h;
    img->IsSelected = false;

    // 3. Registro Centralizado no Runtime do App (Gera "Image1", "Image2"...)
    OS_RegisterControl(app, img, "Image");

    // 4. Criação no subsistema gráfico do Kernel usando os 2 argumentos limpos (Parent e Name)
    img->KernelHandle = (uint64_t)gui_create_image((TWinControl*)app->MainWindow, img->Name);
    
    if (img->KernelHandle == 0) {
        // Fallback de segurança se o Kernel falhar ao instanciar o widget
        free(img);
        return NULL;
    }

    // 5. Define o caminho do arquivo de imagem no Kernel via PROP_CAPTION (se fornecido)
    if (path && path[0] != '\0') {
        gui_set_prop((void*)img->KernelHandle, PROP_CAPTION, (uintptr_t)path);
    }

    // 6. Sincronização de Geometria com o subsistema do Kernel usando a syscall estável
    gui_set_prop((void*)img->KernelHandle, PROP_LEFT,   (uint64_t)img->Left);
    gui_set_prop((void*)img->KernelHandle, PROP_TOP,    (uint64_t)img->Top);
    gui_set_prop((void*)img->KernelHandle, PROP_WIDTH,  (uint64_t)img->Width);
    gui_set_prop((void*)img->KernelHandle, PROP_HEIGHT, (uint64_t)img->Height);

    return img;
}
