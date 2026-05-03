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
#ifndef COCO3
#include "font.h"
#endif

#ifdef COCO3
#include "charset_coco3.h"
#include "charset_coco3_icons.h"

/* Screen layout (40x25 chars, 8x8 4bpp tiles):
 *   - Mapped to logical $8000 via MMU task 1, physical $68000 (blocks 52-55).
 *   - 4 bytes per char-row (8 px x 4bpp), 160 bytes per pixel-row.
 *   - 32000 bytes total.
 */
#define COCO3_SCREEN_PTR ((byte *)0x8000)
#define COCO3_BYTES_PER_ROW 160
#define COCO3_BYTES_PER_CHAR 4
#define COCO3_WIDTH 40
#define COCO3_HEIGHT 25

/* MMU mapping for the graphics task: low-half code/data unchanged, upper
 * half mapped to the four graphics blocks that hold the screen buffer. */
static const byte video_mmu_blocks[8] = {
    56, 57, 58, 59,
    52, 53, 54, 55,
};

/* RGB palette - same values battleship uses (verified to give black bg / white fg).
 * Mutable, non-static so cmoc emits a normal data section copy.
 */
byte palette[16] = {
     0,   /*  0 black      */
     7,   /*  1 dark gray  */
    56,   /*  2 light gray */
    63,   /*  3 white      */
    28,   /*  4 teal       */
    11,   /*  5 light blue */
     1,   /*  6 dark blue  */
     9,   /*  7 sea blue   */
    25,   /*  8 foam       */
    27,   /*  9 light foam */
     4,   /* 10 dark red   */
    36,   /* 11 red        */
    38,   /* 12 red orange */
    52,   /* 13 orange     */
    54,   /* 14 yellow     */
    63,   /* 15 white      */
};

static byte paletteBackup[16];
static word oldGime;

/* Switch the GIME's active MMU task ($FF91 bit 0).
 *  - mmu_use_main:  task 0 -- normal mapping (code, data, BASIC ROM area).
 *  - mmu_use_video: task 1 -- screen buffer at $8000-$FFFF for drawing.
 * Plain C writes (rather than inline asm) so cmoc tracks register usage. */
#define mmu_use_main()  (*(byte *)0xFF91 = 0)
#define mmu_use_video() (*(byte *)0xFF91 = 1)
#define BEGIN_GFX       disableInterrupts(); mmu_use_video();
#define END_GFX         mmu_use_main(); enableInterrupts();

#endif /* COCO3 */

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
#ifndef COCO3
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
#endif

/**
 * @brief Put character ch in font at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param ch Character to put (0-127)
 * @param dbl Make the character double height
 */
void putc(int x, int y, char c, char ch, bool dbl)
{
#ifdef COCO3
  /* On CoCo 3, treat caller's pixel coords as if they were CoCo 1/2 4x8 cells
   * and remap to 8x8 cells in the 40x25 grid. Caller patterns like
   *   putc(col*4, row*8, ...)
   * therefore land at hires_putc(col, row, ...).
   * The dbl-height case is collapsed to single height for now.
   */
  (void)c; (void)dbl;
  if (ch < 0x20) return;
  hires_putc((unsigned char)(x >> 2), (unsigned char)(y >> 3), ch, false);
#else
  for (int i = 0; i < 8; i++) // 8 rows
  {
    byte realch;

    // Font array starts at 0x20,
    // So skip any value less than that
    // And subtract 0x20 from any valued passed in.
    if (ch < 0x20)
    {
      return;
    }
    else
    {
      realch = ch - 0x20;
    }

    unsigned char b = font4x8[(unsigned char)realch][i];

    for (int j = 0; j < 4; j++) // 4 columns
    {
      if (b & (1 << (3 - j)))   // test bit 3..0 (left to right)
      {
        pset(x + j, y, c);
        if (dbl)
        {
          pset(x + j, y + 1, c);
        }
      }
    }

    y++;
    if (dbl)
    {
      y++;
    }
  }
#endif
}

/**
 * @brief Put string s using putc at x,y with color c
 * @param x horizontal position (0-127)
 * @param y vertical position (0-191)
 * @param c Color (0-3)
 * @param s NULL terminated string to place on graphics screen
 * @param dbl Make the characters double height
 */
