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
#include "task-launcher.h"
#include "task-settings.h"
#include "xutils.h"

G_DEFINE_TYPE (TaskWindow, task_window, G_TYPE_OBJECT);

enum
{
  PROP_0,
  PROP_WINDOW
};

enum
{
  NAME_CHANGED,
  ICON_CHANGED,
  ACTIVE_CHANGED,
  NEEDS_ATTENTION,
  WORKSPACE_CHANGED,
  MESSAGE_CHANGED,
  PROGRESS_CHANGED,
  HIDDEN_CHANGED,
  RUNNING_CHANGED,

  LAST_SIGNAL
};
static guint32 _window_signals[LAST_SIGNAL] = { 0 };

/* Forwards */
static gint          _get_pid         (TaskWindow     *window);
static const gchar * _get_name        (TaskWindow     *window);
static GdkPixbuf   * _get_icon        (TaskWindow     *window);
static gboolean      _is_on_workspace (TaskWindow     *window,
                                       WnckWorkspace  *space);
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

  /* We implement the necessary funtions for a normal window */
  klass->get_pid         = _get_pid;
  klass->get_name        = _get_name;
  klass->get_icon        = _get_icon;
  klass->is_on_workspace = _is_on_workspace;
  klass->activate        = NULL;
  klass->popup_menu      = NULL;

  /* Install signals */
  _window_signals[NAME_CHANGED] =
		g_signal_new ("name-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, name_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING, 
			      G_TYPE_NONE,
            1, G_TYPE_STRING); 

  _window_signals[ICON_CHANGED] =
		g_signal_new ("icon-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, icon_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT, 
			      G_TYPE_NONE,
            1, GDK_TYPE_PIXBUF); 
  
  _window_signals[ACTIVE_CHANGED] =
		g_signal_new ("active-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, active_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN); 

  _window_signals[NEEDS_ATTENTION] =
		g_signal_new ("needs-attention",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, needs_attention),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN);   

  _window_signals[WORKSPACE_CHANGED] =
		g_signal_new ("workspace-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, workspace_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
            1, WNCK_TYPE_WORKSPACE);

  _window_signals[MESSAGE_CHANGED] =
		g_signal_new ("message-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, message_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
            1, G_TYPE_STRING);

  _window_signals[PROGRESS_CHANGED] =
		g_signal_new ("progress-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, progress_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
            1, G_TYPE_FLOAT);

  _window_signals[HIDDEN_CHANGED] =
		g_signal_new ("hidden-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, hidden_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN);

  _window_signals[RUNNING_CHANGED] =
		g_signal_new ("running-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, running_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN);


  /* Install properties */
  pspec = g_param_spec_object ("taskwindow",
                               "Window",
                               "WnckWindow",
                               WNCK_TYPE_WINDOW,
                               G_PARAM_READWRITE);
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
  priv->needs_attention = FALSE;
  priv->is_active = FALSE;
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

void
task_window_append_utility (TaskWindow    *window,
                            WnckWindow    *wnckwin)
{
  TaskWindowPrivate *priv;

  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  priv->utilities = g_slist_append (priv->utilities, wnckwin);
}

/*
 * Handling of the main WnckWindow
 */
static void
window_closed (TaskWindow *window, WnckWindow *old_window)
{
  g_signal_emit (window, _window_signals[RUNNING_CHANGED], 0, FALSE);
  
  if (!TASK_IS_LAUNCHER (window))
    g_object_unref (G_OBJECT (window));
}

static void
on_window_name_changed (WnckWindow *wnckwin, TaskWindow *window)
{
  TaskWindowPrivate *priv;

  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));
  priv = window->priv;

  g_signal_emit (window, _window_signals[NAME_CHANGED], 
                 0, wnck_window_get_name (wnckwin));
}

static void
on_window_icon_changed (WnckWindow *wnckwin, TaskWindow *window)
{
  TaskSettings *s = task_settings_get_default ();
  GdkPixbuf    *pixbuf;

  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));
  
  pixbuf = _wnck_get_icon_at_size (wnckwin, s->panel_size-2, s->panel_size-2);
  task_window_update_icon (window, pixbuf);
  g_object_unref (pixbuf);
}

static void
on_window_workspace_changed (WnckWindow *wnckwin, TaskWindow *window)
{
  TaskWindowPrivate *priv;
    
  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));
  priv = window->priv;
  
  g_signal_emit (window, _window_signals[WORKSPACE_CHANGED], 
                 0, wnck_window_get_workspace (wnckwin));
}

static void
on_window_state_changed (WnckWindow      *wnckwin,
                         WnckWindowState  changed_mask,
                         WnckWindowState  state,
                         TaskWindow      *window)
{
  TaskWindowPrivate *priv;
  gboolean           hidden = FALSE;
  gboolean           needs_attention = FALSE;
    
  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));
  priv = window->priv;

  if (state & WNCK_WINDOW_STATE_SKIP_TASKLIST)
    hidden = TRUE;

  if (priv->hidden != hidden)
  {
    priv->hidden = hidden;
    g_signal_emit (window, _window_signals[HIDDEN_CHANGED], 0, hidden);
  }

  needs_attention = wnck_window_or_transient_needs_attention (wnckwin);

  if (priv->needs_attention != needs_attention)
  {
    priv->needs_attention = needs_attention;
    g_signal_emit (window, _window_signals[NEEDS_ATTENTION], 
                   0, needs_attention);
  }
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

  g_signal_connect (wnckwin, "name-changed", 
                    G_CALLBACK (on_window_name_changed), window);
  g_signal_connect (wnckwin, "icon-changed",
                    G_CALLBACK (on_window_icon_changed), window);
  g_signal_connect (wnckwin, "workspace-changed",
                    G_CALLBACK (on_window_workspace_changed), window);
  g_signal_connect (wnckwin, "state-changed", 
                    G_CALLBACK (on_window_state_changed), window);

  g_signal_emit (window, _window_signals[RUNNING_CHANGED], 0, TRUE);
}

