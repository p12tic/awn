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

#include "awn-bar.h"

#include "awn-x.h"

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>
#include <gdk/gdkx.h>
#include <math.h>
#define PI 3.14159265

G_DEFINE_TYPE (AwnBar, awn_bar, GTK_TYPE_WINDOW)

#define AWN_BAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        AWN_TYPE_BAR, AwnBarPrivate))

typedef struct {
        AwnSettings *settings;
        gint dest_width;
        gint current_width;
        
        GList *seps;

} AwnBarPrivate;

#define AWN_BAR_DEFAULT_WIDTH		1024

static AwnSettings *settings		= NULL;
static gint dest_width			= 0;
static gint current_width 		= 400;
static int separator			= 0;
static int draw_separator		= 0;
int bar_height = 0;
int icon_offset = 0;
static GtkWidgetClass *parent_class = NULL;

static void _on_alpha_screen_changed (GtkWidget* pWidget, GdkScreen* pOldScreen, GtkWidget* pLabel);

static void _update_input_shape (GtkWidget* window, int width, int height);
static gboolean  _on_configure (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData);
static void _position_window (GtkWidget *window);


static void
awn_bar_class_init( AwnBarClass *this_class )
{
        GObjectClass *g_obj_class;
        GtkObjectClass *object_class;
        GtkWidgetClass *widget_class;
        
        g_obj_class = G_OBJECT_CLASS( this_class );
        object_class = (GtkObjectClass*) this_class;
        widget_class = GTK_WIDGET_CLASS( this_class );


        parent_class = gtk_type_class (gtk_widget_get_type ());

	g_type_class_add_private (g_obj_class, sizeof (AwnBarPrivate));
}

static void
awn_bar_init( AwnBar *bar )
{
	AwnBarPrivate *priv;

        priv = AWN_BAR_GET_PRIVATE (bar);

        priv->seps = NULL;
}

GtkWidget *
awn_bar_new( AwnSettings *set )
{
        settings = set;
        AwnBar *this = g_object_new(AWN_TYPE_BAR, 
        			    "type", GTK_WINDOW_TOPLEVEL,
        			    "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
        			    NULL);
        _on_alpha_screen_changed (GTK_WIDGET(this), NULL, NULL);
        gtk_widget_set_app_paintable (GTK_WIDGET(this), TRUE);

        bar_height = set->bar_height;
        
        _position_window(GTK_WIDGET(this));
        
        g_signal_connect (G_OBJECT (this), "expose-event",
			  G_CALLBACK (_on_expose), NULL);
	
	g_signal_connect (G_OBJECT (this), "configure-event",
			  G_CALLBACK (_on_configure), NULL);       
        
#if GTK_CHECK_VERSION(2,9,0)
        _update_input_shape (GTK_WIDGET(this), AWN_BAR_DEFAULT_WIDTH,(bar_height+2)*2);
#endif        
        
        return GTK_WIDGET(this);
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
render_rect (cairo_t *cr, double x, double y, double width, double height, double offset  )
{
	if (settings->rounded_corners && settings->bar_angle == 0 ) {
		/* modified from cairo snippets page */
		double x0  = x ,  	
		y0	   = y ,
		rect_width  = width,
		rect_height = height,
		radius = settings->corner_radius + offset; 

		double x1,y1;

		x1=x0+rect_width;
		y1=y0+rect_height;
	
		cairo_move_to  (cr, x0, y0 + radius);
		cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
		cairo_line_to (cr, x1 - radius, y0);
		cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
		cairo_line_to (cr, x1 , y1 );
		cairo_line_to (cr, x0 , y1);
	
        	cairo_close_path (cr);
	}
	else
	{
		/* Let the bar 4px of the bottom => 3d look*/
		if( settings->bar_angle != 0 ) {
			y-=3;
		}

		double x0  = x,  	
		y0	   = y,
		x1	   = x+width,
		y1	   = y+height;

		cairo_move_to  (cr, x0 + apply_perspective_x(width, height/2, 0)    , y0 + apply_perspective_y( height ) );
		cairo_line_to  (cr, x0 + apply_perspective_x(width, height/2, width), y0 + apply_perspective_y( height ) );
		cairo_line_to  (cr, x0 + apply_perspective_x(width, 2*offset, width), y1-2*offset);
		cairo_line_to  (cr, x0 + apply_perspective_x(width, 2*offset, 0), y1-2*offset);

	       	cairo_close_path (cr);
	}

}

float
apply_perspective_x( double width, double height, double x )
{
	if( settings->bar_angle == 0 )
		return x;
	else
		return (width/2-x)/(width/2*tan((90-settings->bar_angle)*PI/180))*height+x;
}

float
apply_perspective_y( double height )
{
	if( settings->bar_angle == 0 )
		return 0;
	else
		return height/2;
}

static void
glass_engine (cairo_t *cr, double x, gint width, gint height, gint offset)
{
	cairo_pattern_t *pat;
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	/* main gradient */
	pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, height);
	cairo_pattern_add_color_stop_rgba (pat, 0.5, 
						settings->g_step_1.red, 
				   		settings->g_step_1.green, 
				   		settings->g_step_1.blue,
				   		settings->g_step_1.alpha);
	cairo_pattern_add_color_stop_rgba (pat, 1, 
						settings->g_step_2.red, 
				   		settings->g_step_2.green, 
				   		settings->g_step_2.blue,
				   		settings->g_step_2.alpha);
	render_rect (cr, x, height/2-offset, width, height/2+offset, 0);
	cairo_set_source(cr, pat);
	cairo_fill(cr);
	cairo_pattern_destroy(pat);
}