#ifndef COCO3
/* Draw ch inverted: every pixel of the 4x8 cell is set to fg, with the
 * font glyph cut out (left at the previous pixel value, typically the
 * cleared bg).  Used by puts() when a \001 toggle is active. */
static void putc_inv(int x, int y, char c, char ch, bool dbl)
{
  byte realch;
  if (ch < 0x20) return;
  realch = (byte)(ch - 0x20);
  for (int i = 0; i < 8; i++)
  {
    /* Bottom 4 bits of the row inverted -- now 1 = "fill" (fg block),
     * 0 = "leave alone" (the glyph cutout). */
    unsigned char b = (unsigned char)(~font4x8[realch][i]) & 0x0F;
    for (int j = 0; j < 4; j++)
    {
      if (b & (1 << (3 - j)))
      {
        pset(x + j, y, c);
        if (dbl) pset(x + j, y + 1, c);
      }
    }
    y++;
    if (dbl) y++;
  }
}
#endif

void puts(int x, int y, char c, const char *s, bool dbl)
{
#ifdef COCO3
  /* All COCO3 callers pass WHITE; drop color and delegate to drawText. */
  (void)c; (void)dbl;
  drawText((unsigned char)(x >> 2), (unsigned char)(y >> 3), s);
#else
  char ch;
  bool inv = false;
  while ((ch = *s++) != 0)
  {
    if (ch == 0x01) { inv = !inv; continue; }
    if (inv) putc_inv(x, y, c, ch, dbl);
    else     putc    (x, y, c, ch, dbl);
    x += 4;

    if (x > 128)
    {
      x = 0;
      y += 4;
      if (dbl)
      {
        y += 12;
      }
    }
  }
#endif
}

#ifdef COCO3
/* Forward decl -- expand_2bpp_row is defined later in this file. */
void expand_2bpp_row(byte b0, byte b1, byte *dest, const byte *pal);

/* Draw an icon at char (cx, cy).  Icon image data is inline 32x32 px in
 * 2bpp form (4x4 char tiles), and each icon carries its own 4-entry
 * palette so different icons can use different colors out of the
 * 16-color screen palette. */
void put_icon(unsigned char cx, unsigned char cy, unsigned char idx)
{
  unsigned char ty, tx, r;
  const byte *src;
  const byte *pal;
  byte *row_base;

  if (idx >= ICONS_COCO3_N) idx = 0;
  pal = icon_palette[idx];

  BEGIN_GFX
  for (ty = 0; ty < ICON_TILES_SIDE; ty++)
  {
    for (tx = 0; tx < ICON_TILES_SIDE; tx++)
    {
      src = &charset_coco3_icons[idx][ty * ICON_TILES_SIDE + tx][0];
      row_base = COCO3_SCREEN_PTR
               + (word)(cy + ty) * 8 * COCO3_BYTES_PER_ROW
               + (word)(cx + tx) * COCO3_BYTES_PER_CHAR;
      for (r = 0; r < 8; r++)
      {
        expand_2bpp_row(src[0], src[1], row_base, pal);
        src += 2;
        row_base += COCO3_BYTES_PER_ROW;
      }
    }
  }
  END_GFX
}

#else
/* place icon on 8x8 boundary at pixel coords (x, y) -- CoCo 1/2 only */
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
#endif

#ifndef COCO3
/**
 * @brief set screen buffer address to BASIC location
 */
void set_screenbuffer(void)
{
  screenBuffer = (byte *) (((word) * (byte *) 0x00BC) << 8);
}
#endif

/**
 * @brief set up graphics for PMODE 3
 * @param c color set to use
 */
