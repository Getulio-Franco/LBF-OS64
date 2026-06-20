#include "iasynth.h"
#include "../system/string.h"

// Pesos da Rede Neural de Contexto (Analisa a vibe da pergunta)
// Dim 0: Quantidade de palavras casuais (por, favor, ola)
// Dim 1: Tamanho da frase
// Dim 2: Foco no resultado (qual, resultado)
static const float PESOS_CONTEXTO[2][3] = {
    { 1.0f,  0.5f,  0.8f}, // Neurônio 0: Vibe Conversacional / Longa
    {-1.0f, -0.8f, -0.5f}  // Neurônio 1: Vibe Direta / Robótica / Curta
};
static const float BIAS_CONTEXTO[2] = {-1.0f, 0.5f};

static void extrair_valor_bruto(const char* raw_string, char* valor_extraido) {
    valor_extraido[0] = '\0';
    if (!raw_string || strlen(raw_string) == 0) return;
    
    const char* ptr = raw_string;
    while (*ptr != '\0' && *ptr != ':') ptr++;
    if (*ptr == ':') {
        ptr++; while (*ptr == ' ') ptr++;
        int i = 0;
        while (*ptr != '\0' && *ptr != '\n' && i < 31) valor_extraido[i++] = *ptr++;
        valor_extraido[i] = '\0';
    }
}

void iasynth_gerar_resposta(const char* prompt_original, const char* raw_result, char* buffer_saida) {
    char valor_bruto[32] = {0};
    extrair_valor_bruto(raw_result, valor_bruto);

    // Se não há resultado, encerra
    if (strlen(valor_bruto) == 0) {
        strcpy(buffer_saida, "Desculpe, nao consegui extrair um processamento valido.");
        return;
    }

    // === 1. Extração de Features Semânticas do Prompt original ===
    float feat_casual = 0.0f;
    float feat_tamanho = 0.0f;
    float feat_resultado = 0.0f;
    
    char copia[256];
    strcpy(copia, prompt_original);
    char* token = strtok(copia, " ,.?!()");
    
    while (token != NULL) {
        feat_tamanho += 0.2f; // Cada palavra aumenta o tamanho
        if (strcmp(token, "favor") == 0 || strcmp(token, "ola") == 0) feat_casual += 1.0f;
        if (strcmp(token, "qual") == 0 || strcmp(token, "resultado") == 0) feat_resultado += 1.0f;
        token = strtok(NULL, " ,.?!()");
    }

    // === 2. Executa a Rede de Discernimento de Tom ===
    float out_conversacional = (feat_casual * PESOS_CONTEXTO[0][0]) + (feat_tamanho * PESOS_CONTEXTO[0][1]) + (feat_resultado * PESOS_CONTEXTO[0][2]) + BIAS_CONTEXTO[0];
    float out_direto = (feat_casual * PESOS_CONTEXTO[1][0]) + (feat_tamanho * PESOS_CONTEXTO[1][1]) + (feat_resultado * PESOS_CONTEXTO[1][2]) + BIAS_CONTEXTO[1];

    // === 3. Costura da Linguagem Natural (Geração) ===
    buffer_saida[0] = '\0';
    
    // Constrói a frase dinamicamente baseado no neurônio vencedor
    if (out_conversacional > out_direto) {
        // Modo humano/explicativo
        strcpy(buffer_saida, "Baseado na sua pergunta, analisei o contexto e o resultado e: ");
        strcat(buffer_saida, valor_bruto);
        strcat(buffer_saida, ".");
    } else {
        // Modo direto/máquina (O usuário foi curto, a IA é curta)
        strcpy(buffer_saida, "Resultado computado: ");
        strcat(buffer_saida, valor_bruto);
    }
}
