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
#include <config.h>
#endif

/**
 * SECTION: awn-vfs
 * @short_description: VFS-related API wrappers and utility functions.
 * @include: libawn/awn-vfs.h
 *
 * Contains a file monitoring wrapper API and utility functions whose
 * implementations depend on the compile-time configuration.
 */
#include "libawn/awn-vfs.h"

#ifdef LIBAWN_USE_GNOME
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-module-shared.h>
#elif defined (LIBAWN_USE_XFCE)
/* Already included in awn-vfs.h */
#else
struct _AwnVfsMonitor {
	AwnVfsMonitorType type;
#if GLIB_CHECK_VERSION(2,15,1)
	GFileMonitor *monitor;
#else
	union {
		GFileMonitor *file;
		GDirectoryMonitor *directory;
	} monitor;
#endif
};
#endif

typedef struct {
	AwnVfsMonitorFunc callback;
	gpointer data;
} AwnVfsMonitorData;

static int awn_vfs_monitor_event_type_to_native (AwnVfsMonitorEvent event)
{
	int native_event = -1;
	switch (event) {
		case AWN_VFS_MONITOR_EVENT_CHANGED:
#ifdef LIBAWN_USE_GNOME
			native_event = GNOME_VFS_MONITOR_EVENT_CHANGED;
#elif defined(LIBAWN_USE_XFCE)
			native_event = THUNAR_VFS_MONITOR_EVENT_CHANGED;
#else
			native_event = G_FILE_MONITOR_EVENT_CHANGED;
#endif
			break;
		case AWN_VFS_MONITOR_EVENT_CREATED:
#ifdef LIBAWN_USE_GNOME
			native_event = GNOME_VFS_MONITOR_EVENT_CREATED;
#elif defined(LIBAWN_USE_XFCE)
			native_event = THUNAR_VFS_MONITOR_EVENT_CREATED;
#else
			native_event = G_FILE_MONITOR_EVENT_CREATED;
#endif
			break;
		case AWN_VFS_MONITOR_EVENT_DELETED:
#ifdef LIBAWN_USE_GNOME
			native_event = GNOME_VFS_MONITOR_EVENT_DELETED;
#elif defined(LIBAWN_USE_XFCE)
			native_event = THUNAR_VFS_MONITOR_EVENT_DELETED;
#else
			native_event = G_FILE_MONITOR_EVENT_DELETED;
#endif
			break;
	}
	return native_event;
}

static AwnVfsMonitorEvent awn_vfs_monitor_native_event_type_to_awn (int native_event)
{
	AwnVfsMonitorEvent event = -1;
	switch (native_event) {
#ifdef LIBAWN_USE_GNOME
		case GNOME_VFS_MONITOR_EVENT_CHANGED:
#elif defined(LIBAWN_USE_XFCE)
		case THUNAR_VFS_MONITOR_EVENT_CHANGED:
#else
		case G_FILE_MONITOR_EVENT_CHANGED:
#endif
			event = AWN_VFS_MONITOR_EVENT_CHANGED;
			break;
#ifdef LIBAWN_USE_GNOME
		case GNOME_VFS_MONITOR_EVENT_CREATED:
#elif defined(LIBAWN_USE_XFCE)
		case THUNAR_VFS_MONITOR_EVENT_CREATED:
#else
		case G_FILE_MONITOR_EVENT_CREATED:
#endif
			event = AWN_VFS_MONITOR_EVENT_CREATED;
			break;
#ifdef LIBAWN_USE_GNOME
		case GNOME_VFS_MONITOR_EVENT_DELETED:
#elif defined(LIBAWN_USE_XFCE)
		case THUNAR_VFS_MONITOR_EVENT_DELETED:
#else
		case G_FILE_MONITOR_EVENT_DELETED:
#endif
			event = AWN_VFS_MONITOR_EVENT_DELETED;
			break;
	}
	return event;
}