static void
pattern_engine (cairo_t *cr, double x, gint width, gint height, gint offset)
{
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_surface_t *image;
        cairo_pattern_t *pattern;
        
	//image = cairo_image_surface_create_from_png ("/usr/share/nautilus/patterns/terracotta.png");
        image = cairo_image_surface_create_from_png (settings->pattern_uri);
        pattern = cairo_pattern_create_for_surface (image);
        cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

        cairo_set_source (cr, pattern);
	render_rect  (cr, x, (height/2)-offset, width, (height/2)+offset, 0);
        
        cairo_save(cr);
        	cairo_clip(cr);
		cairo_paint_with_alpha(cr, settings->pattern_alpha);
        cairo_restore(cr);

        cairo_pattern_destroy (pattern);
        cairo_surface_destroy (image);
}

static void 
render (AwnBar *bar, cairo_t *cr, gint x_width, gint height)
{
	AwnBarPrivate *priv = AWN_BAR_GET_PRIVATE (bar);
        gint width = current_width;
        
        if( settings->bar_angle != 0 ) {
		icon_offset = 0;
		/* count extra space on left and right from bar */
		if(settings->icon_offset-3<=height/4){
			width += 2*apply_perspective_x(width, settings->icon_offset-3, 0);
		} else {
			width += 2*apply_perspective_x(width, height/4, 0);
		}
	} else {
		icon_offset = settings->icon_offset;
	}
       
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	
	double x = (settings->monitor.width-width)/2;
	
	cairo_move_to(cr, x, 0);
	cairo_set_line_width(cr, 1.0);
	
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	
	glass_engine(cr, x, width, height, icon_offset);
	
	if (settings->render_pattern)
		pattern_engine(cr, x, width, height, icon_offset);
	
	/* hilight gradient */
	cairo_pattern_t *pat;
	
	pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, height);
	cairo_pattern_add_color_stop_rgba (pat, 0.5,  
						settings->g_histep_1.red, 
				   		settings->g_histep_1.green, 
				   		settings->g_histep_1.blue,
				   		settings->g_histep_1.alpha);
	cairo_pattern_add_color_stop_rgba (pat, 1,   
						settings->g_histep_2.red, 
				   		settings->g_histep_2.green, 
				   		settings->g_histep_2.blue,
				   		settings->g_histep_2.alpha);
	
	if (settings->bar_angle == 0)
		render_rect (cr, x+1, height/2-icon_offset, width-2, height/5, 0);
	else {
		render_rect (cr, 
                             x+3+apply_perspective_x(width, 
                                                     bar_height/4, 
                                                     0),
                             height*7/10, 
                             width-6-2*apply_perspective_x(width, 
                                                           bar_height/4, 
                                                           0), 
                             height/5, 0);
        }
	cairo_set_source (cr, pat);
	cairo_fill (cr);

        if (settings->bar_angle != 0)
        {
                cairo_set_source_rgba (cr, settings->border_color.red,
                                           settings->border_color.green,
                                           settings->border_color.blue,
                                           settings->border_color.alpha+0.2);
                cairo_rectangle (cr, x, height-3, width, 3);
                cairo_fill (cr);
                
                /* Remove thetwo trailing triangles */
                //cairo_save (cr);
                //cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
                //cairo_set_source_rgba (cr, 1, 1, 1, 0);
                //cairo_rectangle (cr, x, height-3, 3, height-3);
                //cairo_fill (cr);
                //cairo_rectangle (cr, x+3+width-6, height-3, 3, height-3);
                //cairo_fill (cr);
                //cairo_restore (cr);
        }
        cairo_pattern_destroy (pat);
	
	/* internal hi-lightborder */
	cairo_set_source_rgba (cr, settings->hilight_color.red, 
				   settings->hilight_color.green, 
				   settings->hilight_color.blue,
				   settings->hilight_color.alpha);
	render_rect (cr, x+1.5, (height/2-icon_offset)+1.5, width-3, (height/2+icon_offset), 1);
	cairo_stroke(cr);
	
	/* border */
	cairo_set_source_rgba (cr, settings->border_color.red, 
				   settings->border_color.green, 
				   settings->border_color.blue,
				   settings->border_color.alpha);
	render_rect (cr, x+0.5, (height/2-icon_offset)+1, width-1, (height/2+icon_offset), 0);
	cairo_stroke(cr);

	/* separator */
	if (draw_separator && settings->show_separator) {
		double real_x = (settings->monitor.width-dest_width)/2.0;
		if (current_width > dest_width )
			real_x = x + current_width - dest_width;
		else
			real_x = x + dest_width - current_width;
		
		if (settings->bar_angle != 0)
		{
			if(settings->icon_offset-3<=height/4){
				real_x += apply_perspective_x(width, settings->icon_offset-3, 0);
			} else {
				real_x += apply_perspective_x(width, height/4, 0);
			}
                }
		cairo_set_line_width (cr, 1.0);
		
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, separator-2.5), bar_height- icon_offset+3 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator-2.5), (bar_height + 2) * 2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator-2.5), (bar_height + 2) * 2);
		cairo_set_source_rgba (cr, settings->hilight_color.red, 
				   	   settings->hilight_color.green, 
				           settings->hilight_color.blue,
				           settings->hilight_color.alpha);
		
		cairo_stroke(cr);
		
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, separator-1.5), bar_height- icon_offset+2 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator-1.5), (bar_height + 2) * 2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator-1.5), (bar_height + 2) * 2);
		cairo_set_source_rgba (cr, settings->border_color.red, 
				   	   settings->border_color.green, 
				           settings->border_color.blue,
				           settings->border_color.alpha);
		
		cairo_stroke(cr);
		
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, separator-0.5), bar_height- icon_offset+2 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator-0.5), (bar_height + 2) * 2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator-0.5), (bar_height + 2) * 2);
		cairo_set_source_rgba (cr, settings->sep_color.red, 
				   	   settings->sep_color.green, 
				           settings->sep_color.blue,
				           settings->sep_color.alpha);
		cairo_stroke(cr);
	

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, separator+0.5), bar_height- icon_offset+2 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator+0.5), (bar_height + 2) * 2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator-0.5), (bar_height + 2) * 2);
		cairo_set_source_rgba (cr, settings->border_color.red, 
				   	   settings->border_color.green, 
				           settings->border_color.blue,
				           settings->border_color.alpha);
		cairo_stroke(cr);

		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, separator+1.5), bar_height- icon_offset+3 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator+1.5), (bar_height + 2) * 2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3, separator+1.5), (bar_height + 2) * 2);
		cairo_set_source_rgba (cr, settings->hilight_color.red, 
				   	   settings->hilight_color.green, 
				           settings->hilight_color.blue,
				           settings->hilight_color.alpha);
		cairo_stroke(cr);
	}

        GList *s;
        for (s = priv->seps; s != NULL; s = s->next) {
                if (!GTK_IS_WIDGET (s->data))
                        continue;

                gint sep = (GTK_WIDGET (s->data))->allocation.x;
                sep += (GTK_WIDGET (s->data))->allocation.width/2;

                double real_x = (settings->monitor.width-dest_width)/2.0;
		if (current_width > dest_width )
			real_x = x + current_width - dest_width;
		else
			real_x = x + dest_width - current_width;
		
                if (settings->bar_angle != 0) {
			if(settings->icon_offset-3<=height/4){
				real_x += apply_perspective_x(width, settings->icon_offset-3, 0);
			} else {
				real_x += apply_perspective_x(width, height/4, 0);
			}
		}

		cairo_set_line_width (cr, 1.0);
		
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, sep-2.5), bar_height- icon_offset+3 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep-2.5), (settings->bar_height+2)*2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep-2.5), (settings->bar_height+2)*2);
		cairo_set_source_rgba (cr, settings->hilight_color.red, 
				   	   settings->hilight_color.green, 
				           settings->hilight_color.blue,
				           settings->hilight_color.alpha);
		
		cairo_stroke(cr);
		
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, sep-1.5), bar_height- icon_offset+2 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep-1.5), (settings->bar_height+2)*2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep-1.5), (settings->bar_height+2)*2);
		cairo_set_source_rgba (cr, settings->border_color.red, 
				   	   settings->border_color.green, 
				           settings->border_color.blue,
				           settings->border_color.alpha);
		
		cairo_stroke(cr);
		
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, sep-0.5), bar_height- icon_offset+2 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep-0.5), (settings->bar_height+2)*2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep-0.5), (settings->bar_height+2)*2);
		cairo_set_source_rgba (cr, settings->sep_color.red, 
				   	   settings->sep_color.green, 
				           settings->sep_color.blue,
				           settings->sep_color.alpha);
		cairo_stroke(cr);
	

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, sep+0.5), bar_height- icon_offset+2 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep+0.5), (settings->bar_height+2)*2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep+0.5), (settings->bar_height+2)*2);
		cairo_set_source_rgba (cr, settings->border_color.red, 
				   	   settings->border_color.green, 
				           settings->border_color.blue,
				           settings->border_color.alpha);
		cairo_stroke(cr);

		
		cairo_move_to (cr, real_x+apply_perspective_x(width, bar_height/2, sep+1.5), bar_height- icon_offset+3 + apply_perspective_y(bar_height));
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep+1.5), (settings->bar_height+2)*2-3);
		cairo_line_to (cr, real_x+apply_perspective_x(width, 3,sep+1.5), (settings->bar_height+2)*2);
		cairo_set_source_rgba (cr, settings->hilight_color.red, 
				   	   settings->hilight_color.green, 
				           settings->hilight_color.blue,
				           settings->hilight_color.alpha);
		cairo_stroke(cr);

        }
}

