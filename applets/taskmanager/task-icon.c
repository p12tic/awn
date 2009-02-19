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
#include "task-settings.h"

G_DEFINE_TYPE (TaskIcon, task_icon, AWN_TYPE_ICON);

#define TASK_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ICON, \
  TaskIconPrivate))

struct _TaskIconPrivate
{
  GSList *windows;
  GdkPixbuf *icon;

  gboolean gets_dragged;

  guint    drag_tag;
  gboolean drag_motion;
  guint    drag_time;

  gint old_width;
  gint old_height;
  gint old_x;
  gint old_y;

  guint update_geometry_id;
};

enum
{
  PROP_0,

  PROP_WINDOW
};

enum
{
  ENSURE_LAYOUT,
  DRAG_STARTED,
  DRAG_ENDED,
  DRAG_MOVE,

  LAST_SIGNAL
};
static guint32 _icon_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry drop_types[] = 
{
  { "STRING", 0, 0 },
  { "text/plain", 0,  },
  { "text/uri-list", 0, 0 },
  { "awn/task-icon", 0, 0 }
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);

enum {
        TARGET_TASK_ICON
};

static const GtkTargetEntry task_icon_type[] = 
{
  { "awn/task-icon", 0, TARGET_TASK_ICON }
};
static const gint n_task_icon_type = G_N_ELEMENTS (task_icon_type);

/* Forwards */

static gboolean  task_icon_configure_event      (GtkWidget          *widget, 
                                                 GdkEventConfigure  *event);
static gboolean  task_icon_button_release_event (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_icon_button_press_event   (GtkWidget      *widget,
                                                 GdkEventButton *event);
static void      task_icon_drag_begin           (GtkWidget      *widget,
                                                 GdkDragContext *context);
static void      task_icon_drag_end             (GtkWidget      *widget,
                                                 GdkDragContext *context);
static gboolean  task_icon_drag_failed          (GtkWidget      *widget,
                                                 GdkDragContext *drag_context,
                                                 GtkDragResult   result);
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
static void      task_icon_drag_data_get        (GtkWidget *widget, 
                                                 GdkDragContext *context, 
                                                 GtkSelectionData *selection_data,
                                                 guint target_type, 
                                                 guint time);


static gboolean _update_geometry(GtkWidget *widget);

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

  if(priv->update_geometry_id)
    g_source_remove(priv->update_geometry_id);

  G_OBJECT_CLASS (task_icon_parent_class)->finalize (object);
}

static void
task_icon_constructed (GObject *object)
{
  TaskIconPrivate *priv = TASK_ICON (object)->priv;
  GtkWidget *widget = GTK_WIDGET(object);
 
  priv->update_geometry_id = g_timeout_add_seconds (1, (GSourceFunc)_update_geometry, widget);
}
 
static gboolean 
_update_geometry(GtkWidget *widget)
{
  gint x,y;
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);

  priv = TASK_ICON (widget)->priv;

  gdk_window_get_origin (widget->window, &x, &y);
  if(priv->old_x != x || priv->old_y != y)
  {
    priv->old_x = x;
    priv->old_y = y;
    task_icon_refresh_geometry (TASK_ICON (widget));
  }

  return TRUE;
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

  wid_class->configure_event      = task_icon_configure_event;
  wid_class->button_release_event = task_icon_button_release_event;
  wid_class->button_press_event   = task_icon_button_press_event;
  wid_class->drag_begin           = task_icon_drag_begin;
  wid_class->drag_end             = task_icon_drag_end;
  wid_class->drag_motion          = task_icon_drag_motion;
  wid_class->drag_leave           = task_icon_drag_leave;
  wid_class->drag_data_received   = task_icon_drag_data_received;
  wid_class->drag_data_get        = task_icon_drag_data_get;

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
  _icon_signals[DRAG_STARTED] =
		g_signal_new ("drag-started",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, drag_started),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, 
			      G_TYPE_NONE, 0);
  _icon_signals[DRAG_ENDED] =
		g_signal_new ("drag-ended",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, drag_ended),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, 
			      G_TYPE_NONE, 0);
  _icon_signals[DRAG_MOVE] =
		g_signal_new ("drag-move",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, drag_move),
			      NULL, NULL,
			      gtk_marshal_NONE__INT_INT, 
			      G_TYPE_NONE, 2,
            G_TYPE_INT, G_TYPE_INT);

  g_type_class_add_private (obj_class, sizeof (TaskIconPrivate));
}

