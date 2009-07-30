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
 *             Rodney Cryderman <rcryderman@gmail.com>
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

//#define DEBUG 1

G_DEFINE_TYPE (TaskIcon, task_icon, AWN_TYPE_THEMED_ICON)

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

  //The main item being used for the icon (and if alone for the text)
  TaskItem *main_item;

  //Whetever this icon is visible or not
  gboolean visible;

  //An overlay for showing number of items
  AwnOverlayText *overlay_text;

  //A Config client
  AwnConfigClient * client;
  
  GdkPixbuf *icon;
  AwnApplet *applet;
  GtkWidget *dialog;
  
  GtkWidget * menu;

  gboolean draggable;
  gboolean gets_dragged;

  guint    drag_tag;
  gboolean drag_motion;
  guint    drag_time;

  gint old_width;
  gint old_height;
  gint old_x;
  gint old_y;
  
  guint  max_indicators;
  guint  txt_indicator_threshold;

  gint update_geometry_id;
  
  guint ephemeral_count;

  gboolean  inhibit_focus_loss;
  
  /*prop*/
  gboolean  enable_long_press;
  
  gboolean  long_press;     /*set to TRUE when there has been a long press so the clicked event can be ignored*/
  
};

enum
{
  PROP_0,

  PROP_APPLET,
  PROP_DRAGGABLE,
  PROP_MAX_INDICATORS,
  PROP_TXT_INDICATOR_THRESHOLD,
  PROP_USE_LONG_PRESS
};

