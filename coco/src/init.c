/**
 * Weather / init.c
 *
 * Based on @bocianu's code
 *
 * @author Thomas Cherryhomes
 * @email thom dot cherryhomes at gmail dot com
 *
 */

#include <cmoc.h>
#include <coco.h>

#include "gfx.h"

void init(void) { 
    gfx(1);
    gfx_cls(CYAN);
}
