#include "cursor_engine.h"
#include "../system/graphics.h" 

#define MOUSE_W 16
#define MOUSE_H 16

static uint32_t mouse_backbuffer[MOUSE_W * MOUSE_H];

static const uint8_t mouse_cursor_map[12] = {
    0b10000000, 0b11000000, 0b11100000, 0b11110000,
    0b11111000, 0b11111100, 0b11111110, 0b11110000,
    0b11011000, 0b10001100, 0b00001100, 0b00000000
};

void cursor_restore_bg(int mx, int my, int screen_w, int screen_h) {
    for (int y = 0; y < MOUSE_H; y++) {
        for (int x = 0; x < MOUSE_W; x++) {
            int sx = mx + x;
            int sy = my + y;
            if (sx < screen_w && sy < screen_h && sx >= 0 && sy >= 0) {
                // USA sys_draw_put_pixel conforme seu graphics.c
                sys_draw_put_pixel(sx, sy, mouse_backbuffer[y * MOUSE_W + x]);
            }
        }
    }
}

void cursor_save_bg(int mx, int my, int screen_w, int screen_h) {
    for (int y = 0; y < MOUSE_H; y++) {
        for (int x = 0; x < MOUSE_W; x++) {
            int sx = mx + x;
            int sy = my + y;
            if (sx < screen_w && sy < screen_h && sx >= 0 && sy >= 0) {
                // USA sys_draw_get_pixel conforme seu graphics.c
                mouse_backbuffer[y * MOUSE_W + x] = sys_draw_get_pixel(sx, sy);
            }
        }
    }
}

void cursor_draw(int mx, int my, int screen_w, int screen_h) {
    for (int row = 0; row < 12; row++) {
        for (int col = 0; col < 8; col++) {
            if ((mouse_cursor_map[row] >> (7 - col)) & 1) {
                int sx = mx + col;
                int sy = my + row;
                if(sx >= 0 && sx < screen_w && sy >= 0 && sy < screen_h) {
                    // USA sys_draw_put_pixel para desenhar o cursor
                    sys_draw_put_pixel(sx, sy, 0x00FFFFFF); 
                }
            }
        }
    }
}
