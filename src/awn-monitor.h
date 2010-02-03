/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifndef	_AWN_MONITOR_H
#define	_AWN_MONITOR_H

#include <glib.h>
#include <gtk/gtk.h>

#include <libdesktop-agnostic/config.h>

G_BEGIN_DECLS

#define AWN_TYPE_MONITOR (awn_monitor_get_type())

#define AWN_MONITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_MONITOR, \
  AwnMonitor))

#define AWN_MONITOR_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_MONITOR, \
  AwnMonitorClass))

#define AWN_IS_MONITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_MONITOR))

#define AWN_IS_MONITOR_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
  AWN_TYPE_MONITOR))

#define AWN_MONITOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
  AWN_TYPE_MONITOR, AwnMonitorClass))

typedef struct _AwnMonitor AwnMonitor;
typedef struct _AwnMonitorClass AwnMonitorClass;
typedef struct _AwnMonitorPrivate AwnMonitorPrivate;

struct _AwnMonitor {
  GObject parent;

  gint   width;
  gint   height;
  gint   x_offset;
  gint   y_offset;
  gfloat align;

  /*< private >*/
  AwnMonitorPrivate *priv;
};

struct _AwnMonitorClass {
  GObjectClass parent_class;

  /*< signals >*/
  void (*geometry_changed) (AwnMonitor *monitor);
};

GType awn_monitor_get_type(void) G_GNUC_CONST;


AwnMonitor * awn_monitor_new_from_config (DesktopAgnosticConfigClient *client);

G_END_DECLS


#endif /* _AWN_MONITOR_H */

