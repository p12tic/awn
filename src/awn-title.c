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

#include "awn-title.h"

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <gdk/gdkx.h>

G_DEFINE_TYPE (AwnTitle, awn_title, GTK_TYPE_WINDOW)

#define M_PI		3.14159265358979323846 

static gint AWN_TITLE_DEFAULT_WIDTH		= 1024;

static AwnSettings *settings = NULL; 


static void awn_title_destroy (GtkObject *object);
static void _on_alpha_screen_changed (GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel);
static gboolean _on_expose (GtkWidget *widget, GdkEventExpose *expose, AwnTitle *title);
static void _update_input_shape (GtkWidget* window, int width, int height);
static gboolean  _on_configure (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData);
static void _position_window (GtkWidget *window);


static void
awn_title_class_init( AwnTitleClass *this_class )
{
        GObjectClass *g_obj_class;
        GtkObjectClass *object_class;
        GtkWidgetClass *widget_class;
        
        g_obj_class = G_OBJECT_CLASS( this_class );
        object_class = (GtkObjectClass*) this_class;
        widget_class = GTK_WIDGET_CLASS( this_class );
        
        object_class->destroy =awn_title_destroy;
        //parent_class = gtk_type_class (gtk_widget_get_type ());
        
}

static void
awn_title_init( AwnTitle *title )
{

}

GtkWidget *
awn_title_new( AwnSettings *sets )
{
        settings = sets;
        AwnTitle *this = g_object_new(AWN_TITLE_TYPE, 
        			    "type", GTK_WINDOW_TOPLEVEL,
        			    "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
        			    NULL);
        _on_alpha_screen_changed (GTK_WIDGET(this), NULL, NULL);
        gtk_widget_set_app_paintable (GTK_WIDGET(this), TRUE);
        
        
        
        gtk_window_resize (GTK_WINDOW(this), settings->monitor.width, settings->bar_height + 2);
        
        g_signal_connect (G_OBJECT (this),"destroy",
			  G_CALLBACK (gtk_main_quit), NULL);
	
	g_signal_connect (G_OBJECT (this), "expose-event",
			  G_CALLBACK (_on_expose), (gpointer)this);
	
	
	g_signal_connect (G_OBJECT (this), "configure-event",
			  G_CALLBACK (_on_configure), NULL);       
        
#if GTK_CHECK_VERSION(2,9,0)
        _update_input_shape (GTK_WIDGET(this), AWN_TITLE_DEFAULT_WIDTH, settings->bar_height + 2);
#endif  
        
               
        _position_window(GTK_WIDGET(this));
        
        this->text = NULL;
        this->x_pos = 0;
        
        return GTK_WIDGET(this);
}

