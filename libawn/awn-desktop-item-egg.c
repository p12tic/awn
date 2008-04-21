/*
 *  Copyright (C) 2007, 2008 Mark Lee <avant-wn@lazymalevolence.com>
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
#include "egg/eggdesktopfile.h"

#include "awn-desktop-item.h"

struct _AwnDesktopItem {
	EggDesktopFile *desktop_file;
	GKeyFile *key_file;
};

/* GType function */
GType awn_desktop_item_get_type (void)
{
	return egg_desktop_file_get_type ();
}

/* Wrapper functions */

static void awn_desktop_item_initialize (AwnDesktopItem *item, gchar *filename)
{
	GError *err = NULL;
	gchar *default_desktop_entry = g_strdup_printf ("[%s]\nType=Unknown", EGG_DESKTOP_FILE_GROUP);
	item->desktop_file = g_new0 (EggDesktopFile, 1);
	item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_UNRECOGNIZED;
	item->desktop_file->source = g_strdup (filename);
	item->desktop_file->name = g_strdup ("");
	item->desktop_file->icon = g_strdup ("");
	item->key_file = g_key_file_new ();
	if (!g_key_file_load_from_data (item->key_file, default_desktop_entry, (gsize)strlen (default_desktop_entry), G_KEY_FILE_NONE, &err)) {
		g_error ("Could not load the default desktop entry: %s", err->message);
		g_error_free (err);
	}
	item->desktop_file->key_file = item->key_file;
	g_free (default_desktop_entry);
}

AwnDesktopItem *awn_desktop_item_new (gchar *filename)
{
	AwnDesktopItem *item = g_malloc (sizeof (AwnDesktopItem));
	GError *err = NULL;
	if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
		item->desktop_file = egg_desktop_file_new (filename, &err);
		if (err) {
			g_warning ("Could not load the desktop item at '%s': %s",
				   filename, err->message);
			g_error_free (err);
		}
		if (item->desktop_file) {
			item->key_file = egg_desktop_file_get_key_file (item->desktop_file);
		} else {
			awn_desktop_item_initialize (item, filename);
		}
	} else {
		awn_desktop_item_initialize (item, filename);
	}
	return item;
}

AwnDesktopItem *awn_desktop_item_copy (const AwnDesktopItem *item)
{
	AwnDesktopItem *new_item = g_malloc (sizeof (AwnDesktopItem));
	new_item->desktop_file = egg_desktop_file_copy (item->desktop_file);
	return new_item;
}

gchar *awn_desktop_item_get_filename (AwnDesktopItem *item)
{
	return (gchar*)egg_desktop_file_get_source (item->desktop_file);
}

gchar *awn_desktop_item_get_item_type (AwnDesktopItem *item)
{
	return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_TYPE);
}

void awn_desktop_item_set_item_type (AwnDesktopItem *item, gchar *item_type)
{
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_TYPE, item_type);
	if (strcmp (item_type, "Application") == 0) {
		item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_APPLICATION;
	} else if (strcmp (item_type, "Link") == 0) {
		item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_LINK;
	} else if (strcmp (item_type, "Directory") == 0) {
		item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_DIRECTORY;
	} else {
		item->desktop_file->type = EGG_DESKTOP_FILE_TYPE_UNRECOGNIZED;
	}
}

gchar *awn_desktop_item_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme)
{
	/* adapted from libgnome-desktop/gnome-desktop-item.c, r4904
	 * gnome_desktop_item_get_icon(), gnome_desktop_item_find_icon()
	 */
	gchar *full = NULL;
	gchar *icon = (gchar*)egg_desktop_file_get_icon (item->desktop_file);
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
}

void awn_desktop_item_set_icon (AwnDesktopItem *item, gchar *icon)
{
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_ICON, icon);
	item->desktop_file->icon = g_strdup (icon);
}

gchar *awn_desktop_item_get_name (AwnDesktopItem *item)
{
	return (gchar*)egg_desktop_file_get_name (item->desktop_file);
}

void awn_desktop_item_set_name (AwnDesktopItem *item, gchar *name)
{
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_NAME, name);
	item->desktop_file->name = g_strdup (name);
}

gchar *awn_desktop_item_get_exec (AwnDesktopItem *item)
{
	return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_EXEC);
}

void awn_desktop_item_set_exec (AwnDesktopItem *item, gchar *exec)
{
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_EXEC, exec);
}

gchar *awn_desktop_item_get_string (AwnDesktopItem *item, gchar *key)
{
	gchar *value;
	GError *err = NULL;
	value = g_key_file_get_string (egg_desktop_file_get_key_file (item->desktop_file), EGG_DESKTOP_FILE_GROUP, key, &err);
	if (err) {
		g_warning("Could not get the value of '%s' from '%s': %s",
		          key, awn_desktop_item_get_filename (item), err->message);
	}
	return value;
}

void awn_desktop_item_set_string (AwnDesktopItem *item, gchar *key, gchar *value)
{
	g_key_file_set_string (egg_desktop_file_get_key_file (item->desktop_file),
	                       EGG_DESKTOP_FILE_GROUP, key, value);
}

gchar *awn_desktop_item_get_localestring (AwnDesktopItem *item, gchar *key)
{
	gchar *value;
	GError *err = NULL;
	const gchar* const *langs = g_get_language_names ();
	guint i = 0;
	guint langs_len = g_strv_length ((gchar**)langs);
	do {
		value = g_key_file_get_locale_string (egg_desktop_file_get_key_file (item->desktop_file),
		                                      EGG_DESKTOP_FILE_GROUP, key, langs[i], &err);
		i++;
	} while (err && i < langs_len);
	if (err) {
		g_warning("Could not get the value of '%s' from '%s': %s",
		          key, awn_desktop_item_get_filename (item), err->message);
	}
	return value;
}

void awn_desktop_item_set_localestring (AwnDesktopItem *item, gchar *key, gchar *locale, gchar *value)
{
	if (locale == NULL) {
		/* use the first locale from g_get_language_names () */
		locale = (gchar*)(g_get_language_names ()[0]);
	}
	g_key_file_set_locale_string (egg_desktop_file_get_key_file (item->desktop_file),
	                              EGG_DESKTOP_FILE_GROUP, key, locale, value);
}

gboolean awn_desktop_item_exists (AwnDesktopItem *item)
{
	return egg_desktop_file_can_launch (item->desktop_file, NULL);
}

gint awn_desktop_item_launch (AwnDesktopItem *item, GSList *documents, GError **err)
{
	GPid pid;
	egg_desktop_file_launch (item->desktop_file, documents, err,
	                         EGG_DESKTOP_FILE_LAUNCH_RETURN_PID, &pid,
	                         NULL);
	return pid;
}

void awn_desktop_item_save (AwnDesktopItem *item, gchar *new_filename, GError **err)
{
	gchar *data;
	gchar *filename;
	gsize data_len;
	data = g_key_file_to_data (egg_desktop_file_get_key_file (item->desktop_file), &data_len, err);
	if (err) {
		g_free (data);
		return;
	} else {
		if (new_filename) {
			filename = new_filename;
			item->desktop_file->source = g_strdup (filename);
		} else {
			filename = awn_desktop_item_get_filename (item);
		}
		/* requires glib 2.8 */
		g_file_set_contents (filename, data, data_len, err);
		g_free (data);
	}
}

void awn_desktop_item_free (AwnDesktopItem *item)
{
	egg_desktop_file_free (item->desktop_file);
	g_free (item);
}
/*  vim: set noet ts=8 sts=8 sw=8 : */
