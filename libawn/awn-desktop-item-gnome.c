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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdkscreen.h>

#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnome/gnome-desktop-item.h>

/**
 * SECTION: awn-desktop-item
 * @short_description: A desktop item-specific API wrapper.
 * @include: libawn/awn-desktop-item.h
 *
 * Contains a desktop item wrapper API whose implementation depends on
 * the compile-time configuration.
 */
#include "awn-desktop-item.h"

struct _AwnDesktopItem {
	GnomeDesktopItem *desktop_file;
};

/* GType function */
GType awn_desktop_item_get_type (void)
{
	return gnome_desktop_item_get_type ();
}

/* Wrapper functions */

/**
 * awn_desktop_item_new:
 * @filename: The path to the desktop file.  The base name of the path does not
 * need to exist at the time that this function is called.
 *
 * Creates a new desktop item structure.  If @filename exists, it attempts to
 * parse the file at that path as a desktop file.  Otherwise, it is used as the
 * path that will be used if awn_desktop_item_save() is called.
 * Returns: a newly created item structure.  When it is no longer needed, it
 * needs to be freed via awn_desktop_item_unref().
 */
AwnDesktopItem *awn_desktop_item_new (gchar *filename)
{
	AwnDesktopItem *item = g_malloc (sizeof (AwnDesktopItem));
	item->desktop_file = gnome_desktop_item_new_from_file (filename, 0, NULL);
	if (!item->desktop_file) {
		item->desktop_file = gnome_desktop_item_new ();
	}
	return item;
}

/**
 * awn_desktop_item_copy:
 * @item: The desktop item to copy.
 *
 * Creates a copy of the structure.
 * Returns: A copy of the structure.
 */
AwnDesktopItem *awn_desktop_item_copy (const AwnDesktopItem *item)
{
	AwnDesktopItem *new_item = g_malloc (sizeof (AwnDesktopItem));
	new_item->desktop_file = gnome_desktop_item_copy (item->desktop_file);
	return new_item;
}

/**
 * awn_desktop_item_get_filename:
 * @item: The desktop item structure that is being queried.
 *
 * Retrieves the filename that is associated with the desktop item.
 *
 * Returns: The filename associated with the desktop item
 */
gchar *awn_desktop_item_get_filename (AwnDesktopItem *item)
{
	return gnome_vfs_get_local_path_from_uri (
	           gnome_desktop_item_get_location (item->desktop_file)
	       );
}

/**
 * awn_desktop_item_get_item_type:
 * @item: The desktop item structure that is being queried.
 *
 * Retrieves the type of the desktop item, for example
 * "Application" or "Directory".
 * Returns: the item type name.
 */
gchar *awn_desktop_item_get_item_type (AwnDesktopItem *item)
{
	return (char*)gnome_desktop_item_get_string (item->desktop_file, GNOME_DESKTOP_ITEM_TYPE);
}

/**
 * awn_desktop_item_set_item_type:
 * @item: The desktop item structure that is being modified.
 * @item_type: The new item type of the desktop item.
 *
 * Sets the type of the desktop item, for example "Application"
 * or "Directory".
 */
void awn_desktop_item_set_item_type (AwnDesktopItem *item, gchar *item_type)
{
	gnome_desktop_item_set_string (item->desktop_file, GNOME_DESKTOP_ITEM_TYPE, item_type);
}

/**
 * awn_desktop_item_get_icon:
 * @item: The desktop item structre that is being queried.
 * @icon_theme: The icon theme in which the function searches for the icon.
 *
 * Retrieves the icon associated with the desktop item.
 * Returns: the absolute filename of the icon.
 */
gchar *awn_desktop_item_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme)
{
	return gnome_desktop_item_get_icon (item->desktop_file, icon_theme);
}

/**
 * awn_desktop_item_set_icon:
 * @item: The desktop item structure that is being modified.
 * @icon: The name of the icon, preferably per the Freedesktop.org Icon Naming
 * Specification.
 *
 * Sets the icon name of the desktop item.
 */
void awn_desktop_item_set_icon (AwnDesktopItem *item, gchar *icon)
{
	gnome_desktop_item_set_string (item->desktop_file, GNOME_DESKTOP_ITEM_ICON, icon);
}

/**
 * awn_desktop_item_get_name:
 * @item: The desktop item structure that is being queried.
 *
 * Retrieves the name of the desktop item.
 * Returns: the item name.
 */
gchar *awn_desktop_item_get_name (AwnDesktopItem *item)
{
	return (gchar*)gnome_desktop_item_get_localestring (item->desktop_file, GNOME_DESKTOP_ITEM_NAME);
}

/**
 * awn_desktop_item_set_name:
 * @item: The desktop item structure that is being modified.
 * @name: The new name of the desktop item.
 *
 * Sets the name of the desktop item.
 */
void awn_desktop_item_set_name (AwnDesktopItem *item, gchar *name)
{
	gnome_desktop_item_set_string (item->desktop_file, GNOME_DESKTOP_ITEM_NAME, name);
}

