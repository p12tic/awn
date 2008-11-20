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

#include "task-settings.h"
#include "xutils.h"

G_DEFINE_TYPE (TaskWindow, task_window, G_TYPE_OBJECT);

#define TASK_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_WINDOW, \
  TaskWindowPrivate))

struct _TaskWindowPrivate
{
  WnckWindow *window;
  GSList     *utilities;

  /* Properties */
  gchar   *message;
  gfloat   progress;
  gboolean hidden;
};

enum
{
  PROP_0,
  PROP_WINDOW
};

/* Forwards */
gint          _get_pid         (TaskWindow    *window);
const gchar * _get_name        (TaskWindow    *window);
GdkPixbuf   * _get_icon        (TaskWindow    *window);
void          _set_icon        (TaskWindow    *window,
                                GdkPixbuf     *pixbuf);
gboolean      _is_on_workspace (TaskWindow    *window,
                                  WnckWorkspace *space);
void          _activate        (TaskWindow    *window,
                                guint32        timestamp);

static void   task_window_set_window (TaskWindow *window,
                                      WnckWindow *wnckwin);

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
      task_window_set_window (taskwin, g_value_get_object (value));
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

  klass->get_pid         = _get_pid;
  klass->get_name        = _get_name;
  klass->get_icon        = _get_icon;
  klass->set_icon        = _set_icon;
  klass->is_on_workspace = _is_on_workspace;
  klass->activate        = _activate;

  /* Install properties first */
  pspec = g_param_spec_object ("taskwindow",
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

  priv->message = NULL;
  priv->progress = 0;
  priv->hidden = FALSE;
}

TaskWindow *
task_window_new (WnckWindow *window)
{
  TaskWindow *win = NULL;

  win = g_object_new (TASK_TYPE_WINDOW,
                         "taskwindow", window,
                         NULL);

  return win;
}

/*
 * Handling of the main WnckWindow
 */
static void
window_closed (TaskWindow *window, WnckWindow *old_window)
{
  g_object_unref (G_OBJECT (window));
}

static void
task_window_set_window (TaskWindow *window, WnckWindow *wnckwin)
{
  TaskWindowPrivate *priv;

  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  priv->window = wnckwin;

  g_object_weak_ref (G_OBJECT (priv->window), 
                     (GWeakNotify)window_closed, window);
}

/*
 * Public functions
 */
gulong 
task_window_get_xid (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), 0);

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_xid (window->priv->window);
  
  return 0;
}

gint 
task_window_get_pid (TaskWindow    *window)
{
  TaskWindowClass *klass;

  g_return_val_if_fail (TASK_IS_WINDOW (window), 0);
  
  klass = TASK_WINDOW_GET_CLASS (window);
  g_return_val_if_fail (klass->get_pid, 0);

  return klass->get_pid (window);
}

gboolean   
task_window_get_wm_class (TaskWindow    *window,
                          gchar        **res_name,
                          gchar        **class_name)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), -1);
 
  *res_name = NULL;
  *class_name = NULL;
  
  if (WNCK_IS_WINDOW (window->priv->window))
  {
    _wnck_get_wmclass (wnck_window_get_xid (window->priv->window),
                       res_name, class_name);
    
    if (*res_name || *class_name)
      return TRUE;
  }

  return FALSE;
}

const gchar * 
task_window_get_name (TaskWindow    *window)
{
  TaskWindowClass *klass;

  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);
  
  klass = TASK_WINDOW_GET_CLASS (window);
  g_return_val_if_fail (klass->get_name, NULL);

  return klass->get_name (window);
}

GdkPixbuf     * 
task_window_get_icon (TaskWindow    *window)
{
  TaskWindowClass *klass;

  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);
  
  klass = TASK_WINDOW_GET_CLASS (window);
  g_return_val_if_fail (klass->get_icon, NULL);

  return klass->get_icon (window);
}

void
task_window_set_icon (TaskWindow    *window,
                      GdkPixbuf     *pixbuf)
{
  TaskWindowClass *klass;

  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  
  klass = TASK_WINDOW_GET_CLASS (window);
  g_return_if_fail (klass->set_icon);

  return klass->set_icon (window, pixbuf);
}

gboolean   
task_window_is_active (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_is_active (window->priv->window);
  
  return FALSE;
}

gboolean 
task_window_needs_attention (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_or_transient_needs_attention (window->priv->window);

  return FALSE;
}

