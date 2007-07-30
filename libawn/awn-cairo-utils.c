
#include <gtk/gtk.h>

#include "awn-cairo-utils.h"

//
// awn_cairo_rounded_rect - draws a rounded rectangle via cairo
//
void awn_cairo_rounded_rect( cairo_t *cr, int x0, int y0, int width, int height, double radius, AwnCairoRoundCorners state ) {
	double x1,y1;

	x1 = x0 + width;
	y1 = y0 + height;

	// top left corner
	if ( (state & ROUND_TOP_LEFT) == ROUND_TOP_LEFT ) {
		cairo_move_to( cr, x0, y0 + radius );
		cairo_curve_to( cr, x0 , y0, x0 , y0, x0 + radius, y0 );
	} else {
		cairo_move_to( cr, x0, y0 );
	}

	// top right
	if ( (state & ROUND_TOP_RIGHT) == ROUND_TOP_RIGHT ) {
		cairo_line_to( cr, x1 - radius, y0 );
		cairo_curve_to( cr, x1, y0, x1, y0, x1, y0 + radius );
	} else {
		cairo_line_to( cr, x1, y0 );
	}

	// bottom right
	if ( (state & ROUND_BOTTOM_RIGHT) == ROUND_BOTTOM_RIGHT ) {
		cairo_line_to( cr, x1 , y1 - radius );
		cairo_curve_to( cr, x1, y1, x1, y1, x1 - radius, y1 );
	} else {
		cairo_line_to( cr, x1, y1 );
	}

	// bottom left
	if ( (state & ROUND_BOTTOM_LEFT) == ROUND_BOTTOM_LEFT ) {
		cairo_line_to( cr, x0 + radius, y1 );
		cairo_curve_to( cr, x0, y1, x0, y1, x0, y1 - radius );
	} else {
		cairo_line_to( cr, x0, y1 );
	}

   	cairo_close_path (cr);
}


