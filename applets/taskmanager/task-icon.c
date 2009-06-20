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
 *             Hannes Verschore <hv1989@gmail.com>
 *
 */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>

#include "libawn/gseal-transition.h"

#include "taskmanager-marshal.h"
#include "task-icon.h"

#include "task-launcher.h"
#include "task-settings.h"

G_DEFINE_TYPE (TaskIcon, task_icon, AWN_TYPE_ICON)

#define TASK_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ICON, \
  TaskIconPrivate))

struct _TaskIconPrivate
{
  //List containing the TaskItems
  GSList *items;

  //The number of TaskItems that get shown
  guint shown_items;

  //The number of TaskWindows (subclass of TaskItem) that needs attention
  guint needs_attention;

  //The number of TaskWindows (subclass of TaskItem) that have the active state.
  guint is_active;

  TaskItem *main_item;

  GdkPixbuf *icon;
  GtkWidget *dialog;

  gboolean draggable;
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

  PROP_DRAGGABLE
};

enum
{
  ENSURE_LAYOUT,

  SOURCE_DRAG_FAIL,
  SOURCE_DRAG_BEGIN,
  SOURCE_DRAG_END,

  DEST_DRAG_MOVE,
  DEST_DRAG_LEAVE,

  LAST_SIGNAL
};
static guint32 _icon_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry drop_types[] = 
{
  { (gchar*)"STRING", 0, 0 },
  { (gchar*)"text/plain", 0,  },
  { (gchar*)"text/uri-list", 0, 0 },
  { (gchar*)"awn/task-icon", 0, 0 }
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);

enum {
        TARGET_TASK_ICON
};

static const GtkTargetEntry task_icon_type[] = 
{
  { (gchar*)"awn/task-icon", 0, TARGET_TASK_ICON }
};
static const gint n_task_icon_type = G_N_ELEMENTS (task_icon_type);

/* Forwards */

static gboolean  task_icon_configure_event      (GtkWidget          *widget, 
                                                 GdkEventConfigure  *event);
