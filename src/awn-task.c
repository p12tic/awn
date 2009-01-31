/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <string.h>
#include <math.h>
#ifdef LIBAWN_USE_GNOME
#include <libgnome/libgnome.h>
#else
#include <glib/gi18n.h>
#endif

#if !GTK_CHECK_VERSION(2,14,0)
#define g_timeout_add_seconds(seconds, callback, user_data) g_timeout_add(seconds * 1000, callback, user_data)
#endif

#include <libawn/awn-config-client.h>
#include <libawn/awn-effects.h>
#include <libawn/awn-vfs.h>

#include "awn-task.h"
#include "awn-x.h"

#include "awn-utils.h"
#include "awn-marshallers.h"

#define AWN_TASK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), AWN_TYPE_TASK, AwnTaskPrivate))

G_DEFINE_TYPE(AwnTask, awn_task, GTK_TYPE_DRAWING_AREA);

#define  M_PI      3.14159265358979323846
#define  AWN_CLICK_IDLE_TIME   450
#define PIXBUF_SAVE_PATH ".config/awn/custom-icons"

/* FORWARD DECLERATIONS */

static gboolean awn_task_expose(GtkWidget *task, GdkEventExpose *event);
static gboolean awn_task_button_press(GtkWidget *task, GdkEventButton *event);
static gboolean awn_task_scroll_event(GtkWidget *task, GdkEventScroll *event);  //Ceceppa
static gboolean awn_task_drag_motion(GtkWidget *task,
                                     GdkDragContext *context, gint x, gint y, guint t);
static void
awn_task_drag_leave(GtkWidget *widget, GdkDragContext *drag_context,
                    guint time);
static void awn_task_create_menu(AwnTask *task, GtkMenu *menu);

static const gchar* awn_task_get_title(GtkWidget *);
static void _task_refresh(GtkWidget *obj);


/* STRUCTS & ENUMS */

typedef enum
{
  AWN_TASK_MENU_TYPE_NORMAL,
  AWN_TASK_MENU_TYPE_CHECK
} AwnTaskMenuType;

typedef struct
{
  AwnTaskMenuType  type;
  GtkWidget  *item;
  gint    id;
  gboolean  active;


} AwnTaskMenuItem;

typedef struct _AwnTaskPrivate AwnTaskPrivate;

struct _AwnTaskPrivate
{
  AwnTaskManager   *task_manager;
  AwnSettings   *settings;

  AwnDesktopItem *item;
  gint pid;

  gchar *application;

  gboolean is_launcher;
  AwnSmartLauncher launcher;
  gboolean is_starter;

  WnckWindow *window;
  AwnTitle *title;

  gboolean is_active;
  gboolean drag_hover;

  GdkPixbuf *icon;
  GdkPixbuf *reflect;

  gint progress;

  gboolean info;
  gchar *info_text;

  /* EFFECT VARIABLE */
  AwnEffects * effects;

  /* MenuItems */
  AwnTaskMenuItem *menu_items[5];

  /* random */
  guint timestamp;
  /* signal handler ID's, for clean close */
  gulong icon_changed;
  gulong state_changed;
  gulong name_changed;
  gulong geometry_changed;

  /* Ceceppa: old window position */
  gint old_x;
  gint old_y;
  int timer_count;

  gboolean dispose_has_run;
};

enum
{
  MENU_ITEM_CLICKED,
  CHECK_ITEM_CLICKED,
  LAST_SIGNAL
};

static guint awn_task_signals[LAST_SIGNAL] = { 0 };

/* GLOBALS */

static gpointer global_awn_task_parent_class = NULL;
static gint menu_item_id = 100;
static WnckWindow *last_active_window = NULL;       //Ceceppa

static const GtkTargetEntry drop_types[] =
{
  { "STRING", 0, 0 },
  { "text/plain", 0, 0},
  { "text/uri-list", 0, 0}
};
static const gint n_drop_types = G_N_ELEMENTS(drop_types);

static void
awn_task_dispose(GObject *obj)
{
  AwnTaskPrivate *priv = AWN_TASK_GET_PRIVATE(AWN_TASK(obj));

  if (priv->dispose_has_run) return;

  priv->dispose_has_run = TRUE;

  awn_effects_finalize(priv->effects);

  if (priv->window) {
    if (priv->icon_changed) {
      g_signal_handler_disconnect((gpointer)priv->window,
                                  priv->icon_changed);
      priv->icon_changed = 0;
    }
    if (priv->state_changed) {
      g_signal_handler_disconnect((gpointer)priv->window,
                                  priv->state_changed);
      priv->state_changed = 0;
    }
    if (priv->name_changed) {
      g_signal_handler_disconnect((gpointer)priv->window,
                                  priv->name_changed);
      priv->name_changed = 0;
    }
    if (priv->geometry_changed) {
      g_signal_handler_disconnect((gpointer)priv->window,
                                  priv->geometry_changed);
      priv->geometry_changed = 0;
    }
  }

  G_OBJECT_CLASS(global_awn_task_parent_class)->dispose(obj);
}

static void
awn_task_finalize(GObject *obj)
{
  AwnTaskPrivate *priv = AWN_TASK_GET_PRIVATE(AWN_TASK(obj));

  awn_effects_free(priv->effects);

  G_OBJECT_CLASS(global_awn_task_parent_class)->finalize(obj);
}

static void
awn_task_class_init(AwnTaskClass *class)
{
  GObjectClass *obj_class;
  GtkWidgetClass *widget_class;

  obj_class = G_OBJECT_CLASS(class);
  widget_class = GTK_WIDGET_CLASS(class);

  obj_class->dispose = awn_task_dispose;
  obj_class->finalize = awn_task_finalize;

  global_awn_task_parent_class = g_type_class_peek_parent(class);

  /* GtkWidget signals */
  widget_class->expose_event = awn_task_expose;
  widget_class->button_press_event = awn_task_button_press;
  widget_class->scroll_event = awn_task_scroll_event;         //Ceceppa: mouse wheel
  widget_class->drag_motion = awn_task_drag_motion;
  widget_class->drag_leave = awn_task_drag_leave;

  g_type_class_add_private(obj_class, sizeof(AwnTaskPrivate));

  awn_task_signals[MENU_ITEM_CLICKED] =
    g_signal_new("menu_item_clicked",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AwnTaskClass, menu_item_clicked),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__UINT,
                 G_TYPE_NONE,
                 1,
                 G_TYPE_UINT);

  awn_task_signals[CHECK_ITEM_CLICKED] =
    g_signal_new("check_item_clicked",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AwnTaskClass, check_item_clicked),
                 NULL, NULL,
                 _awn_marshal_VOID__UINT_BOOLEAN,
                 G_TYPE_NONE,
                 2,
                 G_TYPE_UINT, G_TYPE_BOOLEAN);
}

