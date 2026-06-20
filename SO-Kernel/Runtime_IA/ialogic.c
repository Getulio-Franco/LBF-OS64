#include "ialogic.h"
#include "../system/string.h"

// ============================================================================
// [PARTE 1: SUBSISTEMA SEMÂNTICO (ANTIGO IALANG LÓGICO)]
// ============================================================================

#define TAM_VETOR_SEMANTICO 4  // Tamanho do vetor de significado de cada palavra
#define TOTAL_PALAVRAS 12      // Tamanho do vocabulário da IA

typedef struct {
    const char* palavra;
    int token_id;
} DicionarioToken;

// Dicionário de Tokens integrado para Portas Lógicas
static const DicionarioToken VOCABULARIO[TOTAL_PALAVRAS] = {
    {"and", 0}, {"or", 1}, {"not", 2}, {"nand", 3}, {"nor", 4}, {"xor", 5}, {"xnor", 6},
    {"qual", 7}, {"resultado", 8}, {"porta", 9}, {"nivel", 10}, {"logico", 11}
};

// Matriz de Embedding integrada para representação espacial das palavras
static const float MATRIZ_EMBEDDING[TOTAL_PALAVRAS][TAM_VETOR_SEMANTICO] = {
    { 0.95f, -0.10f,  0.05f,  0.10f}, // 0: and
    {-0.15f,  0.88f,  0.12f, -0.05f}, // 1: or
    { 0.05f,  0.02f,  0.91f,  0.20f}, // 2: not
    { 0.75f, -0.30f,  0.45f,  0.15f}, // 3: nand
    {-0.45f,  0.65f,  0.35f, -0.10f}, // 4: nor
    { 0.20f,  0.15f, -0.20f,  0.89f}, // 5: xor
    { 0.50f,  0.10f,  0.30f,  0.75f}, // 6: xnor
    { 0.01f,  0.01f,  0.02f,  0.01f}, // 7: qual
    { 0.02f,  0.02f,  0.01f,  0.02f}, // 8: resultado
    { 0.05f,  0.05f,  0.05f,  0.05f}, // 9: porta
    { 0.01f,  0.01f,  0.01f,  0.01f}, // 10: nivel
    { 0.02f,  0.02f,  0.02f,  0.02f}  // 11: logico
};

// Pesos do Classificador Linear de Intenções (Identificação da Porta)
static const float PESOS_CLASSIFICADOR[7][TAM_VETOR_SEMANTICO] = {
    { 1.20f, -0.50f, -0.50f, -0.50f}, // Neurônio 0: AND
    {-0.50f,  1.20f, -0.50f, -0.50f}, // Neurônio 1: OR
    {-0.50f, -0.50f,  1.20f, -0.50f}, // Neurônio 2: NOT
    { 0.80f, -0.80f,  0.60f, -0.50f}, // Neurônio 3: NAND
    {-0.80f,  0.80f,  0.50f, -0.50f}, // Neurônio 4: NOR
    {-0.50f, -0.50f, -0.50f,  1.20f}, // Neurônio 5: XOR
    { 0.60f, -0.50f, -0.50f,  0.90f}  // Neurônio 6: XNOR
};

static int obter_token(const char* palavra) {
    for (int i = 0; i < TOTAL_PALAVRAS; i++) {
        if (strcmp(palavra, VOCABULARIO[i].palavra) == 0) {
            return VOCABULARIO[i].token_id;
        }
    }
    return -1;
}

// ============================================================================
// [PARTE 2: SUBSISTEMA MULTICAMADAS (EXECUÇÃO DA REDE MLP)]
// ============================================================================

