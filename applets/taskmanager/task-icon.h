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
 */

#ifndef _TASK_ICON_H_
#define _TASK_ICON_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libawn/libawn.h>

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
  AwnThemedIcon        parent;	

  TaskIconPrivate *priv;
};

struct _TaskIconClass
{
  AwnThemedIconClass   parent_class;

  /*< vtable, not signals >*/
  
  /*< signals >*/
  void (*visible_changed) (TaskIcon *icon);
  void (*source_drag_fail) (TaskIcon *icon);
  void (*source_drag_begin) (TaskIcon *icon);
  void (*source_drag_end) (TaskIcon *icon);
  void (*dest_drag_motion) (TaskIcon *icon);
  void (*dest_drag_leave) (TaskIcon *icon);
};

// circular dependency :/ 
#include "task-item.h"
#include "task-window.h"
#include "task-launcher.h"

GType           task_icon_get_type          (void) G_GNUC_CONST;

GtkWidget*      task_icon_new               (AwnApplet     *applet);

gboolean        task_icon_is_visible        (TaskIcon      *icon);
gboolean        task_icon_contains_launcher (TaskIcon      *icon);
TaskItem  *     task_icon_get_launcher      (TaskIcon      *icon);

guint           task_icon_count_items       (TaskIcon      *icon);
guint           task_icon_count_ephemeral_items (TaskIcon * icon);
guint           task_icon_count_tasklist_windows (TaskIcon * icon);

void            task_icon_increment_ephemeral_count (TaskIcon *icon);


void            task_icon_append_item       (TaskIcon      *icon,
                                             TaskItem      *item);
void            task_icon_append_ephemeral_item (TaskIcon      *icon,
                                            TaskItem      *item);

void            task_icon_remove_item       (TaskIcon      *icon,
                                             TaskItem      *item);
guint           task_icon_match_item        (TaskIcon      *icon,
                                             TaskItem      *item);

//void            task_icon_remove_windows  (TaskIcon      *icon);

void            task_icon_refresh_icon      (TaskIcon      *icon,guint size);

void            task_icon_set_draggable     (TaskIcon      *icon, 
                                             gboolean       draggable);

GSList *        task_icon_get_items         (TaskIcon     *icon);

GtkWidget *     task_icon_get_dialog        (TaskIcon     *icon);

void            task_icon_set_inhibit_focus_loss (TaskIcon *icon, gboolean val);

void            task_icon_schedule_geometry_refresh (TaskIcon *icon);

void            task_icon_moving_item       (TaskIcon *dest, TaskIcon * src, TaskItem *item);
#endif /* _TASK_ICON_H_ */

