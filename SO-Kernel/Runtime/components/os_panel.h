#ifndef RUNTIME_OS_PANEL_H
#define RUNTIME_OS_PANEL_H

#include "../controls.h"

// Definição da estrutura para o componente Panel em Modo Runtime
typedef struct {
    TOSControl base;    // Herança estrutural unificada (Substitui TVCLControl)
    int BevelWidth;     // Propriedade exclusiva do painel para espessura do chanfro
} TOSPanel;

// Instancia um novo Painel (Container) associado ao ambiente do App
// Retorna o ponteiro base TOSControl para facilitar a indexação no vetor de controles
TOSControl* OS_CreatePanel(TAppEnvironment* app, int x, int y, int w, int h);

void OS_SetParent(TOSControl* control, TOSControl* new_parent);

#endif // RUNTIME_OS_PANEL_H
