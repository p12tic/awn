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

/* VFS */
#ifdef USE_GNOME
#include <libgnomevfs/gnome-vfs.h>
#elif defined(USE_XFCE)
#include <thunar-vfs/thunar-vfs.h>
#endif

#include "awn-desktop-item.h"

/* helper functions for xfce */
#ifdef USE_XFCE
const gchar *desktop_categories[4] = {"Name", "Exec", "Icon", "Path"};

static gchar **awn_xfce_explode_exec (gchar *exec, GList *extra_argv)
{
	gchar **exploded = g_strsplit (exec, " ", 0);
	guint len = g_strv_length (exploded);
	guint i;
	gchar *argv_str = "";
	for (i = 0; i < len; i++) {
		if (g_strrstr (exploded[i], "%") == NULL) {
			if (g_ascii_strcasecmp (argv_str, "") == 0) {
				argv_str = g_strdup(exploded[i]);
			} else {
				argv_str = g_strconcat (argv_str, " ", exploded[i], NULL);
			}
		}
	}
	g_strfreev (exploded);
	if (extra_argv != NULL) {
		len = g_list_length (extra_argv);
		for (i = 0; i < len; i++) {
			argv_str = g_strconcat (argv_str, " ", (gchar *)g_list_nth_data(extra_argv, i), NULL);
		}
	}
	return g_strsplit (argv_str, " ", 0);
}

static gchar *awn_xfce_desktop_file_get_string (AwnDesktopItem *item, gchar *key, gboolean translatable)
{
	gchar *value = NULL;
	if (xfce_desktop_entry_get_string (item, key, translatable, &value)) {
		return value;
	} else {
		g_warning("Could not get the value of '%s' from '%s'",
		          key, awn_desktop_item_get_filename (item));
		return NULL;
	}
}

static gchar *awn_xfce_get_real_exec (gchar *exec)
{
	/* if not an absolute path, manually search through the path list
	 * since it looks like gdk_spawn_on_screen doesn't do it for us.
	 * Adapted from /libgnome-desktop/gnome-desktop-item.c, r4904
	 * exec_exists()
	 */
	if (g_path_is_absolute (exec)) {
		if (access (exec, X_OK) == 0) {
			return exec;
		} else {
			return NULL;
		}
	} else {
		return g_find_program_in_path (exec);
	}
}
#endif

/* GType function */
GType awn_desktop_item_get_type (void)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_type ();
#elif defined(USE_XFCE)
	return xfce_desktop_entry_get_type ();
#endif
}

/* Wrapper functions */

AwnDesktopItem *awn_desktop_item_new (gchar *uri)
{
	AwnDesktopItem *item = NULL;
#ifdef USE_GNOME
	item = gnome_desktop_item_new_from_file (uri,
	                                         GNOME_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS,
	                                         NULL);
#elif defined(USE_XFCE)
	item = xfce_desktop_entry_new (uri, desktop_categories, G_N_ELEMENTS(desktop_categories));
#endif
	return item;
}

gchar *awn_desktop_item_get_filename (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_vfs_get_local_path_from_uri (
	           gnome_desktop_item_get_location (item)
	       );
#elif defined(USE_XFCE)
	return (gchar *)xfce_desktop_entry_get_file (item);
#endif
}

gchar *awn_desktop_item_get_item_type (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_TYPE);
#elif defined(USE_XFCE)
	return awn_xfce_desktop_file_get_string (item, "Type", FALSE);
#endif
}

gchar *awn_desktop_item_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_icon (item, icon_theme);
#elif defined(USE_XFCE)
	/* adapted from libgnome-desktop/gnome-desktop-item.c, r4904
	 * gnome_desktop_item_get_icon(), gnome_desktop_item_find_icon()
	 */
	gchar *full = NULL;
	gchar *icon = awn_xfce_desktop_file_get_string (item, "Icon", FALSE);
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

gchar *awn_desktop_item_get_name (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_localestring (item,
	                                            GNOME_DESKTOP_ITEM_NAME);
#elif defined(USE_XFCE)
	return awn_xfce_desktop_file_get_string (item, "Name", TRUE);
#endif

}