static void
awn_task_init(AwnTask *task)
{
  gtk_widget_add_events(GTK_WIDGET(task), GDK_ALL_EVENTS_MASK);

  gtk_drag_dest_set(GTK_WIDGET(task),
                    GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                    drop_types, n_drop_types,
                    GDK_ACTION_MOVE | GDK_ACTION_COPY);


#if !GTK_CHECK_VERSION(2,9,0)
  /* TODO: equivalent of following function: */
#else
  //gtk_drag_dest_set_track_motion  (GTK_WIDGET (task),TRUE);
#endif
  gtk_drag_dest_add_uri_targets(GTK_WIDGET(task));
  /* set all priv variables */
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  priv->item = NULL;
  priv->pid = -1;
  priv->application = NULL;
  priv->is_launcher = FALSE;
  priv->is_starter = FALSE;
  priv->window = NULL;
  priv->title = NULL;
  priv->icon = NULL;
  priv->reflect = NULL;
  priv->progress = 100;
  priv->info = FALSE;
  priv->info_text = NULL;
  priv->is_active = FALSE;
  priv->drag_hover = FALSE;
  priv->menu_items[0] = NULL;
  priv->menu_items[1] = NULL;
  priv->menu_items[2] = NULL;
  priv->menu_items[3] = NULL;
  priv->menu_items[4] = NULL;
  priv->timer_count = 0;

  /* Ceceppa */
  priv->old_x = -1;
  priv->old_y = -1;

  priv->dispose_has_run = FALSE;

  priv->effects = awn_effects_new_for_widget(GTK_WIDGET(task));
  awn_effects_register(priv->effects, GTK_WIDGET(task));
}

static const gchar* awn_task_get_title(GtkWidget *obj)
{
  return awn_task_get_name(AWN_TASK(obj));
}

/************* EFFECTS *****************/

static void
_task_refresh(GtkWidget *obj)
{
  AwnTask *task = AWN_TASK(obj);
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  gtk_widget_set_size_request(GTK_WIDGET(obj),
                              (priv->settings->task_width),
                              (priv->settings->bar_height + 2) * 2);

  /*
   Note: Timeout will only happen once.  awn_task_manager_refresh_box() always 
   returns FALSE.
   */
  g_timeout_add(50, (GSourceFunc)awn_task_manager_refresh_box,
                priv->task_manager);
}

static void
launch_launched_effect(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  awn_effects_start_ex(priv->effects, AWN_EFFECT_LAUNCHING, NULL, NULL, 10);
}

static gboolean
_shrink_widget(AwnTask *task)
{
  AwnTaskPrivate *priv;

  g_return_val_if_fail(AWN_IS_TASK(task), FALSE);
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->timer_count)
  {
    return TRUE;
  }

  if (priv->is_launcher)
    awn_task_manager_remove_launcher(priv->task_manager, task);
  else
    awn_task_manager_remove_task(priv->task_manager, task);

  priv->title = NULL;

  g_free(priv->application);

  gtk_widget_hide(GTK_WIDGET(task));

  if (priv->icon)
  {
    gdk_pixbuf_unref(priv->icon);
  }

  if (priv->reflect)
  {
    gdk_pixbuf_unref(priv->reflect);
  }

  g_timeout_add_seconds(1, (GSourceFunc)awn_task_manager_refresh_box,

                priv->task_manager);

  gtk_object_destroy(GTK_OBJECT(task));
  task = NULL;

  return FALSE;
}

static void
_task_destroy(GtkWidget *obj)
{
  g_timeout_add(40, (GSourceFunc)_shrink_widget, obj);
}

/**********************  CALLBACKS  **********************/


static void
_rounded_rectangle(cairo_t *cr, double w, double h, double x, double y)
{
  const float RADIUS_CORNERS = 1.5;

  cairo_move_to(cr, x + RADIUS_CORNERS, y);
  cairo_line_to(cr, x + w - RADIUS_CORNERS, y);
  cairo_move_to(cr, w + x, y + RADIUS_CORNERS);
  cairo_line_to(cr, w + x, h + y - RADIUS_CORNERS);
  cairo_move_to(cr, w + x - RADIUS_CORNERS, h + y);
  cairo_line_to(cr, x + RADIUS_CORNERS, h + y);
  cairo_move_to(cr, x, h + y - RADIUS_CORNERS);
  cairo_line_to(cr, x, y + RADIUS_CORNERS);


}

static void
_rounded_corners(cairo_t *cr, double w, double h, double x, double y)
{
  double radius = 1.5;
  cairo_move_to(cr, x + radius, y);
  cairo_arc(cr, x + w - radius, y + radius, radius, M_PI * 1.5, M_PI * 2);
  cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI * 0.5);
  cairo_arc(cr, x + radius,   y + h - radius, radius, M_PI * 0.5, M_PI);
  cairo_arc(cr, x + radius,   y + radius,   radius, M_PI, M_PI * 1.5);

}

static void
_rounded_rect(cairo_t *cr, gint x, gint y, gint w, gint h)
{
  double radius, RADIUS_CORNERS;
  radius = RADIUS_CORNERS = 6;

  cairo_move_to(cr, x + radius, y);
  cairo_arc(cr, x + w - radius, y + radius, radius, M_PI * 1.5, M_PI * 2);
  cairo_arc(cr, x + w - radius, y + h - radius, radius, 0, M_PI * 0.5);
  cairo_arc(cr, x + radius,   y + h - radius, radius, M_PI * 0.5, M_PI);
  cairo_arc(cr, x + radius,   y + radius,   radius, M_PI, M_PI * 1.5);
  cairo_move_to(cr, x + RADIUS_CORNERS, y);
  cairo_line_to(cr, x + w - RADIUS_CORNERS, y);
  cairo_move_to(cr, w + x, y + RADIUS_CORNERS);
  cairo_line_to(cr, w + x, h + y - RADIUS_CORNERS);
  cairo_move_to(cr, w + x - RADIUS_CORNERS, h + y);
  cairo_line_to(cr, x + RADIUS_CORNERS, h + y);
  cairo_move_to(cr, x, h + y - RADIUS_CORNERS);
  cairo_line_to(cr, x, y + RADIUS_CORNERS);

}

