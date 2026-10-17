#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "weatherdefs.h"
#include "openmeteo.h"
#include "draw.h"
#include "gfx.h"

#define	PROGRESS_ADDR	0x628 + 12	// y = 12 x = 12
#define	PROGRESS_MAX	5
#define	DOT_SET			0x20
#define	DOT_FLASH		0x60
#define	DOT_CLEAR		0xa0

extern LOCATION current;

int	err;

//
// get key-in and return
//
char get_keyin() 
{
	return cgetc();
}


void draw_cursor(void)
{
	unsigned short xpos, ypos;
	get_position(&xpos, &ypos);
	toggle_invert();
	draw_char(' ');
	toggle_invert();
	set_position(xpos, ypos); // restore position
}

void clear_cursor(void)
{
	unsigned short xpos, ypos;
	get_position(&xpos, &ypos);
	erase_char();
	set_position(xpos, ypos); // restore position
}

bool input_text(char *buf, const char *init, unsigned char bufsize)
{
	unsigned char i;
	unsigned short xpos, ypos;
	char ch;

	strncpy(buf, init, bufsize);
	buf[bufsize-1] = '\0';
	i = (unsigned char)strlen(buf);

	draw_text(buf);
	draw_cursor();

	for(;;)
	{
		ch = get_keyin();
		if (ch == 10) { // EOL key
			clear_cursor();
			break; 
		}
		else if (ch == 27) { // Escape key
			clear_cursor();
			return false; 
		}
		else if (ch == 0x7F) { // Delete key
			if (i) {
				buf[--i] = '\0';
				clear_cursor();
				get_position(&xpos, &ypos);
				set_position(xpos-8, ypos);
				draw_cursor();
			}
		} else if (ch >= 32 && ch <= 126) { // Printable characters
			if (i < bufsize - 1) {
				buf[i] = ch;
				buf[++i] = '\0';
				clear_cursor();
				draw_char(ch);
				draw_cursor();
			}
		}
	}
	return true;
}

void disp_status(const char *msg)
{
	set_position(12, 248);
	clear_part(40, 8);
	set_color(MONO_WHITE);
	draw_text(msg);
}

//
// handle_err
//
void handle_err(char *message)
{
    if (err !=  0) {
		disp_status("Error: ");
		draw_text(message);
		get_keyin(); 
        exit(1);
    }
}
//
//
//
// void disp_menu(char *str)
// {
// 	gotoxy(8, 1);
// 	cprintf(str);

void disp_menu_item(char ch, char *s)
{
	toggle_invert();
	draw_char(ch);
	toggle_invert();
	draw_text(s);
}

void disp_weather_menu(void)
{
	set_position(0, 248);
	set_color(MONO_WHITE);
	disp_menu_item('F', "orecast ");
	disp_menu_item('U', "nits ");
	disp_menu_item('L', "ocation ");
	disp_menu_item('R', "efresh ");
	disp_menu_item('Q', "uit");
}

void disp_forecast_menu(char page)
{
	set_position(0, 248);
	set_color(MONO_WHITE);
	if (page == 1) {
		disp_menu_item('N', "ext ");
	} else {
		disp_menu_item('P', "rev ");
	}
	disp_menu_item('B', "ack");
}


// }
// void gfx_cls(void) {

// 	//set_text();
// 	clear_screen();	
// }

bool change_location(LOCATION *loc)
{
	char input[LINE_LEN]; // 32 chars + null terminator
	char linebuf[LINE_LEN];
	bool retry = false;
	const char *init = loc->city;

	for (;;) {
		clear_screen();

		set_color(MONO_GRAY);
		set_position(8*2, 2*8);
		draw_text_double("Change location");

		set_color(MONO_WHITE);
		set_position(2*8, 6*8);
		if (retry) {
			draw_text("City not found. ");
		}
		draw_text("Input city name");

		set_position(2*8, 10*8);
		draw_text("Blank to detect location");
		set_position(2*8, 11*8);
		draw_text("K0 to cancel");

		set_position(1*8, 7*8);
		draw_char('>');

		if (!input_text(input, init, sizeof(input))) {
			// K0 pressed, return without changing location
			return false;
		}

		if (strlen(input) == 0) {
			// Detect location from IP
			disp_message("Fetching location");
			get_location(loc);
			break;
		}

		// Replace spaces with %20 for URL encoding
		disp_message("Searching city");
		strncpy(linebuf, replaceSpaces(input), sizeof(linebuf) - 1);
		linebuf[sizeof(linebuf) - 1] = '\0';
		if (!om_geocoding(loc, linebuf)) {
			init = input; // set default to the last input for retry
			retry = true;
			continue; // prompt again
		}
		break;
	}
	return true;
}

void disp_message(char *msg)
{
	clear_screen();
	int l = strlen(msg);
	if (l>18) l= 18;
	set_position(18*8-(l<<3), 64);
	set_color(MONO_WHITE);
	draw_text_double(msg);
	set_position(0, 96);
	set_color(MONO_GRAY);
}

void progress_dots(char p) 
{
	set_position(140, 136);
	clear_part(2, 8);
	set_color(MONO_GRAY);
	draw_char(p&7);
}

