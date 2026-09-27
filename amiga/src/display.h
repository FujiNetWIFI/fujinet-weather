#ifndef DISPLAY_H
#define DISPLAY_H

#include "weatherdefs.h"

/* Output goes to one AmigaDOS handle: a RAW: window (ansi = 1) or the
 * Shell's Output() for -t (ansi = 0).  Declared as long to keep dos.h out of
 * the host-side tests; it holds a BPTR. */
void display_init(long handle, int ansi);
void out(const char *s);

void show_weather(const WEATHER *w);
void show_status(const char *msg);          /* one-line status under the menu */
void show_menu(const WEATHER *w);

#endif
