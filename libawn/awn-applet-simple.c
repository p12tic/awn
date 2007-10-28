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
        GdkPixbuf *org_icon;
        GdkPixbuf *icon;
        GdkPixbuf *reflect;

	AwnEffects effects;

        gint icon_width;
        gint icon_height;

	gint bar_height_on_icon_recieved;

        gint offset;
        gint bar_height;
        gint bar_angle;

};

static void 
adjust_icon(AwnAppletSimple *simple)
{
        AwnAppletSimplePrivate *priv;
        GdkPixbuf *old0, *old1;
	int refcount = 0;
     
        priv = simple->priv;

        old0 = priv->icon;
        old1 = priv->reflect;

	if( priv->bar_height == priv->bar_height_on_icon_recieved )
	{
		priv->icon_height = gdk_pixbuf_get_height (priv->org_icon);
		priv->icon_width = gdk_pixbuf_get_width (priv->org_icon);
		priv->icon = gdk_pixbuf_copy (priv->org_icon);
	}
	else
	{
		priv->icon_height = gdk_pixbuf_get_height (priv->org_icon)+priv->bar_height-priv->bar_height_on_icon_recieved;
        	priv->icon_width = (int)((double)priv->icon_height/(double)gdk_pixbuf_get_height (priv->org_icon)*(double)gdk_pixbuf_get_width (priv->org_icon));        
	        priv->icon = gdk_pixbuf_scale_simple(priv->org_icon,
	                                             priv->icon_width,
	                                             priv->icon_height,
	                                             GDK_INTERP_BILINEAR);
        }

	g_object_ref (priv->icon);
        priv->reflect = gdk_pixbuf_flip (priv->icon, FALSE);
        g_object_ref (priv->reflect);
        
        if (old0) {
		for (refcount = (G_OBJECT (old0))->ref_count; refcount > 0; refcount--) {
			g_object_unref (old0);
		}
        }
        if (old1) {
		for (refcount = (G_OBJECT (old1))->ref_count; refcount > 0; refcount--) {
			g_object_unref (old1);
		}
        }
	/* for some reason priv->reflect is not always a valid pixbuf.
	   my suspicion is that we are seeing a gdk bug here.  So...if
	   priv->reflect isn't a good pixbuf, well... let's try making
	   one from priv->org_icon */
	if (!GDK_IS_PIXBUF (priv->reflect)) {
		priv->reflect = gdk_pixbuf_flip (priv->org_icon, FALSE);
	}

	// awn-effects require the window to be 25% bigger than icon
        gtk_widget_set_size_request (GTK_WIDGET (simple), 
                                     priv->icon_width *5/4,
                                     (priv->bar_height + 2 ) * 2);
        gtk_widget_queue_draw (GTK_WIDGET (simple));
}

void
awn_applet_simple_set_icon (AwnAppletSimple *simple, GdkPixbuf *pixbuf)
{
	g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
        
	/* awn_applet_simple_set_icon is not heavily used.
	   Previous inplementation was causing nasty leaks.
	   This fix seems sensible, easy to maintain.
	   And it works.  Note we are making a copy here so
	   the unref in set_temp_icon leaves the user's original
	   untouched.
	 */
	awn_applet_simple_set_temp_icon (simple, gdk_pixbuf_copy (pixbuf));
}

void 
awn_applet_simple_set_temp_icon (AwnAppletSimple *simple, GdkPixbuf *pixbuf)
{
        AwnAppletSimplePrivate *priv;
        GdkPixbuf *old0;
	int refcount;

        g_return_if_fail (AWN_IS_APPLET_SIMPLE (simple));
        g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
        
        priv = simple->priv;

	/* let's make sure that an applet can't screw around with OUR
	   pixbuf.  We'll make our own copy, and free up theirs.
	 */
	old0 = pixbuf;
	pixbuf = gdk_pixbuf_copy (pixbuf);
	g_object_unref (old0);

        old0 = priv->org_icon;
        priv->org_icon = pixbuf;
        priv->bar_height_on_icon_recieved = priv->bar_height;

        if (old0) {
		for (refcount = (G_OBJECT (old0))->ref_count; refcount > 0; refcount--) {
			g_object_unref (old0);
		}
        }

        adjust_icon(simple);
}

