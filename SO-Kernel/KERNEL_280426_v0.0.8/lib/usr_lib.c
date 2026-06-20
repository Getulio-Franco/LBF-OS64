#include "usr_lib.h"
#include <stdint.h>

/* ============================================================================
 * MOTOR DE SYSCALL (Interface com o Kernel)
 * ============================================================================ */

static inline uint64_t _do_syscall(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5) {
    uint64_t ret;
    __asm__ volatile (
        "movq %4, %%r10\n\t"  /* Argumento 4 -> R10 */
        "movq %5, %%r8\n\t"   /* Argumento 5 -> R8  */
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(a4), "r"(a5)
        : "rcx", "r11", "r10", "r8", "memory"
    );
    return ret;
}

#define _syscall(n, a1, a2, a3) _do_syscall((uint64_t)(n), (uint64_t)(a1), (uint64_t)(a2), (uint64_t)(a3), 0, 0)

/* ============================================================================
 * 1. VFS & I/O PADRÃO
 * ============================================================================ */

int usr_open(char *path) { return (int)_syscall(SYS_OPEN, path, 0, 0); }
int usr_read(int fd, uint8_t *buffer, uint32_t size) { return (int)_syscall(SYS_READ, fd, buffer, size); }
int usr_write(int fd, uint8_t *buffer, uint32_t size) { return (int)_syscall(SYS_WRITE, fd, buffer, size); }
char getchar(void) { char c = 0; while (_syscall(SYS_READ, 0, &c, 1) == 0); return c; }
void putchar(char c) { _syscall(SYS_WRITE, 1, &c, 1); }

/* ============================================================================
 * 2. OPERAÇÕES DE DISCO (FAT32)
 * ============================================================================ */

void sys_readdir(void) { _syscall(SYS_FATLS, 0, 0, 0); }
void sys_read_file(const char* filename) { _syscall(SYS_FATCAT, filename, 0, 0); }
void sys_write_file(const char* src, const char* dest) { _syscall(SYS_FATCP, src, dest, 0); }
int sys_mkdir(const char* dirname) { return (int)_syscall(SYS_MKDIR, dirname, 0, 0); }
void sys_chdir(const char* dirname) { _syscall(SYS_CHDIR, dirname, 0, 0); }
int sys_xcopy(const char* src, const char* dest) { return (int)_syscall(SYS_XCOPY, src, dest, 0); }

/* ============================================================================
 * 3. GESTÃO DE PROCESSOS E SISTEMA
 * ============================================================================ */

void sys_exec(const char* filename) { _syscall(SYS_EXEC, filename, 0, 0); }
void sys_exit(void) { _syscall(SYS_EXIT, 0, 0, 0); while(1); }
int syscall_kill(uint64_t pid) { return (int)_syscall(SYS_KILL, pid, 0, 0); }
void usr_ps(void) { _syscall(SYS_PS, 0, 0, 0); }
void usr_clear(void) { _syscall(SYS_CLEAR, 0, 0, 0); }
void sleep(uint32_t ms) { _syscall(SYS_SLEEP, ms, 0, 0); }
uint64_t get_pid(void) { return _syscall(SYS_GETPID, 0, 0, 0); }
uint64_t get_system_ticks(void) { return _syscall(SYS_GET_TICKS, 0, 0, 0); }
void usr_mem_info(size_t* used, size_t* free) { _syscall(SYS_MEM_INFO, used, free, 0); }
uint64_t usr_get_info(int param_id) { return _syscall(SYS_GET_PARAM, param_id, 0, 0); }

/* ============================================================================
 * 4. FUNÇÕES GRÁFICAS (LFB / VESA)
 * ============================================================================ */

void usr_put_pixel(int x, int y, uint32_t color) { _do_syscall(SYS_VIDEO_PUTPIXEL, x, y, color, 0, 0); }
void usr_draw_rect(int x, int y, int w, int h, uint32_t color) { _do_syscall(SYS_VIDEO_RECT, x, y, w, h, color); }
void usr_draw_string(int x, int y, const char* str, uint32_t color, int scale) { _do_syscall(SYS_VIDEO_DRAWSTR, x, y, (uint64_t)str, color, scale); }
void usr_draw_hex(int x, int y, uint64_t val, uint32_t color) { _do_syscall(SYS_VIDEO_DRAWHEX, x, y, val, color, 0); }
void usr_draw_dec(int x, int y, uint32_t val, uint32_t color) { _do_syscall(SYS_VIDEO_DRAWDEC, x, y, val, color, 0); }

/* ============================================================================
 * 5. API GRÁFICA (GUI COMPONENTES 100-129)
 * ============================================================================ */