enum
{
  VISIBLE_CHANGED,

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

static void task_icon_long_press (TaskIcon * icon,gpointer null);
static void task_icon_clicked (TaskIcon * icon,GdkEventButton *event);
static gboolean  task_icon_button_release_event (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_icon_button_press_event   (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_icon_dialog_unfocus       (GtkWidget      *widget,
                                                GdkEventFocus   *event,
                                                TaskIcon        *icon);
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
static void     task_icon_search_main_item (TaskIcon *icon, TaskItem *main_item);
static void     task_icon_active_window_changed (WnckScreen *screen,
                                 WnckWindow *previously_active_window,
                                 TaskIcon *icon);

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
    case PROP_MAX_INDICATORS:
      g_value_set_int (value,priv->max_indicators);
      break;
    case PROP_TXT_INDICATOR_THRESHOLD:
      g_value_set_int (value,priv->txt_indicator_threshold);
      break;
    case PROP_USE_LONG_PRESS:
      g_value_set_boolean (value, priv->enable_long_press);
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
    case PROP_APPLET:
      TASK_ICON (icon)->priv->applet = g_value_get_object (value);
      break;
    case PROP_DRAGGABLE:
      task_icon_set_draggable (icon, g_value_get_boolean (value));
      break;
    case PROP_MAX_INDICATORS:
      icon->priv->max_indicators = g_value_get_int (value);
      task_icon_refresh_visible (TASK_ICON(object));
      break;
    case PROP_TXT_INDICATOR_THRESHOLD:
      icon->priv->txt_indicator_threshold = g_value_get_int (value);
      task_icon_refresh_visible (TASK_ICON(object));
      break;
    case PROP_USE_LONG_PRESS:
      /*TODO Move into a fn */
      if (icon->priv->enable_long_press)
      {
        g_signal_handlers_disconnect_by_func(object, 
                                       G_CALLBACK (task_icon_long_press), 
                                       object);
      }
      icon->priv->enable_long_press = g_value_get_boolean (value);
      if (icon->priv->enable_long_press)
      {
        g_signal_connect (object,"long-press",
                    G_CALLBACK(task_icon_long_press),
                    object);
      }      
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
  if (priv->client)
  {
    awn_config_client_free(priv->client);
    priv->client = NULL;
  }
  if (priv->overlay_text)
  {
    g_object_unref (priv->overlay_text);
    priv->overlay_text = NULL;
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
  g_signal_handlers_disconnect_by_func(wnck_screen_get_default (), 
                                       G_CALLBACK (task_icon_active_window_changed), 
                                       object);

  G_OBJECT_CLASS (task_icon_parent_class)->finalize (object);
}

static void
task_icon_constructed (GObject *object)
{
  AwnConfigBridge    *bridge;
  TaskIconPrivate *priv = TASK_ICON (object)->priv;
  GtkWidget *widget = GTK_WIDGET(object);  

  if ( G_OBJECT_CLASS (task_icon_parent_class)->constructed)
  {
    G_OBJECT_CLASS (task_icon_parent_class)->constructed (object);
  }
  
  priv->dialog = awn_dialog_new_for_widget_with_applet (GTK_WIDGET (object),
                                                        priv->applet);
  g_signal_connect (G_OBJECT (priv->dialog),"focus-out-event",
                    G_CALLBACK (task_icon_dialog_unfocus), object);
  g_signal_connect  (wnck_screen_get_default (), 
                     "active-window-changed",
                     G_CALLBACK (task_icon_active_window_changed), 
                     object);

  //update geometry of icon every second.
  priv->update_geometry_id = g_timeout_add_seconds (1, (GSourceFunc)_update_geometry, widget);

  priv->client = awn_config_client_new_for_applet ("taskmanager", NULL);
  bridge = awn_config_bridge_get_default ();
  
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "max_indicators",
                          object, "max_indicators");  

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "txt_indicator_threshold",
                          object, "txt_indicator_threshold");

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "enable_long_press",
                          object, "enable_long_press");
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
  if (len)
  {
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
  obj_class->dispose      = task_icon_dispose;
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
  pspec = g_param_spec_object ("applet",
                               "Applet",
                               "AwnApplet this icon belongs to",
                               AWN_TYPE_APPLET,
                               G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE);
  g_object_class_install_property (obj_class, PROP_APPLET, pspec);

  pspec = g_param_spec_boolean ("draggable",
                                "Draggable",
                                "TaskIcon is draggable?",
                                TRUE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_DRAGGABLE, pspec);

  pspec = g_param_spec_int ("max_indicators",
                            "max_indicators",
                            "Maxinum nmber of indicators (arrows) under icon",
                            0,
                            3,
                            3,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_MAX_INDICATORS, pspec);
  
  pspec = g_param_spec_int ("txt_indicator_threshold",
                            "txt_indicator_threshold",
                            "Threshold at which the text count appears for tasks",
                            0,
                            9999,
                            3,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_TXT_INDICATOR_THRESHOLD, pspec);

  pspec = g_param_spec_boolean ("enable_long_press",
                                "Use Long Press",
                                "Enable Long Preess",
                                TRUE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_USE_LONG_PRESS, pspec);
  
  /* Install signals */
  _icon_signals[VISIBLE_CHANGED] =
		g_signal_new ("visible_changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskIconClass, visible_changed),
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
  priv->visible = FALSE;
  priv->overlay_text = NULL;
  priv->ephemeral_count = 0;
  
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

/**
 * Creates a new TaskIcon, hides it and returns it.
 * (Hiding is because there are no visible TaskItems yet in the TaskIcon)  
 */
GtkWidget *
task_icon_new (AwnApplet *applet)
{
  GtkWidget *icon = g_object_new (TASK_TYPE_ICON, "applet", applet, NULL);
  gtk_widget_hide (icon);

  //BUG: AwnApplet calls upon start gtk_widget_show_all. So even when gtk_widget_hide
  //     gets called, it will get shown. So I'm forcing it to not listen to 
  //     'gtk_widget_show_all' with this function. FIXME: improve AwnApplet
  gtk_widget_set_no_show_all (icon, TRUE);
  
  return icon;
}

/**
 * The name of the main TaskItem in this TaskIcon changed.
 * So update the tooltip text.
 */
static void
on_main_item_name_changed (TaskItem    *item, 
                           const gchar *name, 
                           TaskIcon    *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));
  if (TASK_IS_WINDOW(item))
  {
#ifdef DEBUG
    g_debug ("%s: window name is %s",__func__,wnck_window_get_name (task_window_get_window(TASK_WINDOW(item))));
#endif
    awn_icon_set_tooltip_text (AWN_ICON (icon), name);
  }
}

/**
 * The icon of the main TaskItem in this TaskIcon changed.
 * So update the icon of the TaskIcon (AwnIcon).
 */
static void
on_main_item_icon_changed (TaskItem   *item, 
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
#ifdef DEBUG
  g_debug ("%s, icon width = %d, height = %d",__func__,gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf));
#endif
  awn_icon_set_from_pixbuf (AWN_ICON (icon), priv->icon);
}

