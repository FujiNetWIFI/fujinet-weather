/**
 * Weather / sprite.c
 *
 * Based on @bocianu's code
 *
 * @author Norman Davie
 *
 */

#include <stdbool.h>
#include <video/tms99x8.h>
#include <eos.h>
#include <string.h>
#include "sprite.h"
#include "constants.h"

SPRITE_ATTRIBUTE sprite_attributes[32];

unsigned char sprite_counter = 0;
unsigned char sprites[32 * 8]; // Length 256;



const unsigned char spritedata[] = {
    // 0 Sunny
    0x04, 0x44, 0x20, 0x07, 0x0F, 0x1F, 0x1F, 0x1F, 0xDF, 0x1F, 0x1F, 0x0F, 0x07, 0x20, 0x44, 0x04, 0x20, 0x22, 0x04, 0xE0, 0xF0, 0xF8, 0xF8, 0xF8, 0xFB, 0xF8, 0xF8, 0xF0, 0xE0, 0x04, 0x22, 0x20,

    // 1 Half Sunny
    0x04, 0x44, 0x20, 0x07, 0x0F, 0x9F, 0x5F, 0x1F, 0x1F, 0x1F, 0x1F, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x20, 0x22, 0x04, 0xE0, 0xF1, 0xFA, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF0, 0xE0, 0x00, 0x00, 0x00,

    // 2 Cloud 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x6F, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xCC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xF0,

    // 3 Cloud 2
    0x00, 0x00, 0x00, 0x03, 0x07, 0x6F, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xCC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00,

    // 4 Cloud 3
    0x00, 0x00, 0x03, 0x07, 0x6F, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xCC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,

    // 5 Cloud 4
    0x03, 0x07, 0x6F, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xCC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // 6 Mist 1
    0x05, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80, 0x00, 0x20, 0x00, 0x40, 0x00, 0x10, 0x00, 0x0B, 0x00, 0xB4, 0x00, 0x08, 0x00, 0x02, 0x00, 0x01, 0x00, 0x02, 0x00, 0x01, 0x00, 0x02, 0x00, 0x68, 0x00,

    // 7 Mist 2
    0x00, 0x00, 0x0D, 0x00, 0x1B, 0x00, 0x37, 0x00, 0x2F, 0x00, 0x1B, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x00, 0xD8, 0x00, 0xEC, 0x00, 0xD8, 0x00, 0xEC, 0x00, 0xD0, 0x00, 0x00, 0x00,

    // 8 Moon
    0x03, 0x0F, 0x1F, 0x3F, 0x3E, 0x7E, 0x7C, 0x7C, 0x7C, 0x7C, 0x7E, 0x3E, 0x3F, 0x1F, 0x0F, 0x03, 0xE0, 0xF0, 0xFC, 0x84, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x84, 0xFC, 0xF0, 0xE0,

    // 9 Cloud 5
    0x03, 0x07, 0x6F, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xCC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // A Cloud 6
    0x03, 0x07, 0x6F, 0xFF, 0xFF, 0xFF, 0x7F, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xCC, 0xFE, 0xFF, 0xFF, 0xFF, 0xFE, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // B Lightning 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x06, 0x0F, 0x00, 0x00, 0x01, 0x07, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x80, 0x00, 0x00, 0xF0, 0x60, 0xC0, 0x80, 0xC0, 0x80, 0x00,

    // C Lightning 2
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0xC0, 0x80, 0x00, 0xF8, 0x30, 0x60, 0xC0, 0xE0, 0xC0, 0x00,

    // D Rain 1
    0x00, 0x00, 0x00, 0x00, 0x08, 0x0A, 0x02, 0x10, 0x14, 0x05, 0x21, 0x28, 0x0A, 0x42, 0x50, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x84, 0xA0, 0x2A, 0x0A, 0x40, 0x54, 0x14, 0x80, 0xA8, 0x20,

    // E Rain 2
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x21, 0x28, 0x0A, 0x42, 0x50, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x41, 0x55, 0x14, 0x80, 0xAA, 0x22,

    // F Snow 1
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x0A, 0x04, 0x20, 0x51, 0x20, 0x00, 0x44, 0xE0, 0x40, 0x00, 0x00, 0x00, 0x80, 0x00, 0x20, 0x50, 0x22, 0x05, 0x92, 0x40, 0x90, 0x00, 0x4A, 0xE0, 0x44,

    // 10 Snow 2
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x24, 0x80, 0x15, 0x4A, 0xA4, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x02, 0x00, 0x84, 0x01, 0x12, 0x45, 0xA2, 0x48,

    // 11 Lightning 3
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x60, 0xC0, 0x80, 0xFC, 0x18, 0x30, 0x60, 0xF0, 0xE0, 0x80
};

