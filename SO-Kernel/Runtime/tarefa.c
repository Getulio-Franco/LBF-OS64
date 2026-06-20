#include "controls.h"
#include "../system/graphics.h"
#include "../gui/wm.h"
#include "../system/string.h"
#include "../system/liblib.h"

// Componentes do Sistema encapsulados
#include "components/TOS_IPC.h"     
#include "components/TOSSerial.h"   

// Biblioteca de Componentes Visuais
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

// Utilitários de dados do Runtime
extern char* OS_Edit_GetText(void* edit);
extern void  OS_Edit_SetText(void* edit, const char* text);

// Estrutura de dados sincronizada com o Kernel
TProcessInfo lista_ps[6];

// Variáveis de controle do ambiente da aplicação
int my_app_slot = -1;
TAppEnvironment MyApp;

int main(int argc, char* argv[]) {
    // Expandido winHeight de 360 para 410 para acomodar a nova seção de Execução de Processos
    int winWidth = 550, winHeight = 410;

    static int ultimo_x = 0;
    static int ultimo_y = 0;
    static int mouse_hold_timer = 0; 

    // Inicialização do Ambiente
    graphics_init_app(winWidth, winHeight);
    wm_init();
    
    my_app_slot = OS_IPC_RegisterApp("Gerenciador de Tarefas LBF", winWidth, winHeight);
    if (my_app_slot == -1) return -1; 
    
    graphics_set_slot(my_app_slot);
    OS_InitApplication(&MyApp, my_app_slot, "Gerenciador de Tarefas LBF", winWidth, winHeight);

    // =========================================================================
    // DESIGN LAYOUT RAD - INTERFACE CONTROLADA
    // =========================================================================
    TOSControl* BtnAtualizar = OS_CreateButton(&MyApp, 10, 40, 200, 30, "ATUALIZAR LISTA");
    
    OS_CreateLabel(&MyApp, 10, 80, "Lista de Processos Ativos no Kernel:");
    TOSMemo* ExeMemo = (TOSMemo*)OS_CreateMemo(&MyApp, 10, 100, 530, 170);
    OS_Memo_AddStr(ExeMemo, "Gerenciador Inicializado. Clique em ATUALIZAR LISTA.\n");

    // --- SEÇÃO 1: FINALIZAR PROCESSO (KILL) ---
    OS_CreateLabel(&MyApp, 10, 285, "Digite o PID do alvo:");
    TOSEdit* EditPID = (TOSEdit*)OS_CreateEdit(&MyApp, 10, 305, 120, 25, "");
    TOSControl* BtnKillUnico = OS_CreateButton(&MyApp, 140, 302, 220, 30, "FINALIZAR PROCESSO (KILL)");

    // --- SEÇÃO 2: NOVA FUNCIONALIDADE - EXECUTAR PROGRAMA (SYS_EXEC) ---
    OS_CreateLabel(&MyApp, 10, 345, "Caminho do Binario ELF:");
    TOSEdit* EditPath = (TOSEdit*)OS_CreateEdit(&MyApp, 10, 365, 250, 25, "");
    TOSControl* BtnExecutar = OS_CreateButton(&MyApp, 270, 362, 270, 30, "EXECUTAR PROCESSO (EXEC)");

    int qtd_processos = 0;

    // =========================================================================
    // LOOP PRINCIPAL DE EVENTOS DO SISTEMA OPERACIONAL
    // =========================================================================
    while(1) {
        bool euTenhoFoco = (IPC_CONTROL->active_focus_slot == my_app_slot);

        if (MyApp.MainWindow) {
            MyApp.MainWindow->ActiveFocus = euTenhoFoco;
        }

        if (euTenhoFoco) {
            char key = get_key();
            OS_ProcessKeyboard(&MyApp, key); 

            // --- PROCESSAMENTO DO MOUSE ---
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

                events_process_mouse(rel_x, rel_y, 1, 0);

                // Gerenciamento de Foco dos Controles de Texto (TMemo, EditPID e EditPath)
                if (rel_x >= ExeMemo->base.Left && rel_x < (ExeMemo->base.Left + ExeMemo->base.Width) &&
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
                else if (rel_x >= EditPID->base.Left && rel_x < (EditPID->base.Left + EditPID->base.Width) &&
                         rel_y >= EditPID->base.Top && rel_y < (EditPID->base.Top + EditPID->base.Height)) {
                    OS_Edit_SetFocus(EditPID);
                }
                else if (rel_x >= EditPath->base.Left && rel_x < (EditPath->base.Left + EditPath->base.Width) &&
                         rel_y >= EditPath->base.Top && rel_y < (EditPath->base.Top + EditPath->base.Height)) {
                    OS_Edit_SetFocus(EditPath); // Mapeamento de Foco para o campo do Executável
                }
                else {
                    gui_set_prop(NULL, PROP_SET_FOCUS, 0); 
                }

                OS_ProcessMouseClick(&MyApp, rel_x, rel_y);

                // --- EVENTO: CLIQUE NO BOTÃO ATUALIZAR ---
                if (rel_x >= BtnAtualizar->Left && rel_x < (BtnAtualizar->Left + BtnAtualizar->Width) &&
                    rel_y >= BtnAtualizar->Top && rel_y < (BtnAtualizar->Top + BtnAtualizar->Height)) {
                    
                    gui_set_prop((void*)BtnAtualizar->KernelHandle, PROP_STATE, 2); 
                    
                    for(int z = 0; z < 6; z++) lista_ps[z].pid = 0;

                    qtd_processos = sys_get_ps_data(lista_ps, 6);
                    
                    OS_Memo_AddStr(ExeMemo, "--- LISTA ATUALIZADA ---\n");
                    char qtd_str[10];
                    itoa((uint64_t)qtd_processos, qtd_str, 10); 
                    OS_Memo_AddStr(ExeMemo, "Processos ativos: ");
                    OS_Memo_AddStr(ExeMemo, qtd_str);
                    OS_Memo_AddStr(ExeMemo, "\n");

                    for(int k = 0; k < 6; k++) {
                        if(lista_ps[k].pid != 0) {
                            char pid_str[10];
                            itoa(lista_ps[k].pid, pid_str, 10); 
                            OS_Memo_AddStr(ExeMemo, "PID: ");
                            OS_Memo_AddStr(ExeMemo, pid_str);
                            OS_Memo_AddStr(ExeMemo, " | Nome: ");
                            OS_Memo_AddStr(ExeMemo, lista_ps[k].name);
                            OS_Memo_AddStr(ExeMemo, "\n");
                        }
                    }
                }
                
// --- EVENTO: CLIQUE NO BOTÃO KILL ---
if (rel_x >= BtnKillUnico->Left && rel_x < (BtnKillUnico->Left + BtnKillUnico->Width) &&
    rel_y >= BtnKillUnico->Top && rel_y < (BtnKillUnico->Top + BtnKillUnico->Height)) {
    
    gui_set_prop((void*)BtnKillUnico->KernelHandle, PROP_STATE, 2);
    
    char* texto_pid = OS_Edit_GetText(EditPID);
    
    if (texto_pid && texto_pid[0] != '\0') {
        int pid_alvo = atoi(texto_pid); 
        
        // Protege os PIDs de 0 a 4 que pertencem ao Kernel e suas tarefas essenciais (como task_b e task_d)
        if (pid_alvo >= 5) {
            
            // =========================================================================
            // 🧹 FAXINA VISUAL DO IPC (RING 3)
            // =========================================================================
            for (int i = 0; i < MAX_EXTERNAL_APPS; i++) {
                if (IPC_WINDOW_LIST[i].pid == (uint64_t)pid_alvo && IPC_WINDOW_LIST[i].is_active == 1) {
                    
                    // Sinaliza para o explorer.elf sumir com a janela imediatamente
                    IPC_WINDOW_LIST[i].is_active = 0;

                    // Zera os buffers gráficos para evitar artefatos residuais na tela
                    uint32_t* b0 = (uint32_t*)(uintptr_t)IPC_WINDOW_LIST[i].buffer_ptr_0;
                    uint32_t* b1 = (uint32_t*)(uintptr_t)IPC_WINDOW_LIST[i].buffer_ptr_1;
                    int tamanho_buffer = IPC_WINDOW_LIST[i].width * IPC_WINDOW_LIST[i].height * 4;
                    
                    if (b0 && tamanho_buffer > 0) memset(b0, 0, tamanho_buffer);
                    if (b1 && tamanho_buffer > 0) memset(b1, 0, tamanho_buffer);
                    
                    break; // Alvo encontrado e limpo, interrompe a busca
                }
            }

            // Dá 20ms (dois ciclos do Explorer) para que a interface nativa processe o fechamento
            sys_sleep(20);

            // =========================================================================
            // 💀 ENCERRAMENTO FÍSICO NO KERNEL (SINAL KILL)
            // =========================================================================
            int res_kill = sys_kill((uint64_t)pid_alvo);
            
            if (res_kill == 0) {
                OS_Memo_AddStr(ExeMemo, "Sinal KILL enviado para o PID: ");
                OS_Memo_AddStr(ExeMemo, texto_pid);
                OS_Memo_AddStr(ExeMemo, "\n");
            } else {
                OS_Memo_AddStr(ExeMemo, "Erro: Falha ao finalizar o processo do PID informado.\n");
            }
            
            OS_Edit_SetText(EditPID, "");
            
        } else if (pid_alvo >= 0 && pid_alvo < 5) {
            OS_Memo_AddStr(ExeMemo, "Erro: Os PIDs de 0 a 4 (Kernel & Core Tasks) sao protegidos!\n");
        } else {
            OS_Memo_AddStr(ExeMemo, "Erro: PID invalido.\n");
        }
    } else {
        OS_Memo_AddStr(ExeMemo, "Aviso: Digite um PID no campo de texto.\n");
    }
}

// --- EVENTO: CLIQUE NO BOTÃO EXECUTAR (SYS_EXEC) ---
if (rel_x >= BtnExecutar->Left && rel_x < (BtnExecutar->Left + BtnExecutar->Width) &&
    rel_y >= BtnExecutar->Top && rel_y < (BtnExecutar->Top + BtnExecutar->Height)) {
    
    gui_set_prop((void*)BtnExecutar->KernelHandle, PROP_STATE, 2);
    
    char* caminho_elf = OS_Edit_GetText(EditPath);
    
    if (caminho_elf && caminho_elf[0] != '\0') {
        // 1. Adiciona as strings ao Memo normalmente
        OS_Memo_AddStr(ExeMemo, "Tentando executar: ");
        OS_Memo_AddStr(ExeMemo, caminho_elf);
        OS_Memo_AddStr(ExeMemo, "\n");
        
        // 2. FORÇA REDESENHO IMEDIATO (Garante que o texto apareça antes de qualquer travamento)
        gui_draw_form(MyApp.MainWindow);
        gui_render_form(MyApp.MainWindow);
        
        // Copia o buffer local direto para o buffer compartilhado ativo
        int back_idx = (IPC_WINDOW_LIST[my_app_slot].active_buffer == 0) ? 1 : 0;
        uint8_t* shared_ptr = (back_idx == 0) 
            ? (uint8_t*)(uintptr_t)IPC_WINDOW_LIST[my_app_slot].buffer_ptr_0 
            : (uint8_t*)(uintptr_t)IPC_WINDOW_LIST[my_app_slot].buffer_ptr_1;
        
        uint8_t* local_ptr = graphics_get_buffer();
        if (shared_ptr && local_ptr) {
            memcpy(shared_ptr, local_ptr, winWidth * winHeight * 4); 
        }
        IPC_WINDOW_LIST[my_app_slot].active_buffer = back_idx;
        
        // NOVO: Dá uma chance para o Window Manager redesenhar a tela ANTES da syscall!
        sys_sleep(10); 

        int retorno_exec = sys_exec(caminho_elf);

        if (retorno_exec == 0) {
            OS_Memo_AddStr(ExeMemo, "Pedido de execucao enviado para a Task B...\n");
            OS_Edit_SetText(EditPath, "");
        } else if (retorno_exec == -2) {
                OS_Memo_AddStr(ExeMemo, "Erro: Kernel ocupado processando outro binario.\n");
            } else {
                OS_Memo_AddStr(ExeMemo, "Erro: Caminho invalido.\n");
            }
        } else {
        OS_Memo_AddStr(ExeMemo, "Aviso: Digite o nome do arquivo .elf (Ex: tamp.elf)\n");
     }
   }
}

skip_mouse_processing:

            // --- MOUSE UP ASÍNCRONO ---
            if (mouse_hold_timer > 0) {
                mouse_hold_timer--; 
                if (mouse_hold_timer == 0) {
                    gui_set_prop((void*)BtnAtualizar->KernelHandle, PROP_STATE, 0); 
                    gui_set_prop((void*)BtnKillUnico->KernelHandle, PROP_STATE, 0); 
                    gui_set_prop((void*)BtnExecutar->KernelHandle, PROP_STATE, 0); // Limpa o estado visual do botão Executar
                    events_process_mouse(ultimo_x, ultimo_y, 0, 0); 
                }
            }
        }

        // --- ATUALIZAÇÃO DO BUFFER GRÁFICO E IPC ---
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
