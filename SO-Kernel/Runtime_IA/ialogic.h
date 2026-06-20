#ifndef IALOGIC_H
#define IALOGIC_H

// Estrutura de pesos calibrada para a rede MLP
typedef struct {
    float h1_w1, h1_w2, h1_bias;
    float h2_w1, h2_w2, h2_bias;
    float out_w1, out_w2, out_bias;
} PesosMLP;

extern PesosMLP BancoDeDadosPortas[7];

// Processa a rede neural multicamadas (MLP) internamente
int mlp_processar(PesosMLP* p, float x1, float x2);

// Nova função unificada: Interpreta o texto e extrai o resultado lógico bruto
void ialogic_interpretar(const char* texto_usuario, char* buffer_resposta);

#endif
