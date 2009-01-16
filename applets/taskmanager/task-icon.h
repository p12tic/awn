/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifndef _TASK_ICON_H_
#define _TASK_ICON_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libawn/libawn.h>

#include "task-window.h"
#include "task-launcher.h"

#define TASK_TYPE_ICON (task_icon_get_type ())

#define TASK_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TASK_TYPE_ICON, TaskIcon))

#define TASK_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TASK_TYPE_ICON, TaskIconClass))

#define TASK_IS_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TASK_TYPE_ICON))

#define TASK_IS_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TASK_TYPE_ICON))

#define TASK_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TASK_TYPE_ICON, TaskIconClass))

typedef struct _TaskIcon        TaskIcon;
typedef struct _TaskIconClass   TaskIconClass;
typedef struct _TaskIconPrivate TaskIconPrivate;
 
struct _TaskIcon
{
  AwnIcon        parent;	

  TaskIconPrivate *priv;
};

struct _TaskIconClass
{
  AwnIconClass   parent_class;

  /*< vtable, not signals >*/
  
  /*< signals >*/
  void (*ensure_layout) (TaskIcon *icon);
  void (*drag_started) (TaskIcon *icon);
  void (*drag_ended) (TaskIcon *icon);
  void (*drag_move) (TaskIcon *icon);
};

GType           task_icon_get_type        (void) G_GNUC_CONST;

GtkWidget*      task_icon_new_for_window  (TaskWindow    *window);

gboolean        task_icon_is_skip_taskbar (TaskIcon      *icon);

gboolean        task_icon_is_in_viewport  (TaskIcon      *icon,
                                           WnckWorkspace *space);

void            task_icon_append_window   (TaskIcon      *icon,
                                           TaskWindow    *window);
gboolean        task_icon_is_launcher     (TaskIcon      *icon);
TaskLauncher*   task_icon_get_launcher    (TaskIcon      *icon);

void            task_icon_refresh_icon    (TaskIcon      *icon);

void            task_icon_refresh_geometry (TaskIcon     *icon);

#endif /* _TASK_ICON_H_ */

