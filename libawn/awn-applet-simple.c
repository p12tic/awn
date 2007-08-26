/*
 * Copyright (c) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "awn-applet.h"
#include "awn-applet-simple.h"

G_DEFINE_TYPE(AwnAppletSimple, awn_applet_simple, AWN_TYPE_APPLET)

#define AWN_APPLET_SIMPLE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
	AWN_TYPE_APPLET_SIMPLE, \
	AwnAppletSimplePrivate))

struct _AwnAppletSimplePrivate
{
        GdkPixbuf *icon;
        GdkPixbuf *reflect;
        gint icon_width;
        gint icon_height;

        gint offset;
        gint bar_height;
};


void
awn_applet_simple_set_icon (AwnAppletSimple *simple, GdkPixbuf *pixbuf)
{
       AwnAppletSimplePrivate *priv;
       GdkPixbuf *old0, *old1;

       g_return_if_fail (AWN_IS_APPLET_SIMPLE (simple));
       g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
       priv = simple->priv;

       old0 = priv->icon;
       old1 = priv->reflect;

       g_object_ref (pixbuf);
       priv->icon = pixbuf;
       priv->reflect = gdk_pixbuf_flip (pixbuf, FALSE);
       g_object_ref (priv->reflect);

       priv->icon_width = gdk_pixbuf_get_width (priv->icon);
       priv->icon_height = gdk_pixbuf_get_height (priv->icon);

       gtk_widget_set_size_request (GTK_WIDGET (simple), 
                                    priv->icon_width + 2, 
                                    ( priv->bar_height + 2 ) * 2);
       gtk_widget_queue_draw (GTK_WIDGET (simple));
}

static gboolean 
_expose_event(GtkWidget *widget, GdkEventExpose *expose) 
{
	AwnAppletSimplePrivate *priv;
        cairo_t *cr;
	gint width, height, bar_height;

	priv = AWN_APPLET_SIMPLE (widget)->priv;

	width = widget->allocation.width;
	height = widget->allocation.height;
        bar_height = priv->bar_height;

        cr = gdk_cairo_create (widget->window);

	/* task back */
	cairo_set_source_rgba (cr, 1, 0, 0, 0.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, -1, -1, width+1, height+1);
	cairo_fill (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);       
        
        
	/* content */
	if (priv->icon) {
		gint x1, y1;

		x1 = (width-priv->icon_width)/2;
		y1 = (height-bar_height) - priv->offset;

		gdk_cairo_set_source_pixbuf (cr, priv->icon, x1, y1);
		cairo_paint (cr);

		if (priv->offset > 0 && priv->reflect)
		{
                        y1 = (height-bar_height) - priv->offset 
                             + priv->icon_height + 1;
			gdk_cairo_set_source_pixbuf (cr, 
                                                     priv->reflect,
                                                     x1, y1);
			cairo_paint_with_alpha(cr, 0.3);
                        g_print ("%d %d\n", x1, y1);
		}
	}       

        /* Don't paint in bottom 3px if there is an bar_angle */
        if ( priv->offset != 0) {
                cairo_save (cr);
                cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
                cairo_set_source_rgba (cr, 1, 1, 1, 0);
                cairo_rectangle (cr, 
                                 0, 
                                 (height-3),
                                 width,
                                 3);
                cairo_fill (cr);
                cairo_restore (cr);
        }
        cairo_destroy (cr);

        return TRUE;
}

static void 
awn_applet_simple_class_init (AwnAppletSimpleClass *klass) 
{
        GtkWidgetClass *widget_class;

        widget_class = GTK_WIDGET_CLASS (klass);
        widget_class->expose_event = _expose_event;
        	
        g_type_class_add_private (G_OBJECT_CLASS (klass), 
                                  sizeof (AwnAppletSimplePrivate));
}

static void 
awn_applet_simple_init (AwnAppletSimple *simple) 
{
        AwnAppletSimplePrivate *priv;

        priv = simple->priv = AWN_APPLET_SIMPLE_GET_PRIVATE (simple);

        priv->icon = NULL;
        priv->reflect = NULL;
        priv->icon_height = 0;
        priv->icon_width = 0;
        priv->offset = 0;

        priv->offset = gconf_client_get_int (gconf_client_get_default (), 
                       "/apps/avant-window-navigator/bar/icon_offset",
                       NULL);
        priv->bar_height = gconf_client_get_int (gconf_client_get_default (), 
                       "/apps/avant-window-navigator/bar/bar_height",
                       NULL);
}

GtkWidget* 
awn_applet_simple_new (const gchar *uid, gint orient, gint height) 
{
	AwnAppletSimple *simple;
  
        simple = g_object_new (AWN_TYPE_APPLET_SIMPLE,
                               "uid", uid,
                               "orient", orient,
                               "height", height,
                               NULL);

        return GTK_WIDGET (simple);
}