/*
 open-meteo.com

WMO Weather interpretation codes (WW)
Code	    Description
0	        Clear sky
1           Mainly clear
2           partly cloudy
3	        overcast
45, 48	    Fog and depositing rime fog
51, 53, 55	Drizzle: Light, moderate, and dense intensity
56, 57	    Freezing Drizzle: Light and dense intensity
61, 63, 65	Rain: Slight, moderate and heavy intensity
66, 67	    Freezing Rain: Light and heavy intensity
71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
77	        Snow grains
80, 81, 82	Rain showers: Slight, moderate, and violent
85, 86	    Snow showers slight and heavy
95*	        Thunderstorm: Slight or moderate
96, 99 *	Thunderstorm with slight and heavy hail

* Thunderstorm forecast with hail is only available in Central Europe
*/

#ifdef USE_METEO

void get_description(char *wmo, char *description)
{
    int wmo_code = atoi(wmo);
    bool sprite_found = true;

    switch (wmo_code)
    {
    case 0:
        strcpy(description, "Clear Sky");
        break;
    case 1:
        strcpy(description, "Mainly Clear");
        break;
    case 2:
        strcpy(description, "Scattered Clouds");
        break;
    case 3:
        strcpy(description, "Overcast");
        break;
    case 45:
    case 48:
        // fog
        strcpy(description, "Fog or Ice Fog");
        break;
    case 51:
        strcpy(description, "Light Drizzle");
        break;
    case 53:
        strcpy(description, "Moderate Drizzle");
        break;
    case 55:
        strcpy(description, "Dense Drizzle");
        break;
    case 61:
        strcpy(description, "Slight Rain");
        break;
    case 63:
        strcpy(description, "Moderate Rain");
        break;
    case 65:
        strcpy(description, "Heavy Rain");
        break;
    case 66:
        strcpy(description, "Light Freezing Rain");
        break;
    case 67:
        strcpy(description, "Heavy Freezing Rain");
        break;
    case 71:
        strcpy(description, "Slight Snow");
        break;
    case 73:
        strcpy(description, "Moderate Snow");
        break;
    case 75:
        strcpy(description, "Heavy Snow");
        break;
    case 77:
        strcpy(description, "Snow Grains");
        break;
    case 80:
        strcpy(description, "Light Showers");
        break;
    case 81:
        strcpy(description, "Moderate Showers");
        break;
    case 82:
        strcpy(description, "Violent Showers");
        break;
    case 85:
        strcpy(description, "Slight Snow Showers");
        break;
    case 86:
        strcpy(description, "Heavy Snow Showers");
        break;
    case 95:
        strcpy(description, "Thunderstorm");
        break;
    case 96:
        strcpy(description, "Thunderstorms with Slight Hail");
        break;
    case 99:
        strcpy(description, "Thunderstorms with Heavy Hail");
        break;
    default:
        sprite_found = false;
    }

    // generic cases
    if (!sprite_found)
    {
        if (wmo_code < 20)
            strcpy(description, "Scattered Clouds");
        else if (wmo_code < 30)
            strcpy(description, "Rain");
        else if (wmo_code < 40)
            strcpy(description, "Snow");
        else if (wmo_code < 50)
            strcpy(description, "Fog");
        else if (wmo_code < 60)
            strcpy(description, "Rain");
        else if (wmo_code < 70)
            strcpy(description, "Rain");
        else if (wmo_code < 80)
            strcpy(description, "Snow");
        else
            strcpy(description, "Thunderstorms");
    }

}