/**
 * The visibility of the main TaskItem in this TaskIcon changed.
 * Because normally the main TaskItem should always be visible,
 * it searches after a new main TaskItem.
 */
static void
on_main_item_visible_changed (TaskItem  *item,
                              gboolean   visible,
                              TaskIcon  *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  /* the main TaskItem should have been visible, so if
     the main TaskItem becomes visible only now,
     it indicates a bug. 
     FIXME: this is possible atm in TaskWindow */
  if (visible) return;

//  task_icon_search_main_item (icon,item);
}

static void
task_icon_active_window_changed (WnckScreen *screen,
                                 WnckWindow *previously_active_window,
                                 TaskIcon *icon)
{
  WnckWindow * active;
  GSList     *i;
  TaskIconPrivate *priv = icon->priv;
  
  active = wnck_screen_get_active_window (screen);
  if (active )
  {
    for (i = priv->items; i; i = i->next)
    {
      TaskItem *item = i->data;

      if (!task_item_is_visible (item)) continue;
      if (!TASK_IS_WINDOW(item)) continue;
      if ( active == task_window_get_window (TASK_WINDOW(item)) )
      {
        task_icon_search_main_item (icon,item);
        break; 
      }
    }
  }
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
    task_icon_search_main_item (icon,NULL);
  }

  task_icon_refresh_visible (icon);

  if (g_slist_length (priv->items) == priv->ephemeral_count)
  {
    g_slist_foreach (priv->items,(GFunc)gtk_widget_destroy,NULL);
    g_slist_free (priv->items);
    priv->items = NULL;
    priv->ephemeral_count = 0;
    //gtk_widget_destroy (GTK_WIDGET (icon));
  }
  else
  {
    /* TODO: Load up with new icon etc */
  }
}

/**
 * Searches for a new main item unless one is provided.
 * A main item is used for displaying its icon and also the text (if there is only one item)
 * Attention: this function doesn't check if it is needed to switch to a new main item.
 */
