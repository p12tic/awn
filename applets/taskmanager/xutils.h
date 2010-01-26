/* Xlib utilities */
/* vim: set sw=2 et: */

/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2005-2007 Vincent Untz
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
#include <X11/Xlib.h>
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

void
_wnck_get_wmclass (Window xwindow,
                   char **res_class,
                   char **res_name);

void
_wnck_get_client_name (Window xwindow, 
                    char **client_name);

GdkPixbuf *
_wnck_get_icon_at_size (WnckWindow *window,
                        gint        width,
                        gint        height);

GdkPixbuf *
xutils_get_named_icon (const gchar *icon_name, 
                       gint         width,
                       gint         height);

G_END_DECLS

#endif /* WNCK_XUTILS_H */