unsigned char get_sprite(char *c)
{
    int wmo_code = atoi(c);
    unsigned char sprite = SPRITE_CLEAR_SKY;
    bool sprite_found = true;


    switch (wmo_code)
    {
        case 0:
        case 1:
            sprite = SPRITE_CLEAR_SKY;
            break;
        case 2:
            sprite = SPRITE_FEW_CLOUDS;
            break;
        case 3:
            sprite = SPRITE_SCATTERED_CLOUDS;
            break;
        case 45:
        case 48:
            // fog
            sprite = SPRITE_MIST;
            break;
        case 51:
        case 53:
        case 55:
            sprite = SPRITE_SHOWER_RAIN;
            break;
        case 61:
        case 63:
        case 65:
            sprite = SPRITE_RAIN;
            break;
        case 66:
        case 67:
            sprite = SPRITE_SNOW;
            break;
        case 71:
        case 73:
        case 75:
        case 77:
            sprite = SPRITE_SNOW;
            break;
        case 80:
        case 81:
        case 82:
            sprite = SPRITE_SHOWER_RAIN;
            break;
        case 85:
        case 86:
            sprite = SPRITE_SNOW;
            break;
        case 95:
        case 96:
        case 99:
            sprite = SPRITE_THUNDERSTORM;
            break;
        default:
            sprite_found = false;
    }


    // generic cases
    if (! sprite_found)
    {
        if (wmo_code < 20)
            sprite = SPRITE_FEW_CLOUDS;
        else
        if (wmo_code < 30)
            sprite = SPRITE_SHOWER_RAIN;
        else 
        if (wmo_code < 40)
            sprite = SPRITE_SNOW;
        else 
        if (wmo_code < 50)
            sprite = SPRITE_MIST;
        else 
        if (wmo_code < 60)
            sprite = SPRITE_SHOWER_RAIN;
        else 
        if (wmo_code < 70)
            sprite = SPRITE_SHOWER_RAIN;
        else 
        if (wmo_code < 80)
            sprite = SPRITE_SNOW;
        else
            sprite = SPRITE_THUNDERSTORM;
    }

    return sprite;
}

#else

// https://openweathermap.org/weather-conditions
 
unsigned char get_sprite(char *c)
{
    if (strcmp(c, "01d") == 0 || strcmp(c, "01n") == 0)
        return SPRITE_CLEAR_SKY;
    else if (strcmp(c, "02d") == 0 || strcmp(c, "02n") == 0)
        return SPRITE_FEW_CLOUDS;
    else if (strcmp(c, "03d") == 0 || strcmp(c, "03n") == 0)
        return SPRITE_SCATTERED_CLOUDS;
    else if (strcmp(c, "04d") == 0 || strcmp(c, "04n") == 0)
        return SPRITE_BROKEN_CLOUDS;
    else if (strcmp(c, "09d") == 0 || strcmp(c, "09n") == 0)
        return SPRITE_SHOWER_RAIN;
    else if (strcmp(c, "10d") == 0 || strcmp(c, "10n") == 0)
        return SPRITE_RAIN;
    else if (strcmp(c, "11d") == 0 || strcmp(c, "11n") == 0)
        return SPRITE_THUNDERSTORM;
    else if (strcmp(c, "13d") == 0 || strcmp(c, "13n") == 0)
        return SPRITE_SNOW;
    else if (strcmp(c, "50d") == 0 || strcmp(c, "50n") == 0)
        return SPRITE_MIST;
}

#endif


/*
clear_all_sprites 
- ensures all the sprites are in memory and removes existing defined sprites from the display

*/
void clear_all_sprites() 
{
    eos_write_vdp_register(1, 0xE3);
    vdp_vwrite(spritedata, VRAM_SPRITE_GENERATOR_TABLE, sizeof(spritedata));

    sprite_counter = 0;
    memset(sprite_attributes, 208, 32 * sizeof(SPRITE_ATTRIBUTE));

    display_sprites();
}

