/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
#include <gdk/gdk.h>

#include "awn-utils.h"

void
awn_utils_make_transparent_bg (GtkWidget *widget)
{
  static GdkPixmap *pixmap = NULL;

  g_return_if_fail (widget->window != NULL);

  if (gtk_widget_is_composited(widget))
  {
    if (pixmap == NULL)
    {
      pixmap = gdk_pixmap_new(widget->window, 1, 1, -1);
      cairo_t *cr = gdk_cairo_create(pixmap);
      cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(cr);
      cairo_destroy(cr);
    }
    gdk_window_set_back_pixmap(widget->window, pixmap, FALSE);
  }
}

static void
on_style_set (GtkWidget *widget, GtkStyle *prev_style)
{
  if (GTK_WIDGET_REALIZED (widget))
    awn_utils_make_transparent_bg (widget);
}

void
awn_utils_ensure_tranparent_bg (GtkWidget *widget)
{
  if (GTK_WIDGET_REALIZED (widget))
    awn_utils_ensure_tranparent_bg (widget);

  // make sure we don't connect the handler multiple times
  g_signal_handlers_disconnect_by_func (widget,
                    G_CALLBACK (awn_utils_make_transparent_bg), NULL);
  g_signal_handlers_disconnect_by_func (widget,
                    G_CALLBACK (on_style_set), NULL);

  g_signal_connect (widget, "realize",
                    G_CALLBACK (awn_utils_make_transparent_bg), NULL);

  g_signal_connect (widget, "style-set",
                    G_CALLBACK (on_style_set), NULL);
}