static gboolean 
_expose_event(GtkWidget *widget, GdkEventExpose *expose) 
{
	AwnAppletSimplePrivate *priv;
        cairo_t *cr;
	gint width, height, bar_height;

	priv = AWN_APPLET_SIMPLE (widget)->priv;

	/* For some reason, priv->reflect is not always a valid pixbuf.
	   my suspicion is that we are seeing a gdk bug here. So... if
	   priv->reflect isn't a good pixbuf well.. let's try making one from
	   priv->org_icon.  I'm not happy as I'm not exactly sure of the root
	   cause of this...  but this does resolve the issue */
	if (!GDK_IS_PIXBUF (priv->reflect)) {
		priv->reflect = gdk_pixbuf_flip (priv->icon, FALSE);
	}
	if (!GDK_IS_PIXBUF (priv->reflect)) {
		priv->reflect = gdk_pixbuf_flip (priv->org_icon, FALSE);
	}

	width = widget->allocation.width;
	height = widget->allocation.height;

	awn_draw_set_window_size(&priv->effects, width, height);

        bar_height = priv->bar_height;

        cr = gdk_cairo_create (widget->window);

	/* task back */
	cairo_set_source_rgba (cr, 1, 0, 0, 0.0);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_rectangle(cr, -1, -1, width+1, height+1);
	cairo_fill (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);       
        
	/* content */
	awn_draw_background(&priv->effects, cr);
	awn_draw_icons(&priv->effects, cr, priv->icon, priv->reflect);
	awn_draw_foreground(&priv->effects, cr);

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
bar_height_changed( GConfClient *client, guint cid, GConfEntry *entry, AwnAppletSimple *simple )
{
        AwnAppletSimplePrivate *priv;
	GConfValue *value = NULL;

	priv = simple->priv;
	value = gconf_entry_get_value(entry);
	priv->bar_height = gconf_value_get_int(value);
	
	adjust_icon(simple);
	
	g_print("bar_height changed\n");
}
static void
icon_offset_changed( GConfClient *client, guint cid, GConfEntry *entry, AwnAppletSimple *simple )
{
        AwnAppletSimplePrivate *priv;
	GConfValue *value = NULL;

	priv = simple->priv;
	value = gconf_entry_get_value(entry);
	priv->offset = gconf_value_get_int(value);
	
	g_print("icon_offset changed\n");
}
static void
bar_angle_changed( GConfClient *client, guint cid, GConfEntry *entry, AwnAppletSimple *simple )
{
        AwnAppletSimplePrivate *priv;
	GConfValue *value = NULL;

	priv = simple->priv;
	value = gconf_entry_get_value(entry);
	priv->bar_angle = gconf_value_get_int(value);
	
        gtk_widget_queue_draw (GTK_WIDGET (simple));
	
	g_print("bar_angle changed\n");
}

static void 
awn_applet_simple_init (AwnAppletSimple *simple) 
{
        AwnAppletSimplePrivate *priv;
        GConfClient *client;

        priv = simple->priv = AWN_APPLET_SIMPLE_GET_PRIVATE (simple);

        priv->icon = NULL;
        priv->org_icon = NULL;
        priv->reflect = NULL;
        priv->icon_height = 0;
        priv->icon_width = 0;
        priv->offset = 0;

	awn_effects_init(G_OBJECT(simple), &priv->effects);
	// register hover effects
	awn_register_effects(G_OBJECT(simple), &priv->effects);
	// start open effect
	awn_effect_start_ex(&priv->effects, AWN_EFFECT_OPENING, 0, 0, 1);

        client = gconf_client_get_default ();
        gconf_client_add_dir(client, "/apps/avant-window-navigator/bar", GCONF_CLIENT_PRELOAD_NONE, NULL);
        priv->offset = gconf_client_get_int (client, 
                       "/apps/avant-window-navigator/bar/icon_offset",
                       NULL);
        priv->bar_height = gconf_client_get_int (client, 
                       "/apps/avant-window-navigator/bar/bar_height",
                       NULL);
        priv->bar_angle = gconf_client_get_int (client, 
                       "/apps/avant-window-navigator/bar/bar_angle",
                       NULL);
        gconf_client_notify_add (client, "/apps/avant-window-navigator/bar/bar_angle", (GConfClientNotifyFunc)bar_angle_changed, simple, NULL, NULL);
        gconf_client_notify_add (client, "/apps/avant-window-navigator/bar/bar_height", (GConfClientNotifyFunc)bar_height_changed, simple, NULL, NULL);
        gconf_client_notify_add (client, "/apps/avant-window-navigator/bar/icon_offset", (GConfClientNotifyFunc)icon_offset_changed, simple, NULL, NULL);
}

AwnEffects*
awn_applet_simple_get_effects(AwnAppletSimple *simple)
{
        AwnAppletSimplePrivate *priv = simple->priv;
	return &priv->effects;
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