static gboolean  task_icon_button_release_event (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_icon_button_press_event   (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_icon_dialog_unfocus       (GtkWidget      *widget,
                                                GdkEventFocus   *event,
                                                gpointer        null);
/* Dnd forwards */
static void      task_icon_drag_data_get        (GtkWidget *widget, 
                                                 GdkDragContext *context, 
                                                 GtkSelectionData *selection_data,
                                                 guint target_type, 
                                                 guint time);

/* DnD 'source' forwards */

static gboolean  task_icon_source_drag_fail     (GtkWidget      *widget,
                                                 GdkDragContext *drag_context,
                                                 GtkDragResult   result);
static void      task_icon_source_drag_begin    (GtkWidget      *widget,
                                                 GdkDragContext *context);
static void      task_icon_source_drag_end      (GtkWidget      *widget,
                                                 GdkDragContext *context);

/* DnD 'destination' forwards */
static gboolean  task_icon_dest_drag_motion         (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           t);
static void      task_icon_dest_drag_leave          (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     guint           time);
static void      task_icon_dest_drag_data_received  (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     GtkSelectionData *data,
                                                     guint           info,
                                                     guint           time);

static gboolean _update_geometry(GtkWidget *widget);
static gboolean task_icon_refresh_geometry (TaskIcon *icon);
static void     task_icon_refresh_visible  (TaskIcon *icon);

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
    case PROP_DRAGGABLE:
      g_value_set_boolean (value, priv->draggable); 
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
    case PROP_DRAGGABLE:
      task_icon_set_draggable (icon, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

/**
 * Finalize the object and remove the list of windows, 
 * the list of launchers and the timer of update_geometry.
 */
static void
task_icon_dispose (GObject *object)
{
  TaskIconPrivate *priv = TASK_ICON_GET_PRIVATE (object);
  /*this needs to be done in dispose, not finalize, due to idiosyncracies of 
   AwnDialog*/
  if (priv->dialog)
  {
    gtk_widget_destroy (priv->dialog);
    priv->dialog = NULL;
  }
  G_OBJECT_CLASS (task_icon_parent_class)->dispose (object);  
}

static void
task_icon_finalize (GObject *object)
{
  TaskIconPrivate *priv = TASK_ICON_GET_PRIVATE (object);

  /* FIXME  Check to see if icon needs to be unreffed */
  if (priv->items)
  {
    g_slist_free (priv->items);
    priv->items = NULL;
  }
  if(priv->update_geometry_id)
  {
    g_source_remove(priv->update_geometry_id);
  }

  G_OBJECT_CLASS (task_icon_parent_class)->finalize (object);
}

static void
task_icon_constructed (GObject *object)
{
  TaskIconPrivate *priv = TASK_ICON (object)->priv;
  GtkWidget *widget = GTK_WIDGET(object);
 
  //update geometry of icon every second.
  priv->update_geometry_id = g_timeout_add_seconds (1, (GSourceFunc)_update_geometry, widget);
}

/**
 * Checks if the position of the widget has changed.
 * Upon change it asks the icon to refresh.
 * returns: TRUE when succeeds
 *          FALSE when widget isn't an icon
 */
static gboolean 
_update_geometry(GtkWidget *widget)
{
  return TRUE; //TODO solve
  
  gint x,y;
  TaskIconPrivate *priv;
  GdkWindow *win;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);

  priv = TASK_ICON (widget)->priv;

  win = gtk_widget_get_window (widget);
  gdk_window_get_origin (win, &x, &y);

  if(priv->old_x != x || priv->old_y != y)
  {
    priv->old_x = x;
    priv->old_y = y;
    task_icon_refresh_geometry (TASK_ICON (widget));
  }

  return TRUE;
}

/**
 * Set the icon geometry of the windows in a task-icon.
 * This equals to the minimize position of the window.
 * TODO: not done (part2)
 */
static gboolean
task_icon_refresh_geometry (TaskIcon *icon)
{
  TaskSettings *settings;
  TaskIconPrivate *priv;
  GtkWidget *widget;
  GdkWindow *win;
  GSList    *w;
  gint      x, y, ww, width, height;
  gint      i = 0, len = 0;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);

  priv = icon->priv;
  widget = GTK_WIDGET (icon);

  //get the position of the widget
  win = gtk_widget_get_window (widget);
  gdk_window_get_origin (win, &x, &y);

  settings = task_settings_get_default ();

  switch (settings->orient)
  {
    case AWN_ORIENTATION_RIGHT:
    case AWN_ORIENTATION_LEFT:
      ww = GTK_WIDGET (icon)->allocation.height;
      break;
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      ww = GTK_WIDGET (icon)->allocation.width;
      break;
    default:
      g_error ("Orientation isn't right, left, top, bottom ??");
      break;
  }

  /* FIXME: Do something clever here to allow the user to "scrub" the icon
   * for the windows.
   */
  len = g_slist_length (priv->items);
  ww = ww/len;
  for (w = priv->items; w; w = w->next)
  {
    if (!TASK_IS_WINDOW (w->data)) continue;

    TaskWindow *window = TASK_WINDOW (w->data);

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

static void
task_icon_class_init (TaskIconClass *klass)
{
  GParamSpec     *pspec;
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->constructed  = task_icon_constructed;
  obj_class->set_property = task_icon_set_property;
  obj_class->get_property = task_icon_get_property;
  obj_class->dispose     = task_icon_dispose;
  obj_class->finalize     = task_icon_finalize;

  wid_class->configure_event      = task_icon_configure_event;
  wid_class->button_release_event = task_icon_button_release_event;
  wid_class->button_press_event   = task_icon_button_press_event;
  wid_class->drag_begin           = task_icon_source_drag_begin;
  wid_class->drag_end             = task_icon_source_drag_end;
  wid_class->drag_data_get        = task_icon_drag_data_get;
  wid_class->drag_motion          = task_icon_dest_drag_motion;
  wid_class->drag_leave           = task_icon_dest_drag_leave;
  wid_class->drag_data_received   = task_icon_dest_drag_data_received;

  /* Install properties first */
  pspec = g_param_spec_boolean ("draggable",
                                "Draggable",
                                "TaskIcon is draggable?",
                                TRUE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_DRAGGABLE, pspec);

  /* Install signals */
  _icon_signals[ENSURE_LAYOUT] =
		g_signal_new ("ensure-layout",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, ensure_layout),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, 
			      G_TYPE_NONE, 0);
  _icon_signals[SOURCE_DRAG_BEGIN] =
		g_signal_new ("source-drag-begin",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, source_drag_begin),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, 
			      G_TYPE_NONE, 0);
  _icon_signals[SOURCE_DRAG_FAIL] =
		g_signal_new ("source-drag-fail",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, source_drag_fail),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, 
			      G_TYPE_NONE, 0);
  _icon_signals[SOURCE_DRAG_END] =
		g_signal_new ("source-drag-end",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, source_drag_end),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID, 
			      G_TYPE_NONE, 0);
  _icon_signals[DEST_DRAG_MOVE] =
		g_signal_new ("dest-drag-motion",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, dest_drag_motion),
			      NULL, NULL,
			      taskmanager_marshal_VOID__INT_INT, 
			      G_TYPE_NONE, 2,
            G_TYPE_INT, G_TYPE_INT);
  _icon_signals[DEST_DRAG_LEAVE] =
		g_signal_new ("dest-drag-leave",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, dest_drag_leave),
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

  priv->dialog = awn_dialog_new_for_widget(GTK_WIDGET(icon));
  gtk_widget_show_all (priv->dialog); /*FIXME*/
  gtk_widget_hide (priv->dialog);
  g_signal_connect (G_OBJECT (priv->dialog),"focus-out-event",
                    G_CALLBACK (task_icon_dialog_unfocus),NULL);  
  priv->icon = NULL;
  priv->items = NULL;
  priv->drag_tag = 0;
  priv->drag_motion = FALSE;
  priv->gets_dragged = FALSE;
  priv->update_geometry_id = 0;
  priv->shown_items = 0;
  priv->needs_attention = 0;
  priv->is_active = 0;
  priv->main_item = NULL;

  awn_icon_set_orientation (AWN_ICON (icon), AWN_ORIENTATION_BOTTOM);

  /* D&D accept dragged objs */
  gtk_widget_add_events (GTK_WIDGET (icon), GDK_ALL_EVENTS_MASK);
  gtk_drag_dest_set (GTK_WIDGET (icon), 
                     GTK_DEST_DEFAULT_DROP,
                     drop_types, n_drop_types,
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);
  gtk_drag_dest_add_uri_targets (GTK_WIDGET (icon));
  gtk_drag_dest_add_text_targets (GTK_WIDGET (icon));
  g_signal_connect (G_OBJECT (icon), "drag-failed",
                    G_CALLBACK (task_icon_source_drag_fail), NULL);

  /* D&D support dragging itself */
  gtk_drag_source_set (GTK_WIDGET (icon),
                       GDK_BUTTON1_MASK,
                       task_icon_type, n_task_icon_type,
                       GDK_ACTION_MOVE);
}


