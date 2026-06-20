#ifndef RUNTIME_CONTROLS_H
#define RUNTIME_CONTROLS_H

#include <stdint.h>
#include <stdbool.h>
#include "../system/liblib.h"
#include "../system/malloc.h"
#include "../system/string.h"
#include "../gui/gui.h" // AQUI JÁ TEMOS OS TYPE_EDIT, TYPE_LABEL, etc.

#define MAX_APP_CONTROLS 64

// Estrutura Base para os componentes em Modo Runtime
typedef struct {
    int Type;
    int Left;
    int Top;
    int Width;
    int Height;
    uint64_t KernelHandle; // Ponteiro retornado pelo Kernel (gui_create_...)
    bool IsSelected;       // Útil para saber se tem o foco do teclado
    char Name[32];
} TOSControl;

// Contexto do Aplicativo (.elf)
typedef struct {
    int SlotID;
    TForm* MainWindow;
    TOSControl* Controls[MAX_APP_CONTROLS];
    int ControlCount;
    TOSControl* ActiveFocus; 
} TAppEnvironment;

// Motor Central
void OS_InitApplication(TAppEnvironment* app, int slot_id, const char* title, int w, int h);
void OS_RegisterControl(TAppEnvironment* app, TOSControl* ctrl, const char* prefix);

// Funções de Eventos
bool OS_ProcessMouseClick(TAppEnvironment* app, int mouse_x, int mouse_y);
void OS_ProcessKeyboard(TAppEnvironment* app, char key);

#endif // RUNTIME_CONTROLS_H
