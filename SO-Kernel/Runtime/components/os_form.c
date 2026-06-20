#include "os_form.h"
#include "../system/malloc.h"
#include "../system/string.h"
#include "../gui/wm.h"

TOSForm* OS_CreateForm(const char* title, int x, int y, int width, int height) {
    // 1. Aloca e limpa a estrutura de controle do Formulário em Ring 3
    TOSForm* vform = (TOSForm*)malloc(sizeof(TOSForm));
    if (!vform) return NULL;
    memset(vform, 0, sizeof(TOSForm));

    // 2. Configura as propriedades geométricas locais de Runtime
    vform->Left = x;
    vform->Top = y;
    vform->Width = width;
    vform->Height = height;
    vform->Modal = false;

    // Define uma string de fallback segura caso o título seja nulo
    const char* window_title = (title && title[0] != '\0') ? title : "Form";

    // 3. Invoca o Kernel usando o tipo correto TForm* do seu subsistema
    TForm* kernel_win = gui_create_form("SubForm", (char*)window_title, 1);
    
    if (!kernel_win) {
        free(vform);
        return NULL;
    }
    
    // Guarda o ponteiro real de 64 bits convertido para uint64_t
    vform->FormHandle = (uint64_t)kernel_win;

    // 4. Sincroniza a geometria inicial com a GUI do Kernel via Syscall gui_set_prop
    gui_set_prop((void*)kernel_win, PROP_LEFT,   (uint64_t)vform->Left);
    gui_set_prop((void*)kernel_win, PROP_TOP,    (uint64_t)vform->Top);
    gui_set_prop((void*)kernel_win, PROP_WIDTH,  (uint64_t)vform->Width);
    gui_set_prop((void*)kernel_win, PROP_HEIGHT, (uint64_t)vform->Height);

    // 5. Configura o arrasto usando o membro correto 'Win'
    kernel_win->Win.Draggable = true;
    
    // 6. Adiciona ao Window Manager passando o TForm* nativo esperado
    wm_add_window(kernel_win);

    return vform;
}

void OS_DestroyForm(TOSForm* form) {
    if (!form) return;
    free(form);
}

/*
🧠 Como essa trinca se encaixa perfeitamente agora:
Janela Inicial do App: Quando o programa principal inicializa, ele chama OS_InitApplication. Essa função cria automaticamente o MainWindow do aplicativo (que é o Form Raiz).

Componentes Filhos: Todos os componentes básicos que portamos (OS_CreateEdit, OS_CreateButton, etc.) usam o app->MainWindow como pai (Parent) e se acoplam perfeitamente a ele.

Telas de Senha / Modais adicionais: Se antes do seu app abrir você precisar de uma tela de login ou formulário acessório, a sua aplicação pode simplesmente rodar:


TOSForm* login_form = OS_CreateForm("Segurança: Digite a Senha", 200, 200, 300, 150);

// Cria um edit e um botão apontando para o login_form->FormHandle...
Com essa última peça, você tem um ecossistema completo de interface gráfica, orientado a objetos em C pura, extremamente leve e desacoplado do "Designer" da IDE. Pronto para compilar no Kernel ou como binário nativo!
*/