static void
task_icon_search_main_item (TaskIcon *icon, TaskItem *main_item)
{
  TaskIconPrivate *priv;
  GSList * i;
  
  g_return_if_fail (TASK_IS_ICON (icon));

  priv = icon->priv;

  if (!main_item)
  {
    for (i = priv->items; i; i = i->next)
    {
      TaskItem *item = i->data;

      if (!task_item_is_visible (item)) continue;
      if (!TASK_IS_WINDOW (item)) continue;

      if (wnck_window_needs_attention (task_window_get_window (TASK_WINDOW(item))) )
      {
#ifdef DEBUG
        g_debug ("%s: Match #1",__func__);
#endif        
        main_item = item;
        break;
      }
    }
  }
  if (!main_item)
  {
    for (i = priv->items; i; i = i->next)
    {
      TaskItem *item = i->data;

      if (!task_item_is_visible (item)) continue;
      if (!TASK_IS_WINDOW (item)) continue;
      
      if (wnck_window_is_most_recently_activated (task_window_get_window (TASK_WINDOW(item))))
      {
#ifdef DEBUG
        g_debug ("%s: Match #2",__func__);
#endif        
        main_item = item;
        break;
      }
    }
  }
  
  if (!main_item)
  {
    for (i = priv->items; i; i = i->next)
    {
      TaskItem *item = i->data;

      if (!task_item_is_visible (item)) continue;
      if (!TASK_IS_WINDOW (item)) continue;

#ifdef DEBUG
        g_debug ("%s: Match #3",__func__);
#endif              
      main_item = item;
      break;
    }
  }

  if (!main_item)
  {
    for (i = priv->items; i; i = i->next)
    {
      TaskItem *item = i->data;

      if (!task_item_is_visible (item)) continue;
#ifdef DEBUG
        g_debug ("%s: Match #4",__func__);
#endif                    
      main_item = item;
      break;
    }
  }
  //remove signals of old main_item
  if (priv->main_item)
  {
    g_signal_handlers_disconnect_by_func(priv->main_item, 
                                         G_CALLBACK (on_main_item_name_changed), icon);
    g_signal_handlers_disconnect_by_func(priv->main_item, 
                                         G_CALLBACK (on_main_item_icon_changed), icon);
    g_signal_handlers_disconnect_by_func(priv->main_item, 
                                         G_CALLBACK (on_main_item_visible_changed), icon);
    priv->main_item = NULL;
  }

  if (main_item)
  {
    priv->main_item = main_item;
    /*
     ok... what to do here?  FIXME 
     For the moment we'll keep the icon as the launcher icon.
     */
//    priv->icon = task_item_get_icon (priv->main_item);
    
    priv->items = g_slist_remove (priv->items, priv->main_item);
    priv->items = g_slist_prepend (priv->items,priv->main_item);
#ifdef DEBUG
    g_debug ("%s, icon width = %d, height = %d",__func__,gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));
#endif    
    awn_icon_set_from_pixbuf (AWN_ICON (icon), priv->icon);
    awn_icon_set_tooltip_text (AWN_ICON (icon), 
                               task_item_get_name (priv->main_item));
    g_signal_connect (priv->main_item, "name-changed",
                      G_CALLBACK (on_main_item_name_changed), icon);
    g_signal_connect (priv->main_item, "icon-changed",
                      G_CALLBACK (on_main_item_icon_changed), icon);
    g_signal_connect (priv->main_item, "visible-changed",
                      G_CALLBACK (on_main_item_visible_changed), icon);
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
  guint count = 0;
  guint count_windows = 0;

  g_return_if_fail (TASK_IS_ICON (icon));

  priv = icon->priv;

  for (w = priv->items; w; w = w->next)
  {
    TaskItem *item = w->data;

    if (!task_item_is_visible (item)) continue;
    count++;

    if (!TASK_IS_WINDOW (item)) continue;
    count_windows++;
  }

  /*Conditional operator*/
  awn_icon_set_indicator_count (AWN_ICON (icon), 
                                count_windows > priv->max_indicators?
                                priv->max_indicators:count_windows);

  if (count_windows >= priv->txt_indicator_threshold )
  {
    if (!priv->overlay_text)
    {
      priv->overlay_text = awn_overlay_text_new ();
      awn_overlayable_add_overlay (AWN_OVERLAYABLE (icon), 
                                   AWN_OVERLAY (priv->overlay_text));
      g_object_set (G_OBJECT (priv->overlay_text),
                    "gravity", GDK_GRAVITY_SOUTH_EAST, 
                    "font-sizing", AWN_FONT_SIZE_LARGE,
                    "apply-effects", TRUE,
                    NULL);
    }

    gchar* count_str = g_strdup_printf ("%i",count_windows);
    g_object_set (G_OBJECT (priv->overlay_text),
                  "text", count_str,
                  "active",TRUE,
                  NULL);
    g_free (count_str);
  }
  else
  {
    if (priv->overlay_text)
    {
      g_object_set (priv->overlay_text,
                    "active",FALSE,
                    NULL);
    }
  }

  if (count != priv->shown_items)
  {  
    if (count == 0)
    {
      priv->visible = FALSE;
    }
    else
    {
      if (!priv->main_item)
        task_icon_search_main_item (icon,NULL);
      
      priv->visible = TRUE;
    }

    g_signal_emit (icon, _icon_signals[VISIBLE_CHANGED], 0);
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
  
  task_icon_search_main_item (icon,TASK_ITEM(window));

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

    if (progress != -1)
    {
      progress += task_window_get_progress (TASK_WINDOW (item));
      len++;
    }
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
                         gboolean   visible,
                         TaskIcon   *icon)
{
  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (TASK_IS_ITEM (item));
  
  task_icon_refresh_visible (icon);
}

/**
 * Public Functions
 */

/**
 * Returns whetever this icon should be visible.
 * (That means it should contain atleast 1 visible item)
 */
gboolean
task_icon_is_visible (TaskIcon      *icon)
{
  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);
  
  return icon->priv->visible;
}