static void
draw(GtkWidget *task, cairo_t *cr)
{
  AwnTaskPrivate *priv;
  AwnSettings *settings;
  double width, height;

  priv = AWN_TASK_GET_PRIVATE(task);
  settings = priv->settings;

  width = task->allocation.width;
  height = task->allocation.height;

  awn_effects_draw_set_window_size(priv->effects, width, height);

  gint resize_offset = settings->bar_height + 12 - settings->task_width;

  /* task back */
  cairo_set_source_rgba(cr, 1, 0, 0, 0.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_rectangle(cr, -1, -1, width + 1, height + 1);
  cairo_fill(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  /* active */

  if (priv->window && wnck_window_is_active(priv->window))
  {
    cairo_set_source_rgba(cr, settings->border_color.red,
                          settings->border_color.green,
                          settings->border_color.blue,
                          0.2);

    if (settings->bar_angle < 0)
    {
      _rounded_rect(cr, 0 , settings->bar_height + resize_offset - priv->effects->curve_offset,
                    width, settings->bar_height - resize_offset);

      cairo_fill(cr);
    }
    else
    {
      _rounded_rect(cr, 0 , settings->bar_height + resize_offset,
                    width, settings->bar_height - resize_offset);

      cairo_fill(cr);

      cairo_set_source_rgba(cr, settings->border_color.red,
                            settings->border_color.green,
                            settings->border_color.blue,
                            0.2 / 3);
      _rounded_rect(cr, 0 , settings->bar_height*2,
                    width, settings->icon_offset);
      cairo_fill(cr);
    }
  }

  /* use libawn to draw */
  awn_effects_draw_background(priv->effects, cr);

  awn_effects_draw_icons(priv->effects, cr, priv->icon, priv->reflect);

  awn_effects_draw_foreground(priv->effects, cr);

  /* arrows */
  double x1;

  double arrow_top;

  x1 = width / 2.0;

  if (settings->bar_angle >= 0)
  {
    arrow_top = (settings->bar_height * 2) + settings->arrow_offset;
    cairo_set_source_rgba(cr, settings->arrow_color.red,
                          settings->arrow_color.green,
                          settings->arrow_color.blue,
                          settings->arrow_color.alpha);
    cairo_move_to(cr, x1 - 5, arrow_top);
    cairo_line_to(cr, x1, arrow_top - 5);
    cairo_line_to(cr, x1 + 5, arrow_top);
    cairo_close_path(cr);
  }
  else
  {
    cairo_set_source_rgba(cr, settings->arrow_color.red,
                          settings->arrow_color.green,
                          settings->arrow_color.blue,
                          settings->arrow_color.alpha);
    cairo_arc(cr, width / 2., (settings->bar_height * 2) - 5, 3., 0., 2 * M_PI);
  }

  if (settings->tasks_have_arrows)
  {
    if (priv->window != NULL)
      cairo_fill(cr);

  }
  else if (priv->is_launcher && (priv->window == NULL))
    cairo_fill(cr);
  else
  {
    ;
  }

  /* progress meter */
  cairo_new_path(cr);

  if (priv->progress != 100)
  {
    cairo_move_to(cr, width / 2.0, settings->bar_height *1.5);
    cairo_set_source_rgba(cr, settings->background.red,
                          settings->background.green,
                          settings->background.blue,
                          settings->background.alpha);

    cairo_arc(cr, width / 2.0, settings->bar_height *1.5, 15, 0, 360.0 * (M_PI / 180.0));
    cairo_fill(cr);

    cairo_move_to(cr, width / 2.0, settings->bar_height *1.5);
    cairo_set_source_rgba(cr, settings->text_color.red,
                          settings->text_color.green,
                          settings->text_color.blue,
                          0.8);
    cairo_arc(cr, width / 2.0,
              settings->bar_height * 1.5,
              12, 270.0 * (M_PI / 180.0),
              (270.0 + ((double)(priv->progress / 100.0) * 360.0))  * (M_PI / 180.0));
    cairo_fill(cr);
  }

  if ((priv->progress == 100) && priv->info)
  {

    cairo_text_extents_t extents;
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    gint len = strlen(priv->info_text);

    if (len > 8)
      cairo_set_font_size(cr, 8);
    else if (len > 4)
      cairo_set_font_size(cr, 10);
    else
      cairo_set_font_size(cr, 12);

    cairo_text_extents(cr, priv->info_text, &extents);

    cairo_move_to(cr, width / 2.0, settings->bar_height *1.5);

    cairo_set_source_rgba(cr, settings->background.red,
                          settings->background.green,
                          settings->background.blue,
                          settings->background.alpha);

    _rounded_rectangle(cr, (double) extents.width + 8.0,
                       (double) extents.height + 84.0,
                       (width / 2.0) - 4.0 - ((extents.width + extents.x_bearing) / 2.0),
                       (settings->bar_height *1.5) - (extents.height / 2.0) - 4.0);

    _rounded_corners(cr, (double) extents.width + 8.0,
                     (double) extents.height + 8.0,
                     (width / 2.0) - 4.0 - ((extents.width + extents.x_bearing) / 2.0),
                     (settings->bar_height *1.5) - (extents.height / 2.0) - 4.0);

    cairo_fill(cr);


    cairo_set_source_rgba(cr, settings->text_color.red,
                          settings->text_color.green,
                          settings->text_color.blue,
                          0.8);

    cairo_move_to(cr, (width / 2.0) - ((extents.width + extents.x_bearing) / 2.0) - 1, (settings->bar_height *1.5) + (extents.height / 2.0));

    cairo_show_text(cr, priv->info_text);
  }
}

static gboolean
awn_task_expose(GtkWidget *task, GdkEventExpose *event)
{
  AwnTaskPrivate *priv;
  cairo_t *cr;
  priv = AWN_TASK_GET_PRIVATE(task);

  /* get a cairo_t */
  cr = gdk_cairo_create(task->window);

  draw(task, cr);

  cairo_destroy(cr);

  return FALSE;
}

static void
awn_task_launch_unique(AwnTask *task, const char *arg_str)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);;

  GError *err = NULL;
  int pid = awn_desktop_item_launch(priv->item, NULL, &err);

  if (err)
  {
    g_print("Error: %s", err->message);
    g_error_free(err);
  }
  else
    g_print("Launched application : %d\n", pid);

}

static void
awn_task_launch(AwnTask *task, const char *arg_str)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);;

  GError *err = NULL;
  priv->pid = awn_desktop_item_launch(priv->item, NULL, &err);

  if (err)
  {
    g_print("Error: %s", err->message);
    g_error_free(err);
  }
  else
    g_print("Launched application : %d\n", priv->pid);

}

/*Callback to start awn-manager. */
static gboolean
awn_start_awn_manager(GtkMenuItem *menuitem, gpointer null)
{
  GError *err = NULL;
  g_spawn_command_line_async("awn-manager", &err);

  if (err)
  {
    g_warning("Failed to start awn-manager: %s\n", err->message);
    g_error_free(err);
  }

  return TRUE;
}

static void
awn_task_activate_transient (AwnTask *task, guint32 time)
{
  AwnTaskPrivate *priv = AWN_TASK_GET_PRIVATE(task);
  WnckScreen *screen;
  WnckWorkspace *active_workspace, *window_workspace;

  if (!priv->window) return;

  gint activate_behavior =
    awn_task_manager_get_activate_behavior (priv->task_manager);

  switch (activate_behavior)
  {
    case AWN_ACTIVATE_MOVE_TO_TASK_VIEWPORT:
      screen = wnck_window_get_screen(priv->window);
      active_workspace = wnck_screen_get_active_workspace (screen);
      window_workspace = wnck_window_get_workspace (priv->window);

      if (active_workspace && window_workspace &&
            !wnck_window_is_on_workspace(priv->window, active_workspace))
        wnck_workspace_activate(window_workspace, time);

      break;

    case AWN_ACTIVATE_MOVE_TASK_TO_ACTIVE_VIEWPORT:
      screen = wnck_window_get_screen(priv->window);
      active_workspace = wnck_screen_get_active_workspace (screen);

      if (active_workspace &&
            !wnck_window_is_on_workspace(priv->window, active_workspace))
        wnck_window_move_to_workspace(priv->window, active_workspace);

      break;

    case AWN_ACTIVATE_DEFAULT:
    default:
      break;
  }
  // last step is always activating the window
  wnck_window_activate_transient (priv->window, time);
}

