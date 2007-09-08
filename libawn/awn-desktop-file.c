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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* VFS */
#ifdef USE_GNOME
#include <libgnomevfs/gnome-vfs.h>
#elif defined(USE_XFCE)
#include <thunar-vfs/thunar-vfs.h>
#endif

#include "awn-desktop-file.h"

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
		          key, awn_desktop_file_get_filename (item));
		return NULL;
	}
}
#endif

AwnDesktopItem *awn_desktop_file_new (gchar *uri)
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

gchar *awn_desktop_file_get_filename (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_vfs_get_local_path_from_uri (
	           gnome_desktop_item_get_location (item)
	       );
#elif defined(USE_XFCE)
	return (gchar *)xfce_desktop_entry_get_file (item);
#endif
}

gchar *awn_desktop_file_get_icon (AwnDesktopItem *item, GtkIconTheme *icon_theme)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_icon (item, icon_theme);
#elif defined(USE_XFCE)
	gchar *icon_name = awn_xfce_desktop_file_get_string (item, "Icon", FALSE);
	/* TODO: replicate functionality of gnome_desktop_get_icon w/o
	 * looking at the gnome_desktop source
	 */
	return icon_name;
#endif
}

gchar *awn_desktop_file_get_name (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_localestring (item,
	                                            GNOME_DESKTOP_ITEM_NAME);
#elif defined(USE_XFCE)
	return awn_xfce_desktop_file_get_string (item, "Name", TRUE);
#endif

}

gchar *awn_desktop_file_get_exec (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	return gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_EXEC);
#elif defined(USE_XFCE)
	return awn_xfce_desktop_file_get_string (item, "Exec", FALSE);
#endif
}

gint awn_desktop_file_launch (AwnDesktopItem *item, GList *extra_argv, GError **err)
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
	gchar *exec = awn_desktop_file_get_exec (item);
	gchar *path = awn_xfce_desktop_file_get_string (item, "Path", FALSE);
	/* if not an absolute path, manually search through the path list
	 * since it looks like gdk_spawn_on_screen doesn't do it for us.
	 */
	if (!g_str_has_prefix (exec, G_DIR_SEPARATOR_S)) {
		gchar **dirs = g_strsplit (g_getenv ("PATH"), G_SEARCHPATH_SEPARATOR_S, 0);
		guint len = g_strv_length (dirs);
		guint i;
		for (i = 0; i < len; i++) {
			gchar *tmp_exec = g_strconcat(dirs[i], G_DIR_SEPARATOR_S, exec, NULL);
			if (g_file_test (tmp_exec, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_EXECUTABLE)) {
				g_free(exec);
				exec = tmp_exec;
				break;
			}
		}
	}
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

void awn_desktop_file_unref (AwnDesktopItem *item)
{
#ifdef USE_GNOME
	gnome_desktop_item_unref(item);
#elif defined(USE_XFCE)
	g_object_unref(item);
#endif
}

GList *awn_desktop_file_get_pathlist_from_string (gchar *paths, GError **err)
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
    GError *error = &err;
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