GtkWidget *
task_icon_new ()
{
  GtkWidget *icon = g_object_new (TASK_TYPE_ICON, NULL);
  gtk_widget_hide (icon);

  //BUG: AwnApplet calls upon start gtk_widget_show_all. So even when gtk_widget_hide
  //     gets called, it will get shown. So I'm forcing it to not listen to 
  //     'gtk_widget_show_all' with this function. FIXME: improve AwnApplet
  gtk_widget_set_no_show_all (icon, TRUE);
  
  return icon;
}

static void
on_item_name_changed (TaskItem    *item, 
                      const gchar *name, 
                      TaskIcon    *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  awn_icon_set_tooltip_text (AWN_ICON (icon), name);
}

static void
on_item_icon_changed (TaskItem   *item, 
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

/**
 * Notify that the icon that a window is closed. The window gets
 * removed from the list and when this icon doesn't has any 
 * launchers and no windows it will get destroyed.
 */
static void
_destroyed_task_item (TaskIcon *icon, TaskItem *old_item)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (TASK_IS_ITEM (old_item));

  priv = icon->priv;
  priv->items = g_slist_remove (priv->items, old_item);

  if (old_item == priv->main_item)
  {
    g_signal_handlers_disconnect_by_func(priv->main_item, 
                                         G_CALLBACK (on_item_name_changed), icon);
    g_signal_handlers_disconnect_by_func(priv->main_item, 
                                         G_CALLBACK (on_item_icon_changed), icon);
    //FIXME: choose a new main_item.
  }
  task_icon_refresh_visible (icon);

  if (g_slist_length (priv->items) == 0)
  {
    gtk_widget_destroy (GTK_WIDGET (icon));
  }
  else
  {
    /* TODO: Load up with new icon etc */
  }
}