PesosMLP BancoDeDadosPortas[7] = {
    { 0.5f,  0.5f, -0.7f,   0.0f,  0.0f,  0.0f,   1.0f,  0.0f, -0.5f }, // AND
    { 0.5f,  0.5f, -0.2f,   0.0f,  0.0f,  0.0f,   1.0f,  0.0f, -0.5f }, // OR
    {-1.0f,  0.0f,  0.5f,   0.0f,  0.0f,  0.0f,   1.0f,  0.0f, -0.5f }, // NOT
    {-0.5f, -0.5f,  0.7f,   0.0f,  0.0f,  0.0f,   1.0f,  0.0f, -0.5f }, // NAND
    {-0.5f, -0.5f,  0.2f,   0.0f,  0.0f,  0.0f,   1.0f,  0.0f, -0.5f }, // NOR
    { 1.0f,  1.0f, -0.5f,   1.0f,  1.0f, -1.5f,   1.0f, -2.0f, -0.5f }, // XOR
    { 1.0f,  1.0f, -0.5f,   1.0f,  1.0f, -1.5f,  -2.0f,  1.5f,  0.7f }  // XNOR
};

static int mlp_ativar(float soma) {
    return (soma > 0) ? 1 : 0;
}

int mlp_processar(PesosMLP* p, float x1, float x2) {
    float soma_h1 = (x1 * p->h1_w1) + (x2 * p->h1_w2) + p->h1_bias;
    int out_h1 = mlp_ativar(soma_h1);

    int out_h2 = 0;
    if (p->h2_w1 != 0.0f || p->h2_w2 != 0.0f || p->h2_bias != 0.0f) {
        float soma_h2 = (x1 * p->h2_w1) + (x2 * p->h2_w2) + p->h2_bias;
        out_h2 = mlp_ativar(soma_h2);
    }

    float soma_out = ((float)out_h1 * p->out_w1) + ((float)out_h2 * p->out_w2) + p->out_bias;
    return mlp_ativar(soma_out);
}

// ============================================================================
// [PARTE 3: INTERFACE DE ENTRADA UNIFICADA]
// ============================================================================

void ialogic_interpretar(const char* texto_usuario, char* buffer_resposta) {
    float vetor_acumulado[TAM_VETOR_SEMANTICO] = {0.0f, 0.0f, 0.0f, 0.0f};
    float x1 = 0.0f, x2 = 0.0f;
    int cont_entradas = 0;

    char texto[256];
    int idx = 0;

    // Normalização de caixa e extração imediata de bits (0 ou 1)
    for (int i = 0; texto_usuario[i] != '\0' && i < 255; i++) {
        char c = texto_usuario[i];
        if (c >= 'A' && c <= 'Z') texto[i] = c + 32;
        else texto[i] = c;
        
        if (c == '0' || c == '1') {
            float val = (c == '1') ? 1.0f : 0.0f;
            if (cont_entradas == 0) { x1 = val; cont_entradas = 1; }
            else if (cont_entradas == 1) { x2 = val; cont_entradas = 2; }
        }
        idx = i + 1;
    }
    texto[idx] = '\0';

    // Varredura por tokens de texto para alimentar o acumulador geométrico
    char* token_str = strtok(texto, " ,.?!()");
    while (token_str != NULL) {
        int token_id = obter_token(token_str);
        if (token_id != -1) {
            for (int d = 0; d < TAM_VETOR_SEMANTICO; d++) {
                vetor_acumulado[d] += MATRIZ_EMBEDDING[token_id][d];
            }
        }
        token_str = strtok(NULL, " ,.?!()");
    }

    // Inferência por multiplicação de matrizes da intenção lógica
    int id_porta_escolhida = 0;
    float maior_ativacao = -9999.0f;

    for (int p = 0; p < 7; p++) {
        float ativacao_porta = 0.0f;
        for (int d = 0; d < TAM_VETOR_SEMANTICO; d++) {
            ativacao_porta += vetor_acumulado[d] * PESOS_CLASSIFICADOR[p][d];
        }
        if (ativacao_porta > maior_ativacao) {
            maior_ativacao = ativacao_porta;
            id_porta_escolhida = p;
        }
    }

    // Processamento estrito na rede MLP interna do arquivo
    int resultado = mlp_processar(&BancoDeDadosPortas[id_porta_escolhida], x1, x2);

    // Formatação unificada do resultado bruto preparado para o Tradutor futuro
    strcpy(buffer_resposta, "RAW_LOGIC_RESULT: ");
    strcat(buffer_resposta, (resultado == 1) ? "1\n" : "0\n");
}
