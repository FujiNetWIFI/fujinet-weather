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
#include "openmeteo.h"
#include "strutil.h"
#include "gfx.h"
#include "font.h"

#define	PROGRESS_X 26
#define PROGRESS_Y 80
#define MENU_Y 184

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
  for (int i = 0; i < 8; i++) // 8 rows
  {
    unsigned char b = font4x8[(unsigned char)ch][i];

    for (int j = 0; j < 4; j++) // 4 columns
    {
      if (b & (1 << (3 - j)))   // test bit 3..0 (left to right)
        pset(x + j, y, c);
    }

    y++;
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
    unsigned char b = font4x8[(unsigned char)ch][i];

    for (int j = 0; j < 4; j++)
    {
      if (b & (1 << (3 - j)))   // reverse bit order
      {
        pset(x + j, y, c);
        pset(x + j, y + 1, c);
      }
    }

    y += 2;
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
    putc(x, y, c, *s++);
    x += 4;

    if (x > 128)
    {
      x = 0;
      y += 4;
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
    putc_dbl(x, y, c, *s++);
    x += 4;

    if (x > 128)
    {
      x = 0;
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
	puts(0,PROGRESS_Y, WHITE, msg);
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
    printf("ERROR: %s", message);
    locate(0,1);
    printf(" CODE: %02X", err);
    locate(0,2);
    printf("%s", "[PLEASE PRESS ANY KEY (EXIT)]");
    waitkey(0);
    exit(1);
  }
}

void progress_dots(char p) 
{
	char	i;
	char	color;

  for (i = 0; i < 5; i++)
  {
    if (p > i)
    {
      color = WHITE;
    }
    else
    {
      color = CYAN;
    }

    putc((PROGRESS_X+i) * 4, PROGRESS_Y, color, '.');
  }
}

void disp_menu(char *str) 
{
	puts(0, MENU_Y, WHITE, str);
}

uint8_t input()
{
  char shift = false;
  char k;

  while (true)
  {
    k = inkey();

    if (isKeyPressed(KEY_PROBE_SHIFT, KEY_BIT_SHIFT))
    {
      shift = 0x00;
    }
    else
    {
      if (k > '@' && k < '[')
        shift = 0x20;
    }

    if (k)
      return k + shift;
  }
}

void get_line(char *buf, uint8_t max_len, int x, int y)
{
  uint8_t c;
  uint8_t i = 0;
  int init_x = x;

  memset(buf, 0, max_len+1);

  do
  {
    c = input();

    if (isprint(c))
    {
      putc(x, y, WHITE, c);
      buf[i] = c;
      if (i < max_len - 1)
      { 
        i++;
        x += 4;
      }

    }
    else if (c == KEY_LEFT_ARROW)
    {
      if (i)
      {
        x -=4;
        putc(x, y, CYAN, buf[i-1]);
        --i;
        buf[i] = '\0';
      }
    }
  } while (c != KEY_ENTER);

  buf[i] = '\0';
}

void change_location(LOCATION *loc)
{
  char input[LINE_LEN];
  char linebuf[LINE_LEN];

  gfx_cls(CYAN);
  puts(4, PROGRESS_Y, WHITE, "Change location");
  puts(4, PROGRESS_Y + 8, WHITE, "Input city name, or");
  puts(4, PROGRESS_Y + 16, WHITE, "ENTER for current location");
  puts(4, PROGRESS_Y + 24, WHITE, ">");

  get_line(input, LINE_LEN - 1, 8, PROGRESS_Y + 24);
  if (strlen(input) == 0)
  {
    *loc = current;
  }
  else
  {
    // Replace spaces with %20 for URL encoding
    puts(4, PROGRESS_Y + 40, WHITE, "Validating city...");
    replaceSpaces(input);
    if (!om_geocoding(loc, input))
    {
      gfx_cls(CYAN);
      sprintf(linebuf, "City '%s' was not found.", input);
      puts(8, PROGRESS_Y, WHITE, linebuf );
      puts(8, PROGRESS_Y + 8, WHITE, "Using current location.");
      puts(8, PROGRESS_Y + 16, WHITE, "Press any key to continue.");
      waitkey(0);
      *loc = current;
    }
  }

  gfx_cls(CYAN);
}
