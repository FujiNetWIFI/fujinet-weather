#ifndef DRAW_H
#define DRAW_H

#define MONO_WHITE       0
#define MONO_GRAY        1
#define MONO_WHITE_BLINK 2
#define MONO_GRAY_BLINK  3

#define COLOR_GREEN  0
#define COLOR_RED    1
#define COLOR_BLUE   2
#define COLOR_VIOLET 3


void set_position(unsigned short xpos, unsigned short ypos);
void get_position(unsigned short *x, unsigned short *y);

void set_color(unsigned char color);
void toggle_invert(void);

void draw_text(const char *s);
void draw_text_double(const char *s);

// void draw_char(unsigned char c);
// void draw_char_double(unsigned char c);
// void draw_bitmap(const unsigned char *bp);
// void clear_part(unsigned short width, unsigned short height);
// TODO
void erase_char(void);

extern void __FASTCALL__ draw_char(unsigned char c);
extern void __FASTCALL__ draw_char_double(unsigned char c);
extern void __FASTCALL__ draw_bitmap(const unsigned char *bp);
// wh: high byte = width in bytes (6 px each), low byte = height in lines
extern void __FASTCALL__ clear_part_asm(unsigned short wh);

// clear area at current position, pixels and color
// width 1..255 bytes, height 1..255 lines or 0 = 256 lines
#define clear_part(width, height) clear_part_asm(((width) << 8) | (height))

// clear whole screen (48 visible bytes of all 256 lines), position is set to 0, 0
void clear_screen(void);


#endif