/**
 *
 */
static void
task_icon_refresh_visible (TaskIcon *icon)
{
  TaskIconPrivate *priv;
  GSList *w;
  TaskItem *vis_item = NULL;
  guint count = 0;

  g_return_if_fail (TASK_IS_ICON (icon));

  priv = icon->priv;

  for (w = priv->items; w; w = w->next)
  {
    TaskItem *item = w->data;

    if (!task_item_is_visible (item)) continue;

    vis_item = item;
    count++;
  }

  if (count != priv->shown_items)
  {
    g_debug("shown items changed: %i", count);
  
    if (count == 0)
    {
      /* 
       * There are no visible items in the icon,
       * so no reason to show itself.
       */
      if (priv->main_item)
      {
        g_signal_handlers_disconnect_by_func(priv->main_item, 
                                             G_CALLBACK (on_item_name_changed), icon);
        g_signal_handlers_disconnect_by_func(priv->main_item, 
                                             G_CALLBACK (on_item_icon_changed), icon);
        priv->main_item = NULL;
      }
      
      gtk_widget_hide (GTK_WIDGET (icon));
    }
    else if (count == 1)
    {
      /*
       * There is ONE item visible in the icon,
       * That becomes the main item and will hook
       * to name/icon changes
       */
      priv->main_item = vis_item;
      priv->icon = task_item_get_icon (priv->main_item);
      awn_icon_set_from_pixbuf (AWN_ICON (icon), priv->icon);
      awn_icon_set_tooltip_text (AWN_ICON (icon), 
                                 task_item_get_name (priv->main_item));

      g_signal_connect (priv->main_item, "name-changed",
                        G_CALLBACK (on_item_name_changed), icon);
      g_signal_connect (priv->main_item, "icon-changed",
                        G_CALLBACK (on_item_icon_changed), icon);

      //awn_icon_set_indicator_count (AWN_ICON (icon), 
      //                  TASK_IS_LAUNCHER (priv->main_item) ? 1 : 0);
      
      gtk_widget_show (GTK_WIDGET (icon));
    }
    else
    {
      //awn_icon_set_indicator_count (AWN_ICON (icon), 0);
      
      g_return_if_fail (priv->main_item);
    }
  }

  priv->shown_items = count;
}

/**
 * The 'active' state of a TaskWindow changed.
 * If this is the only TaskWindow that's active,
 * the TaskIcon will get an active state.
 * If it the last TaskWindow that isn't active anymore
 * the TaskIcon will get an inactive state too.
 * STATE: adjusted
 * PROBLEM: It shouldn't get called when the state didn't change.
            Else the count of windows that need have the active state will be off.
 */
static void
on_window_active_changed (TaskWindow *window, 
                          gboolean    is_active, 
                          TaskIcon   *icon)
{
  TaskIconPrivate *priv;
  GSList *w;
  guint count = 0;

  g_return_if_fail (TASK_IS_ICON (icon));

  priv = icon->priv;

  for (w = priv->items; w; w = w->next)
  {
    TaskItem *item = w->data;

    if (!TASK_IS_WINDOW (item)) continue;
    if (!task_item_is_visible (item)) continue;
    if (!task_window_is_active (TASK_WINDOW (item))) continue;

    count++;
  }

  if (priv->is_active == 0 && count == 1)
  {
      awn_icon_set_is_active (AWN_ICON (icon), TRUE);
  }
  else if (priv->is_active == 1 && count == 0)
  {
      awn_icon_set_is_active (AWN_ICON (icon), FALSE);
  }
  
  priv->is_active = count;
}

