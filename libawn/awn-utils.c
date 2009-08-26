/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
 *  Copyright (C) 2009 Mark Lee <avant-wn@lazymalevolence.com>
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
 *
 * Authors: Michal Hruby (awn_utils_ensure_transparent_bg,
 *                        awn_utils_make_transparent_bg)
 *          Mark Lee (awn_utils_gslist_to_gvaluearray)
 */
#include <math.h>
#include <gdk/gdk.h>

#include "awn-utils.h"
#include "gseal-transition.h"

void
awn_utils_make_transparent_bg (GtkWidget *widget)
{
  static GdkPixmap *pixmap = NULL;
  GdkWindow *win;

  win = gtk_widget_get_window (widget);
  g_return_if_fail (win != NULL);

  if (gtk_widget_is_composited(widget))
  {
    if (pixmap == NULL)
    {
      pixmap = gdk_pixmap_new (win, 1, 1, -1);
      cairo_t *cr = gdk_cairo_create(pixmap);
      cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(cr);
      cairo_destroy(cr);
    }
    gdk_window_set_back_pixmap (win, pixmap, FALSE);
  }
}

static void
on_style_set (GtkWidget *widget, GtkStyle *prev_style)
{
  if (GTK_WIDGET_REALIZED (widget))
    awn_utils_make_transparent_bg (widget);
}

static void
on_composited_change (GtkWidget *widget, gpointer data)
{
  if (gtk_widget_is_composited (widget))
  {
    awn_utils_make_transparent_bg (widget);
  }
  else
  {
    // use standard background
    gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, NULL);
  }
}

void
awn_utils_ensure_transparent_bg (GtkWidget *widget)
{
  if (GTK_WIDGET_REALIZED (widget))
    awn_utils_make_transparent_bg (widget);

  // make sure we don't connect the handler multiple times
  g_signal_handlers_disconnect_by_func (widget,
                    G_CALLBACK (awn_utils_make_transparent_bg), NULL);
  g_signal_handlers_disconnect_by_func (widget,
                    G_CALLBACK (on_style_set), NULL);
  g_signal_handlers_disconnect_by_func (widget,
                    G_CALLBACK (on_composited_change), NULL);

  g_signal_connect (widget, "realize",
                    G_CALLBACK (awn_utils_make_transparent_bg), NULL);
  g_signal_connect (widget, "style-set",
                    G_CALLBACK (on_style_set), NULL);
  g_signal_connect (widget, "composited-changed",
                    G_CALLBACK (on_composited_change), NULL);
}

GValueArray*
awn_utils_gslist_to_gvaluearray (GSList *list)
{
  GValueArray *array;

  array = g_value_array_new (g_slist_length (list));
  for (GSList *n = list; n != NULL; n = n->next)
  {
    GValue val = {0};

    g_value_init (&val, G_TYPE_STRING);
    g_value_set_string (&val, (gchar*)n->data);
    g_value_array_append (array, &val);
    g_value_unset (&val);
  }

  return array;
}

gfloat
awn_utils_get_offset_modifier_by_path_type (AwnPathType path_type,
                                            GtkPositionType position,
                                            gfloat offset_modifier,
                                            gint pos_x, gint pos_y,
                                            gint width, gint height)
{
  gfloat result;

  if (width == 0 || height == 0) return 1.0f;

  switch (path_type)
  {
    case AWN_PATH_ELLIPSE:
      switch (position)
      {
        case GTK_POS_LEFT:
        case GTK_POS_RIGHT:
          result = sinf (M_PI * pos_y / height);
          return result * result * offset_modifier + 1.0f;
        default:
          result = sinf (M_PI * pos_x / width);
          return result * result * offset_modifier + 1.0f;
      }
      break;
    default:
      return 1.0f;
  }
}

