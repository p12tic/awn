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

#include "task-icon.h"

G_DEFINE_TYPE (TaskIcon, task_icon, AWN_TYPE_ICON);

#define TASK_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ICON, \
  TaskIconPrivate))

struct _TaskIconPrivate
{
  WnckScreen *screen;
};

/* GObject stuff */
static void
task_icon_class_init (TaskIconClass *klass)
{
  GObjectClass        *obj_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (obj_class, sizeof (TaskIconPrivate));
}

static void
task_icon_init (TaskIcon *icon)
{
  TaskIconPrivate *priv;
  	
  priv = icon->priv = TASK_ICON_GET_PRIVATE (icon);
}

GtkWidget *
task_icon_new (TaskWindow *window)
{
  GtkWidget *icon = NULL;

  icon = g_object_new (TASK_TYPE_ICON,
                       "window", "window",
                       NULL);
  return icon;
}
