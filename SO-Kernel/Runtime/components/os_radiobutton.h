#ifndef RUNTIME_OS_RADIOBUTTON_H
#define RUNTIME_OS_RADIOBUTTON_H

#include "../controls.h"

// Definição da estrutura para o componente RadioButton em Modo Runtime
typedef struct {
    TOSControl base;   // Herança estrutural unificada (Substitui TVCLControl)
    bool Checked;      // Estado interno de seleção
    int GroupIndex;    // Identificador de grupo (Apenas um ativo por grupo)
} TOSRadioButton;

// Instancia um novo RadioButton associado ao ambiente do App
TOSRadioButton* OS_CreateRadioButton(TAppEnvironment* app, int x, int y, const char* caption);

// Função que gerencia a seleção exclusiva do RadioButton e limpa o grupo
void OS_RadioButton_Select(TAppEnvironment* app, TOSRadioButton* target_rb);

#endif // RUNTIME_OS_RADIOBUTTON_H