/*
Displays all the defined sprites at the appropriate positions
*/
void display_sprites(void) 
{ 
    vdp_vwrite(sprite_attributes, VRAM_SPRITE_ATTRIBUTES, 32 * sizeof(SPRITE_ATTRIBUTE)); 
}


/*
Saves a sprite for use, but does not display the sprite until display_sprites is called
*/


void save_sprite(unsigned char x, unsigned char y, unsigned char icon, bool day)
{

    switch (icon)
    {
    case SPRITE_CLEAR_SKY: // clear sky
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = day == true ? PATTERN_SUN_ALL_RAYS : PATTERN_MOON;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_YELLOW;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;
        break;

    case SPRITE_FEW_CLOUDS: // few clouds
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_WHITE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = day == true ? PATTERN_SUN_RAYS_ABOVE : PATTERN_MOON;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_YELLOW;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;
        break;

    case SPRITE_SCATTERED_CLOUDS: // Scattered clouds
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGH_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_WHITE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;
        break;

    case SPRITE_BROKEN_CLOUDS: // broken clouds
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGH_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_GRAY;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGHER_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_BLACK;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        break;

    case SPRITE_SHOWER_RAIN: // shower rain
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_RAIN_SHORT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_BLUE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        if (day == true)
        {
            sprite_attributes[sprite_counter].y = y;
            sprite_attributes[sprite_counter].x = x;
            sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGH_TO_RIGHT;
            sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_GRAY;
            sprite_attributes[sprite_counter].reserved = 0;
            sprite_attributes[sprite_counter].early_clock = 0;
            sprite_counter++;

            sprite_attributes[sprite_counter].y = y;
            sprite_attributes[sprite_counter].x = x;
            sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGHER_TO_RIGHT;
            sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_BLACK;
            sprite_attributes[sprite_counter].reserved = 0;
            sprite_attributes[sprite_counter].early_clock = 0;
            sprite_counter++;
        }
        else
        {
            sprite_attributes[sprite_counter].y = y;
            sprite_attributes[sprite_counter].x = x;
            sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD;
            sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_WHITE;
            sprite_attributes[sprite_counter].reserved = 0;
            sprite_attributes[sprite_counter].early_clock = 0;
            sprite_counter++;

            sprite_attributes[sprite_counter].y = y;
            sprite_attributes[sprite_counter].x = x;
            sprite_attributes[sprite_counter].sprite_pattern = day == true ? PATTERN_SUN_RAYS_ABOVE : PATTERN_MOON;
            sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_YELLOW;
            sprite_attributes[sprite_counter].reserved = 0;
            sprite_attributes[sprite_counter].early_clock = 0;
            sprite_counter++;
        }
        break;

    case SPRITE_RAIN: // rain
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_RAIN_SHORT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_BLUE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_WHITE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = day == true ? PATTERN_SUN_RAYS_ABOVE : PATTERN_MOON;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_YELLOW;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;
        break;

    case SPRITE_THUNDERSTORM: // thunderstorm
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_LIGHTENING_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_YELLOW;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGH_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_GRAY;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGHER_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_BLACK;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_RAIN_SHORT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_DARK_BLUE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;
        break;

    case SPRITE_SNOW: // snow
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_SNOW;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_GRAY;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGH_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_WHITE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_CLOUD_HIGHER_TO_RIGHT;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_BLACK;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

    case SPRITE_MIST: // mist
        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_MIST;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_WHITE;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;

        sprite_attributes[sprite_counter].y = y;
        sprite_attributes[sprite_counter].x = x;
        sprite_attributes[sprite_counter].sprite_pattern = PATTERN_DOTTED_CLOUD;
        sprite_attributes[sprite_counter].color_code = SPRITE_COLOR_GRAY;
        sprite_attributes[sprite_counter].reserved = 0;
        sprite_attributes[sprite_counter].early_clock = 0;
        sprite_counter++;
        break;
    }
}