gboolean 
_on_expose (GtkWidget *widget, GdkEventExpose *expose)
{
	gint width;
	gint height;
	cairo_t *cr = NULL;

	if (!GDK_IS_DRAWABLE (widget->window))
		return FALSE;
	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;

	gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
	render (AWN_BAR (widget), cr, width, height);
	//render3 (cr, width, height);
	cairo_destroy (cr);
	
	_position_window(GTK_WIDGET(widget));
	return FALSE;
}

static void 
render_pixmap (cairo_t *cr, gint width, gint height)
{
	
	//cairo_scale (cr, (double) width, (double) height);
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	
	cairo_pattern_t *pat;
	
	cairo_set_line_width(cr, 0.05);
	pat = cairo_pattern_create_linear (0.0, 0.0, 0.0, 1.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.5, 1.0, 1.0, 1.0, 1);
	cairo_pattern_add_color_stop_rgba (pat, 1, 0.8, 0.8, 0.8, 1);
	cairo_rectangle (cr, -20, -20, 1, 1);
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
		if (pShapeBitmap)
			g_object_unref ((gpointer) pShapeBitmap);
	}
}

static gboolean 
_on_configure (GtkWidget* pWidget, GdkEventConfigure* pEvent, gpointer userData)
{
	gint iNewWidth = pEvent->width;
	gint iNewHeight = pEvent->height;

	/* if size has changed, update shape */
	if (1)
	{
		_update_input_shape (pWidget, iNewWidth, iNewHeight);

	}

	return FALSE;
}

