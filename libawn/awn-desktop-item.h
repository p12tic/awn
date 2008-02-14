/*
 *  Copyright (C) 2007, 2008 Mark Lee <avant-wn@lazymalevolence.com>
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

/**
 * AwnDesktopItem:
 *
 * An alias for the desktop-specific .desktop file-handling structure.
 */
typedef struct _AwnDesktopItem AwnDesktopItem;

#define AWN_TYPE_DESKTOP_ITEM	(awn_desktop_item_get_type ())
/**
 * AWN_DESKTOP_ITEM:
 * @obj: The variable/value to cast
 *
 * Casts a variable/value to be an #AwnDesktopItem.
 */
#define AWN_DESKTOP_ITEM(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_DESKTOP_ITEM, AwnDesktopItem))
GType awn_desktop_item_get_type (void);

/* Wrapper functions */
AwnDesktopItem *awn_desktop_item_new (gchar *filename);
AwnDesktopItem *awn_desktop_item_copy (const AwnDesktopItem *item);
gchar          *awn_desktop_item_get_filename (AwnDesktopItem *item);
gchar          *awn_desktop_item_get_item_type (AwnDesktopItem *item);
void            awn_desktop_item_set_item_type (AwnDesktopItem *item, gchar *item_type);
gchar          *awn_desktop_item_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme);
void            awn_desktop_item_set_icon (AwnDesktopItem *item, gchar *icon);
gchar          *awn_desktop_item_get_name (AwnDesktopItem *item);
void            awn_desktop_item_set_name (AwnDesktopItem *item, gchar *name);
gchar          *awn_desktop_item_get_exec (AwnDesktopItem *item);
void            awn_desktop_item_set_exec (AwnDesktopItem *item, gchar *exec);
gchar          *awn_desktop_item_get_string (AwnDesktopItem *item, gchar *key);
void            awn_desktop_item_set_string (AwnDesktopItem *item, gchar *key, gchar *value);
gchar          *awn_desktop_item_get_localestring (AwnDesktopItem *item, gchar *key);
void            awn_desktop_item_set_localestring (AwnDesktopItem *item, gchar *key, gchar *locale, gchar *value);
gboolean        awn_desktop_item_exists (AwnDesktopItem *item);
gint            awn_desktop_item_launch (AwnDesktopItem *item, GSList *documents, GError **err);
void            awn_desktop_item_save (AwnDesktopItem *item, gchar *new_filename, GError **err);
void            awn_desktop_item_free (AwnDesktopItem *item);

#endif /* _LIBAWN_AWN_DESKTOP_ITEM_H */