uint64_t sys_gui_create_form(const char* n, const char* c) { return _syscall(SYS_GUI_CREATE_FORM, (uint64_t)n, (uint64_t)c, 0); }
uint64_t sys_gui_create_button(uint64_t p, const char* n, const char* c) { return _syscall(SYS_GUI_CREATE_BUTTON, p, (uint64_t)n, (uint64_t)c); }
uint64_t sys_gui_create_edit(uint64_t p, const char* n) { return _syscall(SYS_GUI_CREATE_EDIT, p, (uint64_t)n, 0); }
uint64_t sys_gui_create_label(uint64_t p, const char* n, const char* c) { return _syscall(SYS_GUI_CREATE_LABEL, p, (uint64_t)n, (uint64_t)c); }
uint64_t sys_gui_create_checkbox(uint64_t p, const char* n, const char* c) { return _syscall(SYS_GUI_CREATE_CHECKBOX, p, (uint64_t)n, (uint64_t)c); }
uint64_t sys_gui_create_panel(uint64_t p, const char* n) { return _syscall(SYS_GUI_CREATE_PANEL, p, (uint64_t)n, 0); }
uint64_t sys_gui_create_memo(uint64_t p, const char* n) { return _syscall(SYS_GUI_CREATE_MEMO, p, (uint64_t)n, 0); }
uint64_t sys_gui_create_combobox(uint64_t p, const char* n) { return _syscall(SYS_GUI_CREATE_COMBO, p, (uint64_t)n, 0); }
uint64_t sys_gui_create_radio(uint64_t p, const char* n, const char* c) { return _syscall(SYS_GUI_CREATE_RADIO, p, (uint64_t)n, (uint64_t)c); }
uint64_t sys_gui_create_image(uint64_t p, const char* n) { return _syscall(SYS_GUI_CREATE_IMAGE, p, (uint64_t)n, 0); }

void sys_gui_render(uint64_t obj) { _syscall(SYS_GUI_RENDER, obj, 0, 0); }
void sys_gui_show(uint64_t obj) { _syscall(SYS_GUI_SHOW, obj, 0, 0); }
void sys_gui_hide(uint64_t obj) { _syscall(SYS_GUI_HIDE, obj, 0, 0); }
uint64_t sys_gui_get_event(void) { return _syscall(SYS_GUI_GET_EVENT, 0, 0, 0); }

/* --- Funções de Propriedade --- */

void sys_gui_set_prop(uint64_t obj, uint64_t prop_id, uint64_t value) {
    __asm__ volatile (
        "int $0x80"
        : 
        : "a"((uint64_t)SYS_GUI_SET_PROP), // RAX: Número da Syscall
          "D"(obj),                         // RDI: arg1 (Objeto)
          "S"(prop_id),                     // RSI: arg2 (Propriedade)
          "d"(value)                        // RDX: arg3 (Valor)
        : "memory"
    );
}

uint64_t sys_gui_get_prop(uint64_t obj, uint64_t prop_id) {
    uint64_t ret;
    __asm__ volatile (
        "int $0x80"
        : "=a"(ret)                         // Retorno em RAX
        : "a"((uint64_t)SYS_GUI_GET_PROP), // RAX: Número da Syscall
          "D"(obj),                         // RDI: arg1 (Objeto)
          "S"(prop_id)                      // RSI: arg2 (Propriedade)
        : "memory"
    );
    return ret;
}

/*void sys_gui_set_prop(uint64_t obj, uint64_t prop_id, uint64_t value) {
    asm volatile ("int $0x80" 
                  : 
                  : "a"(SYS_GUI_SET_PROP), "b"(obj), "c"(prop_id), "d"(value)
                  : "memory");
}*/

/*uint64_t sys_gui_get_prop(uint64_t obj, uint64_t prop_id) {
    uint64_t ret;
    asm volatile ("int $0x80" 
                  : "=a"(ret)
                  : "a"(SYS_GUI_GET_PROP), "b"(obj), "c"(prop_id)
                  : "memory");
    return ret;
}*/

/* --- Callbacks e Eventos --- */

void sys_gui_set_callback(uint64_t obj, void (*callback)(void*)) { _syscall(SYS_GUI_SET_CALLBACK, obj, (uint64_t)callback, 0); }
void sys_gui_push_event(uint64_t event_id) { _syscall(SYS_GUI_PUSH_EVENT, event_id, 0, 0); }
void sys_gui_process_click(void* form, int x, int y) { _syscall(SYS_GUI_PROCESS_CLICK, (uint64_t)form, (uint64_t)x, (uint64_t)y); }
void sys_gui_process_hover(void* form, int x, int y) { _syscall(SYS_GUI_PROCESS_HOVER, (uint64_t)form, (uint64_t)x, (uint64_t)y); }
void sys_set_as_desktop(uint64_t form_ptr) { _syscall(SYS_SET_DESKTOP, form_ptr, 0, 0); }

// O programador usa esta função amigável - Gerenciador de Tarefas Grafico
//int usr_get_ps_data(TProcessInfo* buffer, int max_count) {return (int)_do_syscall(SYS_PS, (uint64_t)buffer, (uint64_t)max_count, 0, 0, 0);}