/**
 * Returns whetever this icon contains atleast one (visible?) launcher.
 * TODO: adapt TaskIcon so it has a guint with the numbers of TaskLaunchers/TaskWindows
 */
gboolean
task_icon_contains_launcher (TaskIcon      *icon)
{
  TaskIconPrivate *priv;
  GSList *w;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);

  priv = icon->priv;

  for (w = priv->items; w; w = w->next)
  {
    TaskItem *item = w->data;

    if (!task_item_is_visible (item)) continue;

    if (TASK_IS_LAUNCHER (item))
      return TRUE;
  }
  return FALSE;
}

/**
 * Returns the number of visible and unvisible items this TaskIcon contains.
 */
guint
task_icon_count_items (TaskIcon *icon)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);
  priv = icon->priv;
  
  return g_slist_length (priv->items);
}

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
  /* 
   if we don't have an icon (this is the first item being appended) or
   this item is a launcher then we'll use this item's icon 
   */
  if (!priv->icon || TASK_IS_LAUNCHER(item))
  {
    priv->icon = task_item_get_icon (item);
  }

  
  priv->items = g_slist_append (priv->items, item);
  gtk_widget_show_all (GTK_WIDGET (item));
  gtk_container_add (GTK_CONTAINER (priv->dialog), GTK_WIDGET (item));
  
  g_object_weak_ref (G_OBJECT (item), (GWeakNotify)_destroyed_task_item, icon);

  task_item_set_task_icon (item, icon);

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
  task_icon_search_main_item (icon,item);
}

void
task_icon_append_ephemeral_item (TaskIcon      *icon,
                       TaskItem      *item)
{
  TaskIconPrivate *priv;
  
  g_assert (item);
  g_assert (icon);
  g_return_if_fail (TASK_IS_ICON (icon));
  g_return_if_fail (TASK_IS_LAUNCHER (item));

  priv = icon->priv;

  priv->ephemeral_count++;
  task_icon_append_item (icon,item); 
}