#ifdef LIBAWN_USE_GNOME
static void gnome_vfs_monitor_callback_proxy (GnomeVFSMonitorHandle *handle, const gchar *monitor_uri, const gchar *event_uri, GnomeVFSMonitorEventType event, AwnVfsMonitorData *data)
{
	switch (event) {
		case GNOME_VFS_MONITOR_EVENT_CHANGED:
		case GNOME_VFS_MONITOR_EVENT_CREATED:
		case GNOME_VFS_MONITOR_EVENT_DELETED: {
			AwnVfsMonitorEvent awn_event = awn_vfs_monitor_native_event_type_to_awn (event);
			(data->callback) ((AwnVfsMonitor*)handle, (gchar*)monitor_uri, (gchar*)event_uri, awn_event, data->data);
			break;
		} default:
			/* Don't do anything */
			break;
	}
}
#elif defined (LIBAWN_USE_XFCE)
static void thunar_vfs_monitor_callback_proxy (ThunarVfsMonitor *monitor, ThunarVfsMonitorHandle *handle, ThunarVfsMonitorEvent event, ThunarVfsPath *monitor_uri, ThunarVfsPath *event_uri, AwnVfsMonitorData *data)
{
	gchar *monitor_path = NULL;
	gchar *event_path = NULL;
	if (monitor_uri) {
		monitor_path = thunar_vfs_path_dup_string (monitor_uri);
	}
	if (event_uri) {
		event_path = thunar_vfs_path_dup_string (event_uri);
	}
	AwnVfsMonitorEvent awn_event = awn_vfs_monitor_native_event_type_to_awn (event);
	(data->callback) ((AwnVfsMonitor*)handle, monitor_path, event_path, awn_event, data->data);
	g_free (event_path);
	g_free (monitor_path);
}
#else
static void g_file_monitor_callback_proxy (GFileMonitor *monitor, GFile *file, GFile *other_file, GFileMonitorEvent event, AwnVfsMonitorData *data)
{
	switch (event) {
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_CREATED:
		case G_FILE_MONITOR_EVENT_DELETED: {
			gchar *monitor_path = NULL;
			gchar *event_path = NULL;
			if (file) {
				monitor_path = g_file_get_path (file);
			}
			if (other_file) {
				event_path = g_file_get_path (other_file);
			}
			AwnVfsMonitorEvent awn_event = awn_vfs_monitor_native_event_type_to_awn (event);
			AwnVfsMonitor *handle = g_new (AwnVfsMonitor, 1);
#if GLIB_CHECK_VERSION(2,15,1)
			handle->monitor = monitor;
#else
			handle->monitor.file = monitor;
#endif
			handle->type = AWN_VFS_MONITOR_FILE;
			(data->callback) (handle, monitor_path, event_path, awn_event, data->data);
			g_free (handle);
			break;
		} default:
			/* Don't do anything */
			break;
	}
}
#endif

/**
 * awn_vfs_monitor_add:
 * @path: The path to the file/directory being monitored.  The path does not
 * have to exist on the filesystem at the time this function is called.
 * @monitor_type: States whether the path being monitored is a directory or
 * a file.
 * @callback: The function to be executed when an event occurs with regards
 * to the path.
 * @user_data: Extra data to be used by the @callback function.
 *
 * Adds a monitor callback to the list of callbacks associated with a file
 * or directory.
 *
 * Returns: a reference to a newly created monitor struct.  The caller is
 * obligated to free the memory when the struct is no longer needed.
 */
