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

#ifndef _TASK_LAUNCHER_ICON_H_
#define _TASK_LAUNCHER_ICON_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "task-icon.h"

#define TASK_TYPE_LAUNCHER_ICON (task_launcher_icon_get_type ())

#define TASK_LAUNCHER_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TASK_TYPE_LAUNCHER_ICON, TaskLauncherIcon))

#define TASK_LAUNCHER_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TASK_TYPE_LAUNCHER_ICON, TaskLauncherIconClass))

#define TASK_IS_LAUNCHER_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TASK_TYPE_LAUNCHER_ICON))

#define TASK_IS_LAUNCHER_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TASK_TYPE_LAUNCHER_ICON))

#define TASK_LAUNCHER_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TASK_TYPE_LAUNCHER_ICON, TaskLauncherIconClass))

typedef struct _TaskLauncherIcon        TaskLauncherIcon;
typedef struct _TaskLauncherIconClass   TaskLauncherIconClass;
typedef struct _TaskLauncherIconPrivate TaskLauncherIconPrivate;
 
struct _TaskLauncherIcon
{
  TaskIcon        parent;	

  TaskLauncherIconPrivate *priv;
};

struct _TaskLauncherIconClass
{
  TaskIconClass   parent_class;
};

GType       task_launcher_icon_get_type (void) G_GNUC_CONST;

GtkWidget * task_launcher_icon_new      (const gchar *desktop_path);


#endif /* _TASK_LAUNCHER_ICON_H_ */