const gchar * 
task_window_get_message (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);

  return window->priv->message;
}

gfloat   
task_window_get_progress (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), 0.0);

  return window->priv->progress;
}

gboolean
task_window_is_hidden (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  return window->priv->hidden;
}

gboolean   
task_window_is_on_workspace (TaskWindow    *window,
                             WnckWorkspace *space)
{
  TaskWindowClass *klass;

  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), FALSE);

  klass = TASK_WINDOW_GET_CLASS (window);
  g_return_val_if_fail (klass->is_on_workspace, FALSE);

  return klass->is_on_workspace (window, space);
}

void 
task_window_activate (TaskWindow    *window,
                      guint32        timestamp)
{
  TaskWindowClass *klass;

  g_return_if_fail (TASK_IS_WINDOW (window));
  
  klass = TASK_WINDOW_GET_CLASS (window);
  g_return_if_fail (klass->activate);

  klass->activate (window, timestamp);
}

void  
task_window_minimize (TaskWindow    *window)
{
  TaskWindowPrivate *priv;
  GSList *w;

  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  if (WNCK_IS_WINDOW (window->priv->window))
    wnck_window_minimize (window->priv->window);

  /* Minimize utility windows */
  for (w = priv->utilities; w; w = w->next)
  {
    WnckWindow *win = w->data;

    if (WNCK_IS_WINDOW (win))
      wnck_window_minimize (win);
  }
}

void     
task_window_close (TaskWindow    *window,
                   guint32        timestamp)
{
  TaskWindowPrivate *priv;
  GSList *w;

  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  if (WNCK_IS_WINDOW (priv->window))
    wnck_window_close (priv->window, timestamp);

  /* Minimize utility windows */
  for (w = priv->utilities; w; w = w->next)
  {
    WnckWindow *win = w->data;

    if (WNCK_IS_WINDOW (win))
      wnck_window_close (win, timestamp);
  }
}

void    
task_window_set_icon_geometry (TaskWindow    *window,
                               gint           x,
                               gint           y,
                               gint           width,
                               gint           height)
{
  TaskWindowPrivate *priv;
  GSList *w;

  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  /* FIXME: Do something interesting like dividing the width by the number of
   * WnckWindows so the user can scrub through them
   */

  if (WNCK_IS_WINDOW (priv->window))
    wnck_window_set_icon_geometry (priv->window, x, y, width, height);

  /* Minimize utility windows */
  for (w = priv->utilities; w; w = w->next)
  {
    WnckWindow *win = w->data;

    if (WNCK_IS_WINDOW (win))
      wnck_window_set_icon_geometry (win, x, y, width, height);
  }
}


/*
 * Implemented functions for a standard window without a launcher
 */
gint 
_get_pid (TaskWindow    *window)
{
  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_pid (window->priv->window);

  return 0;
}

const gchar * 
_get_name (TaskWindow    *window)
{
  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_name (window->priv->window);

  return NULL;
}

GdkPixbuf * 
_get_icon (TaskWindow    *window)
{
  TaskSettings *s = task_settings_get_default ();

  if (WNCK_IS_WINDOW (window->priv->window))
    return _wnck_get_icon_at_size (window->priv->window, 
                                   s->panel_size, s->panel_size);

  return NULL;
}

void  
_set_icon (TaskWindow    *window,
           GdkPixbuf     *pixbuf)
{
  /* g_signal_emit (window, _window_signals[ICON_CHANGED], 0, pixbuf) */
}

gboolean 
_is_on_workspace (TaskWindow *window,
                  WnckWorkspace *space)
{
  TaskWindowPrivate *priv = window->priv;

  if (WNCK_IS_WINDOW (priv->window))
    return wnck_window_is_in_viewport (priv->window, space);

  return FALSE;
}

void   
_activate (TaskWindow    *window,
           guint32        timestamp)
{
  TaskWindowPrivate *priv = window->priv;
  GSList *w;

  /* FIXME: If the window(s) needed attention, we need to implement support
   * for the user selecting what we do (normalactivate, move to workspace or
   * move window to this workspace)
   */
  if (WNCK_IS_WINDOW (priv->window))
    wnck_window_activate (priv->window, timestamp);

  for (w = priv->utilities; w; w = w->next)
  {
    WnckWindow *win = w->data;

    if (WNCK_IS_WINDOW (win))
      wnck_window_activate (win, timestamp);
  }
}
