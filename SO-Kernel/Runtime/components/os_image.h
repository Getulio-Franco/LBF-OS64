#ifndef RUNTIME_OS_IMAGE_H
#define RUNTIME_OS_IMAGE_H

#include "../controls.h"

// Instancia um novo componente de Imagem associado ao ambiente do App
// Retorna o ponteiro base TOSControl direto, mantendo o padrão leve de execução
TOSControl* OS_CreateImage(TAppEnvironment* app, int x, int y, int w, int h, const char* path);

#endif // RUNTIME_OS_IMAGE_H