void gfx(unsigned char c)
{
#ifdef COCO3
  byte i;
  byte *pal_io = (byte *)0xFFB0;
  byte *pal_bak = paletteBackup;

  initCoCoSupport();

  disableInterrupts();

  /* Map physical blocks 52-55 (graphics) into MMU task 1 slots 4-7. */
  for (i = 0; i < 8; i++)
    ((byte *)0xFFA8)[i] = video_mmu_blocks[i];

  /* Back up current palette, then explicitly load ours byte by byte.
   * Doing this explicitly avoids any memcpy weirdness on I/O addresses. */
  for (i = 0; i < 16; i++)
  {
    pal_bak[i] = pal_io[i];
    pal_io[i]  = palette[i];
  }

  asm { sync }

  /* Switch to native CoCo 3 graphics mode. */
  *(byte *)0xFF90 = 0x4C;   /* clear CoCo 2 compat bit */
  *(byte *)0xFF98 = 0x80;   /* graphics mode */

  /* GIME mode: 200 lines, 160 bytes/row, 16 colors (320x200x16). */
  *(byte *)0xFF99 = 0x3E;
  *(byte *)0xFF9A = 0;      /* black border */

  /* Point GIME at MMU block 52 (= $D000 = 52<<10). */
  oldGime = *(word *)0xFF9D;
  *(byte *)0xFF9D = 0xD0;
  *(byte *)0xFF9E = 0x00;

  /* Clear screen via the video MMU mapping. */
  mmu_use_video();
  memset(COCO3_SCREEN_PTR, 0, 32000U);
  mmu_use_main();

  enableInterrupts();
  (void)c;  /* color-set arg unused on CoCo 3 */
#else
  initCoCoSupport();
  rgb();
  width(32);
  set_screenbuffer();

  pmode(3,screenBuffer);
  screen(1,c);
#endif
}

/**
 * @brief Tear down graphics state before exiting back to BASIC / cold-start.
 *
 * On CoCo 3 we put the GIME back into CoCo 2 compatibility mode and restore
 * the palette we backed up in gfx() init.  Without this, BASIC's restart
 * after coldStart() would draw text into graphics-mode memory and the user
 * would just see garbage instead of the FujiNet config disk autoboot.
 *
 * On CoCo 1/2 nothing GIME-specific to undo, but we also flip back to a
 * normal text screen for symmetry.
 */
void gfx_shutdown(void)
{
#ifdef COCO3
  byte i;
  byte *pal_io  = (byte *)0xFFB0;
  byte *pal_bak = paletteBackup;

  disableInterrupts();
  /* Pre-clear the text-screen RAM at $0400-$05FF so the moment we flip
   * to text mode the user sees the standard green screen.  0x60 is the
   * inverse space -- BASIC's CLS fill char. */
  memset((byte *)0x0400, 0x60, 0x200);

  /* Park the GIME video offset somewhere harmless (block 0) before the
   * mode flip so we don't briefly display graphics-block content. */
  *(byte *)0xFF9D = 0x00;
  *(byte *)0xFF9E = 0x00;
  *(byte *)0xFF98 = 0x00;   /* text mode */
  *(byte *)0xFF99 = 0x00;

  /* Reset the SAM video registers ($FFC0-$FFC5) to "alphanumeric / text
   * mode" -- any read of the even address clears the corresponding V bit. */
  (void)*(byte *)0xFFC0;   /* V0 = 0 */
  (void)*(byte *)0xFFC2;   /* V1 = 0 */
  (void)*(byte *)0xFFC4;   /* V2 = 0 */

  /* PIA1 port B ($FF22) holds the VDG mode bits the GIME reads in compat
   * mode: bit 7 = AG (graphics/text), 6-4 = GM2/1/0, 3 = CSS.  Force
   * AG=0 so we land in alphanumeric mode, not graphics. */
  *(byte *)0xFF22 = 0x00;

  /* Now actually flip to CoCo 2 compatibility mode (bit 7) with MMU
   * disabled.  RESET in coldStart() relies on this for BASIC's normal
   * text-screen init. */
  *(byte *)0xFF90 = 0x80;

  /* Restore the palette we backed up (CoCo 3 native mode register) so
   * any post-reset compat-mode display isn't tinted by our greens. */
  for (i = 0; i < 16; i++)
    pal_io[i] = pal_bak[i];
  enableInterrupts();
#else
  screen(1, 1);
#endif
}

/**
 * @brief Clear display
 * @param c Color to clear display with
 */
void gfx_cls(unsigned char c)
{
#ifdef COCO3
  /* Fill the screen with palette index c (each byte = 2 pixels of c). */
  byte fill = (c << 4) | (c & 0x0F);
  extern unsigned char panel_drawn;
  panel_drawn = 0;
  BEGIN_GFX
  memset(COCO3_SCREEN_PTR, fill, 32000U);
  END_GFX
#else
  const byte b[4]={0x00,0x55,0xAA,0xFF};

  memset(screenBuffer,b[c],6144);
#endif
}

#ifdef COCO3
/* 2bpp text-pixel value -> 4bpp CoCo 3 palette index, applied at render
 * time.  Adjust to recolor all subsequent text.  Default {0, 3, 3, 3} =
 * "any non-zero pixel -> white", matching the binary CGA source font. */
