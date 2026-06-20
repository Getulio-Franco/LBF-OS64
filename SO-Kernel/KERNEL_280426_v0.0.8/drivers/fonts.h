#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

// ================================
// 🔤 Configuração da fonte padrão
// ================================

#define FONT_WIDTH   8
#define FONT_HEIGHT  8

// ================================
// 📦 Fonte 8x8 (ASCII completo)
// ================================

// Cada caractere:
// 8 linhas (1 byte por linha)
// cada bit = 1 pixel
extern const uint8_t font_8x8[256][8];


// ================================
// 🧠 Helpers (opcional, mas útil)
// ================================

// Retorna largura da fonte
static inline int font_get_width(void) {
    return FONT_WIDTH;
}

// Retorna altura da fonte
static inline int font_get_height(void) {
    return FONT_HEIGHT;
}

#endif
