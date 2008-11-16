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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>

#include "task-window.h"

G_DEFINE_TYPE (TaskWindow, task_window, G_TYPE_OBJECT);

#define TASK_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_WINDOW, \
  TaskWindowPrivate))

struct _TaskWindowPrivate
{
  WnckWindow *window;
  GList      *utilities;
};

enum
{
  PROP_0,
  PROP_WINDOW
};

/* GObject stuff */
static void
task_window_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  TaskWindow *taskwin = TASK_WINDOW (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, taskwin->priv->window); 
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_window_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  TaskWindow *taskwin = TASK_WINDOW (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      taskwin->priv->window = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_window_constructed (GObject *object)
{
  //TaskWindowPrivate *priv = TASK_WINDOW (object)->priv;

}

static void
task_window_class_init (TaskWindowClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructed  = task_window_constructed;
  obj_class->set_property = task_window_set_property;
  obj_class->get_property = task_window_get_property;

  /* Install properties first */
  pspec = g_param_spec_object ("window",
                               "Window",
                               "WnckWindow",
                               WNCK_TYPE_WINDOW,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_WINDOW, pspec);

  g_type_class_add_private (obj_class, sizeof (TaskWindowPrivate));
}

static void
task_window_init (TaskWindow *window)
{
  TaskWindowPrivate *priv;
  	
  priv = window->priv = TASK_WINDOW_GET_PRIVATE (window);
}

TaskWindow *
task_window_new (WnckWindow *window)
{
  TaskWindow *win = NULL;

  window = g_object_new (TASK_TYPE_WINDOW,
                         "window", window,
                         NULL);

  return win;
}
