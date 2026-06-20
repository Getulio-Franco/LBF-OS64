#include "controls.h"                  // O motor de eventos e estrutura básica
#include "../system/graphics.h"
#include "components/os_panel.h"       
#include "components/os_label.h"
#include "components/os_button.h"
#include "components/os_edit.h"
#include "components/os_combobox.h"
#include "components/os_memo.h"
#include "components/os_checkbox.h"
#include "components/os_radiobutton.h"
#include "components/os_image.h"
#include "../gui/wm.h"

#include "../system/liblib.h"
#include "../system/string.h"

// Inclusão dos novos componentes encapsulados
#include "components/TOS_IPC.h"     
#include "components/TOSSerial.h"   

void gui_draw_form(TForm* form);
void gui_render_form(TForm* form);

extern void events_process_mouse(int x, int y, int pressed, int button);
extern char* OS_Edit_GetText(void* edit);
extern void  OS_Edit_SetText(void* edit, const char* text);
extern char* OS_ComboBox_GetText(void* combo);

int my_app_slot = -1;
TAppEnvironment MyApp;

int main(int argc, char* argv[]) {
    // 💡 Aumentamos a largura de 460 para 520 para acomodar o botão de fechar perfeitamente
    int winWidth = 520, winHeight = 320;

    // Variáveis de controle para o motor de clique assíncrono e efeitos visuais
    static int ultimo_x = 0;
    static int ultimo_y = 0;
    static int mouse_hold_timer = 0; 

    // Inicialização básica do subsistema do Runtime
    graphics_init_app(winWidth, winHeight);
    wm_init();
    
    // 1. Inicialização IPC ultra-limpa usando o novo componente encapsulado
    my_app_slot = OS_IPC_RegisterApp("UART Lab Runtimer", winWidth, winHeight);
    if (my_app_slot == -1) return -1; 
    
    graphics_set_slot(my_app_slot);
    OS_InitApplication(&MyApp, my_app_slot, "UART Lab Runtimer", winWidth, winHeight);

    // 2. Componente de Hardware Abstrato (Serial inicializa em COM1 / 9600 bps)
    TOSSerial* serial = OS_CreateSerial(1, 9600);

    // 3. Componentes Visuais (Paradigma Delphi / VCL - Coordenadas Absolutas)
    TOSControl* btnOpen  = OS_CreateButton(&MyApp, 10, 40,  210, 30, "Testar OPEN Serial");
    TOSControl* btnWrite = OS_CreateButton(&MyApp, 10, 80,  210, 30, "Transmitir UART (Texto)");
    TOSControl* btnRead  = OS_CreateButton(&MyApp, 10, 120, 210, 30, "Capturar RX Arduino");
    TOSControl* btnClose = OS_CreateButton(&MyApp, 10, 160, 210, 30, "Fechar Porta Serial");

    TOSControl* lblEnvio = OS_CreateLabel(&MyApp, 10, 205, "Texto para Envio:");
    TOSEdit* editCmd     = (TOSEdit*)OS_CreateEdit(&MyApp, 10, 225, 210, 25, "a");

    TOSControl* lblBaud  = OS_CreateLabel(&MyApp, 10, 270, "Baud Rate:");
    TOSComboBox* cbBaud  = (TOSComboBox*)OS_CreateComboBox(&MyApp, 95, 266, 125, 25);
    
    OS_ComboBox_AddItem(cbBaud, "9600");
    OS_ComboBox_AddItem(cbBaud, "115200");

    TOSControl* lblRx    = OS_CreateLabel(&MyApp, 235, 40, "Terminal Receptor (RX):");
    TOSMemo* memoRx      = (TOSMemo*)OS_CreateMemo(&MyApp, 235, 60, 215, 230);

    // 🔥 NOVO: Botão para fechar o software posicionado na nova área da extrema direita
    TOSControl* btnSair  = OS_CreateButton(&MyApp, 460, 40, 50, 250, "X\n\nF\nE\nC\nH\nA\nR");

    char string_resposta[32] = "Recebido: [ ]";

    // =========================================================================
    // 4. LOOP PRINCIPAL DE EVENTOS DO SISTEMA OPERACIONAL
    // =========================================================================
    while(1) {
        bool euTenhoFoco = (IPC_CONTROL->active_focus_slot == my_app_slot);

        // Modifica o estado de foco direto na struct do formulário principal
        if (MyApp.MainWindow) {
            MyApp.MainWindow->ActiveFocus = euTenhoFoco;
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

                // Processamento padrão do clique nativo no Kernel
                events_process_mouse(rel_x, rel_y, 1, 0);

                // --- GERENCIAMENTO DE FOCO E SCROLL VIA MOUSE EM COMPONENTES DE TEXTO ---
                if (rel_x >= editCmd->base.Left && rel_x < (editCmd->base.Left + editCmd->base.Width) &&
                    rel_y >= editCmd->base.Top && rel_y < (editCmd->base.Top + editCmd->base.Height)) {
                    OS_Edit_SetFocus(editCmd);
                }
                else if (rel_x >= memoRx->base.Left && rel_x < (memoRx->base.Left + memoRx->base.Width) &&
                         rel_y >= memoRx->base.Top && rel_y < (memoRx->base.Top + memoRx->base.Height)) {
                    
                    // Verificação física da barra de rolagem (Scroll lateral do Memo: Últimos 18 pixels)
                    int limite_barra_x = memoRx->base.Left + memoRx->base.Width - 18;
                    if (rel_x >= limite_barra_x) {
                        int metade_vertical = memoRx->base.Top + (memoRx->base.Height / 2);
                        if (rel_y < metade_vertical) {
                            OS_Memo_SetScroll(memoRx, memoRx->ScrollY - 2); // Rola para Cima
                        } else {
                            OS_Memo_SetScroll(memoRx, memoRx->ScrollY + 2); // Rola para Baixo
                        }
                    } else {
                        OS_Memo_SetFocus(memoRx);
                    }
                }
                else {
                    // Remove o cursor se o clique for fora das caixas de texto
                    gui_set_prop(NULL, PROP_SET_FOCUS, 0);
                }

                // A biblioteca embutida calcula cliques padrão e estados internos
                bool resolvido_pela_lib = OS_ProcessMouseClick(&MyApp, rel_x, rel_y);                
                
                // --- TRATAMENTO DOS EVENTOS CUSTOMIZADOS DA APLICAÇÃO ---
                
                // Evento ComboBox: Mudança de Velocidade (Hot-swap)
                TOSControl* cbBaudCtrl = (TOSControl*)cbBaud;
                if (rel_x >= cbBaudCtrl->Left && rel_x < (cbBaudCtrl->Left + cbBaudCtrl->Width) &&
                    rel_y >= cbBaudCtrl->Top && rel_y < (cbBaudCtrl->Top + cbBaudCtrl->Height)) {
                    
                    int novo_baud = (cbBaud->ItemIndex == 0) ? 9600 : 115200;
                    OS_Serial_SetBaud(serial, novo_baud);
                    
                    if (novo_baud == 9600) {
                        OS_Memo_AddStr(memoRx, "Velocidade alterada: 9600 bps\n");
                    } else {
                        OS_Memo_AddStr(memoRx, "Velocidade alterada: 115200 bps\n");
                    }

                    if (serial->Active) {
                        OS_Memo_AddStr(memoRx, "[SISTEMA]: Porta reiniciada com novo Baud.\n");
                    }
                }
                
                // Lógica executada apenas se a biblioteca base não interceptou o clique
                else {
                    // 🔥 NOVO EVENTO: Clicou no Botão de Fechamento do Software
                    if (rel_x >= btnSair->Left && rel_x < (btnSair->Left + btnSair->Width) &&
                        rel_y >= btnSair->Top && rel_y < (btnSair->Top + btnSair->Height)) {
                        
                        gui_set_prop((void*)btnSair->KernelHandle, PROP_STATE, 2); 
                        OS_Memo_AddStr(memoRx, "Solicitando encerramento via IPC...\n");
                        
                        // Garante que a porta serial seja fechada corretamente antes do encerramento físico
                        if (serial && serial->Active) {
                            OS_Serial_Close(serial);
                        }

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

                    // Evento: Botão Abrir Serial
                    else if (rel_x >= btnOpen->Left && rel_x < (btnOpen->Left + btnOpen->Width) &&
                             rel_y >= btnOpen->Top && rel_y < (btnOpen->Top + btnOpen->Height)) {
                        
                        gui_set_prop((void*)btnOpen->KernelHandle, PROP_STATE, 2); // Estado: Pressionado
                        if (!serial->Active) {
                            if (OS_Serial_Open(serial)) {
                                OS_Memo_AddStr(memoRx, "Porta COM1 Inicializada com Sucesso!\n");
                                gui_set_prop((void*)btnOpen->KernelHandle, PROP_COLOR, 0xAAAAAA);  
                                gui_set_prop((void*)btnClose->KernelHandle, PROP_COLOR, 0xCCCCCC); 
                            } else {
                                OS_Memo_AddStr(memoRx, "Erro critico: Falha ao abrir COM1.\n");
                            }
                        }
                    }
                    
                    // Evento: Botão Enviar Texto (TX)
                    else if (rel_x >= btnWrite->Left && rel_x < (btnWrite->Left + btnWrite->Width) &&
                             rel_y >= btnWrite->Top && rel_y < (btnWrite->Top + btnWrite->Height)) {
                        
                        gui_set_prop((void*)btnWrite->KernelHandle, PROP_STATE, 2); // Estado: Pressionado
                        if (serial->Active) {
                            char* comando = OS_Edit_GetText((void*)editCmd);
                            if (OS_Serial_Write(serial, comando) > 0) {
                                OS_Memo_AddStr(memoRx, "TX -> ");
                                OS_Memo_AddStr(memoRx, comando);
                                OS_Memo_AddStr(memoRx, "\n");
                            }
                        } else {
                            OS_Memo_AddStr(memoRx, "Erro: Abra a porta antes de transmitir.\n");
                        }
                    }
                    
                    // Evento: Botão Capturar Texto (RX)
                    else if (rel_x >= btnRead->Left && rel_x < (btnRead->Left + btnRead->Width) &&
                             rel_y >= btnRead->Top && rel_y < (btnRead->Top + btnRead->Height)) {
                        
                        gui_set_prop((void*)btnRead->KernelHandle, PROP_STATE, 2); // Estado: Pressionado
                        if (serial->Active) {
                            char caractere_recebido = 0;
                            if (OS_Serial_Read(serial, (uint8_t*)&caractere_recebido, 1) > 0) {
                                string_resposta[11] = caractere_recebido;
                                OS_Memo_AddStr(memoRx, "RX <- ");
                                OS_Memo_AddStr(memoRx, string_resposta);
                                OS_Memo_AddStr(memoRx, "\n");
                            } else {
                                OS_Memo_AddStr(memoRx, "Buffer RX Vazio.\n");
                            }
                        } else {
                            OS_Memo_AddStr(memoRx, "Erro: Porta inativa.\n");
                        }
                    }
                    
                    // Evento: Botão Fechar Serial
                    else if (rel_x >= btnClose->Left && rel_x < (btnClose->Left + btnClose->Width) &&
                             rel_y >= btnClose->Top && rel_y < (btnClose->Top + btnClose->Height)) {
                        
                        gui_set_prop((void*)btnClose->KernelHandle, PROP_STATE, 2); // Estado: Pressionado
                        if (serial->Active) {
                            OS_Serial_Close(serial);
                            OS_Memo_AddStr(memoRx, "Porta COM1 Fechada e Desalocada.\n");
                            gui_set_prop((void*)btnOpen->KernelHandle, PROP_COLOR, 0xCCCCCC);  
                            gui_set_prop((void*)btnClose->KernelHandle, PROP_COLOR, 0xAAAAAA); 
                        }
                    }
                }
                IPC_WINDOW_LIST[my_app_slot].has_click_event = 0; 
            }

skip_mouse_processing:

            // --- MOUSE UP ASÍNCRONO (DEBOUNCE / LIBERAÇÃO DE ESTADO VISUAL) ---
            if (mouse_hold_timer > 0) {
                mouse_hold_timer--; 
                if (mouse_hold_timer == 0) {
                    // Restaura visualmente todos os botões para o estado normal (0)
                    gui_set_prop((void*)btnOpen->KernelHandle, PROP_STATE, 0);    
                    gui_set_prop((void*)btnWrite->KernelHandle, PROP_STATE, 0);    
                    gui_set_prop((void*)btnRead->KernelHandle, PROP_STATE, 0);    
                    gui_set_prop((void*)btnClose->KernelHandle, PROP_STATE, 0);    
                    gui_set_prop((void*)btnSair->KernelHandle, PROP_STATE, 0); // 👈 Incluído no debounce    
                    
                    events_process_mouse(ultimo_x, ultimo_y, 0, 0); 
                }
            }
        }

        // =========================================================================
        // 5. ATUALIZAÇÃO DO BUFFER GRÁFICO (FLIP ENCAPSULADO VIA RENDERIZADOR BASE)
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
    return 0;
}
