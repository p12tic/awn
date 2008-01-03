/*
 *  Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
 *
 *  For libgnome-desktop portions:
 *  Copyright (C) 1999, 2000 Red Hat Inc.
 *  Copyright (C) 2001 Sid Vicious
 *  All rights reserved.
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

#include <string.h>
#include <gdk/gdkscreen.h>

#ifdef LIBAWN_USE_GNOME
#include <libgnome/gnome-desktop-item.h>
#else
#include "egg/eggdesktopfile.h"
#endif

/**
 * SECTION: awn-desktop-item
 * @short_description: A desktop item-specific API wrapper.
 * @include: libawn/awn-desktop-item.h
 *
 * Contains a desktop item wrapper API whose implementation depends on
 * the compile-time configuration.
 */
#include "awn-desktop-item.h"

#ifdef LIBAWN_USE_GNOME
typedef GnomeDesktopItem _AwnDesktopItem;
#else
typedef EggDesktopFile _AwnDesktopItem;
#endif

/* GType function */
GType awn_desktop_item_get_type (void)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_type ();
#else
	return egg_desktop_file_get_type ();
#endif
}

/* Wrapper functions */

/**
 * awn_desktop_item_new:
 * @uri: The path to the desktop file.  The base name of the path does not need
 * to exist at the time that this function is called.
 *
 * Creates a new desktop item structure.  If @uri exists, it attempts to parse
 * the file at that path as a desktop file.  Otherwise, it is used as the path
 * that will be used if awn_desktop_item_save() is called.
 * Returns: a newly created item structure.  When it is no longer needed, it
 * needs to be freed via awn_desktop_item_unref().
 */
AwnDesktopItem *awn_desktop_item_new (gchar *uri)
{
	AwnDesktopItem *item = NULL;
#ifdef LIBAWN_USE_GNOME
	item = gnome_desktop_item_new_from_file (uri,
	                                         GNOME_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS,
	                                         NULL);
#else
	GError *err = NULL;
	item = (AwnDesktopItem*)egg_desktop_file_new (uri, &err);
	if (err) {
		g_warning ("Could not load the desktop item at '%s': %s",
			   uri, err->message);
	}
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_copy (item);
#else
	return (AwnDesktopItem*)egg_desktop_file_copy ((EggDesktopFile*)item);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_vfs_get_local_path_from_uri (
	           gnome_desktop_item_get_location (item)
	       );
#else
	return (gchar*)egg_desktop_file_get_source ((EggDesktopFile*)item);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_TYPE);
#else
	return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_TYPE);
#endif
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
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_TYPE, item_type);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_TYPE, item_type);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_icon (item, icon_theme);
#else
	/* adapted from libgnome-desktop/gnome-desktop-item.c, r4904
	 * gnome_desktop_item_get_icon(), gnome_desktop_item_find_icon()
	 */
	gchar *full = NULL;
	gchar *icon = (gchar*)egg_desktop_file_get_icon ((EggDesktopFile*)item);
	if (!icon || strcmp (icon, "") == 0) {
		return NULL;
	} else if (g_path_is_absolute (icon)) {
		if (g_file_test (icon, G_FILE_TEST_EXISTS)) {
			return g_strdup (icon);
		} else {
			return NULL;
		}
	} else {
		char *icon_no_extension;
		char *p;

		if (icon_theme == NULL)
			icon_theme = gtk_icon_theme_get_default ();

		icon_no_extension = g_strdup (icon);
		p = strrchr (icon_no_extension, '.');
		if (p &&
		    (strcmp (p, ".png") == 0 ||
		     strcmp (p, ".xpm") == 0 ||
		     strcmp (p, ".svg") == 0)) {
		    *p = 0;
		}

		GtkIconInfo *info;

		info = gtk_icon_theme_lookup_icon (icon_theme,
						   icon_no_extension,
						   48,
						   0);

		full = NULL;
		if (info) {
			full = g_strdup (gtk_icon_info_get_filename (info));
			gtk_icon_info_free (info);
		}

		g_free (icon_no_extension);
	}
	return full;
