/*
 *  Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
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
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 *  vim: set noet :
 */

#ifndef _LIBAWN_AWN_DESKTOP_FILE_H
#define _LIBAWN_AWN_DESKTOP_FILE_H

#ifdef USE_GNOME
#include <libgnome/gnome-desktop-item.h>
#elif defined(USE_XFCE)
#include <libxfce4util/libxfce4util.h>
#endif

#ifdef USE_GNOME
typedef GnomeDesktopItem AwnDesktopItem;
#elif defined(USE_XFCE)
typedef XfceDesktopEntry AwnDesktopItem;
#endif

/* Wrapper functions */
AwnDesktopItem *awn_desktop_file_new (gchar *uri);
gchar          *awn_desktop_file_get_filename (AwnDesktopItem *item);
gchar          *awn_desktop_file_get_item_type (AwnDesktopItem *tem);
gchar          *awn_desktop_file_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme);
gchar          *awn_desktop_file_get_name (AwnDesktopItem *item);
gchar          *awn_desktop_file_get_exec (AwnDesktopItem *item);
gint            awn_desktop_file_launch (AwnDesktopItem *item, GList *extra_argv, GError **err);
void            awn_desktop_file_unref (AwnDesktopItem *item);

/* utility function */
GList          *awn_desktop_file_get_pathlist_from_string (gchar *paths, GError **err);

#endif /* _LIBAWN_AWN_DESKTOP_FILE_H */