static void
_position_window (GtkWidget *window)
{
	int ww, wh;
	int  x, y;
	
	gtk_window_get_size(GTK_WINDOW(window), &ww, &wh);
	
	y = settings->monitor.height - ((bar_height + 2) * 2);
	//x = (int) ( (settings->monitor.width - ww)/2);
	x = 0;
	if ( (settings->monitor.width) != ww) {
		gtk_window_resize(GTK_WINDOW(window), 
				  settings->monitor.width, 
				  ((bar_height+2) *2));
		gtk_window_move(GTK_WINDOW(window), x, y);
	}
	
       if (settings->panel_mode)
                awn_x_set_strut (GTK_WINDOW(window));
}



static gint resizing = 0;


static gint step     = 3;

static gboolean
resize( GtkWidget *window)
{
        if ( dest_width == current_width) {
                resizing = 0;
                return FALSE;
        }
        
        if ( dest_width > current_width) {
                if (dest_width - current_width < 2) {
               	        dest_width = current_width;
               	        resizing = 0;
               	        gtk_widget_queue_draw(GTK_WIDGET(window));
	                return FALSE;
        	}
                current_width += step;
             
        } else {
                if (current_width - dest_width < 2) {
               	        dest_width = current_width;
               	        resizing = 0;
               	        gtk_widget_queue_draw(GTK_WIDGET(window));
	                return FALSE;
        	}        	
        	current_width -= step;
        }
        gtk_widget_queue_draw(GTK_WIDGET(window));

        return TRUE;
}

