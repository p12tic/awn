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
 *             Hannes Verschore <hv1989@gmail.com>
 *
 */

#ifndef _TASK_LAUNCHER_H_
#define _TASK_LAUNCHER_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libwnck/libwnck.h>

#include "task-item.h"

#define TASK_TYPE_LAUNCHER (task_launcher_get_type ())

#define TASK_LAUNCHER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TASK_TYPE_LAUNCHER, TaskLauncher))

#define TASK_LAUNCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TASK_TYPE_LAUNCHER, TaskLauncherClass))

#define TASK_IS_LAUNCHER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TASK_TYPE_LAUNCHER))

#define TASK_IS_LAUNCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TASK_TYPE_LAUNCHER))

#define TASK_LAUNCHER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TASK_TYPE_LAUNCHER, TaskLauncherClass))

typedef struct _TaskLauncher        TaskLauncher;
typedef struct _TaskLauncherClass   TaskLauncherClass;
typedef struct _TaskLauncherPrivate TaskLauncherPrivate;
 
struct _TaskLauncher
{
  TaskItem        parent;	

  TaskLauncherPrivate *priv;
};

struct _TaskLauncherClass
{
  TaskItemClass   parent_class;

  /*< signals >*/
};

GType           task_launcher_get_type             (void) G_GNUC_CONST;

TaskItem      * task_launcher_new_for_desktop_file (const gchar    *path);

const gchar   * task_launcher_get_desktop_path     (TaskLauncher   *launcher);

void            task_launcher_launch_with_data     (TaskLauncher   *launcher,
                                                    GSList         *list);


#endif /* _TASK_LAUNCHER_H_ */

