/**
 * @brief FujiNet weather for CoCo
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE.md, for details.
 * @verbose Graphics primitives for PMODE3 (128x192x2bpp)
 */

#include <cmoc.h>
#include <coco.h>
#include "weatherdefs.h"
#include "gfx.h"
#include "font.h"

#define	DOT_SET			0x20
#define	DOT_FLASH		0x60
#define	DOT_CLEAR		0xa0

#define BUFFER_OFFSET(x,y) ((y << 5) + (x >> 2))
#define PIXEL_OFFSET(x) (x & 0x03)

int	err;

extern LOCATION current;

/**
 * @brief Pointer to screen buffer for PMODE
 */
static byte *screenBuffer;

/**
 * @brief and/or tables for pixel manipulation
 */
byte andTable[4] =
  {0x3F, 0xCF, 0xF3, 0xFC};
byte orTable[4][4] =
  {
    {0x00,0x00,0x00,0x00},
    {0x40,0x10,0x04,0x01},
    {0x80,0x20,0x08,0x02},
    {0xC0,0x30,0x0C,0x03}
  };

/**
 * @brief set pixel at x,y to color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 */
void pset(int x, int y,unsigned char c)
{
  screenBuffer[BUFFER_OFFSET(x,y)] &= andTable[PIXEL_OFFSET(x)];
  screenBuffer[BUFFER_OFFSET(x,y)] |= orTable[c][PIXEL_OFFSET(x)];
}

/**
 * @brief Put character ch in font at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param ch Character to put (0-127)
 */
void putc(int x, int y, char c, char ch)
{
  for (int i = 0; i < 8; i++)
  {
    char b = font[ch][i];
    for (int j = 0; j < 8; j++)
    {
      if (b < 0)
        pset(x, y, c);

      b <<= 1;

      x++;
    }
    y++;
    x -= 8;
  }
}

/**
 * @brief Put character ch in font at x,y with color c double height
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param ch Character to put (0-127)
 */
void putc_dbl(int x, int y, char c, char ch)
{
  for (int i = 0; i < 8; i++)
  {
    char b = font[ch][i];
    for (int j = 0; j < 8; j++)
    {
      if (b < 0)
      {
        pset(x, y, c);
        pset(x, y + 1, c);
      }

      b <<= 1;

      x++;
    }
    y += 2;
    x -= 8;
  }
}

/**
 * @brief Put string s using putc at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param s NULL terminated string to place on graphics screen
 */
void puts(int x, int y, char c, const char *s)
{
  while (*s)
    {
      putc(x,y,c,*s++);
      x += 8;

      if (x > 128)
	{
	  x=0;
	  y += 8;
	}
    }
}

/**
 * @brief Put string s using putc at x,y with color c, double height.
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param s NULL terminated string to place on graphics screen
 */
void puts_dbl(int x, int y, char c, const char *s)
{
  while (*s)
    {
      putc_dbl(x,y,c,*s++);
      x += 8;

      if (x > 128)
	{
	  x=0;
	  y += 16;
	}
    }
}

/**
 * @brief place icon on 8x8 boundary
 * @param x horizontal position (0-127) // must be divisible by 8
 * @param y vertical position (0-191)
 */
void put_icon(int x, int y, byte *icon)
{
  int o = BUFFER_OFFSET(x, y);

  for (int i = 0; i < 24; i++)
  {
    screenBuffer[o] = *icon++;
    screenBuffer[o + 1] = *icon++;
    screenBuffer[o + 2] = *icon++;
    screenBuffer[o + 3] = *icon++;
    screenBuffer[o + 4] = *icon++;
    screenBuffer[o + 5] = *icon++;
    o += 32;
    icon -= 6;

    screenBuffer[o] = *icon++;
    screenBuffer[o + 1] = *icon++;
    screenBuffer[o + 2] = *icon++;
    screenBuffer[o + 3] = *icon++;
    screenBuffer[o + 4] = *icon++;
    screenBuffer[o + 5] = *icon++;
    o += 32;
  }
}

/**
 * @brief set screen buffer address to BASIC location
 */
void set_screenbuffer(void)
{
  screenBuffer = (byte *) (((word) * (byte *) 0x00BC) << 8);
}

/**
 * @brief set up graphics for PMODE 3
 * @param c color set to use
 */
void gfx(unsigned char c)
{
  initCoCoSupport();
  rgb();
  width(32);
  set_screenbuffer();

  pmode(3,screenBuffer);
  screen(1,c);
}

/**
 * @brief Clear display
 * @param c Color to clear display with
 */
void gfx_cls(unsigned char c)
{
  const byte b[4]={0x00,0x55,0xAA,0xFF};

  memset(screenBuffer,b[c],6144);
}


void disp_message(char *msg) 
{
	gfx_cls(CYAN);
	puts(0,80, WHITE, msg);
}

//
// handle_err
//
void handle_err(char *message)
{
  if (err != 0)
  {
    screen(1,1);
    locate(0,0);
    printf("ERROR: %s:CODE=%d\n\r", message, err);
    locate(0,1);
    printf("%s", "[PLEASE PRESS ANY KEY (EXIT)]");
    waitkey(0);
    exit(1);
  }
}

void progress_dots(char p) 
{
	char	i;
	char	value;

	for (i=0; i < 5; i++) {
		if (p > i) {
			value = DOT_CLEAR;
		}
		else {
			value = DOT_SET;
		}
		//POKE(PROGRESS_ADDR + (i*2), value); 
	}
}

void disp_menu(char *str) 
{
	puts(0, 160, WHITE, str);
}

void change_location(LOCATION *loc)
{
  char input[40];

  // gfx_cls(CYAN);
  // puts(80, 16, WHITE, "Change location");
  // puts(160, 80, WHITE, "Input city name");
  // puts(160, 88, WHITE, "[ or hit return: current location ]");
  // puts(0, 104, WHITE, ">");
  // gotoxy(2, 13);
  // gets(input);
  // if (strlen(input) == 0)
  // {
  //   *loc = current;
  // }
  // else
  // {
  //   if (!om_geocoding(loc, input))
  //   {
  //     gotoxy(2, 15);
  //     cprintf("city '%s'was not found", input);
  //     gotoxy(2, 16);
  //     cprintf("using current location");
  //     gotoxy(2, 17);
  //     cprintf("(please hit return to continue)");
  //     gets(input);
  //     *loc = current;
  //   }
  // }
  // gfx_cls(CYAN);

  *loc = current;
}
