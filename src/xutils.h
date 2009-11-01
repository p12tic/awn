/* Xlib utilities */

/*
 * Copyright (C) 2001 Havoc Pennington
 *               2009 Michal Hruby <michal.mhr@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef WNCK_XUTILS_H
#define WNCK_XUTILS_H

#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

void xutils_set_strut (GdkWindow        *gdk_window,
                       GtkPositionType   position,
                       guint32           strut,
                       guint32           strut_start,
                       guint32           strut_end);

GdkRegion* xutils_get_input_shape (GdkWindow *window);

GdkWindow* xutils_get_window_at_pointer (GdkDisplay *gdk_display);

gboolean   xutils_is_window_minimized (GdkWindow *window);

G_END_DECLS

#endif /* WNCK_XUTILS_H */