static gboolean
awn_task_button_press(GtkWidget *task, GdkEventButton *event)
{
  static guint32 past_time; // 3v1n0 double-click (or more) prevention
  AwnTaskPrivate *priv;
  GtkWidget *menu = NULL;

  priv = AWN_TASK_GET_PRIVATE(task);


  if (priv->window)
  {

    switch (event->button)
    {

      case 1:

        if (event->time - past_time > AWN_CLICK_IDLE_TIME)
        {
          past_time = event->time;

          if (wnck_window_is_active(priv->window))
          {
            wnck_window_minimize(priv->window);
            return TRUE;
          }

          awn_task_activate_transient (AWN_TASK (task), event->time);
        }

        break;

      case 2:

        if (priv->is_launcher)
          awn_task_launch_unique(AWN_TASK(task), NULL);

        break;

      case 3:
        menu = wnck_create_window_action_menu(priv->window);

        awn_task_create_menu(AWN_TASK(task), GTK_MENU(menu));

        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
                       NULL, 3, event->time);

        break;

      default:
        return FALSE;
    }
  }
  else if (priv->is_launcher)
  {
    switch (event->button)
    {

      case 1:

        if (event->time - past_time > AWN_CLICK_IDLE_TIME)
        {
          past_time = event->time;
          awn_task_launch(AWN_TASK(task), NULL);
          launch_launched_effect(AWN_TASK(task));
        }

        break;

//    case 2:
//      // Manage middle click on launchers
//      g_print("Middle click pressed for launcher\n");
//      break;

      case 3:
        menu = gtk_menu_new();
        awn_task_create_menu(AWN_TASK(task), GTK_MENU(menu));

        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL,
                       NULL, 3, event->time);
        break;

      default:
        return FALSE;
    }
  }
  else
  {
    ;
  }


  return TRUE;
}

//Ceceppa
static gboolean
awn_task_scroll_event(GtkWidget *task, GdkEventScroll *event)
{
  AwnTaskPrivate *priv;
  gboolean in_viewport;
  int x, y;
  int w, h;
  WnckWindow *focus = NULL;
  WnckScreen *screen;
  WnckWorkspace *space;

  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->is_launcher)
    return TRUE;

  screen = wnck_screen_get_default();

  space  = wnck_screen_get_active_workspace(screen);

  if (event->direction == GDK_SCROLL_UP)
  {
    if (wnck_window_is_active(priv->window))
      return TRUE;

    //Save current position
    wnck_window_get_geometry(priv->window,
                             &x,
                             &y,
                             &w,
                             &h);

    if (priv->old_x == -1 && priv->old_y == -1)
    {
      priv->old_x = x;
      priv->old_y = y;
    }

    //Window is in current viewport? If no, center window
    in_viewport = wnck_window_is_in_viewport(priv->window, space);

    x = in_viewport ? x : (wnck_screen_get_width(screen) - w) / 2;

    y = in_viewport ? y : (wnck_screen_get_height(screen) - h) / 2;

    //Save last active window
    last_active_window = wnck_screen_get_active_window(wnck_screen_get_default());

    focus = priv->window;
  }
  else if (event->direction == GDK_SCROLL_DOWN)
  {
    if (priv->old_x == -1 && priv->old_y == -1)
      return TRUE;

    x = priv->old_x;

    y = priv->old_y;

    focus = wnck_window_is_active(priv->window) ?
            last_active_window :
            wnck_screen_get_active_window(screen);

    priv->old_x = -1;

    priv->old_y = -1;

    last_active_window = NULL;
  }

  wnck_window_set_geometry(priv->window,

                           WNCK_WINDOW_GRAVITY_CURRENT,
                           WNCK_WINDOW_CHANGE_X | WNCK_WINDOW_CHANGE_Y,
                           x, y,
                           w, h);

  if (focus != NULL)
    wnck_window_activate_transient(focus, event->time);

  return TRUE;
}


static void
_task_drag_data_recieved(GtkWidget *widget, GdkDragContext *context,
                         gint x, gint y, GtkSelectionData *selection_data,
                         guint target_type, guint time,
                         AwnTask *task)
{

  GSList      *list;

  gboolean delete_selection_data = FALSE;

  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  gtk_drag_finish(context, TRUE, FALSE, time);

  if (!priv->is_launcher)
  {
    gtk_drag_finish(context, FALSE, delete_selection_data, time);
    return;
  }

  char *res = NULL;

  res = strstr((char *)selection_data->data, ".desktop");

  if (res)
    return;

  GError *err = NULL;

  list = awn_vfs_get_pathlist_from_string((gchar *)selection_data->data, &err);

  priv->pid = awn_desktop_item_launch(priv->item, list, &err);

  if (err)
  {
    g_print("Error: %s", err->message);
    g_error_free(err);
  }
  else
    g_print("Launched application : %d\n", priv->pid);

  g_slist_foreach(list, (GFunc)g_free, NULL);

  g_slist_free(list);

  return;
}


static gboolean
activate_window(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->drag_hover && !wnck_window_is_active(priv->window))
    awn_task_activate_transient(task, priv->timestamp);

  return FALSE;
}

static gboolean
awn_task_drag_motion(GtkWidget *task,
                     GdkDragContext *context, gint x, gint y, guint t)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->settings->auto_hide && priv->settings->hidden)
  {
    awn_show(priv->settings);
  }

  if (priv->window)
  {
    if (!wnck_window_is_active(priv->window))
    {
      priv->drag_hover = TRUE;
      priv->timestamp = gtk_get_current_event_time();
      g_timeout_add_seconds(1, (GSourceFunc)activate_window, (gpointer)task);
    }

    awn_effects_start_ex(priv->effects, AWN_EFFECT_LAUNCHING, NULL, NULL, 1);
  }

  return FALSE;
}

static void
awn_task_drag_leave(GtkWidget *task, GdkDragContext *drag_context,
                    guint time)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  priv->drag_hover = FALSE;

  if (priv->settings->auto_hide && !priv->settings->hidden)
  {
    awn_hide(priv->settings);
  }
}

static void
_task_wnck_icon_changed(WnckWindow *window, AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->is_launcher)
    return;

  GdkPixbuf *old = NULL;

  GdkPixbuf *old_reflect = NULL;

  old = priv->icon;

  old_reflect = priv->reflect;

  int height = priv->settings->bar_height;

  if ((priv->settings->task_width - 12) < height)
  {
    height = priv->settings->task_width - 12;
  }

  priv->icon = awn_x_get_icon_for_window(priv->window,

                                         height, height);
  priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);

  awn_effects_draw_set_icon_size(priv->effects, gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));

  gdk_pixbuf_unref(old);
  gdk_pixbuf_unref(old_reflect);
  gtk_widget_queue_draw(GTK_WIDGET(task));
}

static gboolean
_task_wnck_name_hide(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  awn_title_hide(AWN_TITLE(priv->title), GTK_WIDGET(task));
  awn_effects_stop(priv->effects, AWN_EFFECT_ATTENTION);
  /* If wanted, this could be done in a GDestroyNotify callback if
     we added the timer using g_timeout_add_full () */
  priv->timer_count--;
  return FALSE;
}

static void
_task_wnck_name_changed(WnckWindow *window, AwnTask *task)
{
  g_return_if_fail(AWN_IS_TASK(task));

  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (!priv->window || !priv->settings->name_change_notify)
  {
    return;
  }

  /*
  g_print("Name changed on window '%s'\n",
   wnck_window_get_name (priv->window));
         */

  if (!wnck_window_is_active(priv->window))
  {
    gint x, y;
    gdk_window_get_origin(GTK_WIDGET(task)->window, &x, &y);

    awn_effects_start_ex(priv->effects, AWN_EFFECT_ATTENTION, NULL, NULL, 10);

    awn_title_show(AWN_TITLE(priv->title),
                   GTK_WIDGET(task),
                   awn_task_get_name(AWN_TASK(task)));

    priv->timer_count++;
    g_timeout_add(2500, (GSourceFunc)_task_wnck_name_hide, (gpointer)task);

  }
}