/**
 * The 'needs attention' state of a window changed.
 * If a window needs attention and there isn't one yet, it will
 * start the animation. When every window don't need attention anymore
 * it will stop the animation.
 * STATE: adjusted
 * TODO: h4writer - check if it is possible to interupt animation mid-air,
 * and let it start again, if there is a 2nd/3rd window that needs attention.
 * BUG: when icon becomes visible again it needs to be checked if it needs attention again.
 */
static void
on_window_needs_attention_changed (TaskWindow *window,
                                   gboolean    needs_attention,
                                   TaskIcon   *icon)
{
  TaskIconPrivate *priv;
  GSList *w;
  guint count = 0;

  g_return_if_fail (TASK_IS_ICON (icon));

  priv = icon->priv;

  for (w = priv->items; w; w = w->next)
  {
    TaskItem *item = w->data;

    if (!TASK_IS_WINDOW (item)) continue;
    if (!task_item_is_visible (item)) continue;
    if (!task_window_needs_attention (TASK_WINDOW (item))) continue;

    count++;
  }

  if (priv->needs_attention == 0 && count == 1)
  {
    awn_icon_set_effect (AWN_ICON (icon),AWN_EFFECT_ATTENTION);
  }
  else if (priv->needs_attention == 1 && count == 0)
  {
    awn_effects_stop (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                      AWN_EFFECT_ATTENTION);
  }
  
  priv->needs_attention = count;
}

/**
 * When the progress of a TaskWindow has changed,
 * it will recalculate the process of all the
 * TaskWindows this TaskIcon contains.
 * STATE: adjusted
 */
static void
on_window_progress_changed (TaskWindow   *window,
                            gfloat      adjusted_progress,
                            TaskIcon   *icon)
{
  TaskIconPrivate *priv;
  GSList *w;
  gfloat progress = 0;
  guint len = 0;

  g_return_if_fail (TASK_IS_ICON (icon));

  priv = icon->priv;

  for (w = priv->items; w; w = w->next)
  {
    TaskItem *item = w->data;

    if (!TASK_IS_WINDOW (item)) continue;
    if (!task_item_is_visible (item)) continue;
    
    progress += task_window_get_progress (TASK_WINDOW (item));
    len++;
  }

  awn_icon_set_progress (AWN_ICON (icon), progress/len);
}

/**
 * When a TaskWindow becomes visible or invisible,
 * update the number of shown windows.
 * If because of that the icon has no shown TaskWindows
 * anymore it will get hidden. If there was no shown
 * TaskWindow and now the first one gets visible,
 * then show TaskIcon.
 * STATE: adjusted
 */
static void
on_item_visible_changed (TaskItem   *item,
                         gfloat      visible,
                         TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (TASK_IS_ITEM (item));
  
  task_icon_refresh_visible (icon);
}

/**
 * Public Functions
 */

guint
task_icon_match_item (TaskIcon      *icon,
                      TaskItem      *item_to_match)
{
  TaskIconPrivate *priv;
  GSList *w;
  guint max_score = 0;

  g_return_val_if_fail (TASK_IS_ICON (icon), 0);
  g_return_val_if_fail (TASK_IS_ITEM (item_to_match), 0);

  priv = icon->priv;

  for (w = priv->items; w; w = w->next)
  {
    TaskItem *item = w->data;
    guint score;

    if (!task_item_is_visible (item)) continue;
    
    score = task_item_match (item, item_to_match);
    if (score > max_score)
      max_score = score;
  }
  
  return max_score;
}

/**
 * Adds a TaskWindow to this task-icon
 */
