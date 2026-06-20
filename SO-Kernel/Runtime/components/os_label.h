#ifndef RUNTIME_OS_LABEL_H
#define RUNTIME_OS_LABEL_H

#include "../controls.h"

// Instancia um novo Label associado ao ambiente do App
// Retorna diretamente o ponteiro base TOSControl para simplificar o gerenciamento
TOSControl* OS_CreateLabel(TAppEnvironment* app, int x, int y, const char* caption);

#endif // RUNTIME_OS_LABEL_H
