/*
A simple header file for create terminal based graphics.

Example program:
int main()
{
    if (tb_init()) { // Initialize termbox
        printf("Error initializing termbox.");
         return 1;
    }

    draw_text("Hello, world!", 3, 5, TB_GREEN, TB_DEFAULT);

    struct tb_event ev;
    tb_poll_event(&ev);

    tb_shutdown();
}

*/
#ifndef TERMINAL_GRAPHICS_H
#define TERMINAL_GRAPHICS_H

#include <termbox.h>
#include <math.h>

void draw_text(const char *txt, int x, int y, uint32_t fg, uint32_t bg)
{
    for (int i = 0; txt[i] != '\0'; i++) {
        if (txt[i] == '\n') { // Handle new lines
            draw_text(txt + i + 1, x, y + 1, fg, bg);
            break;
        }
        tb_change_cell(x + i, y, txt[i], fg, bg);
    }
}

void draw_rect(int x, int y, int width, int height,
                  char ch, uint32_t fg, uint32_t bg)
{
    for (int j = 0; j < height; j++)
        for (int i = 0; i < width; i++)
            tb_change_cell(x + i, y + j, ch, fg, bg);
}

void draw_line(int x0, int y0, int x1, int y1, char ch,
                  uint32_t fg, uint32_t bg)
{
    int temp;
    if (x1 < x0) {
        temp = x0;
        x0 = x1;
        x1 = temp;
    }

    if (y1 < y0) {
        temp = y0;
        y0 = y1;
        y1 = temp;
    }
        
    int dx = x1 - x0;
    int dy = y1 - y0;
    int x = x0;
    int y = y0;
    temp = 2 * dy - dx;

    if (dx == 0) {
        for (int i = 0; i < dy; i++)
            tb_change_cell(x, i + y0, ch, fg, bg);
    } else {
        while (x < x1) {
            if (temp < 0) {
                temp = temp + 2 * dy;
            } else {
                y = y + 1;
                temp = temp + 2 * dy - 2 * dx;
            }

            tb_change_cell(x, y, ch, fg, bg);

            x++;
        }
    }
}

#endif
