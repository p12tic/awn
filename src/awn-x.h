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
 *  Notes : Contains functions allowing Awn to get an icon from XOrg using the
 *          xid. Please note that all icon reading code  has been lifted from
 *	    libwnck (XUtils.c), so track bugfixes in libwnck.
*/

#ifndef	_AWN_X_H
#define	_AWN_X_H

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

#include <libawn/awn-desktop-item.h>

GdkPixbuf* awn_x_get_icon (WnckWindow *window, gint width, gint height);

void awn_x_set_icon_geometry (gulong xwindow, int x, int y, int width, int height);

void awn_x_set_strut (GtkWindow *window);

GdkPixbuf * awn_x_get_icon_for_window (WnckWindow *window, gint width, gint height);
GdkPixbuf * awn_x_get_icon_for_launcher (AwnDesktopItem *item, gint width, gint height);
GString * awn_x_get_application_name (WnckWindow *window, WnckApplication *app);

#endif
