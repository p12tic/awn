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

#include "awn-hotspot.h"

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <gdk/gdkx.h>

G_DEFINE_TYPE (AwnHotspot, awn_hotspot, GTK_TYPE_WINDOW)

#define M_PI		3.14159265358979323846  
#define PADDING		20

static gint AWN_HOTSPOT_DEFAULT_WIDTH		= 1024;
static gint AWN_HOTSPOT_DEFAULT_HEIGHT		= 3;

static AwnSettings *settings = NULL;
static const GtkTargetEntry drop_types[] = {
  { "STRING", 0, 0 },
  { "text/plain", 0, 0},
  { "text/uri-list", 0, 0}
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);

static void awn_hotspot_destroy (GtkObject *object);
static void _on_alpha_screen_changed (GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel);
static gboolean _on_expose (GtkWidget *widget, GdkEventExpose *expose, AwnHotspot *hotspot);
static void _update_input_shape (GtkWidget* window, int width, int height);
static gboolean  _on_configure (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData);
static void _position_window (GtkWidget *window);


static void
awn_hotspot_class_init( AwnHotspotClass *this_class )
{
        GObjectClass *g_obj_class;
        GtkObjectClass *object_class;
        GtkWidgetClass *widget_class;
        
        g_obj_class = G_OBJECT_CLASS( this_class );
        object_class = (GtkObjectClass*) this_class;
        widget_class = GTK_WIDGET_CLASS( this_class );
        
        object_class->destroy =awn_hotspot_destroy;
        //parent_class = gtk_type_class (gtk_widget_get_type ());
        
}

static void
awn_hotspot_init( AwnHotspot *hotspot )
{
	gtk_widget_add_events (GTK_WIDGET (hotspot),GDK_ALL_EVENTS_MASK);
	
	gtk_drag_dest_set (GTK_WIDGET (hotspot),
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                           drop_types, n_drop_types,
                           GDK_ACTION_MOVE | GDK_ACTION_COPY);
	
	
	gtk_drag_dest_add_uri_targets (GTK_WIDGET (hotspot));
}

GtkWidget *
awn_hotspot_new( AwnSettings *sets )
{
        settings = sets;
        AwnHotspot *this = g_object_new(AWN_HOTSPOT_TYPE, 
        			    "type", GTK_WINDOW_TOPLEVEL,
        			    "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
        			    NULL);
        _on_alpha_screen_changed (GTK_WIDGET(this), NULL, NULL);
        gtk_widget_set_app_paintable (GTK_WIDGET(this), TRUE);
        
        
        
        if (settings->monitor.width > 0)
		gtk_window_resize (GTK_WINDOW(this), 
        		   settings->monitor.width - (2*PADDING),
        		   AWN_HOTSPOT_DEFAULT_HEIGHT);
        
        g_signal_connect (G_OBJECT (this),"destroy",
			  G_CALLBACK (gtk_main_quit), NULL);
	
	g_signal_connect (G_OBJECT (this), "expose-event",
			  G_CALLBACK (_on_expose), (gpointer)this);
	
	
	g_signal_connect (G_OBJECT (this), "configure-event",
			  G_CALLBACK (_on_configure), NULL);       
        
#if GTK_CHECK_VERSION(2,9,0)
        _update_input_shape (GTK_WIDGET(this), AWN_HOTSPOT_DEFAULT_WIDTH, AWN_HOTSPOT_DEFAULT_HEIGHT);
#endif  
        
               
        _position_window(GTK_WIDGET(this));
        
        return GTK_WIDGET(this);
}

static void   
awn_hotspot_destroy       (GtkObject   *object)
{
  g_print("Destroyed\n");
  g_return_if_fail(object != NULL);
  g_return_if_fail(IS_AWN_HOTSPOT(object));

}

static void 
_on_alpha_screen_changed (GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel)
{                       
	GdkScreen* pScreen = gtk_widget_get_screen (pWidget);
	GdkColormap* pColormap = gdk_screen_get_rgba_colormap (pScreen);
      
	if (!pColormap)
		pColormap = gdk_screen_get_rgb_colormap (pScreen);

	gtk_widget_set_colormap (pWidget, pColormap);
}

static void 
render (cairo_t *cr)
{
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
}

static void 
render_pixmap (cairo_t *cr, gint width, gint height)
{
	
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 30, 0, width-30, height);
	cairo_fill (cr);
	

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

	pShapeBitmap = (GdkBitmap*) gdk_pixmap_new (NULL, width, height, 1);
	if (pShapeBitmap)
	{
		pCairoContext = gdk_cairo_create (pShapeBitmap);
		if (cairo_status (pCairoContext) == CAIRO_STATUS_SUCCESS)
		{
			render_pixmap (pCairoContext, width, height);
			cairo_destroy (pCairoContext);
			
#if !GTK_CHECK_VERSION(2,9,0)
			do_shape_combine_mask (window->window, NULL, 0, 0);
			do_shape_combine_mask (window->window, pShapeBitmap, 0, 0);
#else
			gtk_widget_input_shape_combine_mask (window, NULL, 0, 0);
			gtk_widget_input_shape_combine_mask (window, pShapeBitmap, 0, 0);
#endif
		}
		g_object_unref ((gpointer) pShapeBitmap);
	}
}

static gboolean 
_on_expose (GtkWidget *widget, GdkEventExpose *expose, AwnHotspot *hotspot)
{
	cairo_t *cr = NULL;

	if (!GDK_IS_DRAWABLE (widget->window))
		return FALSE;	
	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;

	render (cr);
	cairo_destroy (cr);
	
	_position_window(GTK_WIDGET(widget));
	return FALSE;
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

static void
_position_window (GtkWidget *window)
{
	gint ww, wh;
	gint x, y;
	
	gtk_window_get_size(GTK_WINDOW(window), &ww, &wh);
	
	x = (int) ((settings->monitor.width - ww) / 2);
	y = (int) (settings->monitor.height-AWN_HOTSPOT_DEFAULT_HEIGHT);
	
	gtk_window_move(GTK_WINDOW(window), x, y);
}