gchar *awn_desktop_item_get_exec (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_EXEC);
#elif defined(USE_XFCE)
	return awn_xfce_desktop_file_get_string (item, "Exec", FALSE);
#endif
}

gchar *awn_desktop_item_get_string (AwnDesktopItem *item, gchar *key)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_string (item, key);
#elif defined(USE_XFCE)
	return awn_xfce_desktop_file_get_string (item, key, FALSE);
#endif
}

gchar *awn_desktop_item_get_localestring (AwnDesktopItem *item, gchar *key)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_localestring (item, key);
#elif defined(USE_XFCE)
	return awn_xfce_desktop_file_get_string (item, key, TRUE);
#endif
}

gboolean awn_desktop_item_exists (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_desktop_item_exists (item);
#elif defined(USE_XFCE)
	/* adapted from libgnome-desktop/gnome-desktop-item.c, r4904
	 * gnome_desktop_item_exists()
	 */
	const gchar *try_exec;
	const gchar *real_try_exec;
	const gchar *exec;

	g_return_val_if_fail (item != NULL, FALSE);

	try_exec = awn_xfce_desktop_file_get_string (item, "TryExec", FALSE);
	real_try_exec = awn_xfce_get_real_exec ((gchar*)try_exec);

	if (try_exec != NULL && real_try_exec == NULL) {
		return FALSE;
	}

	g_free ((gchar*)real_try_exec);

	if (g_ascii_strcasecmp(awn_desktop_item_get_item_type (item), "Application") == 0) {
		int argc;
		gchar **argv;
		const gchar *exe;
		const gchar *real_exec;

		exec = awn_desktop_item_get_exec (item);
		if (exec == NULL)
			return FALSE;

		if ( ! g_shell_parse_argv (exec, &argc, &argv, NULL))
			return FALSE;

		if (argc < 1) {
			g_strfreev (argv);
			return FALSE;
		}

		exe = argv[0];

		real_exec = awn_xfce_get_real_exec ((gchar*)exe);

		if (real_exec == NULL) {
			g_strfreev (argv);
			return FALSE;
		}
		g_strfreev (argv);
		g_free ((gchar*)real_exec);
	}

	return TRUE;
#endif
}

gint awn_desktop_item_launch (AwnDesktopItem *item, GList *extra_argv, GError **err)
{
#ifdef USE_GNOME
	return gnome_desktop_item_launch_on_screen (item,
	                                            NULL,
	                                            0,
	                                            gdk_screen_get_default(),
	                                            -1,
	                                            &err);
#elif defined(USE_XFCE)
	gboolean success;
	int pid;
	gchar *exec = awn_desktop_item_get_exec (item);
	gchar *path = awn_xfce_desktop_file_get_string (item, "Path", FALSE);
	exec = awn_xfce_get_real_exec (exec);
	success = gdk_spawn_on_screen (gdk_screen_get_default(),
	                               (const gchar *)path,
	                               awn_xfce_explode_exec (exec, extra_argv),
	                               NULL,
	                               G_SPAWN_STDOUT_TO_DEV_NULL,
	                               NULL,
	                               NULL,
	                               &pid,
	                               err);
	return pid;
#endif
}

void awn_desktop_item_unref (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	gnome_desktop_item_unref(item);
#elif defined(USE_XFCE)
	g_object_unref(item);
#endif
}

GList *awn_desktop_item_get_pathlist_from_string (gchar *paths, GError **err)
{
	GList *list, *li;
#ifdef USE_GNOME
	list = gnome_vfs_uri_list_parse ((const gchar *) paths);
	for (li = list; li != NULL; li = li->next) {
		GnomeVFSURI *uri = li->data;
		li->data = gnome_vfs_uri_to_string (uri, 0 /* hide_options */);
		gnome_vfs_uri_unref (uri);
	}
#elif defined(USE_XFCE)
	list = thunar_vfs_path_list_from_string ((const gchar *) paths, err);
	GError *error = *err;
	if (error) {
		g_print("Error: %s", error->message);
	} else {
		for (li = list; li != NULL; li = li->next) {
			ThunarVfsPath *uri = li->data;
			li->data = thunar_vfs_path_dup_string (uri);
			thunar_vfs_path_unref (uri);
		}
	}
#endif
	return list;
}
/*  vim: set noet ts=8 : */
