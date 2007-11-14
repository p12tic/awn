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

#ifndef _LIBAWN_AWN_VFS_H
#define _LIBAWN_AWN_VFS_H

#include <glib.h>

#ifdef LIBAWN_USE_GNOME
#include <libgnomevfs/gnome-vfs-monitor.h>
typedef GnomeVFSMonitorHandle AwnVfsMonitor;
#elif defined(LIBAWN_USE_XFCE)
#include <thunar-vfs/thunar-vfs.h>
typedef ThunarVfsMonitorHandle AwnVfsMonitor;
#else
#include <gio/gfilemonitor.h>
#include <gio/gdirectorymonitor.h>
typedef struct _AwnVfsMonitor AwnVfsMonitor;
#endif

typedef enum {
	AWN_VFS_MONITOR_EVENT_CHANGED,
	AWN_VFS_MONITOR_EVENT_CREATED,
	AWN_VFS_MONITOR_EVENT_DELETED
} AwnVfsMonitorEvent;

typedef enum {
	AWN_VFS_MONITOR_FILE,
	AWN_VFS_MONITOR_DIRECTORY
} AwnVfsMonitorType;

typedef void (*AwnVfsMonitorFunc) (AwnVfsMonitor *monitor, gchar *monitor_path, gchar *event_path, AwnVfsMonitorEvent event, gpointer user_data);

/* File/Directory monitoring */
AwnVfsMonitor  *awn_vfs_monitor_add (gchar *path, AwnVfsMonitorType monitor_type, AwnVfsMonitorFunc callback, gpointer user_data);
void            awn_vfs_monitor_emit (AwnVfsMonitor *monitor, gchar *path, AwnVfsMonitorEvent event);
void            awn_vfs_monitor_remove (AwnVfsMonitor *monitor);

/* utility functions */
void            awn_vfs_init (void);
GList          *awn_vfs_get_pathlist_from_string (gchar *paths, GError **err);

#endif /* _LIBAWN_AWN_VFS_H */
/* vim: set noet ts=8 sts=8 sw=8 : */
