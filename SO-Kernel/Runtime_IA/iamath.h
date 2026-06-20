#ifndef IAMATH_H
#define IAMATH_H

typedef struct {
    float peso_x1;
    float peso_x2;
    float bias;
    int tipo_operacao; 
} NeuronioMatematico;

extern const NeuronioMatematico BancoDeDadosMatematico[];

// Processa o cálculo bruto
float iamath_processar(const NeuronioMatematico* modelo, float x1, float x2);

// Nova função unificada: Pega o texto e cospe o resultado matemático calculado
void iamath_interpretar(const char* texto_usuario, char* buffer_resposta);

#endif