static void
_task_wnck_state_changed(WnckWindow *window, WnckWindowState  old,
                         WnckWindowState  new, AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->window == NULL)
    return;

  if (wnck_window_is_skip_tasklist(priv->window))
    gtk_widget_hide(GTK_WIDGET(task));
  else
    gtk_widget_show(GTK_WIDGET(task));

  if (wnck_window_needs_attention(priv->window))
  {
    awn_effects_start(priv->effects, AWN_EFFECT_ATTENTION);
  }
  else
  {
    awn_effects_stop(priv->effects, AWN_EFFECT_ATTENTION);
  }
}

gboolean updating = FALSE;
static gboolean
_viewport_changed_so_update(AwnTaskManager *task_manager)
{
  updating = FALSE;
  awn_task_manager_refresh_box(task_manager);
  return FALSE;
}

static void
_task_wnck_geometry_changed(WnckWindow *window, AwnTask *task)
{
  AwnTaskPrivate *priv;
  WnckScreen* screen;
  int x,y,width,height;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->window == NULL)
    return;

  screen = wnck_window_get_screen(priv->window);  
  wnck_window_get_geometry(priv->window,
                           &x,
                           &y,
                           &width,
                           &height);

  if( x <= -width || x >= wnck_screen_get_width(screen) )
  {
    /* Window is moved off viewport, so update the list of tasks */
    if( !updating )
    {
      updating = TRUE;
      g_timeout_add(50, (GSourceFunc)_viewport_changed_so_update, priv->task_manager);
    }
  }
}

/**********************Gets & Sets **********************/

gboolean
awn_task_get_is_launcher(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  return priv->is_launcher;
}

gboolean
awn_task_set_window(AwnTask *task, WnckWindow *window)
{
  g_return_val_if_fail(WNCK_IS_WINDOW(window), 0);

  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->window != NULL)
    return FALSE;

  gboolean connect_geom_signal = 
	awn_task_manager_screen_has_viewports (priv->task_manager);

  priv->window = window;

  if (!priv->is_launcher)
  {
    gint size = priv->settings->task_width - 12;
    if (size <= 0) size = 1;
    priv->icon = awn_x_get_icon_for_window(priv->window,
                                           size, size);
    if (GDK_IS_PIXBUF (priv->icon))
      priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);
    else
      priv->reflect = NULL;

    awn_effects_draw_set_icon_size(priv->effects, gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));

  }

  priv->icon_changed = g_signal_connect(G_OBJECT(priv->window), "icon_changed",

                                        G_CALLBACK(_task_wnck_icon_changed), (gpointer)task);

  priv->state_changed = g_signal_connect(G_OBJECT(priv->window), "state_changed",
                                         G_CALLBACK(_task_wnck_state_changed), (gpointer)task);

  priv->name_changed = g_signal_connect(G_OBJECT(priv->window), "name_changed",
                                        G_CALLBACK(_task_wnck_name_changed), (gpointer)task);

  if (connect_geom_signal)
  {
    priv->geometry_changed = g_signal_connect(G_OBJECT(priv->window), "geometry_changed",
                                              G_CALLBACK(_task_wnck_geometry_changed), (gpointer)task);
  }

  /* if launcher, set a launch_sequence
  else if starter, stop the launch_sequence, disable starter flag*/

  //awn_task_refresh_icon_geometry(task);

  if (wnck_window_is_skip_tasklist(window))
    return TRUE;

  if (priv->is_launcher)
  {
    awn_effects_stop(priv->effects, AWN_EFFECT_LAUNCHING);
    return TRUE;
  }

  awn_effects_start_ex(priv->effects, AWN_EFFECT_OPENING, NULL, _task_refresh, 1);

  return TRUE;
}

gboolean
awn_task_set_launcher(AwnTask *task, AwnDesktopItem *item)
{
  AwnTaskPrivate *priv;
  gchar *icon_name = NULL;

  priv = AWN_TASK_GET_PRIVATE(task);

  priv->is_launcher = item != NULL;
  if (item == NULL) return FALSE;

  gboolean was_null = priv->item == NULL;

  icon_name = awn_desktop_item_get_icon(item, priv->settings->icon_theme);
  if (!icon_name)
    return FALSE;

  g_free(icon_name);

  priv->item = item;

  priv->icon = awn_x_get_icon_for_launcher(item,
               priv->settings->task_width - 12,
               priv->settings->task_width - 12);

  if (!priv->icon)
  {
    return FALSE;
  }

  priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);

  awn_effects_draw_set_icon_size(priv->effects, gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));

  if (was_null)
  {
    awn_effects_start_ex(priv->effects, AWN_EFFECT_OPENING, 
                         NULL, _task_refresh, 1);
  }

  return TRUE;
}

gboolean
awn_task_is_launcher(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  return priv->is_launcher;
}

WnckWindow *
awn_task_get_window(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  return priv->window;
}

gulong
awn_task_get_xid(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->window == NULL)
    return 0;

  g_return_val_if_fail(WNCK_IS_WINDOW(priv->window), 0);

  return wnck_window_get_xid(priv->window);
}

gint
awn_task_get_pid(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->window != NULL)
    return wnck_window_get_pid(priv->window);

  return priv->pid;
}

void
awn_task_set_is_active(AwnTask *task, gboolean is_active)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->window == NULL)
    return;

  priv->is_active = wnck_window_is_active(priv->window);

  gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_set_needs_attention(AwnTask *task, gboolean needs_attention)
{
  // wow, it's never called
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (needs_attention)
    awn_effects_start(priv->effects, AWN_EFFECT_ATTENTION);
  else
    awn_effects_stop(priv->effects, AWN_EFFECT_ATTENTION);
}

const char*
awn_task_get_name(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  const char *name = NULL;


  if (priv->window)
    name =  wnck_window_get_name(priv->window);

  else if (priv->is_launcher)
    name = awn_desktop_item_get_name(priv->item);
  else
    name =  "No name";

  return name;
}

const char*
awn_task_get_application(AwnTask *task)
{
  AwnTaskPrivate *priv;
  WnckApplication *wnck_app;
  char *app;
  GString *str;

  priv = AWN_TASK_GET_PRIVATE(task);
  app = NULL;

  if (priv->application)
    return priv->application;

  if (priv->is_launcher)
  {

    str = g_string_new(awn_desktop_item_get_exec(priv->item));
    int i = 0;

    for (i = 0; i < str->len; i++)
    {
      if (str->str[i] == ' ')
        break;
    }

    if (i)
      str = g_string_truncate(str, i);

    priv->application = g_strdup(str->str);

    app = g_string_free(str, TRUE);

  }
  else if (priv->window)
  {
    wnck_app = wnck_window_get_application(priv->window);

    if (WNCK_IS_APPLICATION(wnck_app))
    {
      str = g_string_new(wnck_application_get_name(wnck_app));
    }
    else
    {
      str = g_string_new(NULL);
    }

    str = g_string_ascii_down(str);

    priv->application = g_strdup(str->str);
    app = g_string_free(str, TRUE);

  }
  else
    priv->application = NULL;

  return priv->application;
}

void
awn_task_set_title(AwnTask *task, AwnTitle *title)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  priv->title = title;
  awn_effects_set_title(priv->effects, title, awn_task_get_title);
}

AwnSettings*
awn_task_get_settings(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);
  return priv->settings;
}

