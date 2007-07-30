/*
 * trash-monitor.c: monitor the state of the trash directories.
 *
 * Copyright (C) 1999, 2000 Eazel, Inc.
 * Copyright (C) 2004 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "trash-monitor.h"
#include <string.h>
#include <libgnomevfs/gnome-vfs.h>

struct _TrashMonitor {
  GObject parent;

  GHashTable *volumes;
  gint total_item_count;
  gint notify_id;
};

struct _TrashMonitorClass {
  GObjectClass parent_class;

  void (* item_count_changed) (TrashMonitor *monitor);
};

typedef struct {
  TrashMonitor *monitor;
  GnomeVFSVolume *volume;
  gchar *trash_uri;
  GnomeVFSAsyncHandle *find_handle;
  GnomeVFSMonitorHandle *trash_change_monitor;
  gint item_count;
} VolumeInfo;


static void trash_monitor_init (TrashMonitor *monitor);
static void trash_monitor_class_init (TrashMonitorClass *class);

enum {
  ITEM_COUNT_CHANGED,
  LAST_SIGNAL
};
static GObjectClass *parent_class;
static guint signals[LAST_SIGNAL];


static void volume_mounted_callback (GnomeVFSVolumeMonitor *volume_monitor,
				     GnomeVFSVolume *volume,
				     TrashMonitor *monitor);
static void volume_unmount_started_callback (GnomeVFSVolumeMonitor *volume_monitor,
					     GnomeVFSVolume *volume,
					     TrashMonitor *monitor);

static void add_volume    (TrashMonitor *monitor, GnomeVFSVolume *volume);
static void remove_volume (TrashMonitor *monitor, GnomeVFSVolume *volume);

static void trash_changed_queue_notify (TrashMonitor *monitor);

GType
trash_monitor_get_type (void)
{
  static GType monitor_type = 0;

  if (!monitor_type) {
    static const GTypeInfo type_info = {
      sizeof (TrashMonitorClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) trash_monitor_class_init,
      (GClassFinalizeFunc) NULL,
      NULL,

      sizeof (TrashMonitor),
      0,
      (GInstanceInitFunc) trash_monitor_init
    };
    monitor_type = g_type_register_static (G_TYPE_OBJECT, "TrashMonitor",
					   &type_info, 0);
  }
  return monitor_type;
}

static void
trash_monitor_class_init (TrashMonitorClass *class)
{
  parent_class = g_type_class_peek_parent (class);

  signals[ITEM_COUNT_CHANGED] = g_signal_new
    ("item_count_changed",
     G_TYPE_FROM_CLASS (class),
     G_SIGNAL_RUN_LAST,
     G_STRUCT_OFFSET (TrashMonitorClass, item_count_changed),
     NULL, NULL,
     g_cclosure_marshal_VOID__VOID,
     G_TYPE_NONE, 0);
}

static void
trash_monitor_init (TrashMonitor *monitor)
{
  GnomeVFSVolumeMonitor *volume_monitor;
  GList *volumes, *tmp;

  monitor->volumes = g_hash_table_new (NULL, NULL);
  monitor->total_item_count = 0;
  monitor->notify_id = 0;

  volume_monitor = gnome_vfs_get_volume_monitor ();
  
  g_signal_connect_object (volume_monitor, "volume_mounted",
			   G_CALLBACK (volume_mounted_callback),
			   monitor, 0);
  g_signal_connect_object (volume_monitor, "volume_pre_unmount",
			   G_CALLBACK (volume_unmount_started_callback),
			   monitor, 0);

  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (volume_monitor);
  for (tmp = volumes; tmp != NULL; tmp = tmp->next) {
    GnomeVFSVolume *volume = tmp->data;

    add_volume (monitor, volume);
    gnome_vfs_volume_unref (volume);
  }
  g_list_free (volumes);
}

TrashMonitor *
trash_monitor_get (void)
{
  TrashMonitor *monitor = NULL;

  if (!monitor) {
    monitor = g_object_new (TRASH_TYPE_MONITOR, NULL);
  }
  return monitor;
}

static void
volume_mounted_callback (GnomeVFSVolumeMonitor *volume_monitor,
                         GnomeVFSVolume *volume,
                         TrashMonitor *monitor)
{
  add_volume (monitor, volume);
}

static void
volume_unmount_started_callback (GnomeVFSVolumeMonitor *volume_monitor,
                                 GnomeVFSVolume *volume,
                                 TrashMonitor *monitor)
{
  remove_volume (monitor, volume);
}

static void
trash_dir_changed (GnomeVFSMonitorHandle *handle,
		   const gchar *monitor_uri,
		   const gchar *info_uri,
		   GnomeVFSMonitorEventType type,
		   gpointer user_data)
{
  VolumeInfo *volinfo;
  GnomeVFSResult res;
  GList *dirlist, *tmp;
  gint count = 0;
  
  volinfo = user_data;
  
  res = gnome_vfs_directory_list_load (&dirlist, volinfo->trash_uri,
				       GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
  if (res != GNOME_VFS_OK) {
    g_warning("GNOME VFS Error: %s", gnome_vfs_result_to_string (res));
    return;
  }

  for (tmp = dirlist; tmp != NULL; tmp = tmp->next) {
    GnomeVFSFileInfo *info = tmp->data;

    if (!strcmp (info->name, ".") || !strcmp (info->name, ".."))
      continue;
    count++;
  }
  gnome_vfs_file_info_list_free (dirlist);
  
  if (count != volinfo->item_count) {
    volinfo->item_count = count;
    trash_changed_queue_notify (volinfo->monitor);
  }
}

static void
find_directory_callback (GnomeVFSAsyncHandle *handle,
                         GList *results,
                         gpointer callback_data)
{
  VolumeInfo *volinfo;
  GnomeVFSFindDirectoryResult *result;
  GnomeVFSResult res;

  volinfo = callback_data;

  /* we are done finding the volume */
  volinfo->find_handle = NULL;

  /* If we can't find the trash, ignore it silently. */
  result = results->data;
  if (result->result != GNOME_VFS_OK)
    return;

  volinfo->trash_uri = gnome_vfs_uri_to_string (result->uri,
						GNOME_VFS_URI_HIDE_NONE);
  /* g_message ("found trash dir: %s", volinfo->trash_uri); */

  /* simulate a change to update the directory count */
  trash_dir_changed (NULL, NULL, NULL, GNOME_VFS_MONITOR_EVENT_CHANGED,
		     volinfo);

  res = gnome_vfs_monitor_add (&volinfo->trash_change_monitor,
			       volinfo->trash_uri, GNOME_VFS_MONITOR_DIRECTORY,
			       trash_dir_changed,
			       volinfo);

  if (res != GNOME_VFS_OK) {
    g_warning("GNOME VFS Error: %s", gnome_vfs_result_to_string (res));
    volinfo->trash_change_monitor = NULL;
  }
}

