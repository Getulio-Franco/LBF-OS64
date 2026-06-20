#include "../gui/gui.h"
#include "../system/graphics.h" 
#include "../system/liblib.h"
#include "../gui/wm.h"
#include "../system/string.h" 
#include "../events/cursor_engine.h"

extern uint8_t* ram_buffer; 

int z_stack[MAX_EXTERNAL_APPS];

void init_z_stack() {
    for(int i = 0; i < MAX_EXTERNAL_APPS; i++) z_stack[i] = i;
}

void bring_to_front(int slot_index) {
    int current_pos = -1;
    for(int i = 0; i < MAX_EXTERNAL_APPS; i++) {
        if(z_stack[i] == slot_index) { current_pos = i; break; }
    }
    if(current_pos == -1) return;
    for(int i = current_pos; i < MAX_EXTERNAL_APPS - 1; i++) z_stack[i] = z_stack[i+1];
    z_stack[MAX_EXTERNAL_APPS - 1] = slot_index;
}

void compose_app_window(int slot) {
    int active_idx = IPC_WINDOW_LIST[slot].active_buffer;
    uint32_t* app_fb = (active_idx == 0)
        ? (uint32_t*)(uintptr_t)IPC_WINDOW_LIST[slot].buffer_ptr_0
        : (uint32_t*)(uintptr_t)IPC_WINDOW_LIST[slot].buffer_ptr_1;
    
    if (!app_fb || !ram_buffer) return;

    int win_x = IPC_WINDOW_LIST[slot].x;
    int win_y = IPC_WINDOW_LIST[slot].y;
    int win_w = IPC_WINDOW_LIST[slot].width;
    int win_h = IPC_WINDOW_LIST[slot].height;

    // 1. Clipping Vertical (Y)
    int start_y = (win_y < 0) ? -win_y : 0;
    int end_y   = (win_y + win_h > screen_h) ? (screen_h - win_y) : win_h;

    // 2. Clipping Horizontal (X)
    int start_x = (win_x < 0) ? -win_x : 0;
    int end_x   = (win_x + win_w > screen_w) ? (screen_w - win_x) : win_w;

    int copy_w = end_x - start_x;
    if (copy_w <= 0) return; // Fora da tela em X

    for (int y = start_y; y < end_y; y++) {
        int target_y = win_y + y;
        
        // Ponteiro de destino (Buffer da tela)
        uint8_t* dest_line = ram_buffer + (target_y * screen_pitch) + ((win_x + start_x) * bpp_bytes);
        
        // Ponteiro de origem (Buffer do App - sempre 32bpp/4 bytes)
        uint32_t* src_line = app_fb + (y * win_w) + start_x;

        if (bpp_bytes == 4) {
            // Caso rápido: 32bpp (Desktop) <- 32bpp (App)
            memcpy(dest_line, src_line, copy_w * 4);
        } else if (bpp_bytes == 3) {
            // Conversão: 24bpp (Desktop) <- 32bpp (App)
            for (int x = 0; x < copy_w; x++) {
                uint32_t color = src_line[x];
                int off = x * 3;
                dest_line[off]   = color & 0xFF;
                dest_line[off+1] = (color >> 8) & 0xFF;
                dest_line[off+2] = (color >> 16) & 0xFF;
            }
        }
    }
}

