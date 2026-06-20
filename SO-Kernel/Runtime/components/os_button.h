#ifndef RUNTIME_OS_BUTTON_H
#define RUNTIME_OS_BUTTON_H

#include "../controls.h"

// Instancia um novo Botão associado ao ambiente do App com geometria dinâmica
// Retorna o ponteiro base TOSControl direto, economizando casts na aplicação
TOSControl* OS_CreateButton(TAppEnvironment* app, int x, int y, int w, int h, const char* caption);

#endif // RUNTIME_OS_BUTTON_H
