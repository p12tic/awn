
#ifndef __AWN_CAIRO_UTILS_H__
#define __AWN_CAIRO_UTILS_H__

#include <gtk/gtk.h>


typedef enum _RoundCorners AwnCairoRoundCorners;
enum _RoundCorners {
	ROUND_NONE			= 0,
	ROUND_TOP_LEFT		= 1,
	ROUND_TOP_RIGHT		= 2,
	ROUND_BOTTOM_RIGHT	= 4,
	ROUND_BOTTOM_LEFT	= 8,
	ROUND_TOP			= ROUND_TOP_LEFT | ROUND_TOP_RIGHT,
	ROUND_BOTTOM		= ROUND_BOTTOM_LEFT | ROUND_BOTTOM_RIGHT,
	ROUND_LEFT			= ROUND_TOP_LEFT | ROUND_BOTTOM_LEFT,
	ROUND_RIGHT			= ROUND_TOP_RIGHT | ROUND_BOTTOM_RIGHT,
	ROUND_ALL			= ROUND_LEFT | ROUND_RIGHT
};



void awn_cairo_rounded_rect( cairo_t *cr, int x0, int y0, int width, int height, double radius, AwnCairoRoundCorners state );

#endif

