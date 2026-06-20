#include "controls.h"
#include "../system/graphics.h"
#include "../gui/wm.h"
#include "../system/string.h"
#include "../system/liblib.h"

// Componentes do Sistema encapsulados (IPC / Hardware)
#include "components/TOS_IPC.h"     
#include "components/TOSSerial.h"   

// Biblioteca de Componentes Visuais do Ecossistema do S.O.
#include "components/os_panel.h"       
#include "components/os_label.h"
#include "components/os_button.h"
#include "components/os_edit.h"
#include "components/os_combobox.h"
#include "components/os_memo.h"
#include "components/os_checkbox.h"
#include "components/os_radiobutton.h"
#include "components/os_image.h"

// Protótipos obrigatórios de renderização gráfica
void gui_draw_form(TForm* form);
void gui_render_form(TForm* form);

// Inclusão de funções de controle mapeadas diretamente no subsistema do Kernel
extern void events_process_mouse(int x, int y, int pressed, int button);
extern void gui_add_to_parent(TWinControl* parent, TControl* child);

// Utilitários de dados dos componentes visuais do Runtime
extern char* OS_Edit_GetText(void* edit);
extern void  OS_Edit_SetText(void* edit, const char* text);
extern char* OS_ComboBox_GetText(void* combo);

// Variáveis de controle do ambiente da aplicação
int my_app_slot = -1;
TAppEnvironment MyApp;

