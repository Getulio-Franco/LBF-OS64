#include "iarouter.h"
#include "../system/string.h"

// Arquitetura base para os nossos "Neurônios" do Roteador
#define TAM_VETOR 3

// Dicionário Semântico de Treinamento (Reduzido para o exemplo)
// Dim 0: Peso Aritmético | Dim 1: Peso Booleano | Dim 2: Peso de Ação (Verbo/Operador)
typedef struct { const char* palavra; float v[TAM_VETOR]; } Embed;
static const Embed DICIONARIO_ROUTER[] = {
    {"soma", {0.9f, 0.0f, 0.8f}}, {"some", {0.9f, 0.0f, 0.8f}}, 
    {"divida", {0.9f, 0.0f, 0.8f}}, {"multiplique", {0.9f, 0.0f, 0.8f}},
    {"and", {0.0f, 0.9f, 0.8f}}, {"or", {0.0f, 0.9f, 0.8f}}, {"xor", {0.0f, 0.9f, 0.8f}},
    {"qual", {0.1f, 0.1f, 0.0f}}, {"favor", {0.0f, 0.0f, 0.0f}}, {"resultado", {0.2f, 0.2f, 0.1f}}
};
static const int TOTAL_DICIONARIO = 10;

// Pesos da Rede Neural 1: Classificador de Intenção (Descobre qual especialista chamar)
// Mapeia o vetor médio da frase para 2 neurônios de saída (0: Math, 1: Logic)
static const float PESOS_INTENCAO[2][TAM_VETOR] = {
    {1.5f, -0.8f, 0.2f}, // Ativa forte para matemática, inibe lógica
    {-0.8f, 1.5f, 0.2f}  // Ativa forte para lógica, inibe matemática
};
static const float BIAS_INTENCAO[2] = {-0.5f, -0.5f};

// Pesos da Rede Neural 2: Extrator de Entidade (É um operador que devo mandar pra IA?)
// Lê o vetor de UMA palavra. Se a ativação for > 0, é um operador importante.
static const float PESOS_OPERADOR[TAM_VETOR] = {0.5f, 0.5f, 1.2f};
static const float BIAS_OPERADOR = -0.7f;

// Função Auxiliar: Busca o vetor da palavra (retorna zeros se for desconhecida)
static void obter_vetor(const char* palavra, float* vetor_out) {
    vetor_out[0] = 0; vetor_out[1] = 0; vetor_out[2] = 0;
    for (int i = 0; i < TOTAL_DICIONARIO; i++) {
        if (strcmp(palavra, DICIONARIO_ROUTER[i].palavra) == 0) {
            vetor_out[0] = DICIONARIO_ROUTER[i].v[0];
            vetor_out[1] = DICIONARIO_ROUTER[i].v[1];
            vetor_out[2] = DICIONARIO_ROUTER[i].v[2];
            return;
        }
    }
}

RouterDecision iarouter_processar(const char* texto_usuario) {
    RouterDecision decisao = {0, 0, ""};
    float vetor_frase[TAM_VETOR] = {0.0f, 0.0f, 0.0f};
    int palavras_contadas = 0;
    
    char texto_copia[256];
    char operador_encontrado[32] = "";
    char valores_encontrados[32] = "";
    
    // Normalização básica
    for (int i = 0; texto_usuario[i] != '\0' && i < 255; i++) {
        char c = texto_usuario[i];
        texto_copia[i] = (c >= 'A' && c <= 'Z') ? c + 32 : c;
        
        // Entidade Universal: Números são sempre extraídos como valores de entrada
        if (c >= '0' && c <= '9') {
            int len = strlen(valores_encontrados);
            valores_encontrados[len] = c;
            valores_encontrados[len+1] = ' '; // Espaço separador
            valores_encontrados[len+2] = '\0';
        }
    }
    texto_copia[strlen(texto_usuario)] = '\0';

    // Varredura Neural Palavra por Palavra
    char* token = strtok(texto_copia, " ,.?!()");
    while (token != NULL) {
        float v_palavra[TAM_VETOR];
        obter_vetor(token, v_palavra);
        
        // Acumula para a Rede de Intenção
        for (int d = 0; d < TAM_VETOR; d++) vetor_frase[d] += v_palavra[d];
        palavras_contadas++;

        // Executa a Rede Extratora (Essa palavra é o comando real?)
        float ativacao_operador = (v_palavra[0] * PESOS_OPERADOR[0]) + 
                                  (v_palavra[1] * PESOS_OPERADOR[1]) + 
                                  (v_palavra[2] * PESOS_OPERADOR[2]) + BIAS_OPERADOR;
        
        if (ativacao_operador > 0.0f && strlen(operador_encontrado) == 0) {
            strcpy(operador_encontrado, token); // Discerniu que isso é a ação principal!
        }
        
        token = strtok(NULL, " ,.?!()");
    }

    // Executa a Rede de Intenção
    if (palavras_contadas > 0) {
        float out_math = (vetor_frase[0] * PESOS_INTENCAO[0][0]) + (vetor_frase[1] * PESOS_INTENCAO[0][1]) + (vetor_frase[2] * PESOS_INTENCAO[0][2]) + BIAS_INTENCAO[0];
        float out_logic = (vetor_frase[0] * PESOS_INTENCAO[1][0]) + (vetor_frase[1] * PESOS_INTENCAO[1][1]) + (vetor_frase[2] * PESOS_INTENCAO[1][2]) + BIAS_INTENCAO[1];
        
        if (out_math > 0.0f) decisao.acionar_math = 1;
        if (out_logic > 0.0f) decisao.acionar_logic = 1;
    }

    // Limpa e monta o comando independente da enrolação do usuário
    strcpy(decisao.comando_limpo, operador_encontrado);
    strcat(decisao.comando_limpo, " ");
    strcat(decisao.comando_limpo, valores_encontrados);

    return decisao;
}
