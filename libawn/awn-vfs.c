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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* VFS */
#ifdef LIBAWN_USE_GIO
#include <gio/gvfs.h>
#elif defined(LIBAWN_USE_GNOME)
#include <libgnomevfs/gnome-vfs.h>
#elif defined(LIBAWN_USE_XFCE)
#include <thunar-vfs/thunar-vfs.h>
#endif

#include "libawn/awn-vfs.h"

void awn_vfs_init()
{
#ifndef LIBAWN_USE_GIO
#ifdef LIBAWN_USE_GNOME
	gnome_vfs_init ();
#elif defined(LIBAWN_USE_XFCE)
	thunar_vfs_init ();
#endif
#endif /* !LIBAWN_USE_GIO */
}

GList *awn_vfs_get_pathlist_from_string (gchar *paths, GError **err)
{
	GList *list = NULL;
#ifdef LIBAWN_USE_GIO
	gchar **path_list;
	path_list = g_strsplit (paths, "\r\n", 0);
	guint i;
	guint len = g_strv_length (path_list);
	for (i = 0; i < len; i++) {
		list = g_list_append (list, (gpointer)g_strdup (path_list[i]));
	}
	g_strfreev (path_list);
#else
	GList *li;
#ifdef LIBAWN_USE_GNOME
	list = gnome_vfs_uri_list_parse ((const gchar *) paths);
	for (li = list; li != NULL; li = li->next) {
		GnomeVFSURI *uri = li->data;
		li->data = gnome_vfs_uri_to_string (uri, 0 /* hide_options */);
		gnome_vfs_uri_unref (uri);
	}
#elif defined(LIBAWN_USE_XFCE)
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
#endif /* LIBAWN_USE_GIO */
	return list;
}
/*  vim: set noet ts=8 sw=8 : */
