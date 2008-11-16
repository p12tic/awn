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

#include "task-manager.h"

G_DEFINE_TYPE (TaskManager, task_manager, AWN_TYPE_APPLET);

#define TASK_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_MANAGER, \
  TaskManagerPrivate))

struct _TaskManagerPrivate
{
  WnckScreen *screen;

  /* This is what the icons are packed into */
  GtkWidget *box;
};

/* Forwards */
static void on_window_opened            (WnckScreen    *screen, 
                                         WnckWindow    *window,
                                         TaskManager   *taskman);
static void on_active_window_changed    (WnckScreen    *screen, 
                                         WnckWindow    *old_window,
                                         TaskManager   *taskman);
static void on_active_workspace_changed (WnckScreen    *screen, 
                                         WnckWorkspace *old_workspace,
                                         TaskManager   *taskman);
static void on_viewports_changed        (WnckScreen    *screen,
                                         TaskManager   *taskman);

/* GObject stuff */
static void
task_manager_constructed (GObject *object)
{
  TaskManagerPrivate *priv;
  GtkWidget          *widget;
  
  priv = TASK_MANAGER_GET_PRIVATE (object);
  widget = GTK_WIDGET (object);

  /* Create the icon box */
  priv->box = awn_icon_box_new ();
  gtk_container_add (GTK_CONTAINER (widget), priv->box);
  gtk_widget_show (priv->box);

  /* Get the WnckScreen and connect to the relevent signals */
  priv->screen = wnck_screen_get_default ();
  g_signal_connect (priv->screen, "window-opened", 
                    G_CALLBACK (on_window_opened), object);
  g_signal_connect (priv->screen, "active-window-changed",  
                    G_CALLBACK (on_active_window_changed), object);
  g_signal_connect (priv->screen, "active-workspace-changed",
                    G_CALLBACK (on_active_workspace_changed), object);
  g_signal_connect (priv->screen, "viewports-changed",
                    G_CALLBACK (on_viewports_changed), object);
}

static void
task_manager_class_init (TaskManagerClass *klass)
{
  GObjectClass        *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructed = task_manager_constructed;

  g_type_class_add_private (obj_class, sizeof (TaskManagerPrivate));
}

static void
task_manager_init (TaskManager *manager)
{
  TaskManagerPrivate *priv;
  	
  priv = manager->priv = TASK_MANAGER_GET_PRIVATE (manager);

  gtk_container_add (GTK_CONTAINER (manager), gtk_label_new ("Hello"));
}

AwnApplet *
task_manager_new (const gchar *uid,
                  gint         orient,
                  gint         size)
{
  static AwnApplet *manager = NULL;

  if (!manager)
    manager = g_object_new (TASK_TYPE_MANAGER,
                            "uid", uid,
                            "orient", orient,
                            "size", size,
                            NULL);

  return manager;
}

/*
 * WNCK_SCREEN CALLBACKS
 */
static void 
on_window_opened (WnckScreen    *screen, 
                  WnckWindow    *window,
                  TaskManager   *taskman)
{
  g_print ("WINDOW OPENED: %s\n", wnck_window_get_name (window));
}

static void 
on_active_window_changed (WnckScreen    *screen, 
                          WnckWindow    *old_window,
                          TaskManager   *taskman)
{
  g_print ("ACTIVE_WINDOW_CHANGED\n");
}

static void 
on_active_workspace_changed (WnckScreen    *screen, 
                             WnckWorkspace *old_workspace,
                             TaskManager   *taskman)
{
  g_print ("ACTIVE_WORKSPACE_CHANGED\n");
}

static void 
on_viewports_changed (WnckScreen    *screen,
                      TaskManager   *taskman)
{
  g_print ("VIEWPORTS_CHANGED\n");
}
