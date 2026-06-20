#include "iamath.h"
#include "../system/string.h"

// ============================================================================
// [PARTE 1: SUBSISTEMA SEMÂNTICO (ANTIGO IALANG MATEMÁTICO)]
// ============================================================================

#define TAM_VETOR_SEMANTICO 4  // Cada dimensão foca em uma operação matemática
#define TOTAL_PALAVRAS 15      // Vocabulário matemático total

typedef struct {
    const char* palavra;
    int token_id;
} DicionarioToken;

// Dicionário de Tokens integrado
static const DicionarioToken VOCABULARIO[TOTAL_PALAVRAS] = {
    {"some", 0}, {"soma", 1}, {"mais", 2}, {"adicione", 3},
    {"subtraia", 4}, {"menos", 5}, {"diminua", 6},
    {"multiplique", 7}, {"vezes", 8}, {"produto", 9},
    {"divida", 10}, {"dividir", 11}, {"por", 12},
    {"qual", 13}, {"resultado", 14}
};

// Matriz de Embedding integrada e calibrada
static const float MATRIZ_EMBEDDING[TOTAL_PALAVRAS][TAM_VETOR_SEMANTICO] = {
    { 1.00f,  0.00f,  0.00f,  0.00f}, // 0: some        -> Ativa Dimensão 0 (Soma)
    { 1.00f,  0.00f,  0.00f,  0.00f}, // 1: soma        -> Ativa Dimensão 0 (Soma)
    { 1.00f,  0.00f,  0.00f,  0.00f}, // 2: mais        -> Ativa Dimensão 0 (Soma)
    { 1.00f,  0.00f,  0.00f,  0.00f}, // 3: adicione    -> Ativa Dimensão 0 (Soma)
    { 0.00f,  1.00f,  0.00f,  0.00f}, // 4: subtraia    -> Ativa Dimensão 1 (Subtração)
    { 0.00f,  1.00f,  0.00f,  0.00f}, // 5: menos       -> Ativa Dimensão 1 (Subtração)
    { 0.00f,  1.00f,  0.00f,  0.00f}, // 6: diminua     -> Ativa Dimensão 1 (Subtração)
    { 0.00f,  0.00f,  1.00f,  0.00f}, // 7: multiplique -> Ativa Dimensão 2 (Multiplicação)
    { 0.00f,  0.00f,  1.00f,  0.00f}, // 8: vezes       -> Ativa Dimensão 2 (Multiplicação)
    { 0.00f,  0.00f,  1.00f,  0.00f}, // 9: produto     -> Ativa Dimensão 2 (Multiplicação)
    { 0.00f,  0.00f,  0.00f,  1.00f}, // 10: divida     -> Ativa Dimensão 3 (Divisão)
    { 0.00f,  0.00f,  0.00f,  1.00f}, // 11: dividir    -> Ativa Dimensão 3 (Divisão)
    { 0.00f,  0.00f,  0.00f,  0.50f}, // 12: por        -> Ativa de leve a Dimensão 3
    { 0.01f,  0.01f,  0.01f,  0.01f}, // 13: qual       -> Neutro
    { 0.02f,  0.02f,  0.02f,  0.02f}  // 14: resultado  -> Neutro
};

// Pesos do Classificador de Intenção integrado
static const float PESOS_CLASSIFICADOR[4][TAM_VETOR_SEMANTICO] = {
    { 1.50f, -0.50f, -0.50f, -0.50f}, // Neurônio 0: Soma
    {-0.50f,  1.50f, -0.50f, -0.50f}, // Neurônio 1: Subtração
    {-0.50f, -0.50f,  1.50f, -0.50f}, // Neurônio 2: Multiplicação
    {-0.50f, -0.50f, -0.50f,  1.50f}  // Neurônio 3: Divisão
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
// [PARTE 2: SUBSISTEMA DE EXECUÇÃO NEURAL (ALU NEURAL)]
// ============================================================================

const NeuronioMatematico BancoDeDadosMatematico[] = {
    { 1.0f,  1.0f,  0.0f, 0}, // ID 0: Soma
    { 1.0f, -1.0f,  0.0f, 0}, // ID 1: Subtracão
    { 0.0f,  0.0f,  0.0f, 1}, // ID 2: Multiplicação
    { 0.0f,  0.0f,  0.0f, 2}  // ID 3: Divisão
};

float iamath_processar(const NeuronioMatematico* modelo, float x1, float x2) {
    if (modelo->tipo_operacao == 0) {
        return (x1 * modelo->peso_x1) + (x2 * modelo->peso_x2) + modelo->bias;
    } 
    else if (modelo->tipo_operacao == 1) {
        return x1 * x2;
    } 
    else if (modelo->tipo_operacao == 2) {
        if (x2 == 0.0f) return 0.0f; 
        return x1 / x2;
    }
    return 0.0f;
}

// ============================================================================
// [PARTE 3: INTERFACE DE ENTRADA UNIFICADA (A PORTA DE ENTRADA DO AGENTE)]
// ============================================================================

void iamath_interpretar(const char* texto_usuario, char* buffer_resposta) {
    float vetor_acumulado[TAM_VETOR_SEMANTICO] = {0.0f, 0.0f, 0.0f, 0.0f};
    float x1 = 0.0f, x2 = 0.0f;
    int cont_entradas = 0;

    char texto_copia[256];
    int idx = 0;

    // Normalização em string local
    for (int i = 0; texto_usuario[i] != '\0' && i < 255; i++) {
        char c = texto_usuario[i];
        if (c >= 'A' && c <= 'Z') texto_copia[i] = c + 32;
        else texto_copia[i] = c;
        idx = i + 1;
    }
    texto_copia[idx] = '\0';

    // Extração de dados e tokens
    char* token_str = strtok(texto_copia, " ,.?!()");
    while (token_str != NULL) {
        if ((token_str[0] >= '0' && token_str[0] <= '9') || (token_str[0] == '-' && token_str[1] >= '0')) {
            float val = (float)atoi(token_str);
            if (cont_entradas == 0) { x1 = val; cont_entradas = 1; }
            else if (cont_entradas == 1) { x2 = val; cont_entradas = 2; }
        } else {
            int token_id = obter_token(token_str);
            if (token_id != -1) {
                for (int d = 0; d < TAM_VETOR_SEMANTICO; d++) {
                    vetor_acumulado[d] += MATRIZ_EMBEDDING[token_id][d];
                }
            }
        }
        token_str = strtok(NULL, " ,.?!()");
    }

    // Inferência da Intenção
    int id_operacao_escolhida = 0;
    float maior_ativacao = -9999.0f;

    for (int p = 0; p < 4; p++) {
        float ativacao = 0.0f;
        for (int d = 0; d < TAM_VETOR_SEMANTICO; d++) {
            ativacao += vetor_acumulado[d] * PESOS_CLASSIFICADOR[p][d];
        }
        if (ativacao > maior_ativacao) {
            maior_ativacao = ativacao;
            id_operacao_escolhida = p;
        }
    }

    // Execução da operação no Core Matemático interno
    float resultado_math = iamath_processar(&BancoDeDadosMatematico[id_operacao_escolhida], x1, x2);

    // Formatação temporária do resultado bruto (que no futuro irá para a IA Tradutora)
    char str_num[32];
    int_to_string((int)resultado_math, str_num);

    strcpy(buffer_resposta, "RAW_MATH_RESULT: ");
    strcat(buffer_resposta, str_num);
    strcat(buffer_resposta, "\n");
}