/*
 * Public functions
 */
WnckScreen * 
task_window_get_screen (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), wnck_screen_get_default ());

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_screen (window->priv->window);
  
  return wnck_screen_get_default ();
}

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

	g_return_val_if_fail (TASK_IS_WINDOW (window), -1);
  
  klass = TASK_WINDOW_GET_CLASS (window);
  g_return_val_if_fail (klass->get_pid, -1);
	
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

void
task_window_set_name (TaskWindow    *window,
                      const gchar   *name)
{
  g_return_if_fail (TASK_IS_WINDOW (window));

  g_signal_emit (window, _window_signals[NAME_CHANGED], 0, name);
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
task_window_update_icon (TaskWindow    *window,
                         GdkPixbuf     *pixbuf)
{
  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  
  g_signal_emit (window, _window_signals[ICON_CHANGED], 0, pixbuf);
}

gboolean   
task_window_is_active (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_is_active (window->priv->window);
  
  return FALSE;
}

void 
task_window_set_is_active  (TaskWindow    *window,
                            gboolean       is_active)
{
  g_return_if_fail (TASK_IS_WINDOW (window));

  window->priv->is_active = is_active;

  g_signal_emit (window, _window_signals[ACTIVE_CHANGED], 0, is_active);
}

gboolean 
task_window_needs_attention (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  if (WNCK_IS_WINDOW (window->priv->window))
  {
    window->priv->needs_attention = wnck_window_or_transient_needs_attention 
                              (window->priv->window);
    return window->priv->needs_attention;
  }

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

/*
 * Lifted from libwnck/tasklist.c
 */
static gboolean
really_activate (WnckWindow *window, guint32 timestamp)
{
  WnckWindowState  state;
  WnckWorkspace   *active_ws;
  WnckWorkspace   *window_ws;
  WnckScreen      *screen;
  gboolean         switch_workspace_on_unminimize = FALSE;
  gboolean         was_minimised = FALSE;

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
      was_minimised = TRUE;
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
  return was_minimised;
}

void 
task_window_activate (TaskWindow    *window,
                      guint32        timestamp)
{
  TaskWindowClass *klass;
  TaskWindowPrivate *priv = window->priv;
  GSList *w;

  g_return_if_fail (TASK_IS_WINDOW (window));

  klass = TASK_WINDOW_GET_CLASS (window);

  if (WNCK_IS_WINDOW (priv->window))
  {
    gboolean was_minimised;

    was_minimised = really_activate (priv->window, timestamp);

    for (w = priv->utilities; w; w = w->next)
    {
      WnckWindow *win = w->data;

      if (WNCK_IS_WINDOW (win))
      {
        if (was_minimised)
          wnck_window_minimize (win);
        else
          wnck_window_activate_transient (win, timestamp);
      }
    }
  }
  else
  {
    if (klass->activate)
      klass->activate (window, timestamp);
  }
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
task_window_popup_context_menu (TaskWindow     *window,
                                GdkEventButton *event)
{
  TaskWindowClass *klass;
  TaskWindowPrivate *priv;
  GtkWidget         *menu;

  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (event);
  priv = window->priv;
  
  klass = TASK_WINDOW_GET_CLASS (window);

  /* If we have a valid WnckWindow, then create the normal menu and allow
   * custom entires before showing 
   */
  if (WNCK_IS_WINDOW (priv->window))
  {
    menu = wnck_action_menu_new (priv->window);

    if (klass->popup_menu)
      klass->popup_menu (window, GTK_MENU (menu));
  }
  else if (klass->popup_menu)
  {
    /* Let the instance add any custom options */
    menu = gtk_menu_new ();
    if (klass->popup_menu)
      klass->popup_menu (window, GTK_MENU (menu));  
  }
  else
  {
    return;
  }
  
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 
                  NULL, NULL, event->button, event->time);
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


gboolean    
task_window_get_is_running (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  return WNCK_IS_WINDOW (window->priv->window);
}

/*
 * Implemented functions for a standard window without a launcher
 */
static gint 
_get_pid (TaskWindow    *window)
{
	gint value = -1;
  if (WNCK_IS_WINDOW (window->priv->window))
	{
    value = wnck_window_get_pid (window->priv->window);
		value = value ? value : -1; 		//if the pid is 0 return -1.  Bad wnck! Bad! 		
	}
	return value;  
}

static const gchar * 
_get_name (TaskWindow    *window)
{
  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_name (window->priv->window);

  return NULL;
}

static GdkPixbuf * 
_get_icon (TaskWindow    *window)
{
  TaskSettings *s = task_settings_get_default ();

  if (WNCK_IS_WINDOW (window->priv->window))
    return _wnck_get_icon_at_size (window->priv->window, 
                                   s->panel_size-2, s->panel_size-2);

  return NULL;
}

static gboolean 
_is_on_workspace (TaskWindow *window,
                  WnckWorkspace *space)
{
  TaskWindowPrivate *priv = window->priv;

  if (WNCK_IS_WINDOW (priv->window))
    return wnck_window_is_in_viewport (priv->window, space);

  return FALSE;
}
