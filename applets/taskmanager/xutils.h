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
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

typedef struct _WnckIconCache WnckIconCache;

void
_wnck_error_trap_push (void);

int
_wnck_error_trap_pop (void);

void
_wnck_get_wmclass (Window xwindow,
                   char **res_class,
                   char **res_name);

Atom
_wnck_atom_get (const char *atom_name);

const char *
_wnck_atom_name (Atom atom);

GdkPixbuf *
_wnck_gdk_pixbuf_get_from_pixmap (GdkPixbuf * dest,
                                  Pixmap xpixmap,
                                  int src_x,
                                  int src_y,
                                  int dest_x,
                                  int dest_y, int width, int height);

gboolean
_wnck_icon_cache_get_is_fallback (WnckIconCache * icon_cache);

gboolean
_wnck_read_icons_ (Window xwindow,
                   GtkWidget * icon_cache,
                   GdkPixbuf ** iconp,
                   int ideal_width,
                   int ideal_height,
                   GdkPixbuf ** mini_iconp,
                   int ideal_mini_width, int ideal_mini_height);

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
