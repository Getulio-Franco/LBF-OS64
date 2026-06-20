#ifndef RUNTIME_OS_EDIT_H
#define RUNTIME_OS_EDIT_H

#include "../controls.h"

// Definição da estrutura para o componente Edit em Modo Runtime
typedef struct {
    TOSControl base;  // Herança da estrutura base unificada (Substitui TVCLControl)
    char Text[256];   // Buffer local de texto espelhado (Ring 3)
    int CursorPos;
    int MaxLength;
} TOSEdit;

// Protótipo da função de criação adaptada para o TAppEnvironment
TOSEdit* OS_CreateEdit(TAppEnvironment* app, int x, int y, int w, int h, const char* text);

// Função para processar as teclas capturadas redirecionadas pelo motor central
void OS_Edit_AddChar(TOSEdit* edit, char key);
void OS_Edit_SetFocus(TOSEdit* edit);

#endif // RUNTIME_OS_EDIT_H
