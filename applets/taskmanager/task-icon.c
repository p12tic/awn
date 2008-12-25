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

#include "task-launcher.h"

G_DEFINE_TYPE (TaskIcon, task_icon, AWN_TYPE_ICON);

#define TASK_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ICON, \
  TaskIconPrivate))

struct _TaskIconPrivate
{
  GSList *windows;

  guint    drag_tag;
  gboolean drag_motion;
  guint    drag_time;
};

enum
{
  PROP_0,

  PROP_WINDOW
};

enum
{
  ENSURE_LAYOUT,

  LAST_SIGNAL
};
static guint32 _icon_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry drop_types[] = 
{
  { "STRING", 0, 0 },
  { "text/plain", 0,  },
  { "text/uri-list", 0, 0 }
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);

/* Forwards */
static gboolean  task_icon_button_release_event (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_icon_button_press_event   (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_icon_drag_motion          (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 gint            x,
                                                 gint            y,
                                                 guint           t);
static void      task_icon_drag_leave           (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 guint           time);
static void      task_icon_drag_data_received   (GtkWidget      *widget,
                                                 GdkDragContext *context,
                                                 gint            x,
                                                 gint            y,
                                                 GtkSelectionData *data,
                                                 guint           info,
                                                 guint           time);

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
task_icon_finalize (GObject *object)
{
  TaskIconPrivate *priv = TASK_ICON_GET_PRIVATE (object);

  if (priv->windows)
  {
    g_slist_free (priv->windows);
    priv->windows = NULL;
  }

  G_OBJECT_CLASS (task_icon_parent_class)->finalize (object);
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
  obj_class->finalize     = task_icon_finalize;
  
  wid_class->button_release_event = task_icon_button_release_event;
  wid_class->button_press_event   = task_icon_button_press_event;
  wid_class->drag_motion          = task_icon_drag_motion;
  wid_class->drag_leave           = task_icon_drag_leave;
  wid_class->drag_data_received   = task_icon_drag_data_received;
  
  /* Install properties first */
  pspec = g_param_spec_object ("taskwindow",
                               "TaskWindow",
                               "TaskWindow",
                               TASK_TYPE_WINDOW,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_WINDOW, pspec);

  /* Install signals */
  _icon_signals[ENSURE_LAYOUT] =
		g_signal_new ("ensure-layout",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, ensure_layout),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, 
			      G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (TaskIconPrivate));
}

static void
task_icon_init (TaskIcon *icon)
{
  TaskIconPrivate *priv;
  	
  priv = icon->priv = TASK_ICON_GET_PRIVATE (icon);

  priv->windows = NULL;
  priv->drag_tag = 0;
  priv->drag_motion = FALSE;

  awn_icon_set_orientation (AWN_ICON (icon), AWN_ORIENTATION_BOTTOM);

  /* D&D */
  gtk_widget_add_events (GTK_WIDGET (icon), GDK_ALL_EVENTS_MASK);
  gtk_drag_dest_set (GTK_WIDGET (icon), 
                     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                     drop_types, n_drop_types,
                     GDK_ACTION_COPY);
  gtk_drag_dest_add_uri_targets (GTK_WIDGET (icon));
  gtk_drag_dest_add_text_targets (GTK_WIDGET (icon));
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
  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);

  //if (TASK_IS_LAUNCHER_WINDOW (icon->priv->windows->data))
  //  return FALSE;

  if (icon->priv->windows)
    return task_window_is_hidden (icon->priv->windows->data);

  return FALSE;
}

gboolean  
task_icon_is_in_viewport (TaskIcon *icon, WnckWorkspace *space)
{
  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);

  //if (TASK_IS_LAUNCHER_WINDOW (icon->priv->windows->data))
  //  return TRUE;

  if (icon->priv->windows)
    return task_window_is_on_workspace (icon->priv->windows->data, space);

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

static void
on_window_name_changed (TaskWindow  *window, 
                        const gchar *name, 
                        TaskIcon    *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  awn_icon_set_tooltip_text (AWN_ICON (icon), name);
}

static void
on_window_icon_changed (TaskWindow *window, 
                        GdkPixbuf  *pixbuf, 
                        TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  awn_icon_set_from_pixbuf (AWN_ICON (icon), pixbuf);
}

static void
on_window_active_changed (TaskWindow *window, 
                          gboolean    is_active, 
                          TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  awn_icon_set_is_active (AWN_ICON (icon), is_active);
}

static void
on_window_needs_attention_changed (TaskWindow *window,
                                   gboolean    needs_attention,
                                   TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  if (needs_attention)
    awn_icon_set_effect (AWN_ICON (icon),AWN_EFFECT_ATTENTION);
  else
    awn_effects_stop (awn_icon_get_effects (AWN_ICON (icon)), 
                      AWN_EFFECT_ATTENTION);
}

static void
on_window_workspace_changed (TaskWindow    *window,  
                             WnckWorkspace *space,
                             TaskIcon      *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));
  
  g_signal_emit (icon, _icon_signals[ENSURE_LAYOUT], 0);
}

static void
on_window_message_changed (TaskWindow  *window, 
                           const gchar *message,
                           TaskIcon    *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  awn_icon_set_message (AWN_ICON (icon), message);
}

