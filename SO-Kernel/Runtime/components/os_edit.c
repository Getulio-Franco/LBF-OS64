#include "os_edit.h"

// Cria o Edit levemente sem as amarras do "Designer"
TOSEdit* OS_CreateEdit(TAppEnvironment* app, int x, int y, int w, int h, const char* text) {
    if (!app) return NULL;

    // Usando malloc padrão em vez de sys_malloc (corrigindo o aviso)
    TOSEdit* edit = (TOSEdit*)malloc(sizeof(TOSEdit));
    if (!edit) return NULL;
    memset(edit, 0, sizeof(TOSEdit)); 

    // Configuração base da API
    edit->base.Type = TYPE_EDIT; // Herdado do gui.h!
    edit->base.Left = x;
    edit->base.Top = y;
    edit->base.Width = w;  
    edit->base.Height = h;  
    edit->MaxLength = 255; 
    edit->CursorPos = 0;

    if (text) {
        strncpy(edit->Text, text, sizeof(edit->Text) - 1);
        edit->CursorPos = strlen(edit->Text);
    } else {
        edit->Text[0] = '\0';
    }

    // Registra o componente no App (gera o Name automaticamente)
    OS_RegisterControl(app, (TOSControl*)edit, "Edit");

    // CRIAÇÃO NO KERNEL: Corrigido para 2 argumentos (Parent e Name) conforme o seu gui.h!
    edit->base.KernelHandle = (uint64_t)gui_create_edit((TWinControl*)app->MainWindow, edit->base.Name);
    
    if (edit->base.KernelHandle == 0) {
        free(edit);
        return NULL;
    }

    // Sincronização via IPC/Syscall com o Kernel
    gui_set_prop((void*)edit->base.KernelHandle, PROP_LEFT,   (uint64_t)edit->base.Left);
    gui_set_prop((void*)edit->base.KernelHandle, PROP_TOP,    (uint64_t)edit->base.Top);
    gui_set_prop((void*)edit->base.KernelHandle, PROP_WIDTH,  (uint64_t)edit->base.Width);
    gui_set_prop((void*)edit->base.KernelHandle, PROP_HEIGHT, (uint64_t)edit->base.Height);
    gui_set_prop((void*)edit->base.KernelHandle, PROP_COLOR,  0xFFFFFF);
    gui_set_prop((void*)edit->base.KernelHandle, PROP_CAPTION,(uintptr_t)edit->Text); // O Texto vai aqui

    return edit;
}

// O motor de digitação permanece praticamente idêntico ao seu, mas agora tipado para TOSEdit
void OS_Edit_AddChar(TOSEdit* edit, char key) {
    if (!edit || !edit->base.KernelHandle) return;

    int len = strlen(edit->Text);

    if (key == 8 || key == '\b') {
        if (len > 0) {
            edit->Text[len - 1] = '\0';
            edit->CursorPos = len - 1;
        }
    } 
    else if (key >= 32 && key <= 126) {
        int limite = (edit->MaxLength > 0 && edit->MaxLength < 256) ? edit->MaxLength : 255;
        if (len < (limite - 1)) {
            edit->Text[len] = key;
            edit->Text[len + 1] = '\0';
            edit->CursorPos = len + 1;
        }
    }

    gui_set_prop((void*)edit->base.KernelHandle, PROP_CAPTION, (uintptr_t)edit->Text);
}

char* OS_Edit_GetText(void* edit) {
    TOSControl* ctrl = (TOSControl*)edit;
    if (!ctrl || !ctrl->KernelHandle) return "";
    
    // Se o seu Kernel guarda o texto diretamente no Caption do objeto:
    TWinControl* win = (TWinControl*)ctrl->KernelHandle;
    return (char*)win->Control.Caption;
}

void OS_Edit_SetText(void* edit, const char* text) {
    TOSControl* ctrl = (TOSControl*)edit;
    if (!ctrl || !ctrl->KernelHandle) return;
    
    // Atualiza o texto via syscall de propriedade
    gui_set_prop((void*)ctrl->KernelHandle, PROP_CAPTION, (uintptr_t)text);
}

void OS_Edit_SetFocus(TOSEdit* edit) {
    if (!edit || !edit->base.KernelHandle) return;

    // Via syscall segura de propriedade, ordena que o Kernel mude o g_focused_control.
    // Como passamos 1 (true), o Kernel sabe que deve ativar o foco nele.
    gui_set_prop((void*)edit->base.KernelHandle, PROP_SET_FOCUS, 1);
}
