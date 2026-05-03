/**
 * @brief FujiNet weather for CoCo
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE.md, for details.
 * @verbose Graphics primitives for PMODE3 (128x192x2bpp)
 */

#ifndef GFX_H
#define GFX_H

#ifdef COCO3
/* CoCo 3 16-color palette indices used by drawing code.
 * The palette itself is loaded in gfx() init.
 * BGCOLOR is the screen-clear color: BLACK on CoCo 3. */
#define BLACK    0x00
#define WHITE    0x03
#define BGCOLOR  BLACK
#define PURPLE   0x09
#define ORANGE   0x0D
#else
/* CoCo 1/2 PMODE 3 has only 4 colors per palette set.  BGCOLOR is the
 * screen-clear color: CYAN on CoCo 1/2. */
#define WHITE    0x00
#define CYAN     0x01
#define BGCOLOR  CYAN
#define PURPLE   0x02
#define ORANGE   0x03
#endif

#define KEY_ENTER 0x0D
#define KEY_LEFT_ARROW 0x08
#define KEY_BREAK 0x03

#include "weatherdefs.h"
#include "fujinet-fuji.h"

#ifdef COCO3
/* CoCo 3 char-grid drawing primitives.
 * 40 columns x 25 rows on 320x200x16 screen at $8000 (MMU task 1).
 *
 * inv: when true, the glyph is drawn in reverse video -- the cell becomes a
 *      solid fg block with the glyph showing as a black cutout.
 */
void hires_putc(unsigned char cx, unsigned char cy, char ch, bool inv);

/* MSDOS-style char-coord helpers, mirrored so weather/forecast layout
 * code can match the MSDOS port closely. drawText honors \001 inverse
 * toggles; drawTextDouble draws each char as 1 col x 2 rows (vertical
 * stretch). */
void drawText(unsigned char cx, unsigned char cy, const char *s);
void drawTextDouble(unsigned char cx, unsigned char cy, const char *s);
void draw_border(void);
void draw_hdiv(unsigned char y);
void draw_forecast_border(void);

/* 2bpp pixel value -> 4bpp palette index, used by hires_putc to recolor
 * text at render time.  Default {0, 3, 3, 3} = white text on black. */
extern unsigned char text_palette[4];
#endif

/**
 * @brief set pixel at x,y to color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 */
void pset(int x, int y,unsigned char c);

/**
 * @brief Put character ch in font at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param ch Character to put (0-127)
 * @param dbl Make the character double height
 */
void putc(int x, int y, char c, char ch, bool dbl);

/**
 * @brief Put string s using putc at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param s NULL terminated string to place on graphics screen
 * @param dbl Make the characters double height
 */
void puts(int x, int y, char c, const char *s, bool dbl);

/**
 * @brief place icon on 8x8 boundary
 */
#ifdef COCO3
void put_icon(unsigned char cx, unsigned char cy, unsigned char idx);
#else
void put_icon(int x, int y, byte *icon);
#endif

/**
 * @brief set screen buffer address to BASIC location
 */
void set_screenbuffer(void);

/**
 * @brief set up graphics for PMODE 3
 * @param c color set to use
 */
void gfx(unsigned char c);

/**
 * @brief Tear down graphics state.  Restores GIME / palette so BASIC's
 *        restart after coldStart() comes up in normal text mode.
 */
void gfx_shutdown(void);

/**
 * @brief Clear display
 * @param c Color to clear display with
 */
void gfx_cls(unsigned char c);

/**
 * @brief Clear screen and display message
 * @param msg Message to display
 */
void disp_message(char *msg);

/**
 * @brief Print error message and halt
 * @param message Message to display        
 */ 
void handle_err(char *message);

/**
 * @brief Display progress dots
 * @param p Number of dots to display (0-5)     
 */
void progress_dots(char p);

/**
 * @brief Display menu string at bottom of screen
 * @param str Menu string to display
 */
void disp_menu(char *str); 

/**
 * @brief Read new location from user
 * @param loc Pointer to LOCATION struct to fill in 
 */
void change_location(LOCATION *loc);

#endif /* GFX_H */