static gboolean
get_trash_volume (TrashMonitor *monitor,
		  GnomeVFSVolume *volume,
		  VolumeInfo **volinfo,
		  GnomeVFSURI **mount_uri)
{
  char *uri_str;

  *volinfo = g_hash_table_lookup (monitor->volumes, volume);

  if (*volinfo != NULL && (*volinfo)->trash_uri != NULL)
    return FALSE;

  if (!gnome_vfs_volume_handles_trash (volume))
    return FALSE;

  uri_str = gnome_vfs_volume_get_activation_uri (volume);
  *mount_uri = gnome_vfs_uri_new (uri_str);
  g_free (uri_str);

  if (*volinfo == NULL) {
    *volinfo = g_new0 (VolumeInfo, 1);
    (*volinfo)->monitor = monitor;
    (*volinfo)->volume = gnome_vfs_volume_ref (volume);
    g_hash_table_insert (monitor->volumes, volume, *volinfo);
  }

  return TRUE;
}

static void
add_volume (TrashMonitor *monitor, GnomeVFSVolume *volume)
{
  VolumeInfo *volinfo;
  GnomeVFSURI *mount_uri;
  GList vfs_uri_as_list;

  if (!get_trash_volume (monitor, volume, &volinfo, &mount_uri))
    return;

  if (volinfo->find_handle) {
    /* already searchinf for trash */
    gnome_vfs_uri_unref (mount_uri);
    return;
  }

  
  vfs_uri_as_list.data = mount_uri;
  vfs_uri_as_list.next = NULL;
  vfs_uri_as_list.prev = NULL;

  gnome_vfs_async_find_directory (&volinfo->find_handle, &vfs_uri_as_list,
				  GNOME_VFS_DIRECTORY_KIND_TRASH,
				  FALSE, TRUE, 0777,
				  GNOME_VFS_PRIORITY_DEFAULT,
				  find_directory_callback, volinfo);
  gnome_vfs_uri_unref (mount_uri);
}

