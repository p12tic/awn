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

#include "task-icon.h"
#include "task-launcher-icon.h"
#include "task-window.h"

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
on_window_state_changed (WnckWindow      *window,
                         WnckWindowState  changed_mask,
                         WnckWindowState  new_state,
                         TaskManager     *manager)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  /* This signal is only connected for windows which were of type normal/utility
   * and were initially "skip-tasklist". If they are not skip-tasklist anymore
   * we treat them as newly opened windows
   */
  if (!wnck_window_is_skip_tasklist (window))
  {
    on_window_opened (NULL, window, manager);
    return;
  }
}

static gboolean
try_to_place_window (TaskManager *manager, WnckWindow *window)
{
  /* FIXME: */
  return FALSE;
}

static gboolean
try_to_match_window_to_launcher (TaskManager *manager, TaskWindow *window)
{
  /* FIXME: */
  return FALSE;
}

static gboolean
try_to_match_window_to_sn_context (TaskManager *mananger, TaskWindow *window)
{
  /* FIXME: */
  return FALSE;
}

static void 
on_window_opened (WnckScreen    *screen, 
                  WnckWindow    *window,
                  TaskManager   *manager)
{
  TaskManagerPrivate *priv;
  TaskWindow         *taskwin;
  WnckWindowType      type;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  g_return_if_fail (WNCK_IS_WINDOW (window));
  priv = manager->priv;

  type = wnck_window_get_window_type (window);

  switch (type)
  {
    case WNCK_WINDOW_DESKTOP:
    case WNCK_WINDOW_DOCK:
    case WNCK_WINDOW_TOOLBAR:
    case WNCK_WINDOW_MENU:
    case WNCK_WINDOW_SPLASHSCREEN:
      return; /* No need to worry about these */

    default:
      break;
  }

  /* 
   * If it's skip tasklist, connect to the state-changed signal and see if
   * it ever becomes a normal window
   */
  if (wnck_window_is_skip_tasklist (window))
  {
    g_signal_connect (window, "state-changed", 
                      G_CALLBACK (on_window_state_changed), manager);
    return;
  }

  /*
   * If it's a utility window, see if we can find it a home with another, 
   * existing TaskWindow, so we don't have a ton of icons for no reason
   */
  if (type == WNCK_WINDOW_UTILITY && try_to_place_window (manager, window))
  {
    g_debug ("WINDOW PLACED: %s\n", wnck_window_get_name (window));
    return;
  }

  /* 
   * We couldn't append the window to a pre-existing TaskWindow, so we'll need
   * to make a new one
   */
  taskwin = task_window_new (window);
 
  /* Okay, time to check the launchers if we can get a match */
  if (try_to_match_window_to_launcher (manager, taskwin))
  {
    g_debug ("WINDOW MATCHED: %s\n", wnck_window_get_name (window));
    return;
  }

  /* Try the startup-notification windows */
  if (try_to_match_window_to_sn_context (manager, taskwin))
  {
    g_debug ("WINDOW STARTUP: %s\n", wnck_window_get_name (window));
    return;
  }

  /* If we've come this far, the window deserves a spot on the task-manager!
   * Time to create a TaskIcon for it
   */
  
  g_debug ("WINDOW OPENED: %s\n", wnck_window_get_name (window));
}

static void 
on_active_window_changed (WnckScreen    *screen, 
                          WnckWindow    *old_window,
                          TaskManager   *taskman)
{
  g_debug ("ACTIVE_WINDOW_CHANGED\n");
}

static void 
on_active_workspace_changed (WnckScreen    *screen, 
                             WnckWorkspace *old_workspace,
                             TaskManager   *taskman)
{
  g_debug ("ACTIVE_WORKSPACE_CHANGED\n");
}

static void 
on_viewports_changed (WnckScreen    *screen,
                      TaskManager   *taskman)
{
  g_debug ("VIEWPORTS_CHANGED\n");
}