void
task_icon_append_item (TaskIcon      *icon,
                       TaskItem      *item)
{
  TaskIconPrivate *priv;
  
  g_assert (item);
  g_assert (icon);
  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (TASK_IS_ITEM (item));

  priv = icon->priv;

  priv->items = g_slist_append (priv->items, item);
  gtk_widget_show_all (GTK_WIDGET (item));
  gtk_container_add (GTK_CONTAINER (priv->dialog), GTK_WIDGET (item));
  
  g_object_weak_ref (G_OBJECT (item), (GWeakNotify)_destroyed_task_item, icon);

  task_icon_refresh_visible (icon);

  /* Connect item signals */
  g_signal_connect (item, "visible-changed",
                    G_CALLBACK (on_item_visible_changed), icon);
    
  /* Connect window signals */
  if (TASK_IS_WINDOW (item))
  {
    TaskWindow *window = TASK_WINDOW (item);
    g_signal_connect (window, "active-changed",
                      G_CALLBACK (on_window_active_changed), icon);
    g_signal_connect (window, "needs-attention",
                      G_CALLBACK (on_window_needs_attention_changed), icon);
    g_signal_connect (window, "progress-changed",
                      G_CALLBACK (on_window_progress_changed), icon);
  }
}

/**
 * 
 * TODO: h4writer - adjust 2nd round
 */
void
task_icon_refresh_icon (TaskIcon      *icon)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (icon));
  priv = icon->priv;

  if (priv->items && priv->items->data)
    awn_icon_set_from_pixbuf (AWN_ICON (icon), 
                              task_item_get_icon (priv->items->data));
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

/**
 * Whenever there is a release event on the TaskIcon it will do the proper actions.
 * left click: - start launcher = has no (visible) windows
 *             - activate window = when there is only one (visible) window
 *             - show dialog = when there are multiple (visible) windows
 * middle click: - start launcher
 * Returns: TRUE to stop other handlers from being invoked for the event. 
 *          FALSE to propagate the event further.
 * TODO: h4writer - adjust
 */
static gboolean
task_icon_button_release_event (GtkWidget      *widget,
                                GdkEventButton *event)
{
  TaskIconPrivate *priv;
  TaskIcon *icon;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);

  icon = TASK_ICON (widget);
  priv = icon->priv;

  switch (event->button)
  {
    case 1: // left click: (start launcher || activate window || show dialog)

      if(priv->gets_dragged) return FALSE;
    
      if (priv->shown_items == 0)
      {
        g_critical ("TaskIcon: The icons shouldn't contain a visible (and clickable) icon");
        return FALSE;
      }
      else if (priv->shown_items == 1)
      {
        GSList *w;
        /* Find the window/launcher that is shown */
        for (w = priv->items; w; w = w->next)
        {
          TaskItem *item = w->data;

          if (!task_item_is_visible (item)) continue;
          
          task_item_left_click (item, event);

          break;
        }
        return TRUE;
      }
      else
      {
        GSList *w;
        for (w = priv->items; w; w = w->next)
        {
          TaskItem *item = w->data;

          if (!task_item_is_visible (item)) continue;
          
          g_debug ("clicked on: %s", task_item_get_name (item));
        }
        
        //TODO: move to hover?
        if (GTK_WIDGET_VISIBLE (priv->dialog) )
        {
          gtk_widget_hide (priv->dialog);
        }
        else
        {
          gtk_widget_show (priv->dialog);  
        }
      }
      break;

    case 2: // middle click: start launcher

      g_warning ("TaskIcon: FIXME: No support for starting launcher on middle click");

      //TODO: start launcher
      /*if (len >= 1 && TASK_IS_LAUNCHER (priv->windows->data))
      {
        task_launcher_middle_click (priv->windows->data, event);
        return TRUE;
      }*/

      break;

    default:
      break;
  }

  return FALSE;
}

/**
 * Whenever there is a press event on the TaskIcon it will do the proper actions.
 * right click: - show the context menu if there is only one (visible) window
 * Returns: TRUE to stop other handlers from being invoked for the event. 
 *          FALSE to propagate the event further. 
 */