GSList *  
task_icon_get_items (TaskIcon     *icon)
{
  TaskIconPrivate *priv;
  
  g_assert (icon);
  g_return_val_if_fail (TASK_IS_ICON (icon),NULL);

  priv = icon->priv;
  return priv->items;
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

static void
task_icon_long_press (TaskIcon * icon,gpointer null)
{
  TaskIconPrivate *priv;
  priv = icon->priv;

#ifdef DEBUG
  g_debug ("%s",__func__);
#endif
  
  gtk_widget_show (priv->dialog);
  gtk_widget_grab_focus (priv->dialog);
  priv->long_press = TRUE;
}


void            
task_icon_set_inhibit_focus_loss (TaskIcon *icon, gboolean val)
{
  TaskIconPrivate *priv;
  g_return_if_fail (TASK_IS_ICON (icon));
  
  priv = icon->priv;
  priv->inhibit_focus_loss = val;
}
static void
task_icon_clicked (TaskIcon * icon,GdkEventButton *event)
{
  TaskIconPrivate *priv;
  TaskItem        *main_item;
  priv = icon->priv;

  /*If long press is enabled then we try to be smart about what we do on short clicks*/
  if (priv->enable_long_press)
  {
    main_item = priv->main_item;
  }
  else
  {
    main_item = NULL;  /* if main_item is NULL then dialog will always open when >1 item in it*/
  }

  /*hackish way to determine that we already had a long press for this signal.
   clicked signal still fires even if there was a long press*/
  if (priv->long_press)
  {
    priv->long_press = FALSE;
    return;
  }
  
  if(priv->gets_dragged) return;

  if (priv->shown_items == 0)
  {
    g_critical ("TaskIcon: The icons shouldn't contain a visible (and clickable) icon");
    return;
  }
  else if (GTK_WIDGET_VISIBLE (priv->dialog) )
  {
  /*is the dialog open?  if so then it should be closed on icon click*/  
    
    gtk_widget_hide (priv->dialog);
  }  
  else if (priv->shown_items == 1)
  {
    GSList *w;

    if (main_item) 
    {
      /*if we have a main item then pass the click on to that */
      task_item_left_click (main_item,event);
    }
    else
    {
      /* Otherwise Find the window/launcher that is shown and pass the click on*/
      for (w = priv->items; w; w = w->next)
      {
        TaskItem *item = w->data;

        if (!task_item_is_visible (item)) continue;
        
        task_item_left_click (item, event);

        break;
      }
      return;
    }
  }
  else if (main_item)
  {
    /* 
     Reach here if we have main_item set and
     1) Launcher + 1 or more TaskWindow or
     2) No Launcher + 2 or more TaskWindow.
     In either case main_item Should be TaskWindow and we want to act on that.
     */    
    if ( task_window_is_active (TASK_WINDOW(main_item)) )
    {
      task_window_minimize (TASK_WINDOW(main_item));
    }
    else
    {
      task_window_activate (TASK_WINDOW(main_item), event->time);
    }      
  }  
  else if (priv->shown_items == (1 + ( task_icon_contains_launcher (icon)?1:0)))
  {
    /*
     main item not set (for whatever reason).
     This is either a case of 1 Launcher + 1 Window or
     1 Window.
     
     So, find the first TaskWindow and act on it.
     */
    GSList *w;
    for (w = priv->items; w; w = w->next)
    {
      TaskItem *item = w->data;

      if (TASK_IS_WINDOW (item))
      {
        if ( task_window_is_active (TASK_WINDOW(item)) )
        {
          task_window_minimize (TASK_WINDOW(item));
        }
        else
        {
          task_window_activate (TASK_WINDOW(item), event->time);
        }
      }
#ifdef DEBUG
      g_debug ("clicked on: %s", task_item_get_name (item));
#endif
    }            
  }
  else
  {
    /*
     There are multiple TaskWindows.  And main_item is not set..
     therefore we show the dialog
     */
    //TODO: move to hover?
    if (GTK_WIDGET_VISIBLE (priv->dialog) )
    {
      gtk_widget_hide (priv->dialog);
    }
    else
    {
      gtk_widget_show (priv->dialog); 
      gtk_widget_grab_focus (priv->dialog);      
    }
  }
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
    case 1:
      task_icon_clicked (TASK_ICON(widget),event);
      break;
    case 2: // middle click: start launcher

      //TODO: start launcher
      /* Find the launcher that is shown */
      for (GSList *w = priv->items; w; w = w->next)
      {
        TaskItem *item = w->data;

        if (!task_item_is_visible (item)) continue;
        if (!TASK_IS_LAUNCHER (item) ) continue;
        task_item_left_click (item, event);

        break;
      }
      return TRUE;
      break;

    default:
      break;
  }

  return FALSE;
}

/**
 * Whenever there is a press event on the TaskIcon it will do the proper actions.
 * right click: - show the context menu 
 * Returns: TRUE to stop other handlers from being invoked for the event. 
 *          FALSE to propagate the event further. 
 */