#endif
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
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_ICON, icon);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_ICON, icon);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_localestring (item,
	                                            GNOME_DESKTOP_ITEM_NAME);
#else
	return (gchar*)egg_desktop_file_get_name ((EggDesktopFile*)item);
#endif
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
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_NAME, name);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_NAME, name);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_EXEC);
#else
	return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_EXEC);
#endif
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
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_EXEC, exec);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_EXEC, exec);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_string (item, key);
#else
	gchar *value;
	GError *err = NULL;
	value = g_key_file_get_string (egg_desktop_file_get_key_file ((EggDesktopFile*)item), EGG_DESKTOP_FILE_GROUP, key, &err);
	if (err) {
		g_warning("Could not get the value of '%s' from '%s': %s",
		          key, awn_desktop_item_get_filename (item), err->message);
	}
	return value;
#endif
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
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, key, value);
#else
	g_key_file_set_string (egg_desktop_file_get_key_file ((EggDesktopFile*)item),
	                       EGG_DESKTOP_FILE_GROUP, key, value);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_localestring (item, key);
#else
	gchar *value;
	GError *err = NULL;
	const gchar* const *langs = g_get_language_names ();
	guint i = 0;
	guint langs_len = g_strv_length ((gchar**)langs);
	do {
		value = g_key_file_get_locale_string (egg_desktop_file_get_key_file ((EggDesktopFile*)item),
		                                      EGG_DESKTOP_FILE_GROUP, key, langs[i], &err);
		i++;
	} while (err && i < langs_len);
	if (err) {
		g_warning("Could not get the value of '%s' from '%s': %s",
		          key, awn_desktop_item_get_filename (item), err->message);
	}
	return value;
#endif
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
#ifdef LIBAWN_USE_GNOME
	if (locale == NULL) {
		gnome_desktop_item_set_localestring (item, key, value);
	} else {
		gnome_desktop_item_set_localestring_lang (item, key, locale, value);
	}
#else
	if (locale == NULL) {
		/* use the first locale from g_get_language_names () */
		locale = (gchar*)(g_get_language_names ()[0]);
	}
	g_key_file_set_locale_string (egg_desktop_file_get_key_file ((EggDesktopFile*)item),
	                              EGG_DESKTOP_FILE_GROUP, key, locale, value);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_exists (item);
#else
	return egg_desktop_file_can_launch ((EggDesktopFile*)item, NULL);
#endif
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
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_launch_on_screen (item,
	                                            NULL,
	                                            0,
	                                            gdk_screen_get_default(),
	                                            -1,
	                                            err);
#else
	GPid pid;
	egg_desktop_file_launch ((EggDesktopFile*)item, documents, err,
	                         EGG_DESKTOP_FILE_LAUNCH_SCREEN, gdk_screen_get_default(),
	                         EGG_DESKTOP_FILE_LAUNCH_RETURN_PID, &pid,
	                         NULL);
	return pid;
#endif
}

/**
 * awn_desktop_item_save:
 * @item: The desktop item structure to be serialized.
 * @err: The pointer to a #GError structure, which contains an error
 * message when the function fails.
 *
 * Saves the serialized desktop item to disk.
 */
void awn_desktop_item_save (AwnDesktopItem *item, GError **err)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_save (item, NULL, TRUE, err);
#else
	gchar *data;
	gsize data_len;
	data = g_key_file_to_data (egg_desktop_file_get_key_file ((EggDesktopFile*)item), &data_len, err);
	if (err) {
		g_free (data);
		return;
	} else {
		/* requires glib 2.8 */
		g_file_set_contents (awn_desktop_item_get_filename (item), data, data_len, err);
		g_free (data);
	}
#endif
}

/**
 * awn_desktop_item_unref:
 * @item: The item to be freed from memory.
 *
 * Frees the desktop item structure from memory.
 */
void awn_desktop_item_unref (AwnDesktopItem *item)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_unref (item);
#else
	egg_desktop_file_free ((EggDesktopFile*)item);
#endif
}
/*  vim: set noet ts=8 : */