static gboolean  
task_icon_button_press_event (GtkWidget      *widget,
                              GdkEventButton *event)
{
  TaskIconPrivate *priv;
  TaskIcon *icon;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);

  icon = TASK_ICON (widget);
  priv = icon->priv;

  if (event->button != 3) return FALSE;

  if (priv->shown_items == 0)
  {
    g_critical ("TaskIcon: The icons shouldn't contain a visible (and clickable) icon");
    return FALSE;
  }
  else if (priv->shown_items == 1)
  {
    GSList *w;

    /* Find the window/launcher that is shown */
    for (w = priv->items; w; w = w->next)
    {
      TaskItem *item = w->data;

      if (!task_item_is_visible (item)) continue;
      
      task_item_right_click (item, event);

      break;
    }
    return TRUE;
  }
  else
  {
    g_warning ("TaskIcon: FIXME: No support for multiple windows right-click");
    return FALSE;
  }
}

static gboolean  
task_icon_dialog_unfocus (GtkWidget      *widget,
                         GdkEventFocus  *event,
                         gpointer       null)
{
  g_return_val_if_fail (GTK_IS_WIDGET(widget),FALSE);
  gtk_widget_hide (widget);
  return FALSE;
}


/*
 * Drag and Drop code
 * - code to drop things on icons
 * - code to reorder icons through dragging
 */

void
task_icon_set_draggable (TaskIcon *icon, gboolean draggable)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (icon));
  priv = icon->priv;

  priv->draggable = draggable;

  if(draggable)
  {
    gtk_drag_source_set (GTK_WIDGET (icon),
                         GDK_BUTTON1_MASK,
                         task_icon_type, n_task_icon_type,
                         GDK_ACTION_MOVE);
  }
  else
  {
    gtk_drag_source_unset(GTK_WIDGET (icon));
  }
  //g_debug("draggable:%d", draggable);
}

/**
 * TODO: h4writer - second stage
 */
static gboolean
drag_timeout (TaskIcon *icon)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);
  priv = icon->priv;

/*  if (priv->drag_motion == FALSE)
    return FALSE;
  else if (priv->windows->data)
    if (!task_window_is_active(priv->windows->data))
      task_window_activate (priv->windows->data, priv->drag_time);
*/
  return FALSE;
}

static void
task_icon_drag_data_get (GtkWidget *widget, 
                         GdkDragContext *context, 
                         GtkSelectionData *selection_data,
                         guint target_type, 
                         guint time_)
{
  switch(target_type)
  {
    case TARGET_TASK_ICON:
      //FIXME: give the data, so another taskmanager can accept this drop
      gtk_selection_data_set (selection_data, GDK_TARGET_STRING, 8, NULL, 0);
      break;
    default:
      /* Default to some a safe target instead of fail. */
      g_assert_not_reached ();
  }
}

/* DnD 'source' forwards */

static void
task_icon_source_drag_begin (GtkWidget      *widget,
                             GdkDragContext *context)
{
  TaskIconPrivate *priv;
  TaskSettings *settings;

  g_return_if_fail (TASK_IS_ICON (widget));
  priv = TASK_ICON (widget)->priv;

  if(!priv->draggable) return;

  priv->gets_dragged = TRUE;

  settings = task_settings_get_default ();

  gtk_drag_set_icon_pixbuf (context, priv->icon, settings->panel_size/2, settings->panel_size/2);

  g_signal_emit (TASK_ICON (widget), _icon_signals[SOURCE_DRAG_BEGIN], 0);
}

static void
task_icon_source_drag_end (GtkWidget      *widget,
                           GdkDragContext *drag_context)
{
  TaskIconPrivate *priv;

  g_return_if_fail (TASK_IS_ICON (widget));
  priv = TASK_ICON (widget)->priv;

  if(!priv->gets_dragged) return;

  priv->gets_dragged = FALSE;
  g_signal_emit (TASK_ICON (widget), _icon_signals[SOURCE_DRAG_END], 0);
}

static gboolean
task_icon_source_drag_fail (GtkWidget      *widget,
                          GdkDragContext *drag_context,
                          GtkDragResult   result)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);
  priv = TASK_ICON (widget)->priv;

  if(!priv->draggable) return TRUE;
  if(!priv->gets_dragged) return TRUE;

  priv->gets_dragged = FALSE;

  g_signal_emit (TASK_ICON (widget), _icon_signals[SOURCE_DRAG_FAIL], 0);

  return TRUE;
}