static void   
awn_title_destroy       (GtkObject   *object)
{
  g_print("Destroyed\n");
  g_return_if_fail(object != NULL);
  g_return_if_fail(IS_AWN_TITLE(object));

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
_rounded_rectangle (cairo_t *cr, double w, double h, double x, double y)
{
	const float RADIUS_CORNERS = 0.5;

	cairo_move_to (cr, x+RADIUS_CORNERS, y);
	cairo_line_to (cr, x+w-RADIUS_CORNERS, y);
	cairo_move_to (cr, w+x, y+RADIUS_CORNERS);
	cairo_line_to (cr, w+x, h+y-RADIUS_CORNERS);
	cairo_move_to (cr, w+x-RADIUS_CORNERS, h+y);
	cairo_line_to (cr, x+RADIUS_CORNERS, h+y);
	cairo_move_to (cr, x, h+y-RADIUS_CORNERS);
	cairo_line_to (cr, x, y+RADIUS_CORNERS);


}

static void
_rounded_corners (cairo_t *cr, double w, double h, double x, double y)
{
	double radius = 5;
	cairo_move_to (cr, x+radius, y);
	cairo_arc (cr, x+w-radius, y+radius, radius, M_PI * 1.5, M_PI * 2);
	cairo_arc (cr, x+w-radius, y+h-radius, radius, 0, M_PI * 0.5);
	cairo_arc (cr, x+radius,   y+h-radius, radius, M_PI * 0.5, M_PI);
	cairo_arc (cr, x+radius,   y+radius,   radius, M_PI, M_PI * 1.5);

}

static void 
render (cairo_t *cr, const char *utf8, gint width, gint height, gint x_pos)
{
   PangoFontDescription* pDesc = NULL;
   PangoLayout* pLayout = NULL;

	/* task back */
	cairo_set_source_rgba (cr, 1, 0, 0, 0.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, -1, -1, width+1, height+1);
	cairo_fill (cr);
	
	if (!utf8)
   		return;

	/* setup a new pango-layout based on the source-context */
	pLayout = pango_cairo_create_layout (cr);
	if (!pLayout)
	{
		g_print ("demo_textpath(): ");
		g_print ("Could not create pango-layout!\n");
		return;
	}

	/* get a new pango-description */
	pDesc = pango_font_description_new ();
	if (!pDesc)
	{
		g_print ("demo_textpath(): ");
		g_print ("Could not create pango-font-description!\n");
		g_object_unref (pLayout);
		return;
	}

	int font_slant = PANGO_STYLE_NORMAL;
	int font_weight = PANGO_WEIGHT_NORMAL;
	if (settings->italic)
		font_slant = PANGO_STYLE_ITALIC;
	if (settings->bold)
		font_weight = PANGO_WEIGHT_BOLD;
	

	pango_font_description_set_absolute_size (pDesc, PANGO_SCALE*settings->font_size);
	pango_font_description_set_family_static (pDesc, "Sans");
	pango_font_description_set_weight (pDesc, font_weight);
	pango_font_description_set_style (pDesc, font_slant);
	pango_layout_set_font_description (pLayout, pDesc);
	pango_font_description_free (pDesc);

	pango_layout_set_text(pLayout, utf8, -1);
	
	PangoRectangle extents;
	PangoRectangle logical_extents;

	double x,y;
	
	pango_layout_get_pixel_extents(pLayout, &logical_extents, &extents);
	x = (width/2)-(extents.width/2 + extents.x);
	y = (height/2)-(extents.height/2 + extents.y);
	
	x = x_pos - (extents.width/2)+ extents.x;
	x += (settings->corner_radius/2);
	
	if (x <0 )
		x = 0;
	

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* background */
	if (strlen (utf8) < 2)
		cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.0f);
	else 
		cairo_set_source_rgba (cr, settings->background.red, 
					   settings->background.green, 
					   settings->background.blue,
					   settings->background.alpha);

	cairo_translate (cr, 0.5, 0.5);
	
	_rounded_rectangle (cr, (double) extents.width+19, (double) extents.height+7, 
		       (double) x-9.5, (double) y-4.5 );
	
	_rounded_corners (cr, (double) extents.width+20, (double) extents.height+8, 
		       (double) x-9.5, (double) y-4.5);
	
	cairo_fill (cr);	      

	cairo_translate (cr, -0.5, -0.5);
	
	/* shadow */
	cairo_move_to (cr, x+1, y+1);
   //pango_cairo_layout_path(cr, pLayout);
	cairo_set_source_rgba (cr, settings->shadow_color.red, 
				   settings->shadow_color.green, 
				   settings->shadow_color.blue,
				   settings->shadow_color.alpha);
   pango_cairo_show_layout(cr, pLayout);

	
	cairo_move_to (cr, x, y);
	
	/* outline
	cairo_set_source_rgba (cr, 1, 1, 1, 1);
	cairo_text_path (cr, utf8);
	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, 1);
	cairo_set_source_rgba (cr, 0, 0, 0, 0.3);
	cairo_stroke (cr);
	 */
	/* text */
	//cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
	cairo_move_to (cr, x, y);
	cairo_set_source_rgba (cr, settings->text_color.red, 
				   settings->text_color.green, 
				   settings->text_color.blue,
				   settings->text_color.alpha);
   	pango_cairo_show_layout(cr, pLayout);
	
	/*
	cairo_text_path (cr, utf8);
	cairo_fill_preserve (cr);
	cairo_set_source_rgba (cr, settings->shadow_color.red, 
				   settings->shadow_color.green, 
				   settings->shadow_color.blue,
				   1.0);
	cairo_set_line_width (cr, 0.7);
	cairo_stroke (cr);	
	*/
	g_object_unref(pLayout);
}

static void 
render_pixmap (cairo_t *cr, gint width, gint height)
{
	
	cairo_scale (cr, (double) width, (double) height);
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	

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
_on_expose (GtkWidget *widget, GdkEventExpose *expose, AwnTitle *title)
{
	static gint oWidth;
	static gint oHeight;
	gint width;
	gint height;
	cairo_t *cr = NULL;

	if (!GDK_IS_DRAWABLE (widget->window))
		return FALSE;
	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;

	gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
	
	if (title->text)
		render (cr, title->text, width, height, title->x_pos);
	cairo_destroy (cr);
	
	_position_window(GTK_WIDGET(widget));
	oWidth = width;
	oHeight = height;
	
	//void awn_x_set_icon_geometry  (Window xwindow, int x, int y, int width, int height);
	
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
	y = (int) (settings->monitor.height - ((settings->bar_height +2)*2 + settings->icon_offset));
	
	gtk_window_move(GTK_WINDOW(window), x, y);	
}


void 
awn_title_resize(AwnTitle *title)
{
	gtk_window_resize(GTK_WINDOW(title), 1, ((settings->bar_height + 2) * 2));
}


static gint dest_x		= 0;

void 
awn_title_show (AwnTitle *title, const char *name, gint x, gint y)
{
	if (title->text) {
		g_free(title->text);
		title->text = NULL;
	}
	
	title->text = g_strdup(name);
	name = title->text;
	dest_x = x;
	
	if (strlen (name) > 1) {
		gtk_window_set_keep_above (GTK_WINDOW (title), TRUE);
	} else
		gtk_window_set_keep_above (GTK_WINDOW (title), FALSE);
	
	title->x_pos = x;
	_on_expose(GTK_WIDGET(title), NULL, title);
}

void 
awn_title_hide (AwnTitle *title)
{
	if (title->text) {
		g_free(title->text);
		title->text = NULL;
		_on_expose(GTK_WIDGET(title), NULL, title);
	}
	_on_expose(GTK_WIDGET(title), NULL, title);
	//gtk_widget_hide(GTK_WIDGET (title) );
}














