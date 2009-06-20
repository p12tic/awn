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
 * Authored by Hannes Verschore <hv1989@gmail.com>
 *
 */

#ifndef _TASK_DRAG_INDICATOR_H_
#define _TASK_DRAG_INDICATOR_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libawn/libawn.h>

#define TASK_TYPE_DRAG_INDICATOR (task_drag_indicator_get_type ())

#define TASK_DRAG_INDICATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TASK_TYPE_DRAG_INDICATOR, TaskDragIndicator))

#define TASK_DRAG_INDICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TASK_TYPE_DRAG_INDICATOR, TaskDragIndicatorClass))

#define TASK_IS_DRAG_INDICATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TASK_TYPE_DRAG_INDICATOR))

#define TASK_IS_DRAG_INDICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TASK_TYPE_DRAG_INDICATOR))

#define TASK_DRAG_INDICATOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TASK_TYPE_DRAG_INDICATOR, TaskDragIndicatorClass))

typedef struct _TaskDragIndicator        TaskDragIndicator;
typedef struct _TaskDragIndicatorClass   TaskDragIndicatorClass;
 
struct _TaskDragIndicator
{
  AwnIcon        parent;
};

struct _TaskDragIndicatorClass
{
  AwnIconClass   parent_class;

  /*< signals >*/
  void (*dest_drag_motion) (TaskDragIndicator *icon);
  void (*dest_drag_leave) (TaskDragIndicator *icon);
};

GType       task_drag_indicator_get_type        (void) G_GNUC_CONST;

GtkWidget * task_drag_indicator_new  ();

void task_drag_indicator_refresh (TaskDragIndicator      *drag_indicator);

#endif /* _TASK_DRAG_INDICATOR_H_ */