unsigned char text_palette[4] = { 0, 3, 3, 3 };

/* Expand one 8-pixel 2bpp row (2 source bytes; b0 = leftmost 4 px, MSB
 * first) into 4 destination bytes at 4bpp, mapping each 2bpp value
 * through the supplied 4-entry palette.  Non-static so cmoc keeps it as
 * a real function call instead of inlining at every call site. */
void expand_2bpp_row(byte b0, byte b1, byte *dest, const byte *pal)
{
  dest[0] = (pal[(b0 >> 6) & 3] << 4) | pal[(b0 >> 4) & 3];
  dest[1] = (pal[(b0 >> 2) & 3] << 4) | pal[ b0       & 3];
  dest[2] = (pal[(b1 >> 6) & 3] << 4) | pal[(b1 >> 4) & 3];
  dest[3] = (pal[(b1 >> 2) & 3] << 4) | pal[ b1       & 3];
}

/* Place an 8x8 char at (cx, cy) in the 40x25 grid.  Char codes
 * >= CHARSET_COCO3_N are clamped to a blank.  inv flips each 2bpp pixel
 * (00 <-> 11, 01 <-> 10) before palette lookup. */
void hires_putc(unsigned char cx, unsigned char cy, char ch, bool inv)
{
  unsigned char uch = (unsigned char)ch;
  byte *dest = COCO3_SCREEN_PTR
             + (word)cy * 8 * COCO3_BYTES_PER_ROW
             + (word)cx * COCO3_BYTES_PER_CHAR;
  const byte *src;
  byte xor_mask = inv ? 0xFF : 0x00;
  byte i;

  if (uch >= CHARSET_COCO3_N) uch = 0;
  src = &charset_coco3[uch][0];

  BEGIN_GFX
  for (i = 0; i < 8; i++)
  {
    expand_2bpp_row(src[0] ^ xor_mask, src[1] ^ xor_mask, dest, text_palette);
    src  += 2;
    dest += COCO3_BYTES_PER_ROW;
  }
  END_GFX
}

/* 8x8 char stretched to 8x16 (1 col wide x 2 rows tall).  Each source row
 * is rendered into two consecutive output pixel rows. */
static void hires_putc_double(unsigned char cx, unsigned char cy, char ch)
{
  unsigned char uch = (unsigned char)ch;
  byte *dest = COCO3_SCREEN_PTR
             + (word)cy * 8 * COCO3_BYTES_PER_ROW
             + (word)cx * COCO3_BYTES_PER_CHAR;
  const byte *src;
  byte i;

  if (uch >= CHARSET_COCO3_N) uch = 0;
  src = &charset_coco3[uch][0];

  BEGIN_GFX
  for (i = 0; i < 8; i++)
  {
    expand_2bpp_row(src[0], src[1], dest, text_palette);
    dest[COCO3_BYTES_PER_ROW + 0] = dest[0];
    dest[COCO3_BYTES_PER_ROW + 1] = dest[1];
    dest[COCO3_BYTES_PER_ROW + 2] = dest[2];
    dest[COCO3_BYTES_PER_ROW + 3] = dest[3];
    src  += 2;
    dest += COCO3_BYTES_PER_ROW * 2;
  }
  END_GFX
}

/* Char-coord text draw with \001 inverse-toggle support. */
void drawText(unsigned char cx, unsigned char cy, const char *s)
{
  unsigned char inv = 0;
  char ch;
  while ((ch = *s++) != 0)
  {
    if (ch == 0x01) { inv ^= 1; continue; }
    if (cx >= COCO3_WIDTH) { cx = 0; cy++; }
    if (cy >= COCO3_HEIGHT) return;
    hires_putc(cx++, cy, ch, inv);
  }
}

/* Char-coord double-height text. Each char is 1 col x 2 rows tall. */
void drawTextDouble(unsigned char cx, unsigned char cy, const char *s)
{
  while (*s)
  {
    if (cx >= COCO3_WIDTH) { cx = 0; cy += 2; }
    if (cy + 1 >= COCO3_HEIGHT) return;
    hires_putc_double(cx++, cy, *s++);
  }
}

/* Draw a w-cell horizontal line at (x, y) with given left/middle/right glyphs.
 * Used by both the screen border and the disp_message panel. */
