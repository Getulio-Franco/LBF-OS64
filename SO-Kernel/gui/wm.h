#ifndef WM_H
#define WM_H

#include "../gui/gui.h"
#include <stdint.h> 

// --- PROTOCOLO DE COMPOSIÇÃO (NOVO IPC) ---
#define MAX_EXTERNAL_APPS 10

/**
 * Explicação Técnico-Visual da Memória Compartilhada:
 * 1. Lemos o endereço base da VRAM em 0x508.
 * 2. Reservamos um espaço seguro (offset de 10MB) para comunicação entre processos.
 * 3. O Explorer e os Apps mapeiam esta região para trocar coordenadas e ponteiros de pixels.
 */
#define GET_FB_ADDR      ((uint64_t)(*((volatile uint32_t*)0x508)))
#define IPC_SHARED_ADDR  (GET_FB_ADDR + (uint64_t)0xA00000)

typedef struct {
    int is_active;        
    uint64_t buffer_ptr_0; 
    uint64_t buffer_ptr_1; 
    int active_buffer;     
    int pid;               
    int x;                 
    int y;                 
    int width;             
    int height;            
    char title[32];       
    
    // === NOVOS MEMBROS: INTERAÇÃO IPC ===
    int local_click_x;    
    int local_click_y;    
    int has_click_event;  
} AppWindowInfo;

typedef struct {
    int active_focus_slot; 
    uint64_t shared_fb_ptr; 
    int screen_width;
    int screen_height;
    int screen_pitch;  
    int reserved[4];        
} IPC_Global_Control;

#define IPC_CTRL_ADDR    (IPC_SHARED_ADDR + (uintptr_t)(sizeof(AppWindowInfo) * MAX_EXTERNAL_APPS))

#define IPC_WINDOW_LIST ((volatile AppWindowInfo*)(uintptr_t)IPC_SHARED_ADDR)
#define IPC_CONTROL      ((volatile IPC_Global_Control*)(uintptr_t)IPC_CTRL_ADDR)

extern int wm_mouse_x;
extern int wm_mouse_y;
extern int wm_mouse_show;

/* --- Funções do Ciclo Gráfico --- */
void wm_init(void);
void wm_set_desktop(TForm* form); 
void wm_add_window(TForm* form);
void wm_remove_window(TForm* form);
void wm_handle_mouse_event(int x, int y, int event);
void wm_handle_keyboard_event(char key);
void wm_render_pipeline(void);

#endif