/**
 * awn_desktop_item_get_exec:
 * @item: The desktop item structure that is being queried.
 *
 * Retrieves the path of the desktop item.
 * Returns: the item path.
 */
gchar *awn_desktop_item_get_exec (AwnDesktopItem *item)
{
	return (gchar*)gnome_desktop_item_get_string (item->desktop_file, GNOME_DESKTOP_ITEM_EXEC);
}

/**
 * awn_desktop_item_set_exec:
 * @item: The desktop item structure that is being modified.
 * @exec: The new path of the desktop item.
 *
 * Sets the path of the desktop item.
 */
void awn_desktop_item_set_exec (AwnDesktopItem *item, gchar *exec)
{
	gnome_desktop_item_set_string (item->desktop_file, GNOME_DESKTOP_ITEM_EXEC, exec);
}

/**
 * awn_desktop_item_get_string:
 * @item: The desktop item structure that is being queried.
 * @key: The name of the key.
 *
 * Retrieves the value of a key in the desktop item specified.
 * Returns: the value of the key if it exists, or %NULL otherwise.
 */
gchar *awn_desktop_item_get_string (AwnDesktopItem *item, gchar *key)
{
	return (gchar*)gnome_desktop_item_get_string (item->desktop_file, key);
}

/**
 * awn_desktop_item_set_string:
 * @item: The desktop item structure that is being modified.
 * @key: The name of the key.
 * @value: The new value of the key.
 *
 * Changes the value of a key in the desktop item specified.
 */
void awn_desktop_item_set_string (AwnDesktopItem *item, gchar *key, gchar *value)
{
	gnome_desktop_item_set_string (item->desktop_file, key, value);
}

/**
 * awn_desktop_item_get_localestring:
 * @item: The desktop item structure that is being queried.
 * @key: The name of the key.
 *
 * Retrieves a locale-specific value for a key in the desktop item.
 * Returns: the locale-specific value, if it exists, or %NULL otherwise.
 */
gchar *awn_desktop_item_get_localestring (AwnDesktopItem *item, gchar *key)
{
	return (gchar*)gnome_desktop_item_get_localestring (item->desktop_file, key);
}

/**
 * awn_desktop_item_set_localestring:
 * @item: The desktop item structure that is being modified.
 * @key: The name of the key.
 * @locale: The name of the locale.  The format must correspond to the
 * POSIX specification on locale names.
 * @value: The new value of the locale-specific key.
 *
 * Sets a locale-specific value for a key in a desktop item.
 */
void awn_desktop_item_set_localestring (AwnDesktopItem *item, gchar *key, gchar *locale, gchar *value)
{
	if (locale == NULL) {
		gnome_desktop_item_set_localestring (item->desktop_file, key, value);
	} else {
		gnome_desktop_item_set_localestring_lang (item->desktop_file, key, locale, value);
	}
}

/**
 * awn_desktop_item_exists:
 * @item: The desktop item structure that is being queried.
 *
 * Checks to see if the path associated with the desktop item exists
 * and is executable.
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean awn_desktop_item_exists (AwnDesktopItem *item)
{
	return gnome_desktop_item_exists (item->desktop_file);
}

/**
 * awn_desktop_item_launch:
 * @item: The desktop item structure to be launched.
 * @documents: A list of documents that is passed to the path as arguments.
 * Can be %NULL.
 * @err: A pointer to a #GError structure, which contains an error message
 * if the function fails.
 *
 * Launches the path of the desktop item.
 * Returns: the process ID (PID) of the new process.
 */
gint awn_desktop_item_launch (AwnDesktopItem *item, GSList *documents, GError **err)
{
	GList *file_list = NULL;
	GSList *doc = NULL;
	int pid;
	for (doc = documents; doc != NULL; doc = g_slist_next (doc)) {
		file_list = g_list_append (file_list, doc->data);
	}
	pid = gnome_desktop_item_launch_on_screen (item->desktop_file,
	                                           file_list,
	                                           0,
	                                           gdk_screen_get_default(),
	                                           -1,
	                                           err);
	g_list_free (file_list);
	return pid;
}

/**
 * awn_desktop_item_save:
 * @item: The desktop item structure to be serialized.
 * @new_filename: the new name of the file where the serialized data
 * is saved.  If it is %NULL, the filename specified in awn_desktop_item_new()
 * is used.
 * @err: The pointer to a #GError structure, which contains an error
 * message when the function fails.
 *
 * Saves the serialized desktop item to disk.
 */
void awn_desktop_item_save (AwnDesktopItem *item, gchar *new_filename, GError **err)
{
	gnome_desktop_item_save (item->desktop_file, new_filename, TRUE, err);
}

/**
 * awn_desktop_item_free:
 * @item: The item to be freed from memory.
 *
 * Frees the desktop item structure from memory.
 */
void awn_desktop_item_free (AwnDesktopItem *item)
{
	if (item->desktop_file) {
		gnome_desktop_item_unref (item->desktop_file);
	}
	if (item) {
		g_free (item);
	}
}
/*  vim: set noet ts=8 sts=8 sw=8 : */
