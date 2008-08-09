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

struct _AwnVfsMonitor 
{
	AwnVfsMonitorType type;

	GFileMonitor *monitor;
};

typedef struct 
{
	AwnVfsMonitorFunc callback;
	gpointer data;

} AwnVfsMonitorData;

static int 
awn_vfs_monitor_event_type_to_native (AwnVfsMonitorEvent event)
{
  int native_event = -1;
  switch (event) 
  {
    case AWN_VFS_MONITOR_EVENT_CHANGED:
      native_event = G_FILE_MONITOR_EVENT_CHANGED;
      break;
    case AWN_VFS_MONITOR_EVENT_CREATED:
      native_event = G_FILE_MONITOR_EVENT_CREATED;
      break;
    case AWN_VFS_MONITOR_EVENT_DELETED:
      native_event = G_FILE_MONITOR_EVENT_DELETED;
      break;
  }
  return native_event;
}

static AwnVfsMonitorEvent 
awn_vfs_monitor_native_event_type_to_awn (int native_event)
{
  AwnVfsMonitorEvent event = -1;

  switch (native_event) 
  {
    case G_FILE_MONITOR_EVENT_CHANGED:
      event = AWN_VFS_MONITOR_EVENT_CHANGED;
      break;
    case G_FILE_MONITOR_EVENT_CREATED:
      event = AWN_VFS_MONITOR_EVENT_CREATED;
      break;
    case G_FILE_MONITOR_EVENT_DELETED:
      event = AWN_VFS_MONITOR_EVENT_DELETED;
      break;
  }
  return event;
}

static void 
g_file_monitor_callback_proxy (GFileMonitor      *monitor, 
                               GFile             *file, 
                               GFile             *other_file, 
                               GFileMonitorEvent  event, 
                               AwnVfsMonitorData *data)
{
  AwnVfsMonitorEvent  awn_event;
  AwnVfsMonitor       handle;
  gchar              *monitor_path = NULL;
  gchar              *event_path = NULL;

  switch (event) 
  {
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_DELETED: 
      break;
    default:
      return;
			break;
	}

  if (file) 
    monitor_path = g_file_get_path (file);

  if (other_file) 
    event_path = g_file_get_path (other_file);
  
  awn_event = awn_vfs_monitor_native_event_type_to_awn (event);

  handle.monitor = monitor;
  handle.type = AWN_VFS_MONITOR_FILE;
  
  data->callback (&handle, monitor_path, event_path, awn_event, data->data);
}


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
AwnVfsMonitor *
awn_vfs_monitor_add (gchar             *path, 
                     AwnVfsMonitorType  monitor_type, 
                     AwnVfsMonitorFunc  callback, 
                     gpointer           user_data)
{
	AwnVfsMonitor     *monitor;
	AwnVfsMonitorData *data;
  GFile             *file;
  
  data = g_new (AwnVfsMonitorData, 1);
	data->callback = callback;
	data->data = user_data;

  monitor = g_new (AwnVfsMonitor, 1);
	monitor->type = monitor_type;
	
  file = g_file_new_for_path (path);
	
  if (monitor_type == AWN_VFS_MONITOR_FILE) 
    monitor->monitor = g_file_monitor_file (file, 0, NULL, NULL);
  else
  monitor->monitor = g_file_monitor_directory (file, 0, NULL, NULL);

  if (monitor->monitor) 
  {
    g_signal_connect (monitor->monitor, "changed", 
                      G_CALLBACK (g_file_monitor_callback_proxy), data);
  } 
  else 
  {
    g_free (monitor);
    g_free (data);
    monitor = NULL;
  }
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
void 
awn_vfs_monitor_emit (AwnVfsMonitor      *monitor, 
                      gchar              *path, 
                      AwnVfsMonitorEvent  event)
{
  GFile *file;
  GFile *other_file;
  gint   native_event;

	native_event = awn_vfs_monitor_event_type_to_native (event);
	file = g_file_new_for_path (path);
	other_file = file;
	
  g_file_monitor_emit_event (monitor->monitor, file, other_file, 
                             (GFileMonitorEvent)native_event);
}

/**
 * awn_vfs_monitor_remove:
 * @monitor: The structure to remove from monitoring  a path.
 *
 * Removes a monitor structure from monitoring a path.
 */
void 
awn_vfs_monitor_remove (AwnVfsMonitor *monitor)
{
  g_file_monitor_cancel (monitor->monitor);
}

/**
 * awn_vfs_init:
 *
 * Starts up the VFS library routines that Awn uses, if necessary.
 */
void 
awn_vfs_init()
{
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
GSList *
awn_vfs_get_pathlist_from_string (gchar *paths, GError **err)
{
	GSList  *list = NULL;
	gchar  **path_list;
  guint    i, len;
	
  path_list = g_strsplit ((gchar*)paths, "\r\n", 0);
  len = g_strv_length (path_list);
	
  for (i = 0; i < len; i++) 
  {
    list = g_slist_append (list, (gpointer)g_strdup (path_list[i]));
  }
  g_strfreev (path_list);
  *err = NULL;

  return list;
}
/*  vim: set noet ts=2 sts=2 sw=2 : */