void 
awn_bar_resize(GtkWidget *window, gint width)
{
	dest_width = width;        

        if (resizing) {
                return;
                             
        } else {
                resizing = 1;
                g_timeout_add(10, (GSourceFunc)resize, (gpointer)window);
        }
                
}

void 
awn_bar_set_separator_position (GtkWidget *window, int x)
{
	separator = x;
	gtk_widget_queue_draw(GTK_WIDGET(window));
	//g_print ("%d\n", x);
}


gboolean            
awn_bar_separator_expose_event  (GtkWidget      *widget,
                                 GdkEventExpose *event,
                                 GtkWidget 	*bar)
{
	separator = widget->allocation.x;
	gtk_widget_queue_draw(GTK_WIDGET(bar));
	
	return FALSE;
}                                 


void
awn_bar_set_draw_separator (GtkWidget *window, int x)
{
        draw_separator = (x ? 1 : 0);
        gtk_widget_queue_draw(GTK_WIDGET(window));
}

void
awn_bar_add_separator (AwnBar *bar, GtkWidget *widget)
{
        AwnBarPrivate *priv;

        g_return_if_fail (AWN_IS_BAR (bar));
        g_return_if_fail (GTK_IS_WIDGET (widget));

        priv = AWN_BAR_GET_PRIVATE (bar);

        priv->seps = g_list_append (priv->seps, (gpointer)widget);

        gtk_widget_queue_draw (GTK_WIDGET (bar));
}








