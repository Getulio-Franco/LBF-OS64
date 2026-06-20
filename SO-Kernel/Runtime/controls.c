#include "controls.h"
#include "../system/malloc.h" 
#include "../system/string.h"
#include "../system/graphics.h"
#include "../gui/wm.h"

// Registra um controle no contexto do aplicativo (Usa o TOSControl definitivo)
void OS_RegisterControl(TAppEnvironment* app, TOSControl* ctrl, const char* prefix) {
    if (!app || !ctrl) return;
    
    if (app->ControlCount < MAX_APP_CONTROLS) {
        // Gera um nome automático incremental seguro (Ex: Edit1, Memo2...)
        int id = app->ControlCount + 1;
        char id_str[4];
        id_str[0] = '0' + (id % 10);
        id_str[1] = '\0';
        
        strcpy(ctrl->Name, prefix);
        strcat(ctrl->Name, id_str);

        app->Controls[app->ControlCount++] = ctrl;
    }
}

void OS_InitApplication(TAppEnvironment* app, int slot_id, const char* title, int w, int h) {
    app->SlotID = slot_id;
    app->ControlCount = 0;
    app->ActiveFocus = NULL;
    
    // Cria a janela principal do programa através da API do Kernel
    app->MainWindow = gui_create_form("MainApp", (char*)title, 1);
    gui_set_prop(app->MainWindow, PROP_LEFT, 0);
    gui_set_prop(app->MainWindow, PROP_TOP, 0);
    gui_set_prop(app->MainWindow, PROP_WIDTH, w);
    gui_set_prop(app->MainWindow, PROP_HEIGHT, h);
    
    // Configura a janela para ser arrastável pelo Window Manager do S.O.
    app->MainWindow->Win.Draggable = true;
    wm_add_window(app->MainWindow);
}

// =========================================================================
// O MOTOR DE EVENTOS (Tratando TOSControl e KernelHandle)
// =========================================================================

bool OS_ProcessMouseClick(TAppEnvironment* app, int mouse_x, int mouse_y) {
    if (!app) return false;
    
    app->ActiveFocus = NULL; // Reseta o foco do teclado antes de avaliar

    // Varre os controles do Runtime para ver quem foi atingido pelas coordenadas
    for (int i = 0; i < app->ControlCount; i++) {
        TOSControl* ctrl = app->Controls[i];
        
        // Verifica se o clique do mouse ocorreu dentro dos limites geométricos do componente
        if (mouse_x >= ctrl->Left && mouse_x <= (ctrl->Left + ctrl->Width) &&
            mouse_y >= ctrl->Top && mouse_y <= (ctrl->Top + ctrl->Height)) {
            
            // 1. Tratamento nativo do ComboBox (Gira a roleta usando a função segura do componente)
            if (ctrl->Type == TYPE_COMBOBOX) {
                extern void OS_ComboBox_Rotate(void* combo);
                OS_ComboBox_Rotate((void*)ctrl);
                return true; // Interceptado! Indica que a lib já resolveu o clique
            }
            
            // 2. Tratamento nativo do CheckBox (Alterna o estado interno/visual)
            if (ctrl->Type == TYPE_CHECKBOX) {
                extern void OS_CheckBox_Toggle(void* cb);
                OS_CheckBox_Toggle((void*)ctrl);
                return true; 
            }

            // 3. Tratamento nativo do RadioButton (Aplica exclusão mútua no grupo)
            if (ctrl->Type == TYPE_RADIOBUTTON) {
                extern void OS_RadioButton_Select(TAppEnvironment* app, void* rb);
                OS_RadioButton_Select(app, (void*)ctrl);
                return true;
            }
            
            // 4. Se for Edit ou Memo, captura o ponteiro do foco para digitação posterior
            if (ctrl->Type == TYPE_EDIT || ctrl->Type == TYPE_MEMO) {
                app->ActiveFocus = ctrl;
                return true;
            }

            // 5. Imagem ou label passam direto
            if (ctrl->Type == TYPE_IMAGE || ctrl->Type == TYPE_LABEL) {
                return false;
            }
        }
    }
    return false;
}

void OS_ProcessKeyboard(TAppEnvironment* app, char key) {
    if (!app || !app->ActiveFocus || key == 0) return;

    TOSControl* f_ctrl = app->ActiveFocus;

    // Se o controle focado for do tipo EDIT, redireciona o caractere para o os_edit
    if (f_ctrl->Type == TYPE_EDIT) {
        extern void OS_Edit_AddChar(void* edit, char key);
        OS_Edit_AddChar((void*)f_ctrl, key);
    } 
    // Se o controle focado for do tipo MEMO, redireciona para o os_memo
    else if (f_ctrl->Type == TYPE_MEMO) {
        extern void OS_Memo_AddChar(void* memo, char key);
        OS_Memo_AddChar((void*)f_ctrl, key);
    }
}
