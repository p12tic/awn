/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  Notes : Thanks to MacSlow (macslow.thepimp.net) for the transparent & shaped
 *	    window code.
*/

#include "config.h"

#include "awn-window.h"
#include "awn-applet-manager.h"


#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <gdk/gdkx.h>

#if !GTK_CHECK_VERSION(2,14,0)
#define g_timeout_add_seconds(seconds, callback, user_data) g_timeout_add(seconds * 1000, callback, user_data)
#endif

#include "awn-x.h"


G_DEFINE_TYPE (AwnWindow, awn_window, GTK_TYPE_WINDOW) 

#define AWN_WINDOW_DEFAULT_WIDTH		300

static AwnSettings *settings		= NULL;
static gboolean stop_position = TRUE;
static gboolean is_positioning = FALSE;
static gint x_pos = 0;
static gint y_pos = 0;
static gint current_pos = 0;

static const GtkTargetEntry drop_types[] = {
  { "STRING", 0, 0 },
  { "text/plain", 0, 0},
  { "text/uri-list", 0, 0}
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);




static void _update_input_shape (GtkWidget* window, int width, int height);
static gboolean  _on_configure (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData);
static gboolean _on_expose (GtkWidget *widget, GdkEventExpose *expose);


static void
awn_window_class_init( AwnWindowClass *this_class )
{
        GObjectClass *g_obj_class;
        GtkObjectClass *object_class;
        GtkWidgetClass *widget_class;
        
        g_obj_class = G_OBJECT_CLASS( this_class );
        object_class = (GtkObjectClass*) this_class;
        widget_class = GTK_WIDGET_CLASS( this_class );
        
}

static void
awn_window_init( AwnWindow *window )
{
	gtk_widget_add_events (GTK_WIDGET (window),GDK_ALL_EVENTS_MASK);
	
	gtk_drag_dest_set (GTK_WIDGET (window),
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                           drop_types, n_drop_types,
                           GDK_ACTION_MOVE | GDK_ACTION_COPY);
	
	
	gtk_drag_dest_add_uri_targets (GTK_WIDGET (window));
	awn_x_set_strut (GTK_WINDOW(window));
}

static void
_position_timeout (gpointer null)
{
	stop_position = FALSE;
}


void 
awn_window_force_repos ()
{
	stop_position = TRUE;
	g_timeout_add_seconds(5, (GSourceFunc)_position_timeout, NULL);
}

GtkWidget *
awn_window_new( AwnSettings *set )
{
        static gboolean type_registered = FALSE;
        settings = set;
        AwnWindow *this = g_object_new(AWN_WINDOW_TYPE, 
        			    "type", GTK_WINDOW_TOPLEVEL,
        			    "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
        			    NULL);

        if (!type_registered) {
                wnck_set_client_type (WNCK_CLIENT_TYPE_PAGER);
                type_registered = TRUE;
        }

	gtk_window_set_title(GTK_WINDOW(this), "awn_elements");
        _on_alpha_screen_changed (GTK_WIDGET(this), NULL, NULL);
        gtk_widget_set_app_paintable (GTK_WIDGET(this), TRUE);
        gtk_window_resize (GTK_WINDOW(this), AWN_WINDOW_DEFAULT_WIDTH, (set->bar_height + 2) * 2);
        
        g_signal_connect (G_OBJECT (this), "expose-event",
			  G_CALLBACK (_on_expose), NULL);
	
	g_signal_connect (G_OBJECT (this), "configure-event",
			  G_CALLBACK (_on_configure), NULL); 

	g_signal_connect (G_OBJECT (this), "screen-changed",
			  G_CALLBACK (_on_alpha_screen_changed), NULL);       
        
#if GTK_CHECK_VERSION(2,9,0)
        _update_input_shape (GTK_WIDGET(this), AWN_WINDOW_DEFAULT_WIDTH, (set->bar_height + 2) * 2);
#endif  
        
      	g_timeout_add_seconds(5, (GSourceFunc)_position_timeout, NULL);
        return GTK_WIDGET(this);
}


void 
_on_alpha_screen_changed (GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel)
{                       
	GdkScreen* pScreen = gtk_widget_get_screen (pWidget);
	GdkColormap* pColormap = gdk_screen_get_rgba_colormap (pScreen);
      
	if (!pColormap)
		pColormap = gdk_screen_get_rgb_colormap (pScreen);

	gtk_widget_set_colormap (pWidget, pColormap);
}

static void 
render (cairo_t *cr, gint width, gint height)
{
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	return;
}

static gboolean 
_on_expose (GtkWidget *widget, GdkEventExpose *expose)
{
	static gint oWidth = 0;
	static gint oHeight = 0;
	gint width;
	gint height;
	cairo_t *cr = NULL;
	if (!GDK_IS_DRAWABLE (widget->window))
		return FALSE;
	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;

	gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
	render (cr, width, height);
	//render3 (cr, width, height);
	cairo_destroy (cr);
	
	if ( oWidth != width || oHeight != height)
		_position_window(GTK_WIDGET(widget));
	oWidth = width;
	oHeight = height;
	
	gdk_window_raise (widget->window);
	return FALSE;
}

