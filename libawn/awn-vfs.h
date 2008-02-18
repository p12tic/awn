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

#ifndef _LIBAWN_AWN_VFS_H
#define _LIBAWN_AWN_VFS_H

#include <glib.h>

/**
 * AwnVfsMonitor:
 *
 * A wrapper structure for the VFS libraries' file/directory monitoring
 * structure. In the case of the GIO implementation, it is an opaque structure.
 */
#ifdef LIBAWN_USE_GNOME
#include <libgnomevfs/gnome-vfs-monitor.h>
typedef GnomeVFSMonitorHandle AwnVfsMonitor;
#elif defined(LIBAWN_USE_XFCE)
#include <thunar-vfs/thunar-vfs.h>
typedef ThunarVfsMonitorHandle AwnVfsMonitor;
#else
#include <gio/gio.h>
typedef struct _AwnVfsMonitor AwnVfsMonitor;
#endif

/**
 * AwnVfsMonitorEvent:
 * @AWN_VFS_MONITOR_EVENT_CHANGED: Indicates that the path referenced has
 * been changed.
 * @AWN_VFS_MONITOR_EVENT_CREATED: Indicates that the path referenced has
 * been created on the filesystem.
 * @AWN_VFS_MONITOR_EVENT_DELETED: Indicates that the path referenced has
 * been removed from the filesystem.
 *
 * A list of valid monitor event types for use with monitor event emission or
 * retrieving the type of monitor event in the callback.  Due to thunar-vfs's
 * limited support for event types, only "changed", "created", and "deleted"
 * are available for use.
 */
typedef enum {
	AWN_VFS_MONITOR_EVENT_CHANGED,
	AWN_VFS_MONITOR_EVENT_CREATED,
	AWN_VFS_MONITOR_EVENT_DELETED
} AwnVfsMonitorEvent;

/**
 * AwnVfsMonitorType:
 * @AWN_VFS_MONITOR_FILE: Indicates that the #AwnVfsMonitor instance is
 * monitoring a file.
 * @AWN_VFS_MONITOR_DIRECTORY: Indicates that the #AwnVfsMonitor instance is
 * monitoring a directory.
 *
 * The type of path that an #AwnVfsMonitor instance is monitoring.
 */
typedef enum {
	AWN_VFS_MONITOR_FILE,
	AWN_VFS_MONITOR_DIRECTORY
} AwnVfsMonitorType;

/**
 * AwnVfsMonitorFunc:
 * @monitor: The monitor structure.
 * @monitor_path: The path that is associated with the monitor structure.
 * @event_path: In the case of a directory monitor, the path inside the folder
 * that triggered the event.  Otherwise, it is the same as the @monitor_path.
 * @event: The filesystem operation that triggered the callback to be executed.
 * @user_data: The data passed to awn_vfs_monitor_add() so that it could be used
 * in the callback.
 *
 * The function template used for callbacks executed when a filesystem event has
 * occurred on a path that is being monitored.
 */
typedef void (*AwnVfsMonitorFunc) (AwnVfsMonitor *monitor, gchar *monitor_path, gchar *event_path, AwnVfsMonitorEvent event, gpointer user_data);

/* File/Directory monitoring */
AwnVfsMonitor  *awn_vfs_monitor_add (gchar *path, AwnVfsMonitorType monitor_type, AwnVfsMonitorFunc callback, gpointer user_data);
void            awn_vfs_monitor_emit (AwnVfsMonitor *monitor, gchar *path, AwnVfsMonitorEvent event);
void            awn_vfs_monitor_remove (AwnVfsMonitor *monitor);

/* utility functions */
void            awn_vfs_init (void);
GSList         *awn_vfs_get_pathlist_from_string (gchar *paths, GError **err);

#endif /* _LIBAWN_AWN_VFS_H */
/* vim: set noet ts=8 sts=8 sw=8 : */
