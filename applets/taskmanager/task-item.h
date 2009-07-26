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

#ifndef _TASK_ITEM_H_
#define _TASK_ITEM_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TASK_TYPE_ITEM (task_item_get_type ())

#define TASK_ITEM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TASK_TYPE_ITEM, TaskItem))

#define TASK_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TASK_TYPE_ITEM, TaskItemClass))

#define TASK_IS_ITEM(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TASK_TYPE_ITEM))

#define TASK_IS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TASK_TYPE_ITEM))

#define TASK_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TASK_TYPE_ITEM, TaskItemClass))

typedef struct _TaskItem        TaskItem;
typedef struct _TaskItemClass   TaskItemClass;
typedef struct _TaskItemPrivate TaskItemPrivate;

struct _TaskItem
{
  GtkButton     parent;

  TaskItemPrivate *priv;
};

struct _TaskItemClass
{
  GtkButtonClass   parent_class;

  /*< vtable, not signals >*/
  const gchar * (*get_name)        (TaskItem *item);
  GdkPixbuf   * (*get_icon)        (TaskItem *item);
  gboolean      (*is_visible)      (TaskItem *item);
  void          (*left_click)      (TaskItem *item, GdkEventButton *event);
  void          (*right_click)     (TaskItem *item, GdkEventButton *event);
  void          (*middle_click)     (TaskItem *item, GdkEventButton *event);
  guint         (*match)           (TaskItem *item, TaskItem *item_to_match);
  void          (*name_change)     (TaskItem *item, const gchar *name);

  /*< signals >*/
  void (*name_changed)      (TaskItem *item, const gchar *name);
  void (*icon_changed)      (TaskItem *item, GdkPixbuf   *icon);
  void (*visible_changed)   (TaskItem *item, gboolean     visible);
};

GType           task_item_get_type        (void) G_GNUC_CONST;

const gchar * task_item_get_name      (TaskItem *item);
GdkPixbuf   * task_item_get_icon      (TaskItem *item);
gboolean      task_item_is_visible    (TaskItem *item);
void          task_item_left_click    (TaskItem *item, GdkEventButton *event);
void          task_item_right_click   (TaskItem *item, GdkEventButton *event);
void          task_item_middle_click   (TaskItem *item, GdkEventButton *event);
guint         task_item_match         (TaskItem *item, TaskItem *item_to_match);

//TODO: 2nd round: implement
//const gchar   * task_item_get_name          (TaskItem    *item);
//void            task_item_set_name          (TaskItem    *item,
//                                             const gchar   *name);
//GdkPixbuf     * task_item_get_icon          (TaskItem    *item);
//void            task_item_update_icon       (TaskItem    *item,
//                                             GdkPixbuf     *pixbuf);

/* These should be "protected" (used only by derived classes) */
void task_item_emit_name_changed    (TaskItem *item, const gchar *name);
void task_item_emit_icon_changed    (TaskItem *item, GdkPixbuf   *icon);
void task_item_emit_visible_changed (TaskItem *item, gboolean     visible);

G_END_DECLS

#endif /* _TASK_ITEM_H_ */
