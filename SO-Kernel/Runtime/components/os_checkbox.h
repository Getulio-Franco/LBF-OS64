#ifndef RUNTIME_OS_CHECKBOX_H
#define RUNTIME_OS_CHECKBOX_H

#include "../controls.h"

// Definição da estrutura para o componente CheckBox em Modo Runtime
typedef struct {
    TOSControl base;  // Herança estrutural unificada (Substitui TVCLControl)
    bool Checked;     // Estado interno do Check (true = marcado, false = desmarcado)
} TOSCheckBox;

// Instancia um novo CheckBox associado ao ambiente do App
TOSCheckBox* OS_CreateCheckBox(TAppEnvironment* app, int x, int y, const char* caption);

// Função para alternar o estado do CheckBox (Toggle) e atualizar o Kernel
void OS_CheckBox_Toggle(TOSCheckBox* cb);

#endif // RUNTIME_OS_CHECKBOX_H
