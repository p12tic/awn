/*
 * trash-monitor.h: monitor the state of the trash directories.
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

#ifndef __TRASH_MONITOR_H__
#define __TRASH_MONITOR_H__

#include <libgnomevfs/gnome-vfs.h>

typedef struct _TrashMonitor TrashMonitor;
typedef struct _TrashMonitorClass TrashMonitorClass;

#define TRASH_TYPE_MONITOR (trash_monitor_get_type ())
#define TRASH_MONITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TRASH_TYPE_MONITOR, TrashMonitor))
#define TRASH_MONITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TRASH_TYPE_MONITOR, TrashMonitorClass))
#define TRASH_IS_MONITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TRASH_TYPE_MONITOR))
#define TRASH_IS_MONITOR_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), TRASH_TYPE_MONITOR))
#define TRASH_MONITOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TRASH_TYPE_MONITOR, TrashMonitorClass))

GType         trash_monitor_get_type  (void);
TrashMonitor *trash_monitor_get       (void);

/* check if new trash dirs have been created */
void trash_monitor_recheck_trash_dirs (TrashMonitor *monitor);
void trash_monitor_empty_trash        (TrashMonitor *monitor,
				       GnomeVFSAsyncHandle **handle,
				       GnomeVFSAsyncXferProgressCallback func,
				       gpointer user_data);
int  trash_monitor_get_item_count     (TrashMonitor *monitor);

#endif