static void
remove_volume (TrashMonitor *monitor, GnomeVFSVolume *volume)
{
  VolumeInfo *volinfo;

  volinfo = g_hash_table_lookup (monitor->volumes, volume);
  if (volinfo != NULL) {
    g_hash_table_remove (monitor->volumes, volume);

    /* g_message ("removing volume %s", volinfo->trash_uri); */
    if (volinfo->find_handle != NULL)
      gnome_vfs_async_cancel (volinfo->find_handle);
    if (volinfo->trash_change_monitor != NULL)
      gnome_vfs_monitor_cancel (volinfo->trash_change_monitor);

    if (volinfo->trash_uri)
      g_free (volinfo->trash_uri);

    /* if this volume contained some trash, then notify that the trash
     * state has changed */
    if (volinfo->item_count != 0)
      trash_changed_queue_notify (monitor);

    gnome_vfs_volume_unref (volinfo->volume);
    g_free (volinfo);
  }
}

/* --- */

static void
readd_volumes (gpointer key, gpointer value, gpointer user_data)
{
  TrashMonitor *monitor = user_data;
  GnomeVFSVolume *volume;

  volume = key;
  add_volume (monitor, volume);
}
void
trash_monitor_recheck_trash_dirs (TrashMonitor *monitor)
{
  /* call add_volume() on each volume, to add trash dirs where missing */
  g_hash_table_foreach (monitor->volumes, readd_volumes, monitor);
}

/* --- */

void
trash_monitor_empty_trash (TrashMonitor *monitor,
			   GnomeVFSAsyncHandle **handle,
			   GnomeVFSAsyncXferProgressCallback func,
			   gpointer user_data)
{
  GList *trash_dirs = NULL, *volumes, *tmp;
  GnomeVFSVolume *volume;
  GnomeVFSURI *mount_uri, *trash_uri;
  gchar *uri_str;

  /* collect the trash directories */
  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (gnome_vfs_get_volume_monitor ());
  for (tmp = volumes; tmp != NULL; tmp = tmp->next) {
    volume = tmp->data;
    if (gnome_vfs_volume_handles_trash (volume)) {
      /* get the mount point for this volume */
      uri_str = gnome_vfs_volume_get_activation_uri (volume);
      mount_uri = gnome_vfs_uri_new (uri_str);
      g_free (uri_str);

      g_assert (mount_uri != NULL);

      /* Look for the trash directory.  Since we tell it not to create or
       * look for the dir, it doesn't block. */
      if (gnome_vfs_find_directory (mount_uri,
				    GNOME_VFS_DIRECTORY_KIND_TRASH, &trash_uri,
				    FALSE, FALSE, 0777) == GNOME_VFS_OK) {
	trash_dirs = g_list_prepend (trash_dirs, trash_uri);
      }
      gnome_vfs_uri_unref (mount_uri);
    }
    gnome_vfs_volume_unref (volume);
  }
  g_list_free (volumes);

  if (trash_dirs != NULL)
    gnome_vfs_async_xfer (handle, trash_dirs, NULL,
			  GNOME_VFS_XFER_EMPTY_DIRECTORIES,
			  GNOME_VFS_XFER_ERROR_MODE_ABORT,
			  GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
			  GNOME_VFS_PRIORITY_DEFAULT,
			  func, user_data, NULL, NULL);
  gnome_vfs_uri_list_free (trash_dirs);
}


/* --- */

static void
count_items (gpointer key, gpointer value, gpointer user_data)
{
  VolumeInfo *volinfo;
  gint *item_count;

  volinfo = value;
  item_count = user_data;
  *item_count += volinfo->item_count;
}

static gboolean
trash_changed_notify (gpointer user_data)
{
  TrashMonitor *monitor = user_data;
  gint item_count;

  /* reset notification id */
  monitor->notify_id = 0;

  /* count the volumes */
  item_count = 0;
  g_hash_table_foreach (monitor->volumes, count_items, &item_count);

  /* if the item count has changed ... */
  if (item_count != monitor->total_item_count) {
    monitor->total_item_count = item_count;
    /* g_message ("item count is %d", item_count); */
    g_signal_emit (monitor, signals[ITEM_COUNT_CHANGED], 0);
  }

  return FALSE;
}

static void
trash_changed_queue_notify (TrashMonitor *monitor)
{
  /* already queued */
  if (monitor->notify_id != 0)
    return;

  monitor->notify_id = g_idle_add (trash_changed_notify, monitor);
}

int
trash_monitor_get_item_count (TrashMonitor *monitor)
{
  return monitor->total_item_count;
}

/* --- */

#ifdef TEST_TRASH_MONITOR
int
main (int argc, char **argv)
{
  TrashMonitor *monitor;

  if (!gnome_vfs_init ()) {
    g_printerr ("Can not initialise gnome-vfs.\n");
    return 1;
  }

  monitor = trash_monitor_get ();

  g_main_loop_run (g_main_loop_new (NULL, FALSE));

  return 0;
}
#endif