static void
task_icon_init (TaskIcon *icon)
{
  TaskIconPrivate *priv;
  	
  priv = icon->priv = TASK_ICON_GET_PRIVATE (icon);

  priv->icon = NULL;
  priv->windows = NULL;
  priv->drag_tag = 0;
  priv->drag_motion = FALSE;
  priv->gets_dragged = FALSE;
  priv->update_geometry_id = 0;

  awn_icon_set_orientation (AWN_ICON (icon), AWN_ORIENTATION_BOTTOM);

  /* D&D accept dragged objs */
  gtk_widget_add_events (GTK_WIDGET (icon), GDK_ALL_EVENTS_MASK);
  gtk_drag_dest_set (GTK_WIDGET (icon), 
                     GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                     drop_types, n_drop_types,
                     GDK_ACTION_COPY|GDK_ACTION_LINK);
  gtk_drag_dest_add_uri_targets (GTK_WIDGET (icon));
  gtk_drag_dest_add_text_targets (GTK_WIDGET (icon));

  /* D&D support dragging itself */
  gtk_drag_source_set (GTK_WIDGET (icon),
                       GDK_BUTTON1_MASK,
                       task_icon_type, n_task_icon_type,
                       GDK_ACTION_LINK);
  g_signal_connect (G_OBJECT (icon), "drag-failed",
                    G_CALLBACK (task_icon_drag_failed), NULL);

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

  /*if (TASK_IS_LAUNCHER_WINDOW (icon->priv->windows->data))
    return FALSE;*/

  if (icon->priv->windows)
    return task_window_is_hidden (icon->priv->windows->data);

  return FALSE;
}

gboolean  
task_icon_is_in_viewport (TaskIcon *icon, WnckWorkspace *space)
{
  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);

  /*if (TASK_IS_LAUNCHER_WINDOW (icon->priv->windows->data))
    return TRUE;*/

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
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));

  priv = icon->priv;
  g_object_unref (priv->icon);
  g_object_ref(pixbuf);

  priv->icon = pixbuf;
  awn_icon_set_from_pixbuf (AWN_ICON (icon), priv->icon);
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

static void
on_window_running_changed (TaskWindow *window, 
                           gboolean    is_running,
                           TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));
  awn_icon_set_is_running (AWN_ICON (icon), is_running);
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
    priv->icon = task_window_get_icon (window);
    awn_icon_set_from_pixbuf (AWN_ICON (icon), priv->icon);

    awn_icon_set_tooltip_text (AWN_ICON (icon), task_window_get_name (window));
    on_window_needs_attention_changed (window, 
                                       task_window_needs_attention (window), 
                                       icon);

    awn_icon_set_is_running (AWN_ICON (icon), 
                        task_window_get_is_running (window));

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
    g_signal_connect (window, "running-changed",
                      G_CALLBACK (on_window_running_changed), icon);
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
    /* For now do it this way ?! */
    if (TASK_IS_LAUNCHER (priv->windows->data))
      return TRUE;
  }
  return FALSE;
}

TaskLauncher* 
task_icon_get_launcher (TaskIcon      *icon)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);
  priv = icon->priv;

  if (priv->windows)
  {
    /* For now do it this way ?! */
    if (TASK_IS_LAUNCHER (priv->windows->data))
      return TASK_LAUNCHER(priv->windows->data);
  }
  return NULL;
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

gboolean
task_icon_refresh_geometry (TaskIcon *icon)
{
  TaskSettings *settings;
  TaskIconPrivate *priv;
  GtkWidget *widget;
  GSList    *w;
  gint      x, y, ww, width, height;
  gint      i = 0, len = 0;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);
  priv = icon->priv;

  widget = GTK_WIDGET (icon);

  //get the position of the widget
  gdk_window_get_origin (widget->window, &x, &y);

  settings = task_settings_get_default ();

  switch (settings->orient)
  {
    case AWN_ORIENTATION_RIGHT:
      x += settings->panel_size;
      ww = GTK_WIDGET (icon)->allocation.height;
      break;
    case AWN_ORIENTATION_LEFT:
      //x += settings->offset;
      ww = GTK_WIDGET (icon)->allocation.height;
      break;
    case AWN_ORIENTATION_TOP:
      //y += settings->offset;
      ww = GTK_WIDGET (icon)->allocation.width;
      break;
    default:
      y += settings->panel_size;
      ww = GTK_WIDGET (icon)->allocation.width;
      break;
  }

  /* FIXME: Do something clever here to allow the user to "scrub" the icon
   * for the windows.
   */
  len = g_slist_length (priv->windows);
  ww = ww/len;
  for (w = priv->windows; w; w = w->next)
  {
    TaskWindow *window = w->data;
    switch (settings->orient)
    {
      case AWN_ORIENTATION_RIGHT:
        width = settings->panel_size+settings->offset;
        height = ww + (i*ww);
        break;
      case AWN_ORIENTATION_LEFT:
        width = settings->panel_size+settings->offset;
        height = ww + (i*ww);
        break;
      case AWN_ORIENTATION_TOP:
        width = ww + (i*ww);
        height = settings->panel_size+settings->offset;
        break;
      default:
        width = ww + (i*ww);
        height = settings->panel_size+settings->offset;
        break;
    }
    task_window_set_icon_geometry (window, x, y, 
                                   width,
                                   height);
    i++;
  }

  return FALSE;
}