static gboolean  
task_icon_button_press_event (GtkWidget      *widget,
                              GdkEventButton *event)
{
  TaskIconPrivate *priv;
  TaskIcon *icon;
  GtkWidget   *item;
  
  g_return_val_if_fail (TASK_IS_ICON (widget), FALSE);

  icon = TASK_ICON (widget);
  priv = icon->priv;

  if (event->button != 3) return FALSE;

  if (priv->shown_items == 0)
  {
    g_critical ("TaskIcon: The icons shouldn't contain a visible (and clickable) icon");
  }
  else if (priv->main_item)
  {
    /*
     Could create some reusable code in TaskWindow/TaskLauncher.  
     For now just deal with it here...  once the exact layout/behaviour is
     finalized then look into it.  TODO
     */
    if (priv->menu)
    {
      gtk_widget_destroy (priv->menu);
      priv->menu = NULL;
    }
    if (priv->main_item)
    {
      if (TASK_IS_WINDOW (priv->main_item))
      {
        GSList      *iter;
        
        priv->menu = wnck_action_menu_new (task_window_get_window (TASK_WINDOW(priv->main_item)));
        
        item = gtk_separator_menu_item_new();
        gtk_widget_show_all(item);
        gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), item);

        item = awn_applet_create_pref_item();
        gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), item);

        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), item);
        for (iter = priv->items; iter; iter=iter->next)
        {
          GtkWidget   *sub_menu;
          if ( TASK_IS_LAUNCHER (iter->data) )
            continue;
          if ( iter->data == priv->main_item)
            continue;
          item = gtk_menu_item_new_with_label (task_window_get_name (TASK_WINDOW(iter->data)));
          sub_menu = wnck_action_menu_new (task_window_get_window (TASK_WINDOW(iter->data)));          
          gtk_menu_item_set_submenu (GTK_MENU_ITEM(item),sub_menu);
          gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), item);
          gtk_widget_show (item);
          gtk_widget_show (sub_menu);
        }
        if (priv->ephemeral_count == 1)
        {
          item = gtk_menu_item_new_with_label ("Add to Launcher List");
          gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), item);
          gtk_widget_show (item);          
        }
      }
    }
    
    if (!priv->menu)
    {
      priv->menu = gtk_menu_new ();

      item = gtk_separator_menu_item_new();
      gtk_widget_show_all(item);
      gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), item);

      item = awn_applet_create_pref_item();
      gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), item);

      item = gtk_separator_menu_item_new();
      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), item);
      
      if (priv->main_item)
      {
        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_EXECUTE, NULL);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
        gtk_widget_show (item);
      }      
    }
    item = awn_applet_create_about_item (priv->applet,                
           "Copyright 2008,2009 Neil Jagdish Patel <njpatel@gmail.com>\n"
           "          2009 Hannes Verschore <hv1989@gmail.com>\n"
           "          2009 Rodney Cryderman <rcryderman@gmail.com>",
           AWN_APPLET_LICENSE_GPLV2,
           NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), item);

    if (priv->menu)
    {
      gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, 
                      NULL, NULL, event->button, event->time);
      
      g_signal_connect_swapped (priv->menu,"deactivate", 
                                G_CALLBACK(gtk_widget_hide),priv->dialog);
    }
    return TRUE;
  }
  return FALSE;  
}

static gboolean  
task_icon_dialog_unfocus (GtkWidget      *widget,
                         GdkEventFocus  *event,
                         TaskIcon *icon)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (AWN_IS_DIALOG (widget), FALSE);  

  priv = icon->priv;
  
  if (!priv->inhibit_focus_loss)
  {
    gtk_widget_hide (priv->dialog);
  }
  return FALSE;
}

GtkWidget *     
task_icon_get_dialog (TaskIcon *icon)
{
  TaskIconPrivate *priv;

  g_return_val_if_fail (TASK_IS_ICON (icon), FALSE);  

  priv = icon->priv;
  
  return priv->dialog;
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