static void hline(unsigned char x, unsigned char y, unsigned char w,
                  char l, char m, char r)
{
  unsigned char i;
  hires_putc(x, y, l, false);
  for (i = 1; i < (unsigned char)(w - 1); i++)
    hires_putc((unsigned char)(x + i), y, m, false);
  hires_putc((unsigned char)(x + w - 1), y, r, false);
}

void draw_border(void)
{
  unsigned char i;
  hline(0, 0,  40, 0xC9, 0xCD, 0xBB);
  for (i = 1; i < 24; i++)
  {
    hires_putc(0,  i, 0xBA, false);
    hires_putc(39, i, 0xBA, false);
  }
  hline(0, 24, 40, 0xC8, 0xCD, 0xBC);
}

void draw_hdiv(unsigned char y)
{
  hline(0, y, 40, 0xC7, 0xC4, 0xB6);
}

void draw_forecast_border(void)
{
  unsigned char i, x;
  hline(0, 0, 40, 0xC9, 0xCD, 0xBB);
  for (x = 10; x <= 30; x += 10)
    hires_putc(x, 0, 0xD1, false);
  for (i = 1; i <= 21; i++)
  {
    hires_putc(0,  i, 0xBA, false);
    hires_putc(39, i, 0xBA, false);
    hires_putc(10, i, 0xB3, false);
    hires_putc(20, i, 0xB3, false);
    hires_putc(30, i, 0xB3, false);
  }
  hline(0, 22, 40, 0xC7, 0xC4, 0xB6);
  for (x = 10; x <= 30; x += 10)
    hires_putc(x, 22, 0xC1, false);
  hires_putc(0,  23, 0xBA, false);
  hires_putc(39, 23, 0xBA, false);
  hline(0, 24, 40, 0xC8, 0xCD, 0xBC);
}
#endif /* COCO3 */


#ifdef COCO3
/* MSDOS-style centered "fetching" panel: a 30-wide single-row box with the
 * message centered inside. progress_dots() inverts cells progressively to
 * make the message itself act as a progress bar.
 *
 * Total expected progress_dots calls per data fetch is 49 (matches MSDOS),
 * so 49 calls fill the entire 30-cell width.
 */
#define COCO3_PROGRESS_X      5
#define COCO3_PROGRESS_Y      12
#define COCO3_PROGRESS_WIDTH  30
#define COCO3_PROGRESS_TOTAL  49

static char progress_text[COCO3_PROGRESS_WIDTH + 1];
static unsigned char progress_count;
/* True when the box is currently on screen. Cleared by gfx_cls() so the next
 * disp_message redraws the box; while true, disp_message only refreshes the
 * message row + progress bar. */
unsigned char panel_drawn;

void disp_message(char *msg)
{
  unsigned char i, len, pad;

  if (!panel_drawn)
  {
    gfx_cls(BGCOLOR);
    hline(COCO3_PROGRESS_X - 1, COCO3_PROGRESS_Y - 1,
          COCO3_PROGRESS_WIDTH + 2, 0xC9, 0xCD, 0xBB);
    hires_putc(COCO3_PROGRESS_X - 1, COCO3_PROGRESS_Y, 0xBA, false);
    hires_putc(COCO3_PROGRESS_X + COCO3_PROGRESS_WIDTH, COCO3_PROGRESS_Y,
               0xBA, false);
    hline(COCO3_PROGRESS_X - 1, COCO3_PROGRESS_Y + 1,
          COCO3_PROGRESS_WIDTH + 2, 0xC8, 0xCD, 0xBC);
    panel_drawn = 1;
  }

  /* Center the message inside the 30-cell row, padded with spaces. */
  len = (unsigned char)strlen(msg);
  if (len > COCO3_PROGRESS_WIDTH) len = COCO3_PROGRESS_WIDTH;
  memset(progress_text, ' ', COCO3_PROGRESS_WIDTH);
  progress_text[COCO3_PROGRESS_WIDTH] = '\0';
  pad = (COCO3_PROGRESS_WIDTH - len) / 2;
  memcpy(progress_text + pad, msg, len);

  for (i = 0; i < COCO3_PROGRESS_WIDTH; i++)
    hires_putc(COCO3_PROGRESS_X + i, COCO3_PROGRESS_Y,
               progress_text[i], false);

  progress_count = 0;
}
#else
void disp_message(char *msg)
{
	gfx_cls(BGCOLOR);
	puts(0,PROGRESS_Y, WHITE, msg, false);
}
#endif

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