static void 
render_pixmap (cairo_t *cr, gint width, gint height)
{
	double rel_height = (double)(settings->bar_height+settings->icon_offset)/(double)height;
	
	cairo_scale (cr, (double) width, (double) height);
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	
	cairo_pattern_t *pat;
	
	cairo_set_line_width(cr, 0.05);
	pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.5, 1.0, 1.0, 1.0, 1);
	cairo_pattern_add_color_stop_rgba (pat, 1, 0.8, 0.8, 0.8, 1);
	cairo_rectangle (cr, 0, 1.0-rel_height, 1, 1);
	cairo_set_source(cr, pat);
	cairo_fill(cr);
	cairo_pattern_destroy(pat);
	
	
}

#if !GTK_CHECK_VERSION(2,9,0)
/* this is piece by piece taken from gtk+ 2.9.0 (CVS-head with a patch applied
regarding XShape's input-masks) so people without gtk+ >= 2.9.0 can compile and
run input_shape_test.c */
static void 
do_shape_combine_mask (GdkWindow* window, GdkBitmap* mask, gint x, gint y)
{
	Pixmap pixmap;
	int ignore;
	int maj;
	int min;

	if (!XShapeQueryExtension (GDK_WINDOW_XDISPLAY (window), &ignore, &ignore))
		return;

	if (!XShapeQueryVersion (GDK_WINDOW_XDISPLAY (window), &maj, &min))
		return;

	/* for shaped input we need at least XShape 1.1 */
	if (maj != 1 && min < 1)
		return;

	if (mask)
		pixmap = GDK_DRAWABLE_XID (mask);
	else
	{
		x = 0;
		y = 0;
		pixmap = None;
	}

	XShapeCombineMask (GDK_WINDOW_XDISPLAY (window),
					   GDK_DRAWABLE_XID (window),
					   ShapeInput,
					   x,
					   y,
					   pixmap,
					   ShapeSet);
}
#endif

static void 
_update_input_shape (GtkWidget* window, int width, int height)
{
	static GdkBitmap* pShapeBitmap = NULL;
	static cairo_t* pCairoContext = NULL;
	g_return_if_fail (GTK_IS_WINDOW (window));
	
	int curve_extra = 0;
	if(settings->bar_angle < 0){
		curve_extra = height / 3;
	}

	pShapeBitmap = (GdkBitmap*) gdk_pixmap_new (NULL, width, height+curve_extra, 1);

	if (pShapeBitmap)
	{
		pCairoContext = gdk_cairo_create (pShapeBitmap);
		if (cairo_status (pCairoContext) == CAIRO_STATUS_SUCCESS)
		{
			render_pixmap (pCairoContext, width, height);
			cairo_destroy (pCairoContext);


#if !GTK_CHECK_VERSION(2,9,0)
			do_shape_combine_mask (window->window, NULL, 0, 0-curve_extra);
			do_shape_combine_mask (window->window, pShapeBitmap, 0, 0-curve_extra);
#else
			gtk_widget_input_shape_combine_mask (window, NULL, 0, 0-curve_extra);
			gtk_widget_input_shape_combine_mask (window, pShapeBitmap, 0, 0-curve_extra);
#endif
		}

		if (pShapeBitmap)
			g_object_unref ((gpointer) pShapeBitmap);

	}
}

static gboolean 
_on_configure (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData)
{
	gint iNewWidth = pEvent->width;
	gint iNewHeight = pEvent->height;

	if (1)
	{
		_update_input_shape (pWidget, iNewWidth, iNewHeight);

	}

	return FALSE;
}





static gboolean
_position_func (GtkWidget *window)
{
	if (x_pos == current_pos) {
		is_positioning = FALSE;
		on_awn_applet_manager_size_allocate(window, NULL, AWN_APPLET_MANAGER(settings->appman) );
		return FALSE;
	} else if (x_pos > current_pos) {
		current_pos++;
	} else {
		current_pos--;
	}
//	on_awn_applet_manager_size_allocate(window, NULL, AWN_APPLET_MANAGER(settings->appman) );
	gtk_window_move(GTK_WINDOW(window), current_pos, y_pos);
	return TRUE;
}

void
_position_window (GtkWidget *window)
{
	gint ww, wh;
	gtk_window_get_size(GTK_WINDOW(window), &ww, &wh);
	
	x_pos = (int) ((settings->monitor.width - ww) * settings->bar_pos);
	
	if (settings->hidden && !settings->keep_below)
		y_pos = (int)settings->monitor.height;
	else
		//if(settings->bar_angle == 0)
			y_pos = (int) (settings->monitor.height-wh);
		//else
			//y_pos = (int) (settings->monitor.height-wh*9/8+2);
	
	if (stop_position || settings->no_bar_resize_ani) {
		current_pos = x_pos;
		gtk_window_move(GTK_WINDOW(window), x_pos, y_pos);
		return;
	}
	if (!is_positioning) {
		g_timeout_add(20, (GSourceFunc)_position_func, (gpointer)window);
		is_positioning = TRUE;
	}
	//gtk_window_move(GTK_WINDOW(window), x, y);
}

















