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
  GSList *windows;
};

enum
{
  PROP_0,

  PROP_WINDOW
};

/* Forwards */
static gboolean  task_icon_button_release_event (GtkWidget      *widget,
                                                 GdkEventButton *event);

/* GObject stuff */
static void
task_icon_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  TaskIcon        *icon = TASK_ICON (object);
  TaskIconPrivate *priv = icon->priv;

  switch (prop_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, 
                          priv->windows ? priv->windows->data : NULL); 
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_icon_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  TaskIcon *icon = TASK_ICON (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      task_icon_append_window (icon, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_icon_constructed (GObject *object)
{
  //TaskWindowPrivate *priv = TASK_WINDOW (object)->priv;

}

static void
task_icon_class_init (TaskIconClass *klass)
{
  GParamSpec     *pspec;
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->constructed  = task_icon_constructed;
  obj_class->set_property = task_icon_set_property;
  obj_class->get_property = task_icon_get_property;

  wid_class->button_release_event = task_icon_button_release_event;
  
  /* Install properties first */
  pspec = g_param_spec_object ("taskwindow",
                               "TaskWindow",
                               "TaskWindow",
                               TASK_TYPE_WINDOW,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_WINDOW, pspec);

  g_type_class_add_private (obj_class, sizeof (TaskIconPrivate));
}

static void
task_icon_init (TaskIcon *icon)
{
  TaskIconPrivate *priv;
  	
  priv = icon->priv = TASK_ICON_GET_PRIVATE (icon);

  priv->windows = NULL;

  awn_icon_set_orientation (AWN_ICON (icon), AWN_ORIENTATION_BOTTOM);
}

GtkWidget *
task_icon_new_for_window (TaskWindow *window)
{
  GtkWidget *icon = NULL;

  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);

  icon = g_object_new (TASK_TYPE_ICON,
                       "taskwindow", window,
                       NULL);
  return icon;
}

/*
 * Public Functions
 */
gboolean 
task_icon_is_skip_taskbar (TaskIcon *icon)
{
  return FALSE;
}

gboolean  
task_icon_is_in_viewport (TaskIcon *icon, WnckWorkspace *space)
{
  return TRUE;
}

static void
window_closed (TaskIcon *icon, TaskWindow *old_window)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_ICON (icon));
  priv = icon->priv;

  priv->windows = g_slist_remove (priv->windows, old_window);

  if (g_slist_length (priv->windows) == 0)
  {
    gtk_widget_destroy (GTK_WIDGET (icon));
  }
  else
  {
    /* Load up with new icon etc */
  }
}

void
task_icon_append_window (TaskIcon      *icon,
                         TaskWindow    *window)
{
  TaskIconPrivate *priv;
  gboolean first_window = FALSE;

  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = icon->priv;

  /* Is this the first, main, window of this icon? */
  if (priv->windows == NULL)
    first_window = TRUE;
  
  priv->windows = g_slist_append (priv->windows, window);
  g_object_weak_ref (G_OBJECT (window), (GWeakNotify)window_closed, icon);

  /* If it's the first window, let's set-up our icon accordingly */
  if (first_window)
  {
    awn_icon_set_from_pixbuf (AWN_ICON (icon), task_window_get_icon (window));
    awn_icon_set_tooltip_text (AWN_ICON (icon), task_window_get_name (window));
  }
}

gboolean 
task_icon_is_launcher (TaskIcon      *icon)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);
  priv = icon->priv;

  if (priv->windows)
  {
    //if (TASK_IS_LAUNCHER_WINDOW (priv->windows->data))
    //return TRUE;
  }
  return FALSE;
}

void
task_icon_refresh_icon (TaskIcon      *icon)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (icon));
  priv = icon->priv;

  if (priv->windows && priv->windows->data)
    awn_icon_set_from_pixbuf (AWN_ICON (icon), 
                              task_window_get_icon (priv->windows->data));
}

void
task_icon_refresh_geometry (TaskIcon *icon)
{
  TaskIconPrivate *priv;
  GtkWidget *widget;
  GSList    *w;
  gint      x, y, ww;
  gint      i = 0, len = 0;

  g_return_if_fail (TASK_IS_ICON (icon));
  priv = icon->priv;

  widget = GTK_WIDGET (icon);
  gdk_window_get_origin (widget->window, &x, &y);

  /* FIXME: Do something clever here to allow the user to "scrub" the icon
   * for the windows.
   */
  len = g_slist_length (priv->windows);
  ww = widget->allocation.width/len;
  for (w = priv->windows; w; w = w->next)
  {
    TaskWindow *window = w->data;

    task_window_set_icon_geometry (window, x, y, 
                                   ww + (i*ww),
                                   widget->allocation.height);
    i++;
  }
}

/*
 * Widget callbacks
 */
static gboolean
task_icon_button_release_event (GtkWidget      *widget,
                                GdkEventButton *event)
{
  TaskIconPrivate *priv;
  //GSList *w;
  gint len;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);
  priv = TASK_ICON (widget)->priv;

  if (event->button == 1)
  {
    len = g_slist_length (priv->windows);
    if (len == 1)
    {
      TaskWindow *window = priv->windows->data;

      if (task_window_is_active (window))
        task_window_minimize (window);
      else
        task_window_activate (window, event->time);

      return TRUE;
    }
  }
  return FALSE;
}