void
awn_task_refresh_icon_geometry(AwnTask *task)
{
  AwnTaskPrivate *priv;
  static gint old_x = 0;
  static gint old_y = 0;
  gint x, y;
  gint width, height;

  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->window == NULL)
    return;

  gdk_window_get_origin(GTK_WIDGET(task)->window, &x, &y);

  gdk_drawable_get_size(GDK_DRAWABLE(GTK_WIDGET(task)->window),
                        &width, &height);

  //width = priv->icon_width;
  //height = priv->icon_height;

  //printf("width: %d, height: %d\n", width, height);

  if ((x != old_x) || (y != old_y))
  {
    gint res = 0;

    if (x > old_x)
      res = x - old_x;
    else
      res = old_x - x;

    if (res > 10)
    {
      wnck_window_set_icon_geometry(priv->window, x, y, width, height);
    }

    old_x = x;

    old_y = y;
  }

}

void
awn_task_update_icon(AwnTask *task)
{
  AwnTaskPrivate *priv;
  GdkPixbuf *old = NULL;
  GdkPixbuf *old_reflect = NULL;
  priv = AWN_TASK_GET_PRIVATE(task);

  old = priv->icon;
  old_reflect = priv->reflect;

  int height = priv->settings->bar_height;

  if ((priv->settings->task_width - 12) < height)
  {
    height = priv->settings->task_width - 12;
  }

  priv->icon = awn_x_get_icon_for_launcher(priv->item,

               height, height);

  if (!priv->icon)
  {
    priv->icon = old;
    return;
  }

  priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);

  awn_effects_draw_set_icon_size(priv->effects,
                         gdk_pixbuf_get_width(priv->icon),
                         gdk_pixbuf_get_height(priv->icon));

  gdk_pixbuf_unref(old);
  gdk_pixbuf_unref(old_reflect);
}

void
awn_task_set_width(AwnTask *task, gint width)
{
  AwnTaskPrivate *priv;
  GdkPixbuf *old = NULL;
  GdkPixbuf *old_reflect = NULL;

  if (!AWN_IS_TASK(task))
    return;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);

  //if (priv->effects.is_closing.state) {
  // return;
  //}

  old = priv->icon;

  old_reflect = priv->reflect;

  if (priv->is_launcher)
  {
    char * icon_name = awn_desktop_item_get_icon(priv->item, priv->settings->icon_theme);

    if (!icon_name)
    {
      priv->icon = awn_x_get_icon_for_window(priv->window, width - 12, width - 12);
    }
    else
    {
      priv->icon = awn_x_get_icon_for_launcher(priv->item, width - 12, width - 12);
    }

    g_free(icon_name);
  }
  else
  {
    if (WNCK_IS_WINDOW(priv->window))
    {
      priv->icon = awn_x_get_icon_for_window(priv->window, width - 12, width - 12);
    }
  }

  if (G_IS_OBJECT(priv->icon))
  {
    priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);
    awn_effects_draw_set_icon_size(priv->effects, gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));
  }

  if (G_IS_OBJECT(old) && (priv->is_launcher || WNCK_IS_WINDOW(priv->window)))
    gdk_pixbuf_unref(old);

  if (G_IS_OBJECT(old_reflect))
    gdk_pixbuf_unref(old_reflect);

  gtk_widget_set_size_request(GTK_WIDGET(task),
                              width,
                              (priv->settings->bar_height + 2) * 2);

}


AwnDesktopItem*
awn_task_get_item(AwnTask *task)
{
  AwnTaskPrivate *priv;
  g_return_val_if_fail(AWN_IS_TASK(task), NULL);
  priv = AWN_TASK_GET_PRIVATE(task);

  return priv->item;
}

/********************* DBUS FUNCTIONS *******************/

void
awn_task_set_custom_icon(AwnTask *task, GdkPixbuf *icon)
{
  AwnTaskPrivate *priv;
  GdkPixbuf *old_icon;
  GdkPixbuf *old_reflect;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);
  old_icon = priv->icon;
  old_reflect = priv->reflect;

  priv->icon = icon;
  priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);

  awn_effects_draw_set_icon_size(priv->effects, gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));

  g_object_unref(G_OBJECT(old_icon));
  g_object_unref(G_OBJECT(old_reflect));

  gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_unset_custom_icon(AwnTask *task)
{
  AwnTaskPrivate *priv;
  GdkPixbuf *old_icon;
  GdkPixbuf *old_reflect;
  char *icon_name = NULL;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);
  old_icon = priv->icon;
  old_reflect = priv->reflect;

  if (priv->is_launcher)
  {
    icon_name = awn_desktop_item_get_icon(priv->item, priv->settings->icon_theme);

    if (!icon_name)
    {
      priv->icon = awn_x_get_icon_for_window(priv->window, priv->settings->task_width - 12, priv->settings->task_width - 12);
    }
    else
    {
      priv->icon = awn_x_get_icon_for_launcher(priv->item, priv->settings->task_width - 12, priv->settings->task_width - 12);
    }

    g_free(icon_name);
  }
  else
  {
    priv->icon = awn_x_get_icon_for_window(priv->window, priv->settings->bar_height, priv->settings->bar_height);
  }

  priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);

  awn_effects_draw_set_icon_size(priv->effects, gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));

  g_object_unref(G_OBJECT(old_icon));
  g_object_unref(G_OBJECT(old_reflect));

  gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_set_progress(AwnTask *task, gint progress)
{
  AwnTaskPrivate *priv;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);

  if ((progress < 101) && (progress >= 0))
    priv->progress = progress;
  else
    priv->progress = 100;

  gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_set_info(AwnTask *task, const gchar *info)
{
  AwnTaskPrivate *priv;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->info_text)
  {
    g_free(priv->info_text);
    priv->info_text = NULL;
  }

  priv->info = TRUE;

  priv->info_text = g_strdup(info);

  gtk_widget_queue_draw(GTK_WIDGET(task));
}

void
awn_task_unset_info(AwnTask *task)
{
  AwnTaskPrivate *priv;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);

  priv->info = FALSE;

  if (priv->info_text)
  {
    g_free(priv->info_text);
    priv->info_text = NULL;
  }

  gtk_widget_queue_draw(GTK_WIDGET(task));
}

gint
awn_task_add_menu_item(AwnTask *task, GtkMenuItem *item)
{
  AwnTaskPrivate *priv;
  AwnTaskMenuItem *menu_item;

  g_return_val_if_fail(AWN_IS_TASK(task), 0);

  priv = AWN_TASK_GET_PRIVATE(task);

  int i;

  for (i = 0; i < 5; i++)
  {
    if (priv->menu_items[i] == NULL)
    {
      g_object_ref(G_OBJECT(item));
      menu_item = g_new0(AwnTaskMenuItem, 1);
      menu_item->type = AWN_TASK_MENU_TYPE_NORMAL;
      menu_item->item = GTK_WIDGET(item);
      menu_item->active = FALSE;
      priv->menu_items[i] = menu_item;
      menu_item->id =  menu_item_id++;
      return menu_item->id;
    }
  }

  return 0;
}

