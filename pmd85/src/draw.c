#include "draw.h"


extern unsigned char font_data[];

#define VIDEO 0xC000

// not static, used by draw_pmd85.asm too
unsigned short xpos = 0;
unsigned short ypos = 0;
unsigned char *video_ptr = VIDEO;
unsigned char video_bit = 1;
unsigned char color_attr = 0x00;
unsigned char invert_mask = 0x00;


void set_position(unsigned short x, unsigned short y)
{
    static unsigned char mask_bits[6] = {1, 2, 4, 8, 16, 32};
    xpos = x;
    ypos = y;
    video_ptr = (unsigned char *)(VIDEO + 64*ypos + xpos/6);
    video_bit = mask_bits[xpos % 6];
}

void get_position(unsigned short *x, unsigned short *y)
{
    *x = xpos;
    *y = ypos;
}

void set_color(unsigned char color)
{
    color_attr = color << 6;
}

void toggle_invert(void)
{
    invert_mask ^= 0xFF;
}

void draw_text(const char *s)
{
    static char c;
    while(c=*s++) draw_char((unsigned char)c);
}


void draw_text_double(const char *s)
{
    static char c;
    while(c=*s++) draw_char_double((unsigned char)c);
}


void clear_screen(void)
{
    set_position(0, 0);
    clear_part(48, 0);  // height 0 = 256 lines
}


#if 0
// Original C versions of drawing functions

// draw 8x8 px character
void draw_char(unsigned char c)
{
    static unsigned char *vp;
    static unsigned char *cp;
    static char cd;
    static char w;
    static char mask;

    mask = color_attr | video_bit;

    // pointer to character data
    cp = get_char_data(c);

    for(w = 8; w--;) {
        // draw column of pixels
        vp = video_ptr;
        cd = (char)*cp++;
        cd ^= invert_mask;
        if (cd) {
            if (cd < 0) *vp |= mask;
            vp += 64; cd <<= 1;
            if (cd < 0) *vp |= mask;
            vp += 64; cd <<= 1;
            if (cd < 0) *vp |= mask;
            vp += 64; cd <<= 1;
            if (cd < 0) *vp |= mask;
            vp += 64; cd <<= 1;
            if (cd < 0) *vp |= mask;
            vp += 64; cd <<= 1;
            if (cd < 0) *vp |= mask;
            vp += 64; cd <<= 1;
            if (cd < 0) *vp |= mask;
            vp += 64; cd <<= 1;
            if (cd < 0) *vp |= mask;
        }
        // next column
        mask = color_attr | (video_bit <<= 1);
        xpos++;
        if (video_bit == 64)
        {
            // next byte
            mask = color_attr | (video_bit = 1);
            video_ptr++;
            if (xpos == 288) {
                xpos = 0;
                ypos += 8;
                if (ypos >= 256) {
                    ypos = 0;
                    video_ptr = (unsigned char *)VIDEO;
                }
                else {
                    video_ptr += 16 + 7*64; // skip notepad area + next 7 lines
                }
            }
        }
    }
}

// draw character scaled to 16x16 px
void draw_char_double(unsigned char c)
{
    static unsigned char *vp;
    static unsigned char *cp;
    static char cd;
    static char w;
    static char mask;

    // ceil position
    xpos &= ~1;
    video_bit = (video_bit >= 16) ? 16 : (video_bit >= 4) ? 4 : 1;
    mask = color_attr | video_bit | (video_bit << 1);

    cp = get_char_data(c);

    for(w = 8; w--;) {
        // draw two columns of pixels
        vp = video_ptr;
        cd = (char)*cp++;
        cd ^= invert_mask;
        if (cd) {
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;} else {vp += 64;}
            vp += 64; cd <<= 1;
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;} else {vp += 64;}
            vp += 64; cd <<= 1;
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;} else {vp += 64;}
            vp += 64; cd <<= 1;
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;} else {vp += 64;}
            vp += 64; cd <<= 1;
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;} else {vp += 64;}
            vp += 64; cd <<= 1;
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;} else {vp += 64;}
            vp += 64; cd <<= 1;
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;} else {vp += 64;}
            vp += 64; cd <<= 1;
            if (cd < 0) {*vp |= mask; vp += 64; *vp |= mask;}
        }
        // next two columns
        mask <<= 2;
        mask |= color_attr;
        video_bit <<= 2;
        xpos+=2;
        if (video_bit == 64)
        {
            // next byte
            mask = color_attr | 3;
            video_bit = 1;
            video_ptr++;
            if (xpos == 288) {
                xpos = 0;
                ypos += 16;
                if (ypos >= 256) {
                    ypos = 0;
                    video_ptr = (unsigned char *)VIDEO;
                }
                else {
                    video_ptr += 16 + 15*64; // skip notepad area + next 15 lines
                }
            }
        }
    }

}

unsigned char* get_char_data(unsigned char c)
{
    return font_data + 8 * c;
}

#define RLE_ESC 0xE1

// draw bitmap: width in bytes, height in pixels, RLE data column by column
// RLE_ESC, count, value = run of count identical bytes, other bytes as they are
void draw_bitmap(const unsigned char *bp)
{
    static unsigned char *vp;
    static unsigned char *cp;
    static unsigned char h;
    static unsigned char ww;
    static unsigned char hh;
    static unsigned char n;
    static unsigned char v;

    ww = *bp++;
    hh = h = *bp++;

    vp = cp = video_ptr;
    while(ww) {
        v = *bp++;
        n = 1;
        if (v == RLE_ESC) {
            n = *bp++;
            v = *bp++;
        }
        while(n--) {
            *vp = v;
            vp += 64;   // next line
            if (!--hh) {
                // next column, runs may continue there
                hh = h;
                vp = ++cp;
                xpos += 6;
                if (!--ww) break;
            }
        }
    }
    // continue right after bitmap, top aligned
    video_ptr = cp;
}

// clear area at current position (pixels and color), width in bytes, height in lines
void clear_part(unsigned short width, unsigned short height)
{
    unsigned short w;
    unsigned char *vp = video_ptr;
    unsigned char *cp;
    while (height)
    {
        cp = vp;
        for(w = width; w--;)
        {
            *cp = 0; // clear pixels and color
            cp++;
        }
        vp += 64;
        height--;
    }
}

// clear 8x8 px area at current position, advance position to next character
void erase_char(void)
{
    static unsigned char *vp;
    static char w;
    static unsigned char mask;

    mask = ~(video_bit | 0xC0); // clear pixels and color

    for(w = 8; w--;) {
        // clear column of pixels
        vp = video_ptr;
        *vp &= mask; vp += 64;
        *vp &= mask; vp += 64;
        *vp &= mask; vp += 64;
        *vp &= mask; vp += 64;
        *vp &= mask; vp += 64;
        *vp &= mask; vp += 64;
        *vp &= mask; vp += 64;
        *vp &= mask; vp += 64;

        // next column
        mask = ~((video_bit <<= 1) | 0xC0);
        xpos++;
        if (video_bit == 64)
        {
            // next byte
            video_bit = 1;
            mask = 0x3E; // ~0xC1
            video_ptr++;
            if (xpos == 288) {
                xpos = 0;
                ypos += 8;
                if (ypos >= 256) {
                    ypos = 0;
                    video_ptr = (unsigned char *)VIDEO;
                }
                else {
                    video_ptr += 16 + 7*64; // skip notepad area + next 7 lines
                }
            }
        }
    }
}
#endif