AwnVfsMonitor *awn_vfs_monitor_add (gchar *path, AwnVfsMonitorType monitor_type, AwnVfsMonitorFunc callback, gpointer user_data)
{
	AwnVfsMonitor *monitor;
	AwnVfsMonitorData *data = g_new (AwnVfsMonitorData, 1);
	data->callback = callback;
	data->data = user_data;
#ifdef LIBAWN_USE_GNOME
	GnomeVFSMonitorType gmtype = GNOME_VFS_MONITOR_FILE;
	if (monitor_type == AWN_VFS_MONITOR_FILE) {
		gmtype = GNOME_VFS_MONITOR_FILE;
	} else if (monitor_type == AWN_VFS_MONITOR_DIRECTORY) {
		gmtype = GNOME_VFS_MONITOR_DIRECTORY;
	}
	gnome_vfs_monitor_add (&monitor, path, gmtype, (GnomeVFSMonitorCallback)gnome_vfs_monitor_callback_proxy, data);
#elif defined(LIBAWN_USE_XFCE)
	ThunarVfsMonitor *tvfs_monitor = thunar_vfs_monitor_get_default ();
	ThunarVfsPath *uri = thunar_vfs_path_new (path, NULL);
	if (monitor_type == AWN_VFS_MONITOR_FILE) {
		monitor = thunar_vfs_monitor_add_file (tvfs_monitor, uri, (ThunarVfsMonitorCallback)thunar_vfs_monitor_callback_proxy, data);
	} else if (monitor_type == AWN_VFS_MONITOR_DIRECTORY) {
		monitor = thunar_vfs_monitor_add_directory (tvfs_monitor, uri, (ThunarVfsMonitorCallback)thunar_vfs_monitor_callback_proxy, data);
	} else {
		monitor = NULL;
	}
	thunar_vfs_path_unref (uri);
#else
#if GLIB_CHECK_VERSION(2,15,1)
#define FILE_MONITOR monitor->monitor
#define DIR_MONITOR monitor->monitor
#else
#define FILE_MONITOR monitor->monitor.file
#define DIR_MONITOR monitor->monitor.directory
#endif
	monitor = g_new (AwnVfsMonitor, 1);
	monitor->type = monitor_type;
	GFile *file = g_file_new_for_path (path);
	if (monitor_type == AWN_VFS_MONITOR_FILE) {
		/* based on gio/programs/gio-monitor-file.c */
#if GLIB_CHECK_VERSION(2,15,1)
		/* Appends GError** to the params */
		FILE_MONITOR = g_file_monitor_file (file, 0, NULL, NULL);
#else
		FILE_MONITOR = g_file_monitor_file (file, 0, NULL);
#endif
		if (FILE_MONITOR) {
			g_signal_connect (FILE_MONITOR, "changed", (GCallback)g_file_monitor_callback_proxy, data);
		} else {
			g_free (monitor);
			monitor = NULL;
		}
	} else if (monitor_type == AWN_VFS_MONITOR_DIRECTORY) {
		/* based on gio/programs/gio-monitor-dir.c */
#if GLIB_CHECK_VERSION(2,15,2)
		/* Appends GError** to the params */
		DIR_MONITOR = g_file_monitor_directory (file, 0, NULL, NULL);
#else
		DIR_MONITOR = g_file_monitor_directory (file, 0, NULL);
#endif
		if (DIR_MONITOR) {
			g_signal_connect (DIR_MONITOR, "changed", (GCallback)g_file_monitor_callback_proxy, data);
		} else {
			g_free (monitor);
			monitor = NULL;
		}
	}
#endif
	return monitor;
}

/**
 * awn_vfs_monitor_emit:
 * @monitor: The monitor structure.
 * @path: The path on the filesystem that will be passed to the
 * respective callbacks.
 * @event: The type of monitor event to fake.
 *
 * Emits an artificial monitor event to a monitor.
 */
void awn_vfs_monitor_emit (AwnVfsMonitor *monitor, gchar *path, AwnVfsMonitorEvent event)
{
	int native_event = awn_vfs_monitor_event_type_to_native (event);
#ifdef LIBAWN_USE_GNOME
	GnomeVFSURI *uri = gnome_vfs_uri_new (gnome_vfs_get_uri_from_local_path (path));
	gnome_vfs_monitor_callback ((GnomeVFSMethodHandle)monitor, uri, (GnomeVFSMonitorEventType)native_event);
	gnome_vfs_uri_unref (uri);
#elif defined(LIBAWN_USE_XFCE)
	ThunarVfsPath *uri = thunar_vfs_path_new (path, NULL);
	thunar_vfs_monitor_feed ((ThunarVfsMonitor*)monitor, (ThunarVfsMonitorEvent)native_event, uri);
	thunar_vfs_path_unref (uri);
#else
#if GLIB_CHECK_VERSION(2,15,1)
	GFile *file = g_file_new_for_path (path);
	GFile *other_file = file;
	g_file_monitor_emit_event (monitor->monitor, file, other_file, (GFileMonitorEvent)native_event);
#else
	switch (monitor->type) {
		case AWN_VFS_MONITOR_FILE: {
			GFile *file = g_file_new_for_path (path);
			GFile *other_file = file;
			g_file_monitor_emit_event (monitor->monitor.file, file, other_file, (GFileMonitorEvent)native_event);
			break;
		} case AWN_VFS_MONITOR_DIRECTORY: {
			GFile *child = g_file_new_for_path (path);
			GFile *other_file = child;
			g_directory_monitor_emit_event (monitor->monitor.directory, child, other_file, (GFileMonitorEvent)native_event);
			break;
		}
	}
#endif
#endif
}