gint
awn_task_add_check_item(AwnTask *task, GtkMenuItem *item)
{
  AwnTaskPrivate *priv;
  AwnTaskMenuItem *menu_item;

  g_return_val_if_fail(AWN_IS_TASK(task), 0);

  priv = AWN_TASK_GET_PRIVATE(task);

  int i;

  for (i = 0; i < 5; i++)
  {
    if (priv->menu_items[i] == NULL)
    {
      g_object_ref(G_OBJECT(item));
      menu_item = g_new0(AwnTaskMenuItem, 1);
      menu_item->type = AWN_TASK_MENU_TYPE_CHECK;
      menu_item->item = GTK_WIDGET(item);
      menu_item->active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
      priv->menu_items[i] = menu_item;
      menu_item->id =  menu_item_id++;
      return menu_item->id;
    }
  }

  return 0;
}

void
awn_task_set_check_item(AwnTask *task, gint id, gboolean active)
{
  AwnTaskPrivate *priv;
  AwnTaskMenuItem *menu_item;

  g_return_if_fail(AWN_IS_TASK(task));

  priv = AWN_TASK_GET_PRIVATE(task);
  int i;

  for (i = 0; i < 5; i++)
  {
    menu_item = priv->menu_items[i];

    if (menu_item != NULL)
    {
      if (menu_item->id == id)
      {
        menu_item->active = active;
      }
    }
  }
}

/********************* MISC FUNCTIONS *******************/

typedef struct _FileChooserAndTask FileChooserAndTask;

struct _FileChooserAndTask
{
  GtkWidget *filechooser;
  AwnTask *task;
};


static void _task_choose_custom_icon_performed(GtkWidget *dialog, gint res, FileChooserAndTask *fct)
{

  AwnTaskPrivate *priv;
  GdkPixbuf *pixbuf = NULL;
  GError *err = NULL;
  gchar *filename = NULL;
  gchar *save = NULL;
  gchar *name = NULL;
  AwnTask *task = fct->task;
  int i;

  g_return_if_fail(AWN_IS_TASK(task));
  priv = AWN_TASK_GET_PRIVATE(task);

  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fct->filechooser));
  g_free(fct);
  gtk_widget_hide(dialog);
  gtk_widget_destroy(dialog);

  /* If not accept, clean up and return */

  if (res != GTK_RESPONSE_ACCEPT)
  {
    return;
  }

  /* Okay, the user has chosen a new icon, so lets load it up */

  pixbuf = gdk_pixbuf_new_from_file_at_size(filename, priv->settings->bar_height, priv->settings->bar_height, &err);

  g_free(filename);

  /* Check if is actually a pixbuf */
  if (pixbuf == NULL)
  {
    g_warning("Failed to load pixbuf (%s)\n", err->message);
    g_error_free(err);
    return;
  }

  /* So we have a nice new pixbuf, we now want to save it's location
     for the future */
  if (priv->is_launcher)
  {
    name = g_strdup(awn_desktop_item_get_exec(priv->item));
  }
  else
  {
    WnckApplication *app = NULL;
    app = wnck_window_get_application(priv->window);

    if (app == NULL)
    {
      name = NULL;
    }
    else
    {
      GString *gname = awn_x_get_application_name(priv->window, app);
      name = g_strdup(gname->str);
      g_string_free(gname, TRUE);
    }
  }

  if (name == NULL)
  {
    /* Somethings gone very wrong */
    return;
  }

  /* Replace spaces with dashs */
  for (i = 0; i < strlen(name); i++)
  {
    if (name[i] == ' ' || name[i] == '/')
      name[i] = '-';
  }

  /* Now lets build the save-filename and save it */
  save = g_build_filename(g_get_home_dir(),
                          PIXBUF_SAVE_PATH,
                          name,
                          NULL);

  gdk_pixbuf_save(pixbuf, save, "png", &err, NULL);

  g_free(save);

  if (err)
  {
    g_print("%s\n", err->message);
    g_error_free(err);
    return;
  }

  /* Now we have saved the new pixbuf, lets actually set it as the main
     pixbuf and refresh the view */
  g_object_unref(G_OBJECT(priv->icon));

  g_object_unref(G_OBJECT(priv->reflect));

  priv->icon = pixbuf;

  priv->reflect = gdk_pixbuf_flip(priv->icon, FALSE);

  awn_effects_draw_set_icon_size(priv->effects, gdk_pixbuf_get_width(priv->icon), gdk_pixbuf_get_height(priv->icon));

  gtk_widget_queue_draw(GTK_WIDGET(task));

  g_free(name);
}

static void
_task_choose_custom_icon(AwnTask *task)
{
#define PIXBUF_SAVE_PATH ".config/awn/custom-icons"

  AwnTaskPrivate *priv;
  GtkWidget *file;

  g_return_if_fail(AWN_IS_TASK(task));
  priv = AWN_TASK_GET_PRIVATE(task);

  /* Create the dialog */
  file = gtk_file_chooser_dialog_new(_("Choose New Image..."),
                                     GTK_WINDOW(priv->settings->window),
                                     GTK_FILE_CHOOSER_ACTION_OPEN,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                     NULL);

  FileChooserAndTask *fct = g_new(FileChooserAndTask, 1);
  fct->filechooser = file;
  fct->task = task;

  g_signal_connect(file, "response", G_CALLBACK(_task_choose_custom_icon_performed), (gpointer)fct);

  /* Run it */
  gtk_widget_show_all(file);
}

static void
_task_menu_item_clicked(GtkMenuItem *item, AwnTask *task)
{
  AwnTaskPrivate *priv;
  AwnTaskMenuItem *menu_item;
  int i;

  priv = AWN_TASK_GET_PRIVATE(task);

  for (i = 0; i < 5; i++)
  {
    menu_item = priv->menu_items[i];

    if (menu_item != NULL)
    {
      if (GTK_MENU_ITEM(menu_item->item) == item)
      {
        g_signal_emit(G_OBJECT(task), awn_task_signals[MENU_ITEM_CLICKED], 0,
                      (guint) menu_item->id);
      }
    }
  }

}


static void
_task_check_item_clicked(GtkMenuItem *item, AwnTask *task)
{
  AwnTaskPrivate *priv;
  AwnTaskMenuItem *menu_item;
  gboolean active;
  int i;

  priv = AWN_TASK_GET_PRIVATE(task);

  active = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));

  for (i = 0; i < 5; i++)
  {
    menu_item = priv->menu_items[i];

    if (menu_item != NULL)
    {
      if (GTK_MENU_ITEM(menu_item->item) == item)
      {
        g_signal_emit(G_OBJECT(task), awn_task_signals[CHECK_ITEM_CLICKED], 0,
                      (guint) menu_item->id, active);
        menu_item->active  = active;
      }
    }
  }

}

static void
on_change_icon_clicked(GtkButton *button, AwnTask *task)
{
  _task_choose_custom_icon(task);
}

static void
_task_show_prefs(GtkMenuItem *item, AwnTask *task)
{
  _task_choose_custom_icon(task);
  return;

  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *button, *label;

  dialog = gtk_dialog_new_with_buttons(_("Preferences"),
                                       GTK_WINDOW(priv->settings->window),
                                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                       _("Change Icon..."),
                                       3,
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_REJECT,
                                       GTK_STOCK_OK,
                                       GTK_RESPONSE_ACCEPT,
                                       NULL);
  hbox = gtk_hbox_new(FALSE, 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE, 0);

  image = gtk_image_new_from_pixbuf(priv->icon);

  button = gtk_button_new_with_label(_("Change Image..."));
  gtk_button_set_image(GTK_BUTTON(button), image);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

  g_signal_connect(G_OBJECT(button), "clicked",
                   G_CALLBACK(on_change_icon_clicked), task);

  label = gtk_label_new(" ");
  char *app_name = (char*)awn_task_get_application(task);

  if (app_name)
  {
    gchar *markup = g_strdup_printf("<span size='larger' weight='bold'>%s</span>", app_name);
    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
  }

  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

  gtk_widget_show_all(hbox);

  gint res = gtk_dialog_run(GTK_DIALOG(dialog));

  switch (res)
  {

    case 3:
      _task_choose_custom_icon(task);
      break;

    default:
      break;
  }

  gtk_widget_destroy(dialog);
}