/* DnD 'destination' forwards */

static gboolean
task_icon_dest_drag_motion (GtkWidget      *widget,
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
    if(!priv->draggable) return FALSE;

    gdk_drag_status (context, GDK_ACTION_MOVE, t);

    g_signal_emit (TASK_ICON (widget), _icon_signals[DEST_DRAG_MOVE], 0, x, y);
    return TRUE;
  }
  else
  {
    if (priv->drag_tag)
    {
      /* If it is a launcher it should show that it accepts the drag.
         Else only the the timeout should get set to activate the window. */
      
      //TODO: h4writer - 2nd round
      //if (!priv->items || !TASK_IS_LAUNCHER (priv->items->data))
      //  gdk_drag_status (context, GDK_ACTION_DEFAULT, t);
      //else
        gdk_drag_status (context, GDK_ACTION_COPY, t);
      return TRUE;
    }

    priv->drag_motion = TRUE;
    priv->drag_tag = g_timeout_add_seconds (1, (GSourceFunc)drag_timeout, widget);
    priv->drag_time = t;

    return FALSE;
  }
}

static void   
task_icon_dest_drag_leave (GtkWidget      *widget,
                           GdkDragContext *context,
                           guint           time_)
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

  g_signal_emit (TASK_ICON (widget), _icon_signals[DEST_DRAG_LEAVE], 0);
}

static void
task_icon_dest_drag_data_received (GtkWidget      *widget,
                                   GdkDragContext *context,
                                   gint            x,
                                   gint            y,
                                   GtkSelectionData *sdata,
                                   guint           info,
                                   guint           time_)
{
  TaskIconPrivate *priv;
  GSList          *list;
  GError          *error;
  //TaskLauncher    *launcher;
  GdkAtom         target;
  gchar           *target_name;
  gchar           *sdata_data;

  g_return_if_fail (TASK_IS_ICON (widget));
  priv = TASK_ICON (widget)->priv;

  target = gtk_drag_dest_find_target (widget, context, NULL);
  target_name = gdk_atom_name (target);

  /* If it is dragging of the task icon, there is actually no data */
  if (g_strcmp0("awn/task-icon", target_name) == 0)
  {
    gtk_drag_finish (context, TRUE, TRUE, time_);
    return;
  }

  /* If we are not a launcher, we don't care about this */
  //TODO: h4writer - 2nd round
  //if (!priv->windows || !TASK_IS_LAUNCHER (priv->windows->data))
  //{
  //  gtk_drag_finish (context, FALSE, FALSE, time_);
  //  return;
  //}
  //
  //launcher = priv->windows->data;

  sdata_data = (gchar*)gtk_selection_data_get_data (sdata);
  
  /* If we are dealing with a desktop file, then we want to do something else
   * FIXME: This is a crude way of checking
   * FIXME: Emit a signal or something to let the manager know that the user
   * dropped a desktop file
   */
  if (strstr (sdata_data, ".desktop"))
  {
    /*g_signal_emit (icon, _icon_signals[DESKTOP_DROPPED],
     *               0, sdata->data);
     */
    gtk_drag_finish (context, TRUE, FALSE, time_);
  }

  /* We don't handle drops if the launcher already has a window associcated */
  //FIXME: I think this function returns always FALSE (haytjes)
  // and I also think this isn't a bad idea to allow too.
  // I often drop a url on firefox, even when it is already open.
  //if (task_launcher_has_windows (launcher))
  //{
  //  gtk_drag_finish (context, FALSE, FALSE, time_);
  //}
  
  error = NULL;
  list = awn_vfs_get_pathlist_from_string (sdata_data, &error);
  if (error)
  {
    g_warning ("Unable to handle drop: %s", error->message);
    g_error_free (error);
    gtk_drag_finish (context, FALSE, FALSE, time_);
    return;
  }

  //task_launcher_launch_with_data (launcher, list);

  g_slist_foreach (list, (GFunc)g_free, NULL);
  g_slist_free (list);

  gtk_drag_finish (context, TRUE, FALSE, time_);
}