/*
 * Widget callbacks
 */
static gboolean
task_icon_configure_event (GtkWidget          *widget,
                           GdkEventConfigure  *event)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);
  priv = TASK_ICON (widget)->priv;

  if (priv->old_width == event->width && priv->old_height == event->height)
    return FALSE;

  priv->old_width = event->width;
  priv->old_height = event->height;

  g_idle_add ((GSourceFunc)task_icon_refresh_geometry, TASK_ICON (widget));

  return TRUE;
}

static gboolean
task_icon_button_release_event (GtkWidget      *widget,
                                GdkEventButton *event)
{
  TaskIconPrivate *priv;
  /*GSList *w;*/
  gint len;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);
  priv = TASK_ICON (widget)->priv;

  len = g_slist_length (priv->windows);

  if (event->button == 1)
  {
    if(priv->gets_dragged)
    {
      task_icon_drag_end (widget, NULL);
      return FALSE;
    }

    if (len == 1)
    {
      task_window_activate (priv->windows->data, event->time);
      return TRUE;
    }
  }
  else if (event->button == 2)
  {
    if (len == 1 && TASK_IS_LAUNCHER (priv->windows->data))
    {
      task_launcher_middle_click (priv->windows->data, event);
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

static void
task_icon_drag_begin (GtkWidget      *widget,
                      GdkDragContext *context)
{
  TaskIconPrivate *priv;
  TaskSettings *settings;

  g_return_if_fail (TASK_IS_ICON (widget));

  priv = TASK_ICON (widget)->priv;
  priv->gets_dragged = TRUE;

  settings = task_settings_get_default ();

  gtk_drag_set_icon_pixbuf (context, priv->icon, settings->panel_size/2, settings->panel_size/2);

  g_signal_emit (TASK_ICON (widget), _icon_signals[DRAG_STARTED], 0);
}

static void
task_icon_drag_end (GtkWidget      *widget,
                    GdkDragContext *context)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (widget));

  priv = TASK_ICON (widget)->priv;
  priv->gets_dragged = FALSE;

  g_signal_emit (TASK_ICON (widget), _icon_signals[DRAG_ENDED], 0);
}

static gboolean
task_icon_drag_failed (GtkWidget      *widget,
                       GdkDragContext *drag_context,
                       GtkDragResult   result)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);

  priv = TASK_ICON (widget)->priv;
  priv->gets_dragged = FALSE;

  g_signal_emit (TASK_ICON (widget), _icon_signals[DRAG_ENDED], 0);

  return TRUE;
}

static gboolean
task_icon_drag_motion (GtkWidget      *widget,
                       GdkDragContext *context,
                       gint            x,
                       gint            y,
                       guint           t)
{
  TaskIconPrivate *priv;
  GdkAtom target;
  gchar *target_name;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);
  priv = TASK_ICON (widget)->priv;

  target = gtk_drag_dest_find_target (widget, context, NULL);
  target_name = gdk_atom_name (target);

  if (g_strcmp0("awn/task-icon", target_name) == 0)
  {
    g_signal_emit (TASK_ICON (widget), _icon_signals[DRAG_MOVE], 0, x, y);
    return FALSE;
  }
  else
  {
    if (priv->drag_tag)
      return TRUE;

    priv->drag_motion = TRUE;
    priv->drag_tag = g_timeout_add_seconds (1, (GSourceFunc)drag_timeout, widget);
    priv->drag_time = t;

    return FALSE;
  }
}

static void   
task_icon_drag_leave (GtkWidget      *widget,
                      GdkDragContext *context,
                      guint           time)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (widget));
  priv = TASK_ICON (widget)->priv;

  if(priv->drag_motion)
  {
    priv->drag_motion = FALSE;
    g_source_remove (priv->drag_tag);
    priv->drag_tag = 0;
  }
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
  GdkAtom         target;
  gchar           *target_name;

  g_return_if_fail (TASK_IS_ICON (widget));
  priv = TASK_ICON (widget)->priv;

  target = gtk_drag_dest_find_target (widget, context, NULL);
  target_name = gdk_atom_name (target);

  /* If it is dragging of the task icon, there is actually no data */
  if (g_strcmp0("awn/task-icon", target_name) == 0)
  {
    gtk_drag_finish (context, TRUE, TRUE, time);
    return;
  }

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

static void
task_icon_drag_data_get (GtkWidget *widget, 
                         GdkDragContext *context, 
                         GtkSelectionData *selection_data,
                         guint target_type, 
                         guint time)
{
  switch(target_type)
  {
    case TARGET_TASK_ICON:
      gtk_selection_data_set (selection_data, GDK_TARGET_STRING, 8, NULL, 0);
      break;
    default:
      /* Default to some a safe target instead of fail. */
      g_assert_not_reached ();
  }
}
