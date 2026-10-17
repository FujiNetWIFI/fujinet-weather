/**
 * @brief FujiNet weather for CoCo
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE.md, for details.
 * @verbose Graphics primitives for PMODE3 (128x192x2bpp)
 */

#ifndef GFX_H
#define GFX_H

/* CoCo 1/2 PMODE 3 has only 4 colors per palette set.  BGCOLOR is the
 * screen-clear color: CYAN on CoCo 1/2. */
#define WHITE    0x00
#define CYAN     0x01
#define BGCOLOR  CYAN
#define PURPLE   0x02
#define ORANGE   0x03

#define KEY_ENTER 0x0D
#define KEY_LEFT_ARROW 0x08
#define KEY_BREAK 0x03

#include "weatherdefs.h"
#include "fujinet-fuji.h"

/**
 * @brief set pixel at x,y to color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 */
void gfx_pset(int x, int y,unsigned char c);

/**
 * @brief Put character ch in font at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param ch Character to put (0-127)
 * @param dbl Make the character double height
 */
void gfx_putc(int x, int y, char c, char ch, bool dbl);

/**
 * @brief Put string s using putc at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param s NULL terminated string to place on graphics screen
 * @param dbl Make the characters double height
 */
void gfx_puts(int x, int y, char c, const char *s, bool dbl);

/**
 * @brief place icon on 8x8 boundary
 */
void gfx_put_icon(int x, int y, unsigned char *icon);

/**
 * @brief set screen buffer address to BASIC location
 */
void gfx_set_screenbuffer(void);

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

// /**
//  * @brief Clear display
//  */
// void gfx_cls(void);

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

// /**
//  * @brief Display menu string at bottom of screen
//  * @param str Menu string to display
//  */
// void disp_menu(char *str);
void disp_weather_menu(void);
void disp_forecast_menu(char page);

void disp_status(const char *msg);

/**
 * @brief Read new location from user
 * @param loc Pointer to LOCATION struct to fill in 
 */
bool change_location(LOCATION *loc);

char get_keyin();

bool input_text(char *buf, const char *init, unsigned char bufsize);

#endif /* GFX_H */
