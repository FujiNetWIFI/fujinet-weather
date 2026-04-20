#ifndef GFX_H
#define GFX_H

#include <stdbool.h>
#include "weatherdefs.h"

#define WIDTH 40
#define HEIGHT 25

#define KEY_ENTER 0x0D
#define KEY_ESCAPE 0x1B
#define KEY_BACKSPACE 0x08

void initGraphics(void);
void resetGraphics(void);
void resetScreen(void);
void waitvsync(void);
void setBgColor(unsigned char color);

void plot_tile(const unsigned char *tile, unsigned char x, unsigned char y);
void plot_char(unsigned char x, unsigned char y, unsigned char color, char c, bool inv);
void drawBlank(unsigned char x, unsigned char y);

void drawText(unsigned char x, unsigned char y, const char *s);
void drawTextDouble(unsigned char x, unsigned char y, const char *s);

void drawIcon(unsigned char x, unsigned char y, unsigned char icon);
void disp_icon_test(void);

void draw_border(void);
void draw_hdiv(unsigned char y);
void draw_forecast_border(void);

void disp_message(const char *msg);
void progress_step(void);
void handle_err(const char *message);
void disp_menu(const char *str);
void change_location(LOCATION *loc);

#endif
