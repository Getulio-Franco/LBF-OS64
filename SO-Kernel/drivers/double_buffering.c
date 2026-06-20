#include "double_buffering.h"
#include "../drivers/video.h"
#include "../drivers/proc.h"

// --- VARIÁVEIS EXTERNAS ---
extern volatile int mouse_needs_update; 
extern int refresh_screen;
extern volatile uint64_t system_ticks;

// --- VARIÁVEIS DE FPS E CONTROLE (Mantidas as mesmas) ---
static uint64_t fps_frames = 0;      
static uint64_t current_fps = 0;    
static uint64_t last_fps_time = 0; 
static char fps_text[16];

// Função mini_itoa mantida oculta por brevidade...
static void mini_itoa(uint64_t n, char* s) { /* Seu código intacto */ }
static void draw_fps_counter(uint64_t fps) { /* Seu código intacto */ }

/**
 * LÓGICA PRINCIPAL DO MOTOR DE RENDERIZAÇÃO
 */
void db_swap_buffers() {
    uint64_t current_time = system_ticks * 10; 
    
    if (current_time - last_fps_time >= 1000) {
        current_fps = fps_frames;
        fps_frames = 0;
        last_fps_time = current_time;
    }

    if (mouse_needs_update || refresh_screen) {
        __asm__ volatile ("cli"); 
        
        mouse_needs_update = 0;
        refresh_screen = 0;
        
        // Passo A: O Window Manager monta todo o quebra-cabeça na RAM!
       // wm_render_pipeline(); 
      //  video_flush(); // adicionado para resoler numeros na tela
        // Passo B: O FPS é injetado por cima (se necessário)
        process_t* cur = get_current_process();
        if (cur && cur->pid == 1) {
            // draw_fps_counter(current_fps);
        }
       // clear_screen(0x003A6EA5);  //Colocar o deskto ou terminal com tela azul
        // Passo C: Empurra o buffer de RAM para a VRAM (Placa de vídeo)
        video_flush(); 
        
        __asm__ volatile ("sti");
        fps_frames++;
    }
}

uint64_t db_get_fps() { return current_fps; }
