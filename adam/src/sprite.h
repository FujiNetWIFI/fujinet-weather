/**
 * Weather
 *
 * Based on @bocianu's code
 *
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 *
 */

#ifndef SPRITE_H
#define SPRITE_H

#define SPRITE_CLEAR_SKY             0
#define SPRITE_FEW_CLOUDS            1
#define SPRITE_SCATTERED_CLOUDS      2
#define SPRITE_BROKEN_CLOUDS         3
#define SPRITE_SHOWER_RAIN           4
#define SPRITE_RAIN                  5
#define SPRITE_THUNDERSTORM          6
#define SPRITE_SNOW                  7
#define SPRITE_MIST                  8

#define PATTERN_SUN_ALL_RAYS          0
#define PATTERN_SUN_RAYS_ABOVE        4
#define PATTERN_CLOUD                 8
#define PATTERN_CLOUD_HIGH            12
#define PATTERN_CLOUD_HIGH_TO_RIGHT   16
#define PATTERN_CLOUD_HIGHER_TO_RIGHT 20
#define PATTERN_DOTTED_CLOUD          24
#define PATTERN_MIST                  28
#define PATTERN_MOON                  32
#define PATTERN_CLOUD2                36
#define PATTERN_CLOUD3                40
#define PATTERN_LIGHTENING            44
#define PATTERN_LIGHTENING_TO_RIGHT   48
#define PATTERN_RAIN                  52
#define PATTERN_RAIN_SHORT            56
#define PATTERN_SNOW                  60
#define PATTERN_SNOW_SHORT            64
#define PATTERN_LIGHTENING2           68

#define SPRITE_COLOR_TRANSPARENT       0
#define SPRITE_COLOR_BLACK             1
#define SPRITE_COLOR_MEDIUM_GREEN      2
#define SPRITE_COLOR_LIGHT_GREEN       3
#define SPRITE_COLOR_DARK_BLUE         4
#define SPRITE_COLOR_LIGHT_BLUE        5
#define SPRITE_COLOR_DARK_RED          6
#define SPRITE_COLOR_CYAN              7
#define SPRITE_COLOR_MEDIUM_RED        8
#define SPRITE_COLOR_LIGHT_RED         9
#define SPRITE_COLOR_DARK_YELLOW       10
#define SPRITE_COLOR_LIGHT_YELLOW      11
#define SPRITE_COLOR_DARK_GREEN        12
#define SPRITE_COLOR_MAGENTA           13
#define SPRITE_COLOR_GRAY              14
#define SPRITE_COLOR_WHITE             15

unsigned char icon_get(char *c);

typedef struct
{
    unsigned char y;
    unsigned char x;
    unsigned char sprite_pattern;
    unsigned char color_code : 4;
    unsigned char reserved : 3;    // always zero
    unsigned char early_clock : 1; // 1=shift left horizontally 32 pixels

} SPRITE_ATTRIBUTE;

extern const unsigned char sprite_data[32 * 8];
extern SPRITE_ATTRIBUTE sprite_attributes[32];

/* MEMORY MAP

   VRAM_GENERATOR_TABLE
   0000 - 07FF - char set

   VRAM_COLOR_TABLE
   2000 - 07FF - color set

   VRAM_SPRITE_GENERATOR_TABLE
   3800 - 38FF - sprite table

   VRAM_SPRITE_ATTRIBUTES
   3D00 - 3D7F - sprite attributes
*/

#define VRAM_SPRITE_GENERATOR_TABLE 0x3800 // Library of all sprite patterns (Length 256)
#define VRAM_SPRITE_ATTRIBUTES      0x1B00 // (Length 128)


unsigned char get_sprite(char *c);
void clear_all_sprites();
void display_sprites(void);
void write_sprites(void);
void save_sprite(unsigned char x, unsigned char y, unsigned char icon, bool day);

#ifdef USE_METEO
void get_description(char *wmo, char *description);
#endif

#endif /* ICON_H */
