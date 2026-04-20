#include <i86.h>
#include <dos.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "weatherdefs.h"
#include "openmeteo.h"
#include "strutil.h"
#include "gfx.h"

extern unsigned char charset[256][16];
extern unsigned char ascii[256][16];
extern int err;
extern LOCATION current;

#define VIDEO_RAM_ADDR ((unsigned char far *)0xB8000000UL)
unsigned char far *video = VIDEO_RAM_ADDR;

#define VIDEO_LINE_BYTES 80
#define VIDEO_ODD_OFFSET 0x2000

unsigned char prevVideoMode;

/* --- Video setup / teardown --- */

void initGraphics(void)
{
    union REGS r;

    r.h.ah = 0x0f;
    int86(0x10, &r, &r);
    prevVideoMode = r.h.al;

    r.h.ah = 0x00;
    r.h.al = 0x04;
    int86(0x10, &r, &r);

    r.h.ah = 0x0b;
    r.h.bh = 0x00;
    r.h.bl = 0x01;
    int86(0x10, &r, &r);

    r.h.ah = 0x10;
    r.h.al = 0x00;
    r.h.bl = 0;
    r.h.bh = 0x09;
    int86(0x10, &r, &r);

    r.h.ah = 0x10;
    r.h.al = 0x00;
    r.h.bl = 1;
    r.h.bh = 0x08;
    int86(0x10, &r, &r);

    r.h.ah = 0x10;
    r.h.al = 0x00;
    r.h.bl = 2;
    r.h.bh = 0x0E;
    int86(0x10, &r, &r);

    r.h.ah = 0x10;
    r.h.al = 0x00;
    r.h.bl = 3;
    r.h.bh = 0x0f;
    int86(0x10, &r, &r);

    r.w.ax = 0x1010;
    r.w.bx = 0x0E;
    r.h.dh = 63;
    r.h.ch = 63;
    r.h.cl = 0;
    int86(0x10, &r, &r);

    r.w.ax = 0x1010;
    r.w.bx = 0x0F;
    r.h.dh = 63;
    r.h.ch = 63;
    r.h.cl = 63;
    int86(0x10, &r, &r);

    r.w.ax = 0x1010;
    r.w.bx = 0x08;
    r.h.dh = 32;
    r.h.ch = 32;
    r.h.cl = 32;
    int86(0x10, &r, &r);
}

void resetGraphics(void)
{
    union REGS r;

    r.h.ah = 0x00;
    r.h.al = prevVideoMode;
    int86(0x10, &r, &r);
}

void waitvsync(void)
{
    while (!(inp(0x3DA) & 0x08));
    while (inp(0x3DA) & 0x08);
}

void resetScreen(void)
{
    waitvsync();
    _fmemset(&video[0x0000], 0, 8000);
    _fmemset(&video[0x2000], 0, 8000);
    waitvsync();
}

void setBgColor(unsigned char color)
{
    union REGS r;
    r.h.ah = 0x10;
    r.h.al = 0x00;
    r.h.bl = 0;
    r.h.bh = color;
    int86(0x10, &r, &r);
}

/* --- Low-level tile / character rendering --- */

void plot_tile(const unsigned char *tile, unsigned char x, unsigned char y)
{
    unsigned char i = 0;

    y <<= 3;
    x <<= 1;

    for (i = 0; i < 8; i++)
    {
        unsigned char r = y + i;
        unsigned char rh = r >> 1;
        unsigned short ro = rh * VIDEO_LINE_BYTES + x;

        if (r & 1)
            ro += VIDEO_ODD_OFFSET;

        video[ro] = tile[i*2];
        video[ro+1] = tile[i*2+1];
    }
}

void plot_char(unsigned char x, unsigned char y, unsigned char color, char c, bool inv)
{
    unsigned char tile[16];
    unsigned char mask;
    unsigned char i;

    switch(color)
    {
    case 0: mask = 0x00; break;
    case 1: mask = 0x55; break;
    case 2: mask = 0xAA; break;
    default: mask = 0xFF; break;
    }

    for (i = 0; i < 16; i++)
        tile[i] = inv ? ~ascii[(unsigned char)c][i]
                      : ascii[(unsigned char)c][i] & mask;

    plot_tile(tile, x, y);
}

void drawBlank(unsigned char x, unsigned char y)
{
    plot_tile(&charset[0x00][0], x, y);
}

/* --- Text drawing --- */

static int visible_len(const char *s)
{
    int len = 0;
    while (*s)
    {
        if (*s != 0x01)
            len++;
        s++;
    }
    return len;
}

