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

#include "awn-desktop-item.h"

/* helper functions for egg */
#ifndef LIBAWN_USE_GNOME
static GSList *awn_util_glist_to_gslist (GList *list)
{
	GSList *slist;
	guint i;
	guint len = g_list_length (list);
	for (i = 0; i < len; i++) {
		slist = g_slist_append (slist, g_list_nth_data (list, i));
	}
	g_list_free (list);
	return slist;
}
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

AwnDesktopItem *awn_desktop_item_new (gchar *uri)
{
	AwnDesktopItem *item = NULL;
#ifdef LIBAWN_USE_GNOME
	item = gnome_desktop_item_new_from_file (uri,
	                                         GNOME_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS,
	                                         NULL);
#else
	GError *err = NULL;
	item = egg_desktop_file_new (uri, &err);
	if (err) {
		g_warning ("Could not load the desktop item at '%s': %s",
			   uri, err->message);
	}
#endif
	return item;
}

gchar *awn_desktop_item_get_filename (AwnDesktopItem *item)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_vfs_get_local_path_from_uri (
	           gnome_desktop_item_get_location (item)
	       );
#else
	return (gchar*)egg_desktop_file_get_source (item);
#endif
}

gchar *awn_desktop_item_get_item_type (AwnDesktopItem *item)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_TYPE);
#else
	return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_TYPE);
#endif
}

void awn_desktop_item_set_item_type (AwnDesktopItem *item, gchar *item_type)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_TYPE, item_type);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_TYPE, item_type);
#endif
}

gchar *awn_desktop_item_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_icon (item, icon_theme);
#else
	/* adapted from libgnome-desktop/gnome-desktop-item.c, r4904
	 * gnome_desktop_item_get_icon(), gnome_desktop_item_find_icon()
	 */
	gchar *full = NULL;
	gchar *icon = (gchar*)egg_desktop_file_get_icon (item);
	if (strcmp (icon, "") == 0) {
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

void awn_desktop_item_set_icon (AwnDesktopItem *item, gchar *icon)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_ICON, icon);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_ICON, icon);
#endif
}

gchar *awn_desktop_item_get_name (AwnDesktopItem *item)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_localestring (item,
	                                            GNOME_DESKTOP_ITEM_NAME);
#else
	return (gchar*)egg_desktop_file_get_name (item);
#endif
}

void awn_desktop_item_set_name (AwnDesktopItem *item, gchar *name)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_NAME, name);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_NAME, name);
#endif
}

gchar *awn_desktop_item_get_exec (AwnDesktopItem *item)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_EXEC);
#else
	return awn_desktop_item_get_string (item, EGG_DESKTOP_FILE_KEY_EXEC);
#endif
}

void awn_desktop_item_set_exec (AwnDesktopItem *item, gchar *exec)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, GNOME_DESKTOP_ITEM_EXEC, exec);
#else
	awn_desktop_item_set_string (item, EGG_DESKTOP_FILE_KEY_EXEC, exec);
#endif
}

gchar *awn_desktop_item_get_string (AwnDesktopItem *item, gchar *key)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_get_string (item, key);
#else
	gchar *value;
	GError *err = NULL;
	value = g_key_file_get_string (egg_desktop_file_get_key_file (item), EGG_DESKTOP_FILE_GROUP, key, &err);
	if (err) {
		g_warning("Could not get the value of '%s' from '%s': %s",
		          key, awn_desktop_item_get_filename (item), err->message);
	}
	return value;
#endif
}

void awn_desktop_item_set_string (AwnDesktopItem *item, gchar *key, gchar *value)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_set_string (item, key, value);
#else
	g_key_file_set_string (egg_desktop_file_get_key_file (item),
	                       EGG_DESKTOP_FILE_GROUP, key, value);
#endif
}

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
		value = g_key_file_get_locale_string (egg_desktop_file_get_key_file (item),
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
	g_key_file_set_locale_string (egg_desktop_file_get_key_file (item),
	                              EGG_DESKTOP_FILE_GROUP, key, locale, value);
#endif
}

gboolean awn_desktop_item_exists (AwnDesktopItem *item)
{
#ifdef LIBAWN_USE_GNOME
	return gnome_desktop_item_exists (item);
#else
	return egg_desktop_file_can_launch (item, NULL);
#endif
}

gint awn_desktop_item_launch (AwnDesktopItem *item, GList *documents, GError **err)
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
	gboolean success;
	GSList *doc_list = NULL;
	if (egg_desktop_file_accepts_documents (item)) {
		doc_list = awn_util_glist_to_gslist (documents);
	}
	success = egg_desktop_file_launch (item, doc_list, err,
					   EGG_DESKTOP_FILE_LAUNCH_SCREEN, gdk_screen_get_default(),
					   EGG_DESKTOP_FILE_LAUNCH_RETURN_PID, &pid,
					   NULL);
	return pid;
#endif
}

void awn_desktop_item_save (AwnDesktopItem *item, GError **err)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_save (item, NULL, TRUE, err);
#else
	gchar *data;
	gsize data_len;
	data = g_key_file_to_data (egg_desktop_file_get_key_file (item), &data_len, err);
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

void awn_desktop_item_unref (AwnDesktopItem *item)
{
#ifdef LIBAWN_USE_GNOME
	gnome_desktop_item_unref (item);
#else
	egg_desktop_file_free (item);
#endif
}
/*  vim: set noet ts=8 : */
