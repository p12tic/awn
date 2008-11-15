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

#ifndef _TASK_WINDOW_H_
#define _TASK_WINDOW_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libwnck/libwnck.h>

#define TASK_TYPE_WINDOW (task_window_get_type ())

#define TASK_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TASK_TYPE_WINDOW, TaskWindow))

#define TASK_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TASK_TYPE_WINDOW, TaskWindowClass))

#define TASK_IS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TASK_TYPE_WINDOW))

#define TASK_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TASK_TYPE_WINDOW))

#define TASK_WINDOW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TASK_TYPE_WINDOW, TaskWindowClass))

typedef struct _TaskWindow        TaskWindow;
typedef struct _TaskWindowClass   TaskWindowClass;
typedef struct _TaskWindowPrivate TaskWindowPrivate;
 
struct _TaskWindow
{
  GObject        parent;	

  TaskWindowPrivate *priv;
};

struct _TaskWindowClass
{
  GObjectClass   parent_class;
};

GType        task_window_get_type    (void) G_GNUC_CONST;

TaskWindow * task_window_new (WnckWindow *window);

#endif /* _TASK_WINDOW_H_ */