typedef struct
{
  const char *uri;
  AwnSettings *settings;

} AwnListTerm;

static void
_slist_foreach(char *uri, AwnListTerm *term)
{
  AwnSettings *settings = term->settings;

  if (strcmp(uri, term->uri) == 0)
  {
    //g_print ("URI : %s\n", uri);
    settings->launchers = g_slist_remove(settings->launchers, uri);
    //g_slist_foreach(settings->launchers, (GFunc)_slist_print, (gpointer)term);
  }
}

void awn_task_remove(AwnTask *task)
{
  AwnTaskPrivate *priv = AWN_TASK_GET_PRIVATE(task);

  priv->window = NULL;
  /* start closing effect */
  awn_effects_start_ex(priv->effects, 
                       AWN_EFFECT_CLOSING, NULL, _task_destroy, 1);
}

static void
_task_remove_launcher(GtkMenuItem *item, AwnTask *task)
{
  AwnTaskPrivate *priv;
  AwnSettings *settings;
  AwnListTerm term;
  gchar *uri;

  priv = AWN_TASK_GET_PRIVATE(task);
  settings = priv->settings;

  uri = awn_desktop_item_get_filename(priv->item);

  g_print("Remove : %s\n", uri);
  term.uri = uri;
  term.settings = settings;
  g_slist_foreach(settings->launchers, (GFunc)_slist_foreach, (gpointer)&term);

  AwnConfigClient *client = awn_config_client_new();
  awn_config_client_set_list(client, "window_manager", "launchers",
                             AWN_CONFIG_CLIENT_LIST_TYPE_STRING, settings->launchers, NULL);

  awn_task_remove (task);
}

static void
_shell_done(GtkMenuShell *shell, AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->settings->auto_hide == FALSE)
  {
    if (priv->settings->hidden == TRUE)
    {
      awn_show(priv->settings);
    }

    return;
  }

  awn_hide(priv->settings);
}

static void
awn_task_create_menu(AwnTask *task, GtkMenu *menu)
{
  AwnTaskPrivate *priv;
  GtkWidget *item;

  priv = AWN_TASK_GET_PRIVATE(task);
  item = NULL;

  g_signal_connect(GTK_MENU_SHELL(menu), "selection-done",
                   G_CALLBACK(_shell_done), (gpointer)task);

  if (priv->is_launcher && priv->window == NULL)
  {


    item = gtk_image_menu_item_new_from_stock("gtk-remove",
           NULL);
    gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);
    g_signal_connect(G_OBJECT(item), "activate",
                     G_CALLBACK(_task_remove_launcher), (gpointer)task);
  }

  item = gtk_separator_menu_item_new();

  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
  gtk_widget_show(item);

  item = gtk_menu_item_new_with_label(_("Change Icon"));
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
  gtk_widget_show(item);
  g_signal_connect(G_OBJECT(item), "activate",
                   G_CALLBACK(_task_show_prefs), (gpointer)task);

  AwnTaskMenuItem *menu_item;
  int i = 0;

  for (i = 0; i < 5; i++)
  {
    menu_item = priv->menu_items[i];

    if (menu_item != NULL)
    {

      if (i == 0)
      {
        item = gtk_separator_menu_item_new();
        gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
        gtk_widget_show(item);
      }


      g_signal_handlers_disconnect_by_func(G_OBJECT(menu_item->item),

                                           _task_menu_item_clicked, (gpointer)task);
      g_signal_handlers_disconnect_by_func(G_OBJECT(menu_item->item),
                                           _task_check_item_clicked, (gpointer)task);
      gtk_widget_unparent(menu_item->item);
      gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item->item);

      if (menu_item->type == AWN_TASK_MENU_TYPE_CHECK)
      {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item->item),
                                       menu_item->active);
        g_signal_connect(G_OBJECT(menu_item->item), "activate",
                         G_CALLBACK(_task_check_item_clicked), (gpointer)task);
      }
      else
      {
        g_signal_connect(G_OBJECT(menu_item->item), "activate",
                         G_CALLBACK(_task_menu_item_clicked), (gpointer) task);
      }

      gtk_widget_show(menu_item->item);

    }
  }

  item = gtk_image_menu_item_new_with_label(_("Dock Preferences"));

  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,
                                                         GTK_ICON_SIZE_MENU));

  gtk_widget_show_all(item);
  gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
  g_signal_connect(G_OBJECT(item), "activate",
                   G_CALLBACK(awn_start_awn_manager), NULL);


}

/********************* awn_task_new * *******************/

GtkWidget *
awn_task_new(AwnTaskManager *task_manager, AwnSettings *settings)
{
  GtkWidget *task;
  AwnTaskPrivate *priv;
  task = g_object_new(AWN_TYPE_TASK, NULL);
  priv = AWN_TASK_GET_PRIVATE(task);

  priv->task_manager = task_manager;
  priv->settings = settings;

  /* This is code which I will add later for better hover effects over
  the bar */
  g_signal_connect(G_OBJECT(task), "drag-data-received",
                   G_CALLBACK(_task_drag_data_recieved), (gpointer)task);
  g_signal_connect(G_OBJECT(task), "drag-end",
                   G_CALLBACK(_task_drag_data_recieved), (gpointer)task);

  _task_refresh(GTK_WIDGET(task));

  return task;
}

void
awn_task_close(AwnTask *task)
{
  AwnTaskPrivate *priv;
  priv = AWN_TASK_GET_PRIVATE(task);

  if (priv->icon_changed) {
    g_signal_handler_disconnect((gpointer)priv->window,
                              priv->icon_changed);
    priv->icon_changed = 0;
  }
  if (priv->state_changed) {
    g_signal_handler_disconnect((gpointer)priv->window,
                              priv->state_changed);
    priv->state_changed = 0;
  }
  if (priv->name_changed) {
    g_signal_handler_disconnect((gpointer)priv->window,
                              priv->name_changed);
    priv->name_changed = 0;
  }
  if (priv->geometry_changed) {
    g_signal_handler_disconnect((gpointer)priv->window,
                              priv->geometry_changed);
    priv->geometry_changed = 0;
  }

  priv->window = NULL;

  AwnTaskMenuItem *item;
  int i;

  for (i = 0; i < 5; i++)
  {
    item = priv->menu_items[i];

    if (item != NULL)
    {
      if (GTK_IS_WIDGET(item->item))
        gtk_widget_destroy(item->item);

      if (item)
        g_free(item);

      item = priv->menu_items[i] = NULL;
    }

  }


  if (priv->is_launcher)
  {
    gtk_widget_queue_draw(GTK_WIDGET(task));
    return;
  }

  /* start closing effect */
  awn_effects_start_ex(priv->effects, AWN_EFFECT_CLOSING,
                      NULL, _task_destroy, 1);
}