#ifdef COCO3
/* Each call advances an internal counter. The progress bar is drawn by
 * re-rendering the centered message row with cells 0..filled inverted.
 * The caller's `p` argument (a 0..5 wrapping value from increment_dot) is
 * ignored here -- we use a monotonic 0..PROGRESS_TOTAL count instead.
 */
void progress_dots(char p)
{
  unsigned char filled, i;
  (void)p;

  if (progress_count < COCO3_PROGRESS_TOTAL)
    progress_count++;

  filled = (unsigned char)(((unsigned int)progress_count * COCO3_PROGRESS_WIDTH)
                           / COCO3_PROGRESS_TOTAL);

  for (i = 0; i < COCO3_PROGRESS_WIDTH; i++)
    hires_putc(COCO3_PROGRESS_X + i, COCO3_PROGRESS_Y,
               progress_text[i], i < filled);
}
#else
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

    putc((PROGRESS_X+i) * 4, PROGRESS_Y, color, '.', false);
  }
}
#endif

#ifdef COCO3
/* Number of visible glyphs in s, ignoring 0x01 inverse-toggle markers. */
static unsigned char visible_len(const char *s)
{
  unsigned char n = 0;
  while (*s)
  {
    if (*s != 0x01) n++;
    s++;
  }
  return n;
}
#endif

void disp_menu(char *str)
{
#ifdef COCO3
  /* Inside the border, on row 23, centered within cols 1-38. */
  unsigned char vlen = visible_len(str);
  unsigned char cx = vlen < 38 ? 1 + (38 - vlen) / 2 : 1;
  drawText(cx, 23, str);
#else
  puts(0, MENU_Y, WHITE, str, false);
#endif
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

bool get_line(char *buf, uint8_t max_len, int x, int y)
{
  uint8_t c;
  uint8_t i = 0;
  int init_x = x;

  memset(buf, 0, max_len+1);

  do
  {
    // Fakey little "cursor"
    putc(x, y, WHITE, '_', false);

    c = input();

    if (isprint(c))
    {
      // Erase cursor
      putc(x, y, BGCOLOR, '_', false);
      // Display character
      putc(x, y, WHITE, c, false);
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
        // Erase cursor
        putc(x, y, BGCOLOR, '_', false);
        x -=4;
        // Erase character
        putc(x, y, BGCOLOR, buf[i-1], false);
        --i;
        buf[i] = '\0';
      }
    }
    else if (c == KEY_BREAK)
    {
      return false;
    }
  } while (c != KEY_ENTER);

  buf[i] = '\0';
  return true;
}

void change_location(LOCATION *loc)
{
  char input[LINE_LEN];
  char linebuf[LINE_LEN];

  gfx_cls(BGCOLOR);
  puts(4, PROGRESS_Y, WHITE, "Change location", false);
  puts(4, PROGRESS_Y + 8, WHITE, "Input city name,", false);
  puts(4, PROGRESS_Y + 16, WHITE, "ENTER to detect location, or", false);
  puts(4, PROGRESS_Y + 24, WHITE, "BREAK to cancel.", false);
  putc(4, PROGRESS_Y + 32, WHITE, '>', false);

  // If the user hits BREAK, return without doing anything
  if (get_line(input, LINE_LEN - 1, 8, PROGRESS_Y + 32) == false)
  {
    return;
  }
  
  if (strlen(input) == 0)
  {
    // Detect location from IP
    gfx_cls(BGCOLOR);
    disp_message("  Fetching location data...");
	  get_location(loc);
  }
  else
  {
    // Replace spaces with %20 for URL encoding
    puts(4, PROGRESS_Y + 40, WHITE, "Validating city...", false);
    strcpy(linebuf, replaceSpaces(input));
    if (!om_geocoding(loc, linebuf))
    {
      gfx_cls(BGCOLOR);
      sprintf(linebuf, "City '%s'", input);
      puts(8, PROGRESS_Y, WHITE, linebuf, false);
      puts(8, PROGRESS_Y + 8, WHITE, "not found.", false);
      puts(8, PROGRESS_Y + 16, WHITE, "Using current location.", false);
      puts(8, PROGRESS_Y + 32, WHITE, "Press any key to continue.", false);
      waitkey(0);
      *loc = current;
    }
  }

  gfx_cls(BGCOLOR);
}
