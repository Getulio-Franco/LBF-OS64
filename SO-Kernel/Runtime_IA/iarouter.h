#ifndef IAROUTER_H
#define IAROUTER_H

typedef struct {
    int acionar_math;
    int acionar_logic;
    char comando_limpo[64]; // Ex: "soma 1 1" ou "and 1 0" - limpo do ruído
} RouterDecision;

RouterDecision iarouter_processar(const char* texto_usuario);

#endif
