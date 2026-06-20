#ifndef RUNTIME_OS_MEMO_H
#define RUNTIME_OS_MEMO_H

#include "../controls.h"

// ID da Propriedade de Rolagem (Adicione nos seus declares globais se necessário)
#define PROP_SCROLL_Y 65

// Definição da estrutura para o componente Memo em Modo Runtime
typedef struct {
    TOSControl base;    // Herança estrutural unificada (Substitui TVCLControl)
    char* Buffer;       // Ponteiro para o bloco de texto dinâmico na heap do Ring 3
    int AllocatedSize;  // Capacidade atual alocada na heap (em bytes)
    int TextLength;     // Quantidade atual de caracteres inseridos
    int CursorX;        // Coordenadas internas virtuais do cursor
    int CursorY;
    int ScrollY;        // ADICIONADO: Sincronização do scroll em Ring 3
} TOSMemo;

// Instancia um novo Memo dinâmico associado ao ambiente do App
TOSMemo* OS_CreateMemo(TAppEnvironment* app, int x, int y, int w, int h);

// Processa a digitação gerenciando dinamicamente o buffer alocado na heap
void OS_Memo_AddChar(TOSMemo* memo, char key);

// ADICIONADO: Processa a inserção de strings completas (Textos, Logs, Leituras de Porta)
void OS_Memo_AddStr(TOSMemo* memo, const char* str);

// Adicione no seu os_memo.h
void OS_Memo_SetFocus(TOSMemo* memo);

// ADICIONADO: Permite interagir programaticamente ou via mouse com o scroll
void OS_Memo_SetScroll(TOSMemo* memo, int value);

#endif // RUNTIME_OS_MEMO_H
