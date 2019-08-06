#ifndef TERMINAL_GRAPHICS_H
#define TERMINAL_GRAPHICS_H

#include <termbox.h>

void tg_draw_text(char *txt, int x, int y, uint32_t fg, uint32_t bg)
{
    for (int i = 0; txt[i] != '\0'; i++)
        tb_change_cell(x + i, y, txt[i], fg, bg);
}

void tg_draw_rect(int x, int y, int width, int height,
                  char ch, uint32_t fg, uint32_t bg)
{
    for (int j = 0; j < height; j++)
        for (int i = 0; i < width; i++)
            tb_change_cell(x + i, y + j, ch, fg, bg);
}

void tg_draw_line(int x0, int y0, int x1, int y1, char ch,
                  uint32_t fg, uint32_t bg)
{
    int dx = x1 - x0;
    int dy = y1 - y0;
    int x = x0;
    int y = y0;
    int temp = 2 * dy - dx;

    while(x < x1) {
        if(temp < 0) {
            temp = temp + 2 * dy;
        } else {
            y = y + 1;
            temp = temp + 2 * dy - 2 * dx;
        }
        
        tb_change_cell(x, y, ch, fg, bg);
        
        x++;
    }
}

#endif