void drawText(unsigned char x, unsigned char y, const char *s)
{
    char c;
    unsigned char inv = 0;

    while ((c = *s++) != 0)
    {
        if (c == 0x01)
        {
            inv ^= 1;
            continue;
        }
        if (x > 39)
        {
            x = 0;
            y++;
        }
        plot_char(x++, y, 3, c, inv);
    }
}

void drawTextDouble(unsigned char x, unsigned char y, const char *s)
{
    unsigned char top[16], bot[16];
    unsigned char i;

    while (*s)
    {
        unsigned char ch = (unsigned char)*s++;
        for (i = 0; i < 4; i++)
        {
            top[i*4]   = ascii[ch][i*2];
            top[i*4+1] = ascii[ch][i*2+1];
            top[i*4+2] = ascii[ch][i*2];
            top[i*4+3] = ascii[ch][i*2+1];
        }
        for (i = 0; i < 4; i++)
        {
            bot[i*4]   = ascii[ch][(i+4)*2];
            bot[i*4+1] = ascii[ch][(i+4)*2+1];
            bot[i*4+2] = ascii[ch][(i+4)*2];
            bot[i*4+3] = ascii[ch][(i+4)*2+1];
        }
        plot_tile(top, x, y);
        plot_tile(bot, x, y + 1);
        x++;
    }
}

/* --- Icon drawing --- */

void drawIcon(unsigned char x, unsigned char y, unsigned char icon)
{
    unsigned char base = icon * 28;
    unsigned char tx, ty;
    for (ty = 0; ty < 4; ty++)
        for (tx = 0; tx < 7; tx++)
            plot_tile(&charset[base + ty * 7 + tx][0], x + tx, y + ty);
}

void disp_icon_test(void)
{
    unsigned char i, col, row, x, y;
    static char *names[] = {"clear", "few cl", "sct cl", "brk cl", "shower",
                            "rain", "tstorm", "snow", "fog"};

    setBgColor(0x09);
    resetScreen();

    for (i = 0; i < 9; i++)
    {
        col = i % 5;
        row = i / 5;
        x = col * 8;
        y = row * 7 + 1;
        drawIcon(x, y, i);
        drawText(x, y + 4, names[i]);
    }
    drawText(1, 22, "Icon test - press any key");
    getch();
    resetScreen();
}

/* --- Border / divider drawing --- */

void draw_border(void)
{
    unsigned char i;
    plot_char(0, 0, 3, '\xC9', false);
    for (i = 1; i < 39; i++)
        plot_char(i, 0, 3, '\xCD', false);
    plot_char(39, 0, 3, '\xBB', false);
    for (i = 1; i < 24; i++)
    {
        plot_char(0, i, 3, '\xBA', false);
        plot_char(39, i, 3, '\xBA', false);
    }
    plot_char(0, 24, 3, '\xC8', false);
    for (i = 1; i < 39; i++)
        plot_char(i, 24, 3, '\xCD', false);
    plot_char(39, 24, 3, '\xBC', false);
}

void draw_hdiv(unsigned char y)
{
    unsigned char i;
    plot_char(0, y, 3, '\xC7', false);
    for (i = 1; i < 39; i++)
        plot_char(i, y, 3, '\xC4', false);
    plot_char(39, y, 3, '\xB6', false);
}

void draw_forecast_border(void)
{
    unsigned char i, x;
    plot_char(0, 0, 3, '\xC9', false);
    for (i = 1; i < 39; i++)
        plot_char(i, 0, 3, '\xCD', false);
    plot_char(39, 0, 3, '\xBB', false);
    for (x = 10; x <= 30; x += 10)
        plot_char(x, 0, 3, '\xD1', false);
    for (i = 1; i <= 21; i++)
    {
        plot_char(0, i, 3, '\xBA', false);
        plot_char(39, i, 3, '\xBA', false);
        plot_char(10, i, 3, '\xB3', false);
        plot_char(20, i, 3, '\xB3', false);
        plot_char(30, i, 3, '\xB3', false);
    }
    plot_char(0, 22, 3, '\xC7', false);
    for (i = 1; i < 39; i++)
        plot_char(i, 22, 3, '\xC4', false);
    plot_char(39, 22, 3, '\xB6', false);
    for (x = 10; x <= 30; x += 10)
        plot_char(x, 22, 3, '\xC1', false);
    plot_char(0, 23, 3, '\xBA', false);
    plot_char(39, 23, 3, '\xBA', false);
    plot_char(0, 24, 3, '\xC8', false);
    for (i = 1; i < 39; i++)
        plot_char(i, 24, 3, '\xCD', false);
    plot_char(39, 24, 3, '\xBC', false);
}

/* --- UI screens --- */

#define PROGRESS_TOTAL 49
#define PROGRESS_WIDTH 30
#define PROGRESS_X 5
#define PROGRESS_Y 12

