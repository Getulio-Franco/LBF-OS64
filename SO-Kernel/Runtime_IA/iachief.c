#include "Runtime/controls.h"
#include "../system/graphics.h"
#include "../gui/wm.h"
#include "../system/liblib.h"
#include "../system/string.h"

// Componentes da Interface Gráfica do seu S.O.
#include "../Runtime/components/TOS_IPC.h"     
#include "../Runtime/components/os_label.h"
#include "../Runtime/components/os_button.h"
#include "../Runtime/components/os_edit.h"
#include "../Runtime/components/os_memo.h"

// === ECOSSISTEMA DE INTELIGÊNCIA ARTIFICIAL (MoE) ===
#include "iarouter.h"  // Rede Neural 1: Roteador e Extrator de Entidades
#include "iamath.h"    // Especialista 1: Matemática Unificada
#include "ialogic.h"   // Especialista 2: Lógica Digital Unificada
#include "iasynth.h"   // Rede Neural 2: Sintetizador de Linguagem Natural

void gui_draw_form(TForm* form);
void gui_render_form(TForm* form);

extern char* OS_Edit_GetText(void* edit);
extern void  OS_Edit_SetText(void* edit, const char* text);
extern void* OS_IPC_GetSharedBuffer(int app_slot);

int my_app_slot = -1;
TAppEnvironment MyApp;

int main(int argc, char* argv[]) {
    int winWidth = 420, winHeight = 320;

    graphics_init_app(winWidth, winHeight);
    wm_init();
    
    my_app_slot = OS_IPC_RegisterApp("TOS Brain - IA Mae (Chief)", winWidth, winHeight);
    if (my_app_slot == -1) return -1; 
    
    graphics_set_slot(my_app_slot);
    OS_InitApplication(&MyApp, my_app_slot, "TOS Brain - IA Mae (Chief)", winWidth, winHeight);

    // INTERFACE GRÁFICA CONVERSACIONAL INTEGRADA
    OS_CreateLabel(&MyApp, 10, 40, "Digite seu comando (Ex: 'faz o favor de somar 12 e 5' ou 'and 1 1'):");
    
    TOSEdit* EditComando = (TOSEdit*)OS_CreateEdit(&MyApp, 10, 60, 400, 25, "por favor, qual o resultado de some 12 e 5"); 

    TOSControl* BtnPerguntar = OS_CreateButton(&MyApp, 10, 95, 400, 32, "Processar no Nucleo Neural (MoE)");

    OS_CreateLabel(&MyApp, 10, 135, "Terminal de Pensamento da IA (Outputs):");
    TOSMemo* MemoChief = (TOSMemo*)OS_CreateMemo(&MyApp, 10, 155, 400, 130);
    
    OS_Memo_AddStr(MemoChief, "=== TOS Brain OS-MoE v1.0 ===\n");
    OS_Memo_AddStr(MemoChief, "Roteador, Sintetizador e Especialistas carregados em memoria ativa.\n\n");

    bool precisa_redesenhar = true;

    while(1) {
        bool euTenhoFoco = (IPC_CONTROL->active_focus_slot == my_app_slot);
        if (MyApp.MainWindow) MyApp.MainWindow->ActiveFocus = euTenhoFoco;

        if (euTenhoFoco) {
            char key = get_key();
            if (key != 0) {
                OS_ProcessKeyboard(&MyApp, key); 
                precisa_redesenhar = true; 
            }

            if (IPC_WINDOW_LIST[my_app_slot].has_click_event == 1) {
                int rel_x = IPC_WINDOW_LIST[my_app_slot].local_click_x;
                int rel_y = IPC_WINDOW_LIST[my_app_slot].local_click_y;
                
                precisa_redesenhar = true; 

                if (rel_x >= EditComando->base.Left && rel_x < (EditComando->base.Left + EditComando->base.Width) &&
                    rel_y >= EditComando->base.Top && rel_y < (EditComando->base.Top + EditComando->base.Height)) {
                    OS_Edit_SetFocus(EditComando);
                }
                else if (rel_x >= MemoChief->base.Left && rel_x < (MemoChief->base.Left + MemoChief->base.Width) &&
                         rel_y >= MemoChief->base.Top && rel_y < (MemoChief->base.Top + MemoChief->base.Height)) {
                    OS_Memo_SetFocus(MemoChief);
                } else {
                    gui_set_prop(NULL, PROP_SET_FOCUS, 0); 
                }
                
                bool resolvido_pela_lib = OS_ProcessMouseClick(&MyApp, rel_x, rel_y);                
                
                if (!resolvido_pela_lib) {
                    if (rel_x >= BtnPerguntar->Left && rel_x < (BtnPerguntar->Left + BtnPerguntar->Width) &&
                        rel_y >= BtnPerguntar->Top && rel_y < (BtnPerguntar->Top + BtnPerguntar->Height)) {
                        
                        char* comando_usuario = OS_Edit_GetText(EditComando);
                        
                        if (comando_usuario && strlen(comando_usuario) > 0) {
                            OS_Memo_AddStr(MemoChief, "Usuario: ");
                            OS_Memo_AddStr(MemoChief, comando_usuario);
                            OS_Memo_AddStr(MemoChief, "\n");

                            // Buffers de comunicação interna inter-neuronal
                            char raw_math_out[64] = {0};
                            char raw_logic_out[64] = {0};
                            char resposta_final[256] = {0};

                            // ETAPA 1: O Roteador Neural examina o prompt e limpa as entidades
                            RouterDecision decisao = iarouter_processar(comando_usuario);

                            // LOG DE PENSAMENTO INTERNO (Para debug da IA no Terminal)
                            OS_Memo_AddStr(MemoChief, "-> [Router] Cmd Limpo: ");
                            OS_Memo_AddStr(MemoChief, decisao.comando_limpo);
                            OS_Memo_AddStr(MemoChief, "\n");

                            // ETAPA 2: Disparo dinâmico e paralelo dos Especialistas Ativados
                            if (decisao.acionar_math) {
                                iamath_interpretar(decisao.comando_limpo, raw_math_out);
                            }
                            if (decisao.acionar_logic) {
                                ialogic_interpretar(decisao.comando_limpo, raw_logic_out);
                            }

                            // ETAPA 3: O Sintetizador Neural junta os contextos e gera a resposta
                            // Passamos o prompt original (para avaliar a 'vibe/tom') e os outputs dos nós matemáticos
                            if (decisao.acionar_math && !decisao.acionar_logic) {
                                iasynth_gerar_resposta(comando_usuario, raw_math_out, resposta_final);
                            } else {
                                // Se for lógico ou misto, prioriza ou concatena na iasynth adaptada
                                iasynth_gerar_resposta(comando_usuario, raw_logic_out, resposta_final);
                            }

                            // ETAPA 4: Exibição do resultado humanizado na UI
                            OS_Memo_AddStr(MemoChief, "TOS Brain: ");
                            OS_Memo_AddStr(MemoChief, resposta_final);
                            OS_Memo_AddStr(MemoChief, "\n\n");

                            OS_Edit_SetText(EditComando, "");
                        }
                    }
                }
                IPC_WINDOW_LIST[my_app_slot].has_click_event = 0; 
            }
        }

        if (precisa_redesenhar || euTenhoFoco) {
            gui_draw_form(MyApp.MainWindow);
            gui_render_form(MyApp.MainWindow);
            OS_IPC_FlipBuffers(my_app_slot, winWidth, winHeight);
            if (!euTenhoFoco) precisa_redesenhar = false; 
        }

        sys_sleep(euTenhoFoco ? 16 : 100);
    }
    return 0;
}