int main() {
    graphics_init();  
    
    if (screen_w == 0 || screen_h == 0) {
        sys_exit();
    }
    
    wm_init();
    init_z_stack();

    // SETUP DA TASKBAR
    TForm* frmTaskbar = gui_create_form("frmTaskbar", "Taskbar", 1);
    if (frmTaskbar) {
        frmTaskbar->BorderStyle = bsNone;
        frmTaskbar->Win.Control.Left = 0;
        frmTaskbar->Win.Control.Top = screen_h - 40;
        frmTaskbar->Win.Control.Width = screen_w;
        frmTaskbar->Win.Control.Height = 40;
        frmTaskbar->Win.Control.Color = 0xC0C0C0;
        frmTaskbar->Win.Control.Visible = 1;
        
        TButton* btnStart = gui_create_button(&frmTaskbar->Win, "btnStart", "Iniciar");
        if (btnStart) {
            btnStart->Win.Control.Left = 2;
            btnStart->Win.Control.Top = 2;
            btnStart->Win.Control.Width = 80;
            btnStart->Win.Control.Height = 36;
        }
        wm_set_desktop(frmTaskbar);
    }
    
    TLabel* lblStatus = gui_create_label((TWinControl*)frmTaskbar, "lblStatus", "VCL/IPC OS v1.2 [Event-Driven]");
    if (lblStatus) {
        lblStatus->Graphic.Control.Left = 100;
        lblStatus->Graphic.Control.Top = 12;
    }

    int dragging_slot = -1;
    int offX = 0, offY = 0;

    while(1) {
        mouse_t m; 
        get_mouse(&m);

        // =========================================================================
        // 🧹 COLETOR DE SLOTS INATIVOS (CRÍTICO PARA EVITAR EXAUSTÃO DE MEMÓRIA)
        // =========================================================================
        for (int i = 0; i < MAX_EXTERNAL_APPS; i++) {
            // Se o slot possui uma app registrada com PID ativo no Kernel, mas o app se marcou como inativo...
            if (IPC_WINDOW_LIST[i].pid != 0 && IPC_WINDOW_LIST[i].is_active == 0) {
                
                // 1. Libera o foco se a janela morta era a atual focada
                if (IPC_CONTROL->active_focus_slot == i) {
                    IPC_CONTROL->active_focus_slot = -1;
                    if (lblStatus) {
                        strcpy(lblStatus->Graphic.Control.Caption, "VCL/IPC OS v1.2 [Event-Driven]");
                    }
                }

                // 2. Evita ponteiro fantasma se o usuário fechou a janela enquanto a arrastava
                if (dragging_slot == i) {
                    dragging_slot = -1;
                }

                // 3. FAXINA IPC: Reseta os metadados do slot permitindo reuso por novos processos
                IPC_WINDOW_LIST[i].pid = 0;             
                IPC_WINDOW_LIST[i].buffer_ptr_0 = 0;    
                IPC_WINDOW_LIST[i].buffer_ptr_1 = 0;    
                IPC_WINDOW_LIST[i].width = 0;
                IPC_WINDOW_LIST[i].height = 0;
                IPC_WINDOW_LIST[i].has_click_event = 0;
                IPC_WINDOW_LIST[i].title[0] = '\0';     
            }
        }

        // --- SISTEMA DE EVENTOS DE MOUSE MELHORADO ---
        if (m.buttons & 1) { 
            if (dragging_slot == -1) {
                // Varre do topo para o fundo (Z-Index decrecente) para pegar a janela mais visível primeiro
                for (int i = MAX_EXTERNAL_APPS - 1; i >= 0; i--) {
                    int s = z_stack[i];
                    if (IPC_WINDOW_LIST[s].is_active) {
                        
                        // 1. O clique ocorreu dentro da área desta janela?
                        if (m.x >= IPC_WINDOW_LIST[s].x && m.x <= (IPC_WINDOW_LIST[s].x + IPC_WINDOW_LIST[s].width) &&
                            m.y >= IPC_WINDOW_LIST[s].y && m.y <= (IPC_WINDOW_LIST[s].y + IPC_WINDOW_LIST[s].height)) {
                            
                            // Define o foco global para a janela clicada
                            IPC_CONTROL->active_focus_slot = s;
                            bring_to_front(s);
                            
                            // Atualiza a barra de status com o nome da aplicação em foco
                            if (lblStatus) {
                                char caption[128];
                                strcpy(caption, "Foco: ");
                                strcat(caption, (char*)IPC_WINDOW_LIST[s].title);
                                strcpy(lblStatus->Graphic.Control.Caption, caption);
                            }

                            // 2. MELHORIA: Divisão de Zonas de Clique (Barra de Título vs. Corpo do App)
                            if (m.y <= (IPC_WINDOW_LIST[s].y + 25)) {
                                // Zona A: Clique nos 25 pixels superiores -> Ativa o Arrasto (Drag)
                                dragging_slot = s;
                                offX = m.x - IPC_WINDOW_LIST[s].x;
                                offY = m.y - IPC_WINDOW_LIST[s].y;
                            } 
                            else {
                                // Zona B: Clique no corpo do Form -> Traduz coordenadas locais e envia via IPC
                                int local_x = m.x - IPC_WINDOW_LIST[s].x;
                                int local_y = m.y - IPC_WINDOW_LIST[s].y;
                                
                                // Injeta os dados na estrutura que o software externo lê
                                IPC_WINDOW_LIST[s].local_click_x = local_x;
                                IPC_WINDOW_LIST[s].local_click_y = local_y;
                                IPC_WINDOW_LIST[s].has_click_event = 1; 
                            }
                            
                            // Processou o clique na janela do topo, encerra a busca pelas de baixo
                            break;
                        }
                    }
                }
            } else {
                // Executa o arrasto mantendo o deslocamento (offset) correto do clique inicial
                IPC_WINDOW_LIST[dragging_slot].x = m.x - offX;
                IPC_WINDOW_LIST[dragging_slot].y = m.y - offY;
            }
        } else {
            // Soltou o clique, desliga o mecanismo de arrasto
            dragging_slot = -1;
        }

        // --- PIPELINE DE RENDERIZAÇÃO ---
        // 1. Limpa a tela com o Azul Clássico do Windows (0x003A6EA5)
        graphics_clear(0x003A6EA5); 
        
        // 2. Desenha as janelas externas seguindo rigorosamente a ordem do Z-Stack (fundo para o topo)
        for (int i = 0; i < MAX_EXTERNAL_APPS; i++) {
            int s = z_stack[i];
            if (IPC_WINDOW_LIST[s].is_active) {
                compose_app_window(s);
            }
        }

        // 3. Desenha os elementos de interface nativos do Explorer (Taskbar, botões, labels)
        wm_render_pipeline();     
        
        // 4. Desenha o cursor por cima de tudo
        cursor_draw(m.x, m.y, screen_w, screen_h);
        
        // 5. Envia o buffer pronto para a memória física de vídeo (VRAM)
        video_flip(ram_buffer);
        
        // Abre mão da CPU por 10ms para dar fôlego ao multitasking do Kernel
        sys_sleep(10);
    }
}
