#ifndef USR_LIB_H
#define USR_LIB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * IDs DAS SYSCALLS - TABELA MESTRA LBF-VESA
 * ============================================================================
 */

// BLOCO 0-9: VFS / I/O GENÉRICO
#define SYS_OPEN            0
#define SYS_READ            1
#define SYS_WRITE           2
#define SYS_CLOSE           3
#define SYS_CLEAR           4  
#define SYS_MEM_INFO        5

// BLOCO 10-18: FAT32 / DISCO
#define SYS_FATLS           10
#define SYS_FATCAT          11
#define SYS_FATCP           12
#define SYS_FATRM           14
#define SYS_FATAPPEND       15
#define SYS_MKDIR           16
#define SYS_CHDIR           17
#define SYS_XCOPY           18

// BLOCO 19-29: PROCESSOS
#define SYS_EXEC            19
#define SYS_EXIT            20
#define SYS_KILL            21
#define SYS_PS              22
#define SYS_GETPID          23
#define SYS_SET_DESKTOP     24
#define SYS_GET_PS_DATA     25

// BLOCO 30-49: SISTEMA & CLOCK
#define SYS_GET_PARAM       40
#define SYS_SET_PARAM       41
#define SYS_SLEEP           42
#define SYS_GET_TICKS       43
#define SYS_GET_RTC         44

// BLOCO 50-59: VÍDEO DIRETO
#define SYS_VIDEO_PUTPIXEL   50
#define SYS_VIDEO_RECT       51
#define SYS_VIDEO_DRAWSTR    52
#define SYS_VIDEO_DRAWHEX    53
#define SYS_VIDEO_DRAWDEC    54

// BLOCO 100+: GUI ENGINE
#define SYS_GUI_CREATE_FORM      100
#define SYS_GUI_CREATE_BUTTON    101
#define SYS_GUI_CREATE_EDIT      102
#define SYS_GUI_CREATE_LABEL     103
#define SYS_GUI_CREATE_CHECKBOX  104
#define SYS_GUI_CREATE_PANEL     105
#define SYS_GUI_CREATE_MEMO      106
#define SYS_GUI_CREATE_COMBO     107
#define SYS_GUI_CREATE_RADIO     108
#define SYS_GUI_CREATE_IMAGE     109

#define SYS_GUI_RENDER           115
#define SYS_GUI_SET_PROP         116
#define SYS_GUI_GET_PROP         117
#define SYS_GUI_SHOW             118
#define SYS_GUI_HIDE             119
#define SYS_GUI_GET_EVENT        125
#define SYS_GUI_SET_CALLBACK     126
#define SYS_GUI_PUSH_EVENT       127
#define SYS_GUI_PROCESS_CLICK    128
#define SYS_GUI_PROCESS_HOVER    129

/* ============================================================================
 * PROTÓTIPOS DA USR_LIB (Ponte Interna do Sistema)
 * ============================================================================
 */

// I/O, Terminal e Sistema
int      usr_open(char *path);
int      usr_read(int fd, uint8_t *buffer, uint32_t size);
int      usr_write(int fd, uint8_t *buffer, uint32_t size);
void     usr_clear(void);
void     usr_ps(void);
void     usr_mem_info(size_t* used, size_t* free);
uint64_t get_pid(void);
uint64_t get_system_ticks(void);
uint64_t usr_get_info(int param_id);
void     sleep(uint32_t ms);
char     getchar(void);
void     putchar(char c);

// VFS / FAT32
void     sys_readdir(void);
void     sys_read_file(const char* filename);
void     sys_write_file(const char* src, const char* dest);
int      sys_mkdir(const char* dirname);
void     sys_chdir(const char* dirname);
int      sys_xcopy(const char* src, const char* dest);

// Processos
void     sys_exec(const char* filename);
void     sys_exit(void);
int      syscall_kill(uint64_t pid);

// Driver de Vídeo (LFB)
void     usr_put_pixel(int x, int y, uint32_t color);
void     usr_draw_rect(int x, int y, int w, int h, uint32_t color);
void     usr_draw_string(int x, int y, const char* str, uint32_t color, int scale);
void     usr_draw_hex(int x, int y, uint64_t val, uint32_t color);
void     usr_draw_dec(int x, int y, uint32_t val, uint32_t color);

// GUI Engine - Criação
uint64_t sys_gui_create_form(const char* name, const char* caption);
uint64_t sys_gui_create_button(uint64_t parent, const char* name, const char* caption);
uint64_t sys_gui_create_edit(uint64_t parent, const char* name);
uint64_t sys_gui_create_label(uint64_t parent, const char* name, const char* caption);
uint64_t sys_gui_create_checkbox(uint64_t parent, const char* name, const char* caption);
uint64_t sys_gui_create_panel(uint64_t parent, const char* name);
uint64_t sys_gui_create_memo(uint64_t parent, const char* name);
uint64_t sys_gui_create_combobox(uint64_t parent, const char* name);
uint64_t sys_gui_create_radio(uint64_t parent, const char* name, const char* caption);
uint64_t sys_gui_create_image(uint64_t parent, const char* name);

// GUI Engine - Controle e Propriedades (Padronizado uint64_t)
void     sys_gui_render(uint64_t obj);
void     sys_gui_show(uint64_t obj);
void     sys_gui_hide(uint64_t obj);
void     sys_gui_set_prop(uint64_t obj, uint64_t prop_id, uint64_t value);
uint64_t sys_gui_get_prop(uint64_t obj, uint64_t prop_id);

// Eventos e Callbacks
uint64_t sys_gui_get_event(void);
void     sys_gui_push_event(uint64_t event_id);
void     sys_gui_process_click(void* form, int x, int y);
void     sys_gui_process_hover(void* form, int x, int y);
void     sys_gui_set_callback(uint64_t obj, void (*callback)(void*));

// Window Manager / Desktop
void     sys_set_as_desktop(uint64_t form_ptr);

//int      usr_get_ps_data(TProcessInfo* buffer, int max_count);

#endif
