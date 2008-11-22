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

#include <libawn/libawn.h>

#include "task-launcher.h"
#include "task-window.h"

#include "task-settings.h"
#include "xutils.h"

G_DEFINE_TYPE (TaskLauncher, task_launcher, TASK_TYPE_WINDOW);

#define TASK_LAUNCHER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_LAUNCHER, \
  TaskLauncherPrivate))

#define GET_WINDOW_PRIVATE(obj) (TASK_WINDOW(obj)->priv)

struct _TaskLauncherPrivate
{
  gchar          *path;
  AwnDesktopItem *item;

  gchar *name;
  gchar *exec;
  gchar *icon_name;
  gint   pid;
};

enum
{
  PROP_0,
  PROP_DESKTOP_FILE
};

/* Forwards */
static gint          _get_pid         (TaskWindow    *window);
static const gchar * _get_name        (TaskWindow    *window);
static GdkPixbuf   * _get_icon        (TaskWindow    *window);
static gboolean      _is_on_workspace (TaskWindow    *window,
                                       WnckWorkspace *space);
static void          _activate        (TaskWindow    *window,
                                       guint32        timestamp);

static void   task_launcher_set_desktop_file (TaskLauncher *launcher,
                                              const gchar  *path);

/* GObject stuff */
static void
task_launcher_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  TaskLauncher *launcher = TASK_LAUNCHER (object);

  switch (prop_id)
  {
    case PROP_DESKTOP_FILE:
      g_value_set_string (value, launcher->priv->path); 
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_launcher_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  TaskLauncher *launcher = TASK_LAUNCHER (object);

  switch (prop_id)
  {
    case PROP_DESKTOP_FILE:
      task_launcher_set_desktop_file (launcher, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_launcher_class_init (TaskLauncherClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass    *obj_class = G_OBJECT_CLASS (klass);
  TaskWindowClass *win_class = TASK_WINDOW_CLASS (klass);

  obj_class->set_property = task_launcher_set_property;
  obj_class->get_property = task_launcher_get_property;

  /* We implement the necessary funtions for a normal window */
  win_class->get_pid         = _get_pid;
  win_class->get_name        = _get_name;
  win_class->get_icon        = _get_icon;
  win_class->is_on_workspace = _is_on_workspace;
  win_class->activate        = _activate;


  /* Install properties */
  pspec = g_param_spec_string ("desktopfile",
                               "DesktopFile",
                               "Desktop File Path",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_DESKTOP_FILE, pspec);
  
  g_type_class_add_private (obj_class, sizeof (TaskLauncherPrivate));
}

static void
task_launcher_init (TaskLauncher *launcher)
{
  TaskLauncherPrivate *priv;
  	
  priv = launcher->priv = TASK_LAUNCHER_GET_PRIVATE (launcher);
  
  priv->path = NULL;
  priv->item = NULL;
}

TaskLauncher   * 
task_launcher_new_for_desktop_file (const gchar *path)
{
  TaskLauncher *win = NULL;

  if (!g_file_test (path, G_FILE_TEST_EXISTS))
    return NULL;

  win = g_object_new (TASK_TYPE_LAUNCHER,
                      "desktopfile", path,
                      NULL);

  return win;
}

static void
task_launcher_set_desktop_file (TaskLauncher *launcher, const gchar *path)
{
  TaskLauncherPrivate *priv;
 
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));
  priv = launcher->priv;

  priv->path = g_strdup (path);

  priv->item = awn_desktop_item_new (priv->path);

  if (priv->item == NULL)
    return;

  priv->name = awn_desktop_item_get_name (priv->item);
  priv->exec = awn_desktop_item_get_exec (priv->item);
  priv->icon_name = awn_desktop_item_get_string (priv->item, "Icon");

  g_debug ("LAUNCHER: %s\n", priv->name);
}

/*
 * Implemented functions for a standard window without a launcher
 */
static gint 
_get_pid (TaskWindow *window)
{
  TaskLauncher *launcher = TASK_LAUNCHER (window);
  
  if (WNCK_IS_WINDOW (window->priv->window))
  {
    return wnck_window_get_pid (window->priv->window);
  }
  else
  {
    return launcher->priv->pid;
  }
}

static const gchar * 
_get_name (TaskWindow    *window)
{
  TaskLauncher *launcher = TASK_LAUNCHER (window);

  if (WNCK_IS_WINDOW (window->priv->window))
  {
    return wnck_window_get_name (window->priv->window);
  }
  else
  {
    return launcher->priv->name;
  }
}

static GdkPixbuf * 
_get_icon (TaskWindow    *window)
{
  TaskLauncher *launcher = TASK_LAUNCHER (window);
  TaskSettings *s = task_settings_get_default ();

  if (WNCK_IS_WINDOW (window->priv->window))
  {
    return _wnck_get_icon_at_size (window->priv->window, 
                                   s->panel_size, s->panel_size);
  }
  else
  {
    return xutils_get_named_icon (launcher->priv->icon_name,
                                  s->panel_size, s->panel_size);
  }
}

static gboolean 
_is_on_workspace (TaskWindow *window,
                  WnckWorkspace *space)
{
  return TRUE;
}

/*
 * Lifted from libwnck/tasklist.c
 */
static void
really_activate (WnckWindow *window, guint32 timestamp)
{
  WnckWindowState  state;
  WnckWorkspace   *active_ws;
  WnckWorkspace   *window_ws;
  WnckScreen      *screen;
  gboolean         switch_workspace_on_unminimize = FALSE;

  screen = wnck_window_get_screen (window);
  state = wnck_window_get_state (window);
  active_ws = wnck_screen_get_active_workspace (screen);
  window_ws = wnck_window_get_workspace (window);
	
  if (state & WNCK_WINDOW_STATE_MINIMIZED)
  {
    if (window_ws &&
        active_ws != window_ws &&
        !switch_workspace_on_unminimize)
      wnck_workspace_activate (window_ws, timestamp);

    wnck_window_activate_transient (window, timestamp);
  }
  else
  {
    if ((wnck_window_is_active (window) ||
         wnck_window_transient_is_most_recently_activated (window)) &&
         (!window_ws || active_ws == window_ws))
    {
      wnck_window_minimize (window);
      return;
    }
    else
    {
      /* FIXME: THIS IS SICK AND WRONG AND BUGGY. See the end of
       * http://mail.gnome.org/archives/wm-spec-list/005-July/msg0003.html
       * There should only be *one* activate call.
       */
      if (window_ws)
        wnck_workspace_activate (window_ws, timestamp);
     
      wnck_window_activate_transient (window, timestamp);
    }
  } 
}

static void   
_activate (TaskWindow    *window,
           guint32        timestamp)
{
  TaskWindowPrivate *priv = window->priv;
  TaskLauncher *launcher = TASK_LAUNCHER (window);
  GSList       *w;

  /* FIXME: If the window(s) needed attention, we need to implement support
   * for the user selecting what we do (normalactivate, move to workspace or
   * move window to this workspace)
   */
  if (WNCK_IS_WINDOW (priv->window))
  {
    really_activate (priv->window, timestamp);
    wnck_window_activate_transient (priv->window, timestamp);
  }
  else
  {
    GError *error = NULL;
    launcher->priv->pid = awn_desktop_item_launch (launcher->priv->item,
                                                   NULL, &error);

    if (error)
    {
      g_warning ("Unable to launch %s: %s", 
                 launcher->priv->name,
                 error->message);
      g_error_free (error);
    }
    return;
  }

  for (w = priv->utilities; w; w = w->next)
  {
    WnckWindow *win = w->data;

    if (WNCK_IS_WINDOW (win))
      wnck_window_activate_transient (win, timestamp);
  }
}