static void
on_window_progress_changed (TaskWindow *window,
                            gfloat      progress,
                            TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  awn_icon_set_progress (AWN_ICON (icon), progress);
}

static void
on_window_hidden_changed (TaskWindow *window,
                          gboolean    is_hidden,
                          TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  g_signal_emit (icon, _icon_signals[ENSURE_LAYOUT], 0);
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
    GdkPixbuf *pixbuf;

    pixbuf = task_window_get_icon (window);
    awn_icon_set_from_pixbuf (AWN_ICON (icon), pixbuf);
    g_object_unref (pixbuf);

    awn_icon_set_tooltip_text (AWN_ICON (icon), task_window_get_name (window));
    on_window_needs_attention_changed (window, 
                                       task_window_needs_attention (window), 
                                       icon);

    g_signal_connect (window, "name-changed", 
                      G_CALLBACK (on_window_name_changed), icon);
    g_signal_connect (window, "icon-changed",
                      G_CALLBACK (on_window_icon_changed), icon);
    g_signal_connect (window, "active-changed",
                      G_CALLBACK (on_window_active_changed), icon);
    g_signal_connect (window, "needs-attention",
                      G_CALLBACK (on_window_needs_attention_changed), icon);
    g_signal_connect (window, "workspace-changed",
                      G_CALLBACK (on_window_workspace_changed), icon);
    g_signal_connect (window, "message-changed",
                      G_CALLBACK (on_window_message_changed), icon);
    g_signal_connect (window, "progress-changed",
                      G_CALLBACK (on_window_progress_changed), icon);
    g_signal_connect (window, "hidden-changed",
                      G_CALLBACK (on_window_hidden_changed), icon);
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
      task_window_activate (priv->windows->data, event->time);
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean  
task_icon_button_press_event (GtkWidget      *widget,
                              GdkEventButton *event)
{
  TaskIconPrivate *priv;
  guint len;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);
  priv = TASK_ICON (widget)->priv;

  if (event->button != 3)
    return FALSE;

  len = g_slist_length (priv->windows);

  if (len == 1)
  {
    /* We can just ask the window to popup as normal */
    task_window_popup_context_menu (priv->windows->data, event);
    return TRUE;
  }
  else
  {
    g_warning ("TaskIcon: FIXME: No support for multiple windows right-click");
  }
  return FALSE;
}

static gboolean
drag_timeout (TaskIcon *icon)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);
  priv = icon->priv;

  if (priv->drag_motion == FALSE)
    return FALSE;
  else if (priv->windows->data)
    task_window_activate (priv->windows->data, priv->drag_time);

  return FALSE;
}

static gboolean
task_icon_drag_motion (GtkWidget      *widget,
                       GdkDragContext *context,
                       gint            x,
                       gint            y,
                       guint           t)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);
  priv = TASK_ICON (widget)->priv;

  if (priv->drag_tag)
    return TRUE;

  priv->drag_motion = TRUE;
  priv->drag_tag = g_timeout_add_seconds (1, (GSourceFunc)drag_timeout, widget);
  priv->drag_time = t;

  return FALSE;
}

static void   
task_icon_drag_leave (GtkWidget      *widget,
                      GdkDragContext *context,
                      guint           time)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (widget));
  priv = TASK_ICON (widget)->priv;

  priv->drag_motion = FALSE;
  g_source_remove (priv->drag_tag);
  priv->drag_tag = 0;
}

static void
task_icon_drag_data_received (GtkWidget      *widget,
                              GdkDragContext *context,
                              gint            x,
                              gint            y,
                              GtkSelectionData *sdata,
                              guint           info,
                              guint           time)
{
  TaskIconPrivate *priv;
  GSList          *list;
  GError          *error;
  TaskLauncher    *launcher;

  g_return_if_fail (TASK_IS_ICON (widget));
  priv = TASK_ICON (widget)->priv;

  /* If we are not a launcher, we don't care about this */
  if (!priv->windows || !TASK_IS_LAUNCHER (priv->windows->data))
  {
    gtk_drag_finish (context, FALSE, FALSE, time);
    return;
  }

  launcher = priv->windows->data;
  
  /* If we are dealing with a desktop file, then we want to do something else
   * FIXME: This is a crude way of checking
   * FIXME: Emit a signal or something to let the manager know that the user
   * dropped a desktop file
   */
  if (strstr ((gchar*)sdata->data, ".desktop"))
  {
    /*g_signal_emit (icon, _icon_signals[DESKTOP_DROPPED],
     *               0, sdata->data);
     */
    gtk_drag_finish (context, TRUE, FALSE, time);
  }

  /* We don't handle drops if the launcher already has a window associcated */
  if (task_launcher_has_window (launcher))
  {
    gtk_drag_finish (context, FALSE, FALSE, time);
  }
  
  error = NULL;
  list = awn_vfs_get_pathlist_from_string ((gchar*)sdata->data, &error);
  if (error)
  {
    g_warning ("Unable to handle drop: %s", error->message);
    g_error_free (error);
    gtk_drag_finish (context, FALSE, FALSE, time);
    return;
  }

  task_launcher_launch_with_data (launcher, list);

  g_slist_foreach (list, (GFunc)g_free, NULL);
  g_slist_free (list);

  gtk_drag_finish (context, TRUE, FALSE, time);
}
