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

#include "task-item.h"
#include "util.h"

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
  TaskItem        parent;	

  TaskWindowPrivate *priv;
};

struct _TaskWindowClass
{
  TaskItemClass   parent_class;

  /*< vtable >*/

  /*< signals >*/
  void (*active_changed)    (TaskWindow *window, gboolean       is_active);
  void (*needs_attention)   (TaskWindow *window, gboolean       needs_attention);
  void (*workspace_changed) (TaskWindow *window, WnckWorkspace *space);

  void (*message_changed)   (TaskWindow *window, const gchar   *message);
  void (*progress_changed)  (TaskWindow *window, gfloat         progress);
  void (*hidden_changed)    (TaskWindow *window, gboolean       hidden);
  void (*running_changed)   (TaskWindow *window, gboolean       is_running);
};

GType           task_window_get_type          (void) G_GNUC_CONST;

TaskItem      * task_window_new               (AwnApplet * applet,
                                               GObject * proxy,
                                               WnckWindow    *window);

const gchar   * task_window_get_name          (TaskWindow    *window);

WnckScreen    * task_window_get_screen        (TaskWindow    *window);

gulong          task_window_get_xid           (TaskWindow    *window);

gint            task_window_get_pid           (TaskWindow    *window);

gboolean        task_window_get_wm_class      (TaskWindow    *window,
                                               gchar        **res_name,
                                               gchar        **class_name);

gboolean        task_window_get_wm_client     (TaskWindow    *window,
                                               gchar        **client_name);

WnckApplication*task_window_get_application   (TaskWindow    *window);
WnckWindow*     task_window_get_window        (TaskWindow    *window);

gboolean        task_window_is_active         (TaskWindow    *window);
void            task_window_set_is_active     (TaskWindow    *window,
                                               gboolean       is_active);

gboolean        task_window_get_needs_attention (TaskWindow    *window);

const gchar   * task_window_get_message       (TaskWindow    *window);

gfloat          task_window_get_progress      (TaskWindow    *window);

gboolean        task_window_is_visible        (TaskWindow    *window);

gboolean        task_window_is_hidden         (TaskWindow    *window);

void            task_window_set_active_workspace   (TaskWindow    *window,
                                                    WnckWorkspace *space);

gboolean        task_window_is_on_workspace   (TaskWindow    *window,
                                               WnckWorkspace *space);

void            task_window_activate          (TaskWindow    *window,
                                               guint32        timestamp);

void            task_window_minimize          (TaskWindow    *window);

void            task_window_close             (TaskWindow    *window,
                                               guint32        timestamp);

GtkWidget*      task_window_popup_context_menu(TaskWindow     *window,
                                               GdkEventButton *event);

void            task_window_set_icon_geometry (TaskWindow     *window,
                                               gint            x,
                                               gint            y,
                                               gint            width,
                                               gint            height);

gboolean        task_window_get_is_running      (TaskWindow     *window);

gboolean        task_window_matches_wmclass     (TaskWindow * task_window,
                                                 const gchar * name);
WinIconUse      task_window_get_use_win_icon    (TaskWindow * window);

WinIconUse      task_window_get_icon_changes    (TaskWindow * window);

void            task_window_set_use_win_icon    (TaskWindow * item, WinIconUse win_use);

void            task_window_set_hidden          (TaskWindow *window,gboolean hidden);

void            task_window_set_highlighted   (TaskWindow *window, gboolean highlight_state);

const gchar *   task_window_get_client_name     (TaskWindow *window);

gboolean        task_window_get_icon_is_fallback  (TaskWindow * window);

#endif /* _TASK_WINDOW_H_ */

