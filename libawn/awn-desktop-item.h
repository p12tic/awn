/*
 *  Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 *  vim: set noet :
 */

#ifndef _LIBAWN_AWN_DESKTOP_ITEM_H
#define _LIBAWN_AWN_DESKTOP_ITEM_H

#include <gtk/gtkicontheme.h>

#ifdef LIBAWN_USE_GNOME
#include <libgnome/gnome-desktop-item.h>
#elif defined(LIBAWN_USE_XFCE)
#include <libxfce4util/libxfce4util.h>
#endif

#ifdef LIBAWN_USE_GNOME
typedef GnomeDesktopItem AwnDesktopItem;
#elif defined(LIBAWN_USE_XFCE)
typedef XfceDesktopEntry AwnDesktopItem;
#endif

#define AWN_TYPE_DESKTOP_ITEM	(awn_desktop_item_get_type ())
#define AWN_DESKTOP_ITEM(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_DESKTOP_ITEM, AwnDesktopItem))
GType awn_desktop_item_get_type (void);

/* Wrapper functions */
AwnDesktopItem *awn_desktop_item_new (gchar *uri);
gchar          *awn_desktop_item_get_filename (AwnDesktopItem *item);
gchar          *awn_desktop_item_get_item_type (AwnDesktopItem *item);
gchar          *awn_desktop_item_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme);
gchar          *awn_desktop_item_get_name (AwnDesktopItem *item);
gchar          *awn_desktop_item_get_exec (AwnDesktopItem *item);
gchar          *awn_desktop_item_get_string (AwnDesktopItem *item, gchar *key);
gchar          *awn_desktop_item_get_localestring (AwnDesktopItem *item, gchar *key);
gboolean        awn_desktop_item_exists (AwnDesktopItem *item);
gint            awn_desktop_item_launch (AwnDesktopItem *item, GList *extra_argv, GError **err);
void            awn_desktop_item_unref (AwnDesktopItem *item);

#endif /* _LIBAWN_AWN_DESKTOP_ITEM_H */
