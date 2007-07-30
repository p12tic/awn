/*
 * Copyright (c) 2007 Neil Jagdish Patel
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

/* This is one of the most basic applets you can make, it just embeds the 
   WnckPager into the applet container*/ 

#include "config.h"

#include <gtk/gtk.h>
#include <libawn/awn-applet.h>

static gboolean
expose (GtkWidget *widget, GdkEventExpose *event, gpointer null)
{
	cairo_t *cr = NULL;

	if (!GDK_IS_DRAWABLE (widget->window))
		return FALSE;	
	
	cr = gdk_cairo_create (widget->window);
	if (!cr)
		return FALSE;
	
	/* Clear the background to transparent */
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	
	/* Clean up */
	cairo_destroy (cr);

  return TRUE;
}

gboolean
awn_applet_factory_init ( AwnApplet *applet )
{
  g_signal_connect (G_OBJECT (applet), "expose-event", G_CALLBACK (expose), NULL);

  guint height = awn_applet_get_height( applet );
  gtk_widget_set_size_request (GTK_WIDGET (applet), 5, height * 2);
  return TRUE;
}