/**
 * awn_vfs_monitor_remove:
 * @monitor: The structure to remove from monitoring  a path.
 *
 * Removes a monitor structure from monitoring a path.
 */
void awn_vfs_monitor_remove (AwnVfsMonitor *monitor)
{
#ifdef LIBAWN_USE_GNOME
	gnome_vfs_monitor_cancel (monitor);
#elif defined(LIBAWN_USE_XFCE)
	thunar_vfs_monitor_remove (thunar_vfs_monitor_get_default (), monitor);
#else
#if GLIB_CHECK_VERSION(2,15,1)
	g_file_monitor_cancel (monitor->monitor);
#else
	if (monitor->type == AWN_VFS_MONITOR_FILE) {
		g_file_monitor_cancel (monitor->monitor.file);
	} else if (monitor->type == AWN_VFS_MONITOR_DIRECTORY) {
		g_directory_monitor_cancel (monitor->monitor.directory);
	}
#endif
#endif
}

/**
 * awn_vfs_init:
 *
 * Starts up the VFS library routines that Awn uses, if necessary.
 */
void awn_vfs_init()
{
#ifdef LIBAWN_USE_GNOME
	gnome_vfs_init ();
#elif defined(LIBAWN_USE_XFCE)
	thunar_vfs_init ();
#endif
}

/**
 * awn_vfs_get_pathlist_from_string:
 * @paths: A list of strings in a text/uri-list format
 * @err: A pointer to the reference object that is used to determine whether
 * the operation was successful.
 *
 * Converts a string of type text/uri-list into a GSList of URIs.
 *
 * Returns: a list of URIs
 */
GSList *awn_vfs_get_pathlist_from_string (gchar *paths, GError **err)
{
	GSList *list = NULL;
#ifdef LIBAWN_USE_GNOME
	GList *result = NULL;
	GList *li;
	result = gnome_vfs_uri_list_parse ((const gchar *) paths);
	for (li = result; li != NULL; li = li->next) {
		GnomeVFSURI *uri = li->data;
		list = g_slist_append (list, gnome_vfs_uri_to_string (uri, 0 /* hide_options */));
		gnome_vfs_uri_unref (uri);
	}
	g_list_free (result);
	*err = NULL;
#elif defined(LIBAWN_USE_XFCE)
	GList *result = NULL;
	GList *li;
	result = thunar_vfs_path_list_from_string ((const gchar *) paths, err);
	if (*err) {
		g_print ("Error: %s", (*err)->message);
	} else {
		for (li = result; li != NULL; li = li->next) {
			ThunarVfsPath *uri = li->data;
			list = g_slist_append (list, thunar_vfs_path_dup_string (uri));
			thunar_vfs_path_unref (uri);
		}
		g_list_free (result);
	}
#else
	gchar **path_list;
	path_list = g_strsplit ((gchar*)paths, "\r\n", 0);
	guint i;
	guint len = g_strv_length (path_list);
	for (i = 0; i < len; i++) {
		list = g_slist_append (list, (gpointer)g_strdup (path_list[i]));
	}
	g_strfreev (path_list);
	*err = NULL;
#endif
	return list;
}
/*  vim: set noet ts=8 sts=8 sw=8 : */