static char progress_text[PROGRESS_WIDTH + 1];
static unsigned char progress_count;

void disp_message(const char *msg)
{
    unsigned char i, len, pad;

    setBgColor(0x09);
    resetScreen();

    plot_char(PROGRESS_X - 1, PROGRESS_Y - 1, 3, '\xC9', false);
    for (i = 0; i < PROGRESS_WIDTH; i++)
        plot_char(PROGRESS_X + i, PROGRESS_Y - 1, 3, '\xCD', false);
    plot_char(PROGRESS_X + PROGRESS_WIDTH, PROGRESS_Y - 1, 3, '\xBB', false);

    plot_char(PROGRESS_X - 1, PROGRESS_Y, 3, '\xBA', false);
    plot_char(PROGRESS_X + PROGRESS_WIDTH, PROGRESS_Y, 3, '\xBA', false);

    plot_char(PROGRESS_X - 1, PROGRESS_Y + 1, 3, '\xC8', false);
    for (i = 0; i < PROGRESS_WIDTH; i++)
        plot_char(PROGRESS_X + i, PROGRESS_Y + 1, 3, '\xCD', false);
    plot_char(PROGRESS_X + PROGRESS_WIDTH, PROGRESS_Y + 1, 3, '\xBC', false);

    len = strlen(msg);
    if (len > PROGRESS_WIDTH) len = PROGRESS_WIDTH;
    memset(progress_text, ' ', PROGRESS_WIDTH);
    progress_text[PROGRESS_WIDTH] = '\0';
    pad = (PROGRESS_WIDTH - len) / 2;
    memcpy(progress_text + pad, msg, len);

    for (i = 0; i < PROGRESS_WIDTH; i++)
        plot_char(PROGRESS_X + i, PROGRESS_Y, 3, progress_text[i], false);

    progress_count = 0;
}

void progress_step(void)
{
    unsigned char filled, i;

    progress_count++;
    filled = (unsigned char)(((unsigned int)progress_count * PROGRESS_WIDTH) / PROGRESS_TOTAL);

    for (i = 0; i < PROGRESS_WIDTH; i++)
        plot_char(PROGRESS_X + i, PROGRESS_Y, 3, progress_text[i], i < filled);
}

void handle_err(const char *message)
{
    if (err != 0)
    {
        resetGraphics();
        printf("ERROR: %s\n", message);
        printf(" CODE: %02X\n", err);
        printf("[PLEASE PRESS ANY KEY (EXIT)]\n");
        getch();
        exit(1);
    }
}

void disp_menu(const char *str)
{
    int vlen = visible_len(str);
    int x = 1 + (38 - vlen) / 2;
    if (x < 1) x = 1;
    drawText(x, 23, str);
}

bool get_line(char *buf, unsigned char max_len, unsigned char x, unsigned char y)
{
    unsigned char c;
    unsigned char i = 0;

    memset(buf, 0, max_len + 1);

    do
    {
        plot_char(x + i, y, 3, '_', false);
        c = getch();

        if (isprint(c))
        {
            plot_char(x + i, y, 3, c, false);
            buf[i] = c;
            if (i < max_len - 1)
            {
                i++;
            }
        }
        else if (c == KEY_BACKSPACE)
        {
            if (i)
            {
                drawBlank(x + i, y);
                --i;
                drawBlank(x + i, y);
                buf[i] = '\0';
            }
        }
        else if (c == KEY_ESCAPE)
        {
            return false;
        }
    } while (c != KEY_ENTER);

    buf[i] = '\0';
    return true;
}

void change_location(LOCATION *loc)
{
    char input_buf[LINE_LEN];
    char linebuf[LINE_LEN];

    setBgColor(0x09);
    resetScreen();
    drawText(1, 10, "Change location");
    drawText(1, 11, "Input city name,");
    drawText(1, 12, "ENTER to detect location, or");
    drawText(1, 13, "ESC to cancel.");
    plot_char(1, 15, 3, '>', false);

    if (get_line(input_buf, LINE_LEN - 1, 3, 15) == false)
    {
        return;
    }

    if (strlen(input_buf) == 0)
    {
        resetScreen();
        disp_message("Fetching location data");
        get_location(loc);
    }
    else
    {
        drawText(1, 17, "Validating city...");
        strcpy(linebuf, replaceSpaces(input_buf));
        if (!om_geocoding(loc, linebuf))
        {
            resetScreen();
            sprintf(linebuf, "City '%s'", input_buf);
            drawText(1, 10, linebuf);
            drawText(1, 11, "not found.");
            drawText(1, 12, "Using current location.");
            drawText(1, 14, "Press any key to continue.");
            getch();
            *loc = current;
        }
    }

    resetScreen();
}
