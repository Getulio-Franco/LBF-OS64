#ifndef RUNTIME_OS_COMBOBOX_H
#define RUNTIME_OS_COMBOBOX_H

#include "../controls.h"

// Definição da estrutura para o componente ComboBox em Modo Runtime
typedef struct {
    TOSControl base;       // Herança estrutural unificada (Substitui TVCLControl)
    char Items[8][16];     // Guarda até 8 itens de até 15 caracteres + nulo
    int ItemCount;         // Quantidade atual de itens inseridos
    int ItemIndex;         // Índice do item selecionado atualmente (0, 1, 2...)
    bool DroppedDown;      // Retido para compatibilidade futura com listas expansíveis
} TOSComboBox;

// Instancia um novo ComboBox associado ao ambiente do App
TOSComboBox* OS_CreateComboBox(TAppEnvironment* app, int x, int y, int w, int h);

// Adiciona um item na matriz local do componente
void OS_ComboBox_AddItem(TOSComboBox* combo, const char* texto);

// Faz a rotação (carrossel) dos itens e atualiza o display visual
void OS_ComboBox_Rotate(TOSComboBox* combo);

#endif // RUNTIME_OS_COMBOBOX_H