int main(int argc, char* argv[]) {
    int winWidth = 500, winHeight = 360;

    // Variáveis de controle para o motor de clique assíncrono (Debounce/Timer)
    static int ultimo_x = 0;
    static int ultimo_y = 0;
    static int mouse_hold_timer = 0; 

    // =========================================================================
    // 1. INICIALIZAÇÃO DO RUNTIME E REGISTRO DA JANELA
    // =========================================================================
    graphics_init_app(winWidth, winHeight);
    wm_init();
    
    my_app_slot = OS_IPC_RegisterApp("Nome do Software Template", winWidth, winHeight);
    if (my_app_slot == -1) return -1; 
    
    graphics_set_slot(my_app_slot);
    OS_InitApplication(&MyApp, my_app_slot, "Nome do Software Template", winWidth, winHeight);

    // =========================================================================
    // 2. DECLARAÇÃO DE HARDWARE / COMPONENTES DE SISTEMA
    // =========================================================================
    TOSSerial* serial = OS_CreateSerial(1, 9600);

    // =========================================================================
    // 3. PALETA DE COMPONENTES VISUAIS (DESIGN LAYOUT RAD - ABSOLUTO)
    // =========================================================================
    
    // --- COLUNA 1: Interação e Entrada de Dados (Esquerda) ---
    TOSControl* ExeButton    = OS_CreateButton(&MyApp, 10, 40, 220, 30, "Exemplo de TButton");
    TOSControl* ExeLabel     = OS_CreateLabel(&MyApp, 10, 85, "Exemplo de TLabel (Texto Estático):");
    TOSEdit* ExeEdit         = (TOSEdit*)OS_CreateEdit(&MyApp, 10, 105, 220, 25, "Texto do TEdit");
    
    TOSControl* ExeLabelCb   = OS_CreateLabel(&MyApp, 10, 145, "Exemplo de TComboBox:");
    TOSComboBox* ExeComboBox = (TOSComboBox*)OS_CreateComboBox(&MyApp, 10, 165, 220, 25);
    OS_ComboBox_AddItem(ExeComboBox, "Item Opção A");
    OS_ComboBox_AddItem(ExeComboBox, "Item Opção B");

    // --- COLUNA 2: Seletor de Estados (Direita) ---
    TOSCheckBox* ExeCheckBox = OS_CreateCheckBox(&MyApp, 250, 40, "Exemplo de TCheckBox");
    ExeCheckBox->Checked = false;

    TOSRadioButton* ExeRadio1 = OS_CreateRadioButton(&MyApp, 250, 75, "Radio Opção 1");
    ExeRadio1->GroupIndex = 1;
    ExeRadio1->Checked = true;  

    TOSRadioButton* ExeRadio2 = OS_CreateRadioButton(&MyApp, 250, 105, "Radio Opção 2");
    ExeRadio2->GroupIndex = 1;
    ExeRadio2->Checked = false; 
    
    // --- BOTÃO DE FECHAMENTO ---
    TOSControl* CloseButton  = OS_CreateButton(&MyApp, 250, 155, 220, 30, "Fechar Software");
    
    // --- ÁREA INFERIOR: Exibição de Mensagens e Logs ---
    TOSControl* ExeLabelMm   = OS_CreateLabel(&MyApp, 10, 205, "Exemplo de TMemo (Terminal / Log):");
    TOSMemo* ExeMemo         = (TOSMemo*)OS_CreateMemo(&MyApp, 10, 225, 470, 95);
    OS_Memo_AddStr(ExeMemo, "Base estável carregada. Pronto para novos softwares!\n");

    // =========================================================================
    // 4. LOOP PRINCIPAL DE EVENTOS DO SISTEMA OPERACIONAL
    // =========================================================================
    while(1) {
        bool euTenhoFoco = (IPC_CONTROL->active_focus_slot == my_app_slot);

        if (MyApp.MainWindow) {
            MyApp.MainWindow->ActiveFocus = euTenhoFoco;
        }

        // CORREÇÃO CRÍTICA: Se o Explorer ou outro app fechar esta janela externamente,
        // o slot fica inativo. O app deve sair do loop imediatamente para evitar falhas de segurança.
        if (IPC_WINDOW_LIST[my_app_slot].is_active == 0) {
            break;
        }

        if (euTenhoFoco) {
            char key = get_key();
            OS_ProcessKeyboard(&MyApp, key); 

            // --- MOUSE DOWN: PROCESSAMENTO DE ENTRADA GRÁFICA ---
            if (IPC_WINDOW_LIST[my_app_slot].has_click_event == 1) {
                
                if (mouse_hold_timer > 0) {
                    IPC_WINDOW_LIST[my_app_slot].has_click_event = 0;
                    goto skip_mouse_processing; 
                }

                int rel_x = IPC_WINDOW_LIST[my_app_slot].local_click_x;
                int rel_y = IPC_WINDOW_LIST[my_app_slot].local_click_y;
                
                ultimo_x = rel_x;
                ultimo_y = rel_y;
                mouse_hold_timer = 6; 

                // Processamento padrão do clique nativo direto na janela do Kernel
                events_process_mouse(rel_x, rel_y, 1, 0);

                // --- CAPTURA DE FOCO INTERNO MANUAL (EDITS / MEMOS) ---
                if (rel_x >= ExeEdit->base.Left && rel_x < (ExeEdit->base.Left + ExeEdit->base.Width) &&
                    rel_y >= ExeEdit->base.Top && rel_y < (ExeEdit->base.Top + ExeEdit->base.Height)) {
                    OS_Edit_SetFocus(ExeEdit);
                }
                else if (rel_x >= ExeMemo->base.Left && rel_x < (ExeMemo->base.Left + ExeMemo->base.Width) &&
                         rel_y >= ExeMemo->base.Top && rel_y < (ExeMemo->base.Top + ExeMemo->base.Height)) {
                    
                    int limite_barra_x = ExeMemo->base.Left + ExeMemo->base.Width - 18;
                    if (rel_x >= limite_barra_x) {
                        int metade_vertical = ExeMemo->base.Top + (ExeMemo->base.Height / 2);
                        if (rel_y < metade_vertical) {
                            OS_Memo_SetScroll(ExeMemo, ExeMemo->ScrollY - 2); 
                        } else {
                            OS_Memo_SetScroll(ExeMemo, ExeMemo->ScrollY + 2); 
                        }
                    } else {
                        OS_Memo_SetFocus(ExeMemo);
                    }
                }
                else {
                    gui_set_prop(NULL, PROP_SET_FOCUS, 0); 
                }

                // Processador base secundário da biblioteca
                bool resolvido_pela_lib = OS_ProcessMouseClick(&MyApp, rel_x, rel_y);

                // =========================================================================
                // GESTÃO DE EVENTOS DE CLIQUE DOS COMPONENTES
                // =========================================================================
                
                // Evento: Clicou no Botão de Exemplo
                if (rel_x >= ExeButton->Left && rel_x < (ExeButton->Left + ExeButton->Width) &&
                    rel_y >= ExeButton->Top && rel_y < (ExeButton->Top + ExeButton->Height)) {
                    
                    gui_set_prop((void*)ExeButton->KernelHandle, PROP_STATE, 2); 
                    OS_Memo_AddStr(ExeMemo, "Botao principal clicado!\n");
                }
                
                // Evento: Clicou no Botão de Fechamento do Software
                if (rel_x >= CloseButton->Left && rel_x < (CloseButton->Left + CloseButton->Width) &&
                    rel_y >= CloseButton->Top && rel_y < (CloseButton->Top + CloseButton->Height)) {
                    
                    gui_set_prop((void*)CloseButton->KernelHandle, PROP_STATE, 2); 
                    OS_Memo_AddStr(ExeMemo, "Solicitando encerramento via IPC...\n");
                    
                    // Oculta o formulário visualmente no subsistema local antes de matar
                    if (MyApp.MainWindow) {
                        MyApp.MainWindow->Win.Control.Visible = false;
                        gui_set_prop((void*)MyApp.MainWindow, PROP_VISIBLE, 0);
                    }

                    // Limpa cirurgicamente os buffers de vídeo compartilhados para evitar artefatos (fantasmas)
                    uint32_t* b0 = (uint32_t*)(uintptr_t)IPC_WINDOW_LIST[my_app_slot].buffer_ptr_0;
                    uint32_t* b1 = (uint32_t*)(uintptr_t)IPC_WINDOW_LIST[my_app_slot].buffer_ptr_1;
                    if (b0) memset(b0, 0, winWidth * winHeight * 4);
                    if (b1) memset(b1, 0, winWidth * winHeight * 4);

                    // COMUNICAÇÃO IPC: Modifica o estado do slot. O Explorer detectará instantaneamente.
                    IPC_WINDOW_LIST[my_app_slot].is_active = 0;

                    // MARGEM DE SEGURANÇA SEGUINTE (50ms): Dá tempo pro escalonador chavear pro Explorer 
                    // limpar a barra de tarefas e remover o contêiner completamente.
                    sys_sleep(50); 
                    
                    break; 
                }

                // Evento: ComboBox
                TOSControl* cbCtrl = (TOSControl*)ExeComboBox;
                if (rel_x >= cbCtrl->Left && rel_x < (cbCtrl->Left + cbCtrl->Width) &&
                    rel_y >= cbCtrl->Top && rel_y < (cbCtrl->Top + cbCtrl->Height)) {
                    OS_Memo_AddStr(ExeMemo, "ComboBox alterado.\n");
                }

                // Evento: CheckBox
                if (rel_x >= ExeCheckBox->base.Left && rel_x < (ExeCheckBox->base.Left + ExeCheckBox->base.Width) &&
                    rel_y >= ExeCheckBox->base.Top && rel_y < (ExeCheckBox->base.Top + ExeCheckBox->base.Height)) {
                    
                    if (ExeCheckBox->Checked) {
                        OS_Memo_AddStr(ExeMemo, "CheckBox: Habilitado (True)\n");
                    } else {
                        OS_Memo_AddStr(ExeMemo, "CheckBox: Desabilitado (False)\n");
                    }
                    gui_set_prop((void*)ExeCheckBox->base.KernelHandle, PROP_VISIBLE, true);
                }

                // Eventos: RadioButtons
                if (rel_x >= ExeRadio1->base.Left && rel_x < (ExeRadio1->base.Left + ExeRadio1->base.Width) &&
                    rel_y >= ExeRadio1->base.Top && rel_y < (ExeRadio1->base.Top + ExeRadio1->base.Height)) {
                    OS_Memo_AddStr(ExeMemo, "Modo de Operacao alterado: Opcao 1 Ativa\n");
                    gui_set_prop((void*)ExeRadio1->base.KernelHandle, PROP_VISIBLE, true);
                    gui_set_prop((void*)ExeRadio2->base.KernelHandle, PROP_VISIBLE, true);
                }

                if (rel_x >= ExeRadio2->base.Left && rel_x < (ExeRadio2->base.Left + ExeRadio2->base.Width) &&
                    rel_y >= ExeRadio2->base.Top && rel_y < (ExeRadio2->base.Top + ExeRadio2->base.Height)) {
                    OS_Memo_AddStr(ExeMemo, "Modo de Operacao alterado: Opcao 2 Ativa\n");
                    gui_set_prop((void*)ExeRadio1->base.KernelHandle, PROP_VISIBLE, true);
                    gui_set_prop((void*)ExeRadio2->base.KernelHandle, PROP_VISIBLE, true);
                }

                IPC_WINDOW_LIST[my_app_slot].has_click_event = 0;
            }

skip_mouse_processing:

            // --- MOUSE UP ASÍNCRONO ---
            if (mouse_hold_timer > 0) {
                mouse_hold_timer--; 
                if (mouse_hold_timer == 0) {
                    gui_set_prop((void*)ExeButton->KernelHandle, PROP_STATE, 0);    
                    gui_set_prop((void*)CloseButton->KernelHandle, PROP_STATE, 0); 
                    events_process_mouse(ultimo_x, ultimo_y, 0, 0); 
                }
            }
        }

        // =========================================================================
        // 5. ATUALIZAÇÃO DO BUFFER GRÁFICO (60 FPS / ECO)
        // =========================================================================
        gui_draw_form(MyApp.MainWindow);
        gui_render_form(MyApp.MainWindow);
        
        int back_idx = (IPC_WINDOW_LIST[my_app_slot].active_buffer == 0) ? 1 : 0;
        uint8_t* shared_ptr = (back_idx == 0) 
            ? (uint8_t*)(uintptr_t)IPC_WINDOW_LIST[my_app_slot].buffer_ptr_0 
            : (uint8_t*)(uintptr_t)IPC_WINDOW_LIST[my_app_slot].buffer_ptr_1;
        
        uint8_t* local_ptr = graphics_get_buffer();
        if (shared_ptr && local_ptr) {
            memcpy(shared_ptr, local_ptr, winWidth * winHeight * 4); 
        }

        IPC_WINDOW_LIST[my_app_slot].active_buffer = back_idx;
        sys_sleep(euTenhoFoco ? 16 : 100);
    }

    // O grand finale definitivo: chama a interrupção de término do Kernel 
    // com a integridade da interface totalmente assegurada.
    sys_exit(); 
    return 0;
}
