/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *                2009-2010 Michal Hruby <michal.mhr@gmail.com>
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
 *  Authors : Neil Jagdish Patel <njpatel@gmail.com>
 *            Michal Hruby <michal.mhr@gmail.com>
 *
 */

#include "config.h"

#include <gdk/gdkx.h>
#include <glib/gi18n.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libawn/libawn.h>

#include "awn-panel.h"

#include "awn-applet-manager.h"
#include "awn-background.h"
#include "awn-background-flat.h"
#include "awn-background-3d.h"
#include "awn-background-curves.h"
#include "awn-background-edgy.h"
#include "awn-background-floaty.h"
#include "awn-defines.h"
#include "awn-marshal.h"
#include "awn-monitor.h"
#include "awn-throbber.h"
#include "awn-x.h"

#include "libawn/gseal-transition.h"
#include "xutils.h"

G_DEFINE_TYPE (AwnPanel, awn_panel, GTK_TYPE_WINDOW)

#define AWN_PANEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_PANEL, AwnPanelPrivate))

struct _AwnPanelPrivate
{
  gint panel_id;
  DesktopAgnosticConfigClient *client;
  AwnMonitor *monitor;

  GHashTable *inhibits;
  guint startup_inhibit_cookie;

  AwnBackground *bg;

  GtkWidget *alignment;
  GtkWidget *eventbox;
  GtkWidget *box;
  GtkWidget *arrow1;
  GtkWidget *arrow2;
  GtkWidget *manager;
  GtkWidget *viewport;
  GtkWidget *docklet;
  GtkWidget *docklet_closer;
  gint docklet_minsize;
  gboolean docklet_close_on_pos_change;
  gboolean docklet_close_on_mouse_out;
  GtkWidget *menu;

  gboolean composited;
  gboolean panel_mode;
  gboolean expand;
  gboolean animated_resize;

  gint size;
  gint offset;
  gfloat offset_mod;
  GtkPositionType position;
  gint path_type;
  gint style;

  gint autohide_type;

  /* for masks/strut updating */
  gint old_width;
  gint old_height;
  GtkPositionType old_position;

  /* for strut updating */
  gint old_x;
  gint old_y;
  guint strut_update_id;
  guint masks_update_id;

  /* animated resizing */
  gint draw_width;
  gint draw_height;
  guint resize_timer_id;

  guint extra_padding;

  GdkWindow *dnd_proxy_win;
  GdkDragProtocol dnd_proxy_proto;

  guint dnd_mouse_poll_timer_id;
  guint mouse_poll_timer_id;

  /* scrolling */
  guint scroll_timer_id;

  /* autohide stuff */
  gint autohide_hide_delay;
  gint autohide_mouse_poll_delay;

  gulong autohide_start_handler_id;
  gulong autohide_end_handler_id;

  gint hide_counter;
  guint hiding_timer_id;
  guint withdraw_timer_id;
  gint withdraw_redraw_timer;

  guint autohide_start_timer_id;
  gboolean autohide_started;
  gboolean autohide_always_visible;
  gboolean autohide_inhibited;

  /* clickthrough stuff */
  gboolean clickthrough;
  gint clickthrough_type;
  gint last_clickthrough_type;

  /* docklet animating stuff */
  GdkPixmap *dock_snapshot;
  GtkAllocation snapshot_paint_size;
  GdkPixmap *tmp_pixmap;
  gfloat docklet_alpha;
  guint docklet_appear_timer_id;
};

typedef struct _AwnInhibitItem
{
  AwnPanel *panel;

  gchar *description;
  guint cookie;
} AwnInhibitItem;

#define CLICKTHROUGH_OPACITY 0.3

#define ROUND(x) (x < 0 ? x - 0.5 : x + 0.5)

//#define DEBUG_DRAW_AREA
//#define DEBUG_APPLET_AREA

enum 
{
  PROP_0,

  PROP_PANEL_ID,
  PROP_CLIENT,
  PROP_MONITOR,
  PROP_APPLET_MANAGER,
  PROP_PANEL_XID,
  PROP_COMPOSITED,
  PROP_PANEL_MODE,
  PROP_EXPAND,
  PROP_OFFSET,
  PROP_POSITION,
  PROP_SIZE,
  PROP_MAX_SIZE,
  PROP_PATH_TYPE,
  PROP_OFFSET_MODIFIER,
  PROP_AUTOHIDE_TYPE,
  PROP_AUTOHIDE_HIDE_DELAY,
  PROP_AUTOHIDE_POLL_DELAY,
  PROP_STYLE,
  PROP_CLICKTHROUGH
};

enum
{
  AUTOHIDE_TYPE_NONE = 0,
  AUTOHIDE_TYPE_KEEP_BELOW,
  AUTOHIDE_TYPE_FADE_OUT,
  AUTOHIDE_TYPE_TRANSPARENTIZE,

  AUTOHIDE_TYPE_LAST
};

enum
{
  CLICKTHROUGH_NEVER = 0,
  CLICKTHROUGH_ON_CTRL,
  CLICKTHROUGH_ON_NOCTRL,
  
  CLICKTHROUGH_LAST
};

enum
{
  STYLE_NONE = 0,
  STYLE_FLAT,
  STYLE_3D,
  STYLE_CURVES,
  STYLE_EDGY,
  STYLE_FLOATY,

  STYLE_LAST
};

typedef enum
{
  MOUSE_CHECK_EDGE_ONLY,
  MOUSE_CHECK_ACTIVE_MASK,
  MOUSE_CHECK_ENTIRE_WINDOW
} MouseCheckType;

static const GtkTargetEntry drop_types[] = 
{
  { (gchar*)"STRING", 0, 0 },
  { (gchar*)"text/plain", 0, 0 },
  { (gchar*)"text/uri-list", 0, 0 }
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);

enum
{
  SIZE_CHANGED,
  POS_CHANGED,
  OFFSET_CHANGED,
  PROPERTY_CHANGED,
  DESTROY_NOTIFY,
  DESTROY_APPLET,
  AUTOHIDE_START,
  AUTOHIDE_END,

  LAST_SIGNAL
};
static guint _panel_signals[LAST_SIGNAL] = { 0 };

/* 
 * FORWARDS
 */
static void     load_correct_colormap       (GtkWidget *panel);
static void     on_composited_changed       (GtkWidget *widget, gpointer data);
static void     on_applet_embedded          (AwnPanel  *panel,
                                             GtkWidget *applet);
static void     on_applet_removed           (AwnPanel  *panel,
                                             GtkWidget *applet);

static gboolean on_mouse_over               (GtkWidget *widget,
                                             GdkEventCrossing *event);
static gboolean on_mouse_out                (GtkWidget *widget,
                                             GdkEventCrossing *event);

static gboolean on_window_configure         (GtkWidget         *panel,
                                             GdkEventConfigure *event);
static gboolean position_window             (AwnPanel *panel);

static gboolean on_eb_expose                (GtkWidget      *eb, 
                                             GdkEventExpose *event,
                                             AwnPanel       *panel);

static void     on_manager_size_alloc       (GtkWidget      *manager,
                                             GtkAllocation  *alloc,
                                             AwnPanel       *panel);

static gboolean on_window_state_event       (GtkWidget *widget,
                                             GdkEventWindowState *event);

static gboolean poll_mouse_position         (gpointer data);
static gboolean awn_panel_expose            (GtkWidget      *widget, 
                                             GdkEventExpose *event);
static void     awn_panel_size_request      (GtkWidget *widget,
                                             GtkRequisition *requisition);
static gboolean awn_panel_button_press      (GtkWidget      *widget, 
                                             GdkEventButton *event);
static gboolean awn_panel_resize_timeout    (gpointer data);

static void     awn_panel_add               (GtkContainer   *window, 
                                             GtkWidget      *widget);

static void     awn_panel_set_offset        (AwnPanel *panel,
                                             gint      offset);
static void     awn_panel_set_pos_type      (AwnPanel *panel,
                                             GtkPositionType position);
static void     awn_panel_set_size          (AwnPanel *panel,
                                             gint      size);
static void     awn_panel_set_autohide_type (AwnPanel *panel,
                                             gint      type);
static void     awn_panel_set_style         (AwnPanel *panel,
                                             gint      style);
static void     awn_panel_set_panel_mode    (AwnPanel *panel,
                                             gboolean  panel_mode);
static void     awn_panel_set_expand_mode   (AwnPanel *panel,
                                             gboolean  expand);
static void     awn_panel_set_clickthrough_type(AwnPanel *panel,
                                             gint      type);

static guint    awn_panel_disable_autohide  (AwnPanel *panel,
                                             const gchar *app_name,
                                             const gchar *reason);

static void     awn_panel_reset_autohide    (AwnPanel *panel);

static void     on_geometry_changed         (AwnMonitor    *monitor,
                                             AwnPanel      *panel);
static void     on_theme_changed            (AwnBackground *bg,
                                             AwnPanel      *panel);

static gboolean awn_panel_check_mouse_pos   (AwnPanel *panel,
                                             MouseCheckType check_type);

static void     awn_panel_get_draw_rect     (AwnPanel *panel,
                                             GdkRectangle *area,
                                             gint width, gint height);
#ifdef DEBUG_APPLET_AREA
static void     awn_panel_get_applet_rect   (AwnPanel *panel,
                                             GdkRectangle *area,
                                             gint width, gint height);
#endif
static void     awn_panel_refresh_alignment (AwnPanel *panel);

static void     awn_panel_refresh_padding   (AwnPanel *panel,
                                             gpointer user_data);

static void     awn_panel_update_masks      (GtkWidget *panel, 
                                             gint real_width, 
                                             gint real_height);

static void     awn_panel_queue_masks_update(AwnPanel *panel);

static void     awn_panel_docklet_destroy   (AwnPanel *panel);

static void     awn_panel_queue_strut_update(AwnPanel *panel);

static void     awn_panel_set_strut         (AwnPanel *panel);

static void     awn_panel_remove_strut      (AwnPanel *panel);

static gboolean awn_panel_dnd_check         (gpointer data);

static gboolean awn_panel_set_drag_proxy    (AwnPanel *panel,
                                             gboolean check_mouse_pos);

/*
 * GOBJECT CODE 
 */
static gboolean
awn_panel_set_drag_proxy (AwnPanel *panel, gboolean check_mouse_pos)
{
  AwnPanelPrivate *priv = panel->priv;
  GtkWidget *widget = GTK_WIDGET (panel);

  if (check_mouse_pos)
  {
    gboolean in = awn_panel_check_mouse_pos (panel, MOUSE_CHECK_ENTIRE_WINDOW);
    if (!in)
    {
      if (priv->dnd_proxy_win)
      {
        g_object_unref (priv->dnd_proxy_win);
        // FIXME: unset first?
        gtk_drag_dest_set (widget, 0, drop_types, n_drop_types,
                           GDK_ACTION_COPY);

        priv->dnd_proxy_win = NULL;
      }
      return FALSE;
    }
  }

  GdkDisplay *display = gtk_widget_get_display (widget);
  GdkWindow *window = NULL;

#if 1
  GdkScreen *screen;
  gint mouse_x, mouse_y;
  gdk_display_get_pointer (display, &screen, &mouse_x, &mouse_y, NULL);
  GList *windows = gdk_screen_get_window_stack (screen);

  GdkNativeWindow panel_xid = GDK_WINDOW_XID (widget->window);

  // go through window stack list and find window the mouse is on
  for (GList *it = g_list_last (windows); it; it = g_list_previous (it))
  {
    gint win_x, win_y, win_w, win_h;
    GdkWindow *it_window = it->data;

    if (GDK_WINDOW_XID (it_window) == panel_xid || window != NULL)
    {
      g_object_unref (it_window);
      continue;
    }

    // this does screw up d&d when using multiple panels, but at least it
    //   won't crash
    if (gdk_window_get_window_type (it_window) == GDK_WINDOW_TOPLEVEL)
    {
      g_object_unref (it_window);
      continue;
    }

    gdk_window_get_origin (it_window, &win_x, &win_y);
    gdk_window_get_geometry (it_window, NULL, NULL, &win_w, &win_h, NULL);

    // if mouse in, and window is not minimized
    if (mouse_x >= win_x && mouse_x < win_x + win_w &&
        mouse_y >= win_y && mouse_y < win_y + win_h &&
        gdk_window_is_visible (it_window) &&
        xutils_is_window_minimized (it_window) == FALSE)
    {
      window = g_object_ref (it_window);
      // no break so we unref all other windows
    }

    g_object_unref (it_window);
  }

  g_list_free (windows);
#else
  // FIXME: this method is much faster, but when dragging for example a file
  //  from the desktop to different position on the desktop,
  //  it returns a temporary window created by nautilus, therefore doesn't work
  //  on 100% :-(
  window = xutils_get_window_at_pointer (display);
#endif
  if (window != priv->dnd_proxy_win)
  {
    if (window != widget->window && window != NULL && window != widget->window)
    {
      GdkNativeWindow target;

      target = gdk_drag_get_protocol (GDK_WINDOW_XID (window),
                                      &priv->dnd_proxy_proto);

      if (target == 0)
      {
        if (priv->dnd_proxy_win) 
        {
          g_object_unref (priv->dnd_proxy_win);
          gtk_drag_dest_set (widget, 0, drop_types, n_drop_types, GDK_ACTION_COPY);
        }
        priv->dnd_proxy_win = NULL;
        return FALSE;
      }

      // get the target GdkWindow
      if (GDK_WINDOW_XID (window) != target)
      {
        g_object_unref (window);

        // FIXME: this might need ref
        window = gdk_window_lookup_for_display (display, target);
        if (priv->dnd_proxy_win != NULL && window == priv->dnd_proxy_win)
          return TRUE;
        if (window == NULL) window = gdk_window_foreign_new (target);
      }

      priv->dnd_proxy_win = g_object_ref (window);
      gtk_drag_dest_set_proxy (widget, window, priv->dnd_proxy_proto, FALSE);

      g_object_unref (window);
    }
    else if (priv->dnd_proxy_win)
    {
      // FIXME: unset first?
      gtk_drag_dest_set (widget, 0, drop_types, n_drop_types, GDK_ACTION_COPY);

      g_object_unref (priv->dnd_proxy_win);
      priv->dnd_proxy_win = NULL;
    }
  }

  return priv->dnd_proxy_win != NULL;
}

static gboolean
awn_panel_drag_motion (GtkWidget *widget, GdkDragContext *context, 
                       gint x, gint y, guint time_)
{
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (widget);

  awn_panel_set_drag_proxy (AWN_PANEL (widget), FALSE);

  if (priv->dnd_mouse_poll_timer_id == 0)
  {
    priv->dnd_mouse_poll_timer_id = g_timeout_add (40, awn_panel_dnd_check,
                                                   widget);
  }

  gdk_drag_status (context, 0, time_);

  return TRUE;
}

static gboolean
on_startup_complete (AwnPanel *panel)
{
  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);

  awn_panel_uninhibit_autohide (panel, panel->priv->startup_inhibit_cookie);

  return FALSE;
}

static void
on_prefs_activated (GtkMenuItem *item, AwnPanel *panel)
{
  GError *err = NULL;

  gdk_spawn_command_line_on_screen (gtk_widget_get_screen (GTK_WIDGET (panel)),
                                    "awn-settings", &err);

  if (err)
  {
    g_critical ("%s", err->message);
    g_error_free (err);
  }
}

static void
on_close_activated (GtkMenuItem *item, AwnPanel *panel)
{
  gtk_main_quit ();
}

static void
on_about_activated (GtkMenuItem *item, AwnPanel *panel)
{
  GError *err = NULL;

  gdk_spawn_command_line_on_screen (gtk_widget_get_screen (GTK_WIDGET (panel)),
                                    "awn-settings --about", &err);

  if (err)
  {
    g_critical ("%s", err->message);
    g_error_free (err);
  }
}

static void
awn_panel_constructed (GObject *object)
{
  AwnPanelPrivate *priv;
  GtkWidget       *panel;
  GdkScreen       *screen;

  priv = AWN_PANEL_GET_PRIVATE (object);
  panel = GTK_WIDGET (object);

  priv->monitor = awn_monitor_new_from_config (priv->client);
  g_signal_connect (priv->monitor, "geometry_changed",
                    G_CALLBACK (on_geometry_changed), panel);

  g_signal_connect_swapped (priv->monitor, "notify::monitor-align",
                            G_CALLBACK (awn_panel_refresh_alignment), panel);

  /* Inhibit autohide, so we cannot unmap the widget when there are composited
     state switches during startup. */
  priv->startup_inhibit_cookie =
    awn_panel_disable_autohide (AWN_PANEL (panel), "internal", "Panel spawn");
  g_timeout_add (10000, (GSourceFunc)on_startup_complete, panel);

  /* Composited checks/setup */
  screen = gtk_widget_get_screen (panel);
  priv->composited = gdk_screen_is_composited (screen);
  priv->animated_resize = priv->composited;
  g_print ("Screen %s composited\n", priv->composited ? "is" : "isn't");
  load_correct_colormap (panel);

  g_signal_connect (panel, "composited-changed",
                    G_CALLBACK (on_composited_changed), NULL);
  g_signal_connect (panel, "window-state-event",
                    G_CALLBACK (on_window_state_event), NULL);
  g_signal_connect (panel, "delete-event",
                    G_CALLBACK (gtk_true), NULL);
  
  /* Contents */
  priv->manager = awn_applet_manager_new_from_config (priv->client);
  g_signal_connect_swapped (priv->manager, "applet-embedded",
                            G_CALLBACK (on_applet_embedded), panel);
  g_signal_connect_swapped (priv->manager, "applet-removed",
                            G_CALLBACK (on_applet_removed), panel);
  g_signal_connect_swapped (priv->manager, "notify::expands",
                            G_CALLBACK (awn_panel_refresh_alignment), panel);
  g_signal_connect (priv->manager, "size-allocate",
                    G_CALLBACK (on_manager_size_alloc), panel);
  gtk_container_add (GTK_CONTAINER (panel), priv->manager);
  gtk_widget_show_all (priv->manager);
  
  /* FIXME: Now is the time to hook our properties into priv->client */
  
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_PANEL_MODE,
                                       object, "panel_mode", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_EXPAND,
                                       object, "expand", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_POSITION,
                                       object, "position", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_OFFSET,
                                       object, "offset", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_SIZE,
                                       object, "size", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_AUTOHIDE,
                                       object, "autohide-type", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANELS, AWN_PANELS_HIDE_DELAY,
                                       object, "autohide-hide-delay", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANELS, AWN_PANELS_POLL_DELAY,
                                       object, "autohide-poll-delay", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_STYLE,
                                       object, "style", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_CLICKTHROUGH,
                                       object, "clickthrough_type", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);

  /* Size and position */
  g_signal_connect (panel, "configure-event",
                    G_CALLBACK (on_window_configure), NULL);

  position_window (AWN_PANEL (panel));

  gtk_drag_dest_set (panel, 0, drop_types, n_drop_types, GDK_ACTION_COPY);

  /* Prefs/Quit/About menu */
  GtkWidget *item;

  priv->menu = gtk_menu_new ();

  item = gtk_image_menu_item_new_with_label (_("Dock Preferences"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
      gtk_image_new_from_icon_name ("avant-window-navigator",
                                    GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (on_prefs_activated), panel);

  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (on_close_activated), panel);

  item = gtk_image_menu_item_new_with_label (_("About Awn"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
      gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU));
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (on_about_activated), panel);

  awn_utils_show_menu_images (GTK_MENU (priv->menu));

  gtk_widget_show_all (priv->menu);
}

static void
awn_panel_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  AwnPanelPrivate *priv;

  g_return_if_fail (AWN_IS_PANEL (object));
  priv = AWN_PANEL (object)->priv;
  GdkWindow *window;

  switch (prop_id)
  {
    case PROP_PANEL_ID:
      g_value_set_int (value, priv->panel_id);
      break;
    case PROP_CLIENT:
      g_value_set_object (value, priv->client);
      break;
    case PROP_MONITOR:
      g_value_set_pointer (value, priv->monitor);
      break;
    case PROP_APPLET_MANAGER:
      g_value_set_pointer (value, priv->manager);
      break;
    case PROP_PANEL_XID:
      window = gtk_widget_get_window (GTK_WIDGET (object));
      g_value_set_int64 (value, window ? (gint64) GDK_WINDOW_XID (window) : 0);
      break;
    case PROP_COMPOSITED:
      g_value_set_boolean (value, priv->composited);
      break;
    case PROP_PANEL_MODE:
      g_value_set_boolean (value, priv->panel_mode);
      break;
    case PROP_EXPAND:
      g_value_set_boolean (value, priv->expand);
      break;
    case PROP_OFFSET:
      g_value_set_int (value, priv->offset);
      break;
    case PROP_POSITION:
      g_value_set_int (value, priv->position);
      break;
    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;
    case PROP_MAX_SIZE:
    {
      GtkAllocation alloc;

      gtk_widget_get_allocation (priv->manager, &alloc);
      switch (priv->position)
      {
        case GTK_POS_LEFT:
        case GTK_POS_RIGHT:
          g_value_set_int (value, alloc.width);
          break;
        default:
          g_value_set_int (value, alloc.height);
          break;
      }
      break;
    }
    case PROP_PATH_TYPE:
      g_value_set_int (value, priv->path_type);
      break;
    case PROP_OFFSET_MODIFIER:
      g_value_set_float (value, priv->offset_mod);
      break;
    case PROP_AUTOHIDE_TYPE:
      g_value_set_int (value, priv->autohide_type);
      break;
    case PROP_AUTOHIDE_HIDE_DELAY:
      g_value_set_int (value, priv->autohide_hide_delay);
      break;
    case PROP_AUTOHIDE_POLL_DELAY:
      g_value_set_int (value, priv->autohide_mouse_poll_delay);
      break;
    case PROP_STYLE:
      g_value_set_int (value, priv->style);
      break;
    case PROP_CLICKTHROUGH:
      g_value_set_int (value, priv->clickthrough_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_panel_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  AwnPanel        *panel = AWN_PANEL (object);
  AwnPanelPrivate *priv;

  g_return_if_fail (AWN_IS_PANEL (object));
  priv = AWN_PANEL (object)->priv;

  switch (prop_id)
  {
    case PROP_PANEL_ID:
      priv->panel_id = g_value_get_int (value);
      break;
    case PROP_CLIENT:
      priv->client = g_value_get_object (value);
      break;
    case PROP_PANEL_MODE:
      awn_panel_set_panel_mode (panel, g_value_get_boolean (value));
      break;
    case PROP_EXPAND:
      awn_panel_set_expand_mode (panel, g_value_get_boolean (value));
      break;
    case PROP_OFFSET:
      awn_panel_set_offset (panel, g_value_get_int (value));
      break;
    case PROP_POSITION:
      awn_panel_set_pos_type (panel, g_value_get_int (value));
      break;
    case PROP_SIZE:
      awn_panel_set_size (panel, g_value_get_int (value));
      break;
    case PROP_PATH_TYPE:
      priv->path_type = g_value_get_int (value);
      break;
    case PROP_OFFSET_MODIFIER:
      priv->offset_mod = g_value_get_float (value);
      break;
    case PROP_AUTOHIDE_TYPE:
      awn_panel_set_autohide_type (panel, g_value_get_int (value));
      break;
    case PROP_AUTOHIDE_HIDE_DELAY:
      priv->autohide_hide_delay = g_value_get_int (value);
      break;
    case PROP_AUTOHIDE_POLL_DELAY:
      priv->autohide_mouse_poll_delay = g_value_get_int (value);
      if (priv->mouse_poll_timer_id != 0)
      {
        g_source_remove (priv->mouse_poll_timer_id);
        priv->mouse_poll_timer_id =
          g_timeout_add (priv->autohide_mouse_poll_delay,
                         poll_mouse_position, panel);
      }
      break;
    case PROP_STYLE:
      awn_panel_set_style (panel, g_value_get_int (value));
      break;
    case PROP_CLICKTHROUGH:
      awn_panel_set_clickthrough_type (panel, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_panel_show (GtkWidget *widget)
{
  GtkWidget *manager;

  GTK_WIDGET_CLASS (awn_panel_parent_class)->show (widget);

  manager = AWN_PANEL (widget)->priv->manager;
  awn_applet_manager_refresh_applets (AWN_APPLET_MANAGER (manager));
}

static void
awn_panel_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  GtkRequisition child_requisition;
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (widget);
  gint *target_size, *current_draw_size;

  GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));
  if (!GTK_IS_WIDGET (child))
    return;

  gtk_widget_size_request (child, &child_requisition);

  // limit our max width/height
  child_requisition.width = MIN (child_requisition.width,
                                 priv->monitor->width);
  child_requisition.height = MIN (child_requisition.height,
                                  priv->monitor->height);

  gint size = priv->size + priv->offset + priv->extra_padding;

  // if we're animating resizes, we can't just request the size applets
  // require, because the animation must have extra space to draw to
  // use full monitor space, otherwise we'll flicker during resizes
  // FIXME: do this also in non-composited? shouldn't hurt much
  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      requisition->width = priv->expand || priv->animated_resize ?
        priv->monitor->width : child_requisition.width;
      requisition->height = priv->composited ? size + priv->size : size;

      target_size = priv->expand ?
        &(requisition->width) : &(child_requisition.width);
      current_draw_size = &(priv->draw_width);
      
      break;
    case GTK_POS_LEFT:
    case GTK_POS_RIGHT:
    default:
      requisition->width = priv->composited ? size + priv->size : size;
      requisition->height = priv->expand || priv->animated_resize ?
        priv->monitor->height : child_requisition.height;

      target_size = priv->expand ?
        &(requisition->height) : &(child_requisition.height);
      current_draw_size = &(priv->draw_height);

      break;
  }

  if (priv->animated_resize && !priv->expand)
  {
    if (*target_size != *current_draw_size && !priv->resize_timer_id)
    {
      priv->resize_timer_id = g_timeout_add (40, awn_panel_resize_timeout,
                                             widget); // 25 FPS
    }
  }
  else if (priv->expand)
  {
    // this ensures there's a shrinking animation when expand is turned off
    *current_draw_size = *target_size;
  }
}

static gboolean
awn_panel_resize_timeout (gpointer data)
{
  gboolean resize_done;
  gint inc, step;
  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;
  GtkAllocation alloc;
  GdkRectangle rect1, rect2;

  // this is the size we are resizing to
  const gint target_width = MIN (priv->alignment->requisition.width,
                                 priv->monitor->width);
  const gint target_height = MIN (priv->alignment->requisition.height,
                                  priv->monitor->height);

  awn_panel_get_draw_rect (panel, &rect1, 0, 0);

  gtk_widget_get_allocation (GTK_WIDGET (panel), &alloc);

  switch (priv->position)
  {
    case GTK_POS_LEFT:
    case GTK_POS_RIGHT:
      inc = abs (target_height - priv->draw_height);
      step = inc / 7 + 2; // makes the resize shiny
      inc = MIN (inc, step);

      priv->draw_width = alloc.width;
      
      priv->draw_height += priv->draw_height < target_height ? inc : -inc;

      resize_done = priv->draw_height == target_height;
      break;
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
    default:
      inc = abs (target_width - priv->draw_width);
      step = inc / 7 + 2; // makes the resize shiny
      inc = MIN (inc, step);

      priv->draw_width += priv->draw_width < target_width ? inc : -inc;

      priv->draw_height = alloc.height;

      resize_done = priv->draw_width == target_width;
      break;
  }

#if 0
  g_debug ("dw: %d..%d, dh: %d..%d", priv->draw_width, target_width,
                                     priv->draw_height, target_height);
#endif

  // draw_width / height got updated, get the draw rect again
  awn_panel_get_draw_rect (panel, &rect2, 0, 0);

  // invalidate only the background draw region
  gint end_x = MAX (rect1.x + rect1.width, rect2.x + rect2.width);
  gint end_y = MAX (rect1.y + rect1.height, rect2.y + rect2.height);
  GdkRectangle invalid_rect =
  {
    .x = MIN (rect1.x, rect2.x),
    .y = MIN (rect1.y, rect2.y),
    .width = end_x - MIN (rect1.x, rect2.x),
    .height = end_y - MIN (rect1.y, rect2.y)
  };
  gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (panel)),
                              &invalid_rect, FALSE);
  // without this there are some artifacts on sad face & throbbers
  awn_applet_manager_redraw_throbbers (AWN_APPLET_MANAGER (priv->manager));

  // Don't update the input masks here, it gets called too often and some
  // drivers really don't like it. (LP bug #478790)

  if (resize_done)
  {
    gtk_widget_queue_resize (GTK_WIDGET (panel));
    priv->resize_timer_id = 0;

    awn_panel_queue_masks_update (panel);
  }

  return !resize_done;
}

static gboolean
awn_panel_button_press (GtkWidget *widget, GdkEventButton *event)
{
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (widget);

  if (event->button == 3)
  {
    gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, NULL, NULL,
                    event->button, event->time);
    return TRUE;
  }

  return FALSE;
}

static void
awn_panel_refresh_alignment (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;
  gfloat align = priv->monitor ? priv->monitor->align : 0.5;
  gfloat expand = priv->expand && priv->manager &&
    awn_applet_manager_get_expands (AWN_APPLET_MANAGER (priv->manager)) ?
      1.0 : 0.0;

  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      gtk_alignment_set (GTK_ALIGNMENT (priv->alignment),
                         align, 0.5, expand, 1.0);
      break;
    case GTK_POS_LEFT:
    case GTK_POS_RIGHT:
    default:
      gtk_alignment_set (GTK_ALIGNMENT (priv->alignment),
                         0.5, align, 1.0, expand);
      break;
  }

  awn_panel_queue_masks_update (panel);
}

static
void awn_panel_refresh_padding (AwnPanel *panel, gpointer user_data)
{
  AwnPanelPrivate *priv = panel->priv;
  guint top, left, bottom, right;

  if (!priv->bg || !AWN_IS_BACKGROUND (priv->bg))
  {
    gtk_alignment_set_padding (GTK_ALIGNMENT (priv->alignment), 0, 0, 0, 0);
    priv->extra_padding = AWN_EFFECTS_ACTIVE_RECT_PADDING;

    gtk_widget_queue_draw (GTK_WIDGET (panel));
    return;
  }

  /* refresh the padding */
  awn_background_padding_request (priv->bg, priv->position,
                                  &top, &bottom, &left, &right);

  /* never actually set the top padding, its only internal constant */
  /* well actually non-composited env could use also the top padding */
  switch (priv->position)
  {
    case GTK_POS_TOP:
      priv->extra_padding = bottom + top;
      if (priv->composited) bottom = 0;
      break;
    case GTK_POS_BOTTOM:
      priv->extra_padding = bottom + top;
      if (priv->composited) top = 0;
      break;
    case GTK_POS_LEFT:
      priv->extra_padding = left + right;
      if (priv->composited) right = 0;
      break;
    case GTK_POS_RIGHT:
      priv->extra_padding = left + right;
      if (priv->composited) left = 0;
      break;
  }

  priv->extra_padding += AWN_EFFECTS_ACTIVE_RECT_PADDING;

  gtk_alignment_set_padding (GTK_ALIGNMENT (priv->alignment),
                             top, bottom, left, right);

  gtk_widget_queue_draw (GTK_WIDGET (panel));
}

#ifdef DEBUG_APPLET_AREA
static
void awn_panel_get_applet_rect (AwnPanel *panel,
                                GdkRectangle *area,
                                gint width, gint height)
{
  AwnPanelPrivate *priv = panel->priv;
  GtkAllocation alloc;
  GtkAllocation manager_alloc;

  /*
   * We provide a param for width & height, cause for example
   * in the configure event callback allocation field is not yet updated.
   * Otherwise zeroes can be used for width & height
   */
  gtk_widget_get_allocation (GTK_WIDGET (panel), &alloc);
  if (!width) width = alloc.width;
  if (!height) height = alloc.height;

  // FIXME: this whole function should call some method of AppletManager

  gtk_widget_get_allocation (priv->manager, &manager_alloc);
  area->x = manager_alloc.x;
  area->y = manager_alloc.y;
  area->width = manager_alloc.width;
  area->height = manager_alloc.height;

  guint top, bottom, left, right;
  gtk_alignment_get_padding (GTK_ALIGNMENT (priv->alignment),
                             &top, &bottom, &left, &right);

  gint paintable_size = priv->offset + priv->size;

  /* this should work for both composited and non-composited */
  switch (priv->position)
  {
    case GTK_POS_TOP:
      area->y = top;
      area->height = paintable_size;
      break;

    case GTK_POS_BOTTOM:
      area->y = height - paintable_size - bottom;
      area->height = paintable_size;
      break;

    case GTK_POS_RIGHT:
      area->x = width - paintable_size - right;
      area->width = paintable_size;
      break;

    case GTK_POS_LEFT:
    default:
      area->x = left;
      area->width = paintable_size;
  }
}
#endif

static void
awn_panel_get_draw_rect (AwnPanel *panel,
                         GdkRectangle *area,
                         gint width, gint height)
{
  AwnPanelPrivate *priv = panel->priv;
  GtkAllocation alloc;
  gfloat align;

  /* 
   * We provide a param for width & height, cause for example
   * in the configure event callback allocation field is not yet updated.
   * Otherwise zeroes can be used for width & height
   */
  gtk_widget_get_allocation (GTK_WIDGET (panel), &alloc);
  if (!width) width = alloc.width;
  if (!height) height = alloc.height;

  /* if we're not composited the whole window is drawable */
  if (priv->composited == FALSE)
  {
    area->x = 0; area->y = 0;
    area->width = width; area->height = height;
    return;
  }

  gint paintable_size = priv->offset + priv->size + priv->extra_padding;
  align = priv->monitor->align;
  if (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL &&
      (priv->position == GTK_POS_TOP || priv->position == GTK_POS_BOTTOM))
  {
    align = 1.0 - align;
  }

  switch (priv->position)
  {
    case GTK_POS_TOP:
      area->y = 0;
      area->height = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->x = ROUND ((width - priv->draw_width) * align);
        area->width = priv->draw_width;
      }
      else
      {
        area->x = 0;
        area->width = width;
      }
      break;

    case GTK_POS_BOTTOM:
      area->y = height - paintable_size;
      area->height = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->x = ROUND ((width - priv->draw_width) * align);
        area->width = priv->draw_width;
      }
      else
      {
        area->x = 0;
        area->width = width;
      }
      break;

    case GTK_POS_RIGHT:
      area->x = width - paintable_size;
      area->width = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->y = ROUND ((height - priv->draw_height) * align);
        area->height = priv->draw_height;
      }
      else
      {
        area->y = 0;
        area->height = height;
      }
      break;

    case GTK_POS_LEFT:
    default:
      area->x = 0;
      area->width = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->y = ROUND ((height - priv->draw_height) * align);
        area->height = priv->draw_height;
      }
      else
      {
        area->y = 0;
        area->height = height;
      }
      break;
  }
}

static void free_inhibit_item (gpointer data)
{
  AwnInhibitItem *item = data;

  if (item->description) g_free (item->description);

  g_free (item);
}

static GdkRegion*
awn_panel_get_mask (AwnPanel *panel)
{
  AwnPanelPrivate *priv;
  GtkAllocation viewport_alloc;
  GdkRegion *region, *viewport_region;
  gdouble viewport_offset_x, viewport_offset_y;

  priv = panel->priv;

  region = awn_applet_manager_get_mask (AWN_APPLET_MANAGER (priv->manager),
                                        priv->path_type, priv->offset_mod);
  /* the applets are in viewport */
  viewport_offset_x = 
    gtk_adjustment_get_value (
        gtk_viewport_get_hadjustment (GTK_VIEWPORT (priv->viewport)));
  viewport_offset_y = 
    gtk_adjustment_get_value (
        gtk_viewport_get_vadjustment (GTK_VIEWPORT (priv->viewport)));
  gtk_widget_get_allocation (priv->viewport, &viewport_alloc);
  gdk_region_offset (region, viewport_alloc.x - viewport_offset_x,
                     viewport_alloc.y - viewport_offset_y);

  /* intersect with viewport */
  viewport_region = gdk_region_rectangle ((GdkRectangle*)&viewport_alloc);
  gdk_region_intersect (region, viewport_region);
  gdk_region_destroy (viewport_region);

  if (gtk_widget_get_visible (GTK_WIDGET (priv->arrow1)))
  {
    GdkRegion *icon_mask1, *icon_mask2;

    icon_mask1 = awn_icon_get_input_mask (AWN_ICON (priv->arrow1));
    gdk_region_union (region, icon_mask1);

    icon_mask2 = awn_icon_get_input_mask (AWN_ICON (priv->arrow2));
    gdk_region_union (region, icon_mask2);

    gdk_region_destroy (icon_mask1);
    gdk_region_destroy (icon_mask2);
  }

  return region;
}

static gboolean awn_panel_check_mouse_pos (AwnPanel *panel,
                                           MouseCheckType check_type)
{
  g_return_val_if_fail(AWN_IS_PANEL(panel), FALSE);

  GtkWidget *widget = GTK_WIDGET (panel);
  AwnPanelPrivate *priv = panel->priv;
  GdkWindow *panel_win;
  GdkRectangle area;

  gint x, y, window_x, window_y, width, height;
  /* FIXME: probably needs some love to work on multiple monitors */
  gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);
  panel_win = gtk_widget_get_window (widget);
  gdk_window_get_root_origin (panel_win, &window_x, &window_y);

  switch (check_type)
  {
    case MOUSE_CHECK_EDGE_ONLY:
    {
      /* edge detection */
      awn_panel_get_draw_rect (AWN_PANEL (panel), &area, 0, 0);
      window_x += area.x;
      window_y += area.y;
      width = area.width;
      height = area.height;

      switch (priv->position)
      {
        case GTK_POS_LEFT:
          return y >= window_y && y < window_y + height &&
                 x == window_x;
        case GTK_POS_RIGHT:
          return y >= window_y && y < window_y + height &&
                 x == window_x + width - 1;
        case GTK_POS_TOP:
          return x >= window_x && x < window_x + width &&
                 y == window_y;
        case GTK_POS_BOTTOM:
        default:
          return x >= window_x && x < window_x + width &&
                 y == window_y + height - 1;
      }
    }
    case MOUSE_CHECK_ACTIVE_MASK:
    {
      // FIXME: 
      //   best would be to check if mouse is inside InputShape of the window,
      //   but we can't do it because the checks are happening also while
      //   in clickthrough mode
      //   solution could be to save the mask which is created in update_masks
      GdkRegion *region = awn_panel_get_mask (panel);
      gdk_region_offset (region, window_x, window_y);
      gboolean inside_mask = gdk_region_point_in (region, x, y);
      gdk_region_destroy (region);

      if (inside_mask) return TRUE;

      awn_panel_get_draw_rect (AWN_PANEL (panel), &area, 0, 0);
      window_x += area.x;
      window_y += area.y;
      width = area.width;
      height = area.height;

      return (x >= window_x && x < window_x + width &&
              y >= window_y && y < window_y + height);
    }
    case MOUSE_CHECK_ENTIRE_WINDOW:
    {
      GtkAllocation alloc;

      gtk_widget_get_allocation (widget, &alloc);
      width = alloc.width;
      height = alloc.height;
      
      return (x >= window_x && x < window_x + width &&
              y >= window_y && y < window_y + height);
    }
  }

  return FALSE;
}

/* Auto-hide fade out method */
static gboolean 
alpha_blend_hide (gpointer data)
{
  const int HIDE_COUNTER_MAX = 8;
  g_return_val_if_fail (AWN_IS_PANEL (data), FALSE);

  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;
  GdkWindow *win;

  priv->hide_counter++;

  win = gtk_widget_get_window (GTK_WIDGET (panel));

  gdk_window_set_opacity (win, 1 - 1.0 / HIDE_COUNTER_MAX * priv->hide_counter);

  if (priv->hide_counter >= HIDE_COUNTER_MAX)
  {
    priv->hiding_timer_id = 0;
    priv->autohide_always_visible = FALSE; /* see the note in start function */
    gdk_window_set_opacity (win, 1.0);
    gtk_widget_hide (GTK_WIDGET (panel));
    return FALSE;
  }

  return TRUE;
}

static gboolean
alpha_blend_start (AwnPanel *panel, gpointer data)
{
  AwnPanelPrivate *priv = panel->priv;
  priv->hide_counter = 0;
  priv->hiding_timer_id = g_timeout_add (40, alpha_blend_hide, panel);

  /* A hack: we will set autohide_always_visible ourselves
   * when the animation's internal timer expires, so that while the window is
   * in the process of hiding it can be interrupted by hovering over the window
   * itself, but when it's hidden user needs to touch the edge
   */
  return TRUE;
}

static void
alpha_blend_end (AwnPanel *panel, gpointer data)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->hiding_timer_id)
  {
    GdkWindow *win;
    win = gtk_widget_get_window (GTK_WIDGET (panel));
    g_source_remove (priv->hiding_timer_id);
    priv->hiding_timer_id = 0;
    gdk_window_set_opacity (win, 1.0);
  }
  else
  {
    position_window (panel);
    gtk_widget_show (GTK_WIDGET (panel));
    
  }
}

/* Auto-hide keep below method */
static gboolean keep_below_start (AwnPanel *panel, gpointer data)
{
  /* wow, setting keep_below makes the border appear and unsticks us */
  gtk_window_set_decorated (GTK_WINDOW (panel), FALSE);

  gtk_window_set_keep_below (GTK_WINDOW (panel), TRUE);

  return FALSE;
}

static void keep_below_end (AwnPanel *panel, gpointer data)
{
  gtk_window_set_keep_below (GTK_WINDOW (panel), FALSE);
}

/* Auto-hide transparentize method */
static gboolean transparentize_start (AwnPanel *panel, gpointer data)
{
  AwnPanelPrivate *priv = panel->priv;

  priv->last_clickthrough_type = priv->clickthrough_type;
  awn_panel_set_clickthrough_type (panel, CLICKTHROUGH_ON_NOCTRL);

  return FALSE;
}

static void transparentize_end (AwnPanel *panel, gpointer data)
{
  AwnPanelPrivate *priv = panel->priv;

  awn_panel_set_clickthrough_type (panel, priv->last_clickthrough_type);
}

/* Auto-hide internal implementation */
static gboolean
autohide_start_timeout (gpointer data)
{
  g_return_val_if_fail(AWN_IS_PANEL(data), FALSE);
  gboolean signal_ret = FALSE;

  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;

  priv->autohide_start_timer_id = 0;

  if (priv->autohide_inhibited ||
      awn_panel_check_mouse_pos (panel, MOUSE_CHECK_ACTIVE_MASK))
  {
     return FALSE;
  }

  priv->autohide_started = TRUE;
  g_signal_emit (panel, _panel_signals[AUTOHIDE_START], 0, &signal_ret);
  priv->autohide_always_visible = signal_ret;

  return FALSE;
}

static gboolean
poll_mouse_position (gpointer data)
{
  g_return_val_if_fail(AWN_IS_PANEL(data), FALSE);

  GtkWidget *widget = GTK_WIDGET (data);
  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;

  /* Auto-Hide stuff */
  if (priv->autohide_type != AUTOHIDE_TYPE_NONE)
  {
    if (awn_panel_check_mouse_pos (panel,
          !priv->autohide_started || priv->autohide_always_visible ?
          MOUSE_CHECK_ACTIVE_MASK : MOUSE_CHECK_EDGE_ONLY))
    {
      /* we are on the window or its edge */
      if (priv->autohide_start_timer_id)
      {
        g_source_remove (priv->autohide_start_timer_id);
        priv->autohide_start_timer_id = 0;
        return TRUE;
      }
      if (priv->autohide_started)
      {
        priv->autohide_started = FALSE;
        g_signal_emit (panel, _panel_signals[AUTOHIDE_END], 0);
      }
    }
    else if (gtk_widget_get_mapped (GTK_WIDGET (widget)))
    {
      /* mouse is away, panel should start hiding */
      if (priv->autohide_start_timer_id == 0  && !priv->autohide_started)
      {
        /* the timeout will emit autohide-start */
        priv->autohide_start_timer_id =
          g_timeout_add (priv->autohide_hide_delay,
                         autohide_start_timeout, panel);
      }
    }
  }

  /* Docklet close on mouse-out */
  if (priv->docklet && priv->docklet_close_on_mouse_out)
  {
    if (!awn_panel_check_mouse_pos (panel, MOUSE_CHECK_ACTIVE_MASK))
    {
      awn_panel_docklet_destroy (panel);
    }
  }

  /* Clickthrough on CTRL */
  GdkModifierType mask; 
  gdk_display_get_pointer (gdk_display_get_default (), NULL, NULL, NULL, &mask);

  /* specialstate is TRUE whenever someone hovers Awn while holding
   *  the ctrl-key
   */
  gboolean specialstate = (mask & GDK_CONTROL_MASK) &&
                          awn_panel_check_mouse_pos (panel,
                                                     MOUSE_CHECK_ACTIVE_MASK);

  GdkWindow *win;
  win = gtk_widget_get_window (GTK_WIDGET (panel));
  switch (priv->clickthrough_type )
  {
    case CLICKTHROUGH_NEVER:
      if (priv->clickthrough)
      {
        priv->clickthrough = FALSE;
        awn_panel_update_masks (widget, priv->old_width, priv->old_height);
        gdk_window_set_opacity (win, 1.0);
      }
      break;
    case CLICKTHROUGH_ON_CTRL:
      if (priv->clickthrough && !specialstate)
      {
        priv->clickthrough = FALSE;
        awn_panel_update_masks (widget, priv->old_width, priv->old_height);
        gdk_window_set_opacity (win, 1.0);
      }
      else if (!priv->clickthrough && specialstate)
      {
        priv->clickthrough = TRUE;
        awn_panel_update_masks (widget, priv->old_width, priv->old_height);
        gdk_window_set_opacity (win, CLICKTHROUGH_OPACITY);
      }
      break;
    case CLICKTHROUGH_ON_NOCTRL:
      if (priv->clickthrough && specialstate)
      {
        priv->clickthrough = FALSE;
        awn_panel_update_masks (widget, priv->old_width, priv->old_height);
        gdk_window_set_opacity (win, 1.0);
      }
      else if (!priv->clickthrough && !specialstate)
      {
        priv->clickthrough = TRUE;
        awn_panel_update_masks (widget, priv->old_width, priv->old_height);
        gdk_window_set_opacity (win, CLICKTHROUGH_OPACITY);
      }
      break;
  }

  /* DETERMINE WHEN TO STOP POLLING */
  
  /* Keep on polling when autohide */
  if (priv->autohide_type != AUTOHIDE_TYPE_NONE)
    return TRUE;
  
  /* Keep on polling on noctrl clickthrough */
  if (priv->clickthrough_type == CLICKTHROUGH_ON_NOCTRL)
    return TRUE;

  /* Keep on polling when hovering the panel and ctrl clickthrough
   *  is activated
   */
  if (awn_panel_check_mouse_pos (panel, MOUSE_CHECK_ACTIVE_MASK) &&
      priv->clickthrough_type == CLICKTHROUGH_ON_CTRL)
    return TRUE;

  /* Keep on polling if we're in docklet mode and we need to close it */
  if (priv->docklet && priv->docklet_close_on_mouse_out)
    return TRUE;

  /* In other cases, the polling may end */
  priv->mouse_poll_timer_id = 0;
  return FALSE;
}

static gboolean
awn_panel_dnd_check (gpointer data)
{
  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;
  gboolean proxy_set = FALSE;

  proxy_set = awn_panel_set_drag_proxy (panel, TRUE);

  if (!proxy_set)
  {
    // kill the timer
    priv->dnd_mouse_poll_timer_id = 0;
  }
  return proxy_set;
}

static void
awn_panel_dispose (GObject *object)
{
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (object);

  /* Destroy the timers */
  if (priv->hiding_timer_id)
  {
    g_source_remove (priv->hiding_timer_id);
    priv->hiding_timer_id = 0;
  }

  if (priv->mouse_poll_timer_id)
  {
    g_source_remove (priv->mouse_poll_timer_id);
    priv->mouse_poll_timer_id = 0;
  }

  if (priv->autohide_start_timer_id)
  {
    g_source_remove (priv->autohide_start_timer_id);
    priv->autohide_start_timer_id = 0;
  }

  if (priv->resize_timer_id)
  {
    g_source_remove (priv->resize_timer_id);
    priv->resize_timer_id = 0;
  }

  desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                        object, NULL);

  G_OBJECT_CLASS (awn_panel_parent_class)->dispose (object);
}

static void
awn_panel_finalize (GObject *object)
{
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (object);

  if (priv->inhibits)
  {
    g_hash_table_destroy (priv->inhibits);
    priv->inhibits = NULL;
  }

  G_OBJECT_CLASS (awn_panel_parent_class)->finalize (object);
}

#include "awn-panel-glue.h"

static void
awn_panel_class_init (AwnPanelClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *cont_class = GTK_CONTAINER_CLASS (klass);

  obj_class->constructed   = awn_panel_constructed;
  obj_class->dispose       = awn_panel_dispose;
  obj_class->finalize      = awn_panel_finalize;
  obj_class->get_property  = awn_panel_get_property;
  obj_class->set_property  = awn_panel_set_property;
    
  cont_class->add          = awn_panel_add;
  
  wid_class->expose_event  = awn_panel_expose;
  wid_class->show          = awn_panel_show;
  wid_class->size_request  = awn_panel_size_request;
  wid_class->button_press_event = awn_panel_button_press;

#if !GTK_CHECK_VERSION(2, 19, 5)
  wid_class->drag_motion   = awn_panel_drag_motion;
#endif

  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_PANEL_ID,
    g_param_spec_int ("panel-id",
                      "Panel ID",
                      "The panel ID",
                      1, G_MAXINT, 1,
                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_object ("client",
                         "Client",
                         "The configuration client",
                         DESKTOP_AGNOSTIC_CONFIG_TYPE_CLIENT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_MONITOR,
    g_param_spec_pointer ("monitor",
                          "Monitor",
                          "The AwnMonitor",
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_APPLET_MANAGER,
    g_param_spec_pointer ("applet-manager",
                          "Applet manager",
                          "The AwnAppletManager",
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_PANEL_XID,
    g_param_spec_int64   ("panel-xid",
                          "Panel XID",
                          "The XID of the panel",
                          G_MININT64, G_MAXINT64, 0,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_COMPOSITED,
    g_param_spec_boolean ("composited",
                          "Composited",
                          "The window is composited",
                          TRUE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (obj_class,
    PROP_PANEL_MODE,
    g_param_spec_boolean ("panel-mode",
                          "Panel Mode",
                          "The window sets the appropriete panel size hints",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (obj_class,
    PROP_EXPAND,
    g_param_spec_boolean ("expand",
                          "Expand",
                          "The panel will expand to fill the entire "
                          "width/height of the screen",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          G_PARAM_STATIC_STRINGS));
 g_object_class_install_property (obj_class,
    PROP_POSITION,
    g_param_spec_int ("position",
                      "Position",
                      "The position of the panel",
                      0, 3, GTK_POS_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));
   
  g_object_class_install_property (obj_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "Size",
                      "The size of the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (obj_class,
    PROP_MAX_SIZE,
    g_param_spec_int ("max-size",
                      "Max Size",
                      "Maximum size for drawing on the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (obj_class,
    PROP_OFFSET,
    g_param_spec_int ("offset",
                      "Offset",
                      "An offset for applets in the panel",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_PATH_TYPE,
    g_param_spec_int ("path-type",
                      "Path Type",
                      "Path type used by the panel background",
                      0, AWN_PATH_LAST-1, AWN_PATH_LINEAR,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_OFFSET_MODIFIER,
    g_param_spec_float ("offset-modifier",
                        "Offset Modifier",
                        "Offset modifier used by current path type",
                        -G_MAXFLOAT, G_MAXFLOAT, 1.0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_AUTOHIDE_TYPE,
    g_param_spec_int ("autohide-type",
                      "Autohide type",
                      "Type of used autohide",
                      0, AUTOHIDE_TYPE_LAST - 1, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_AUTOHIDE_HIDE_DELAY,
    g_param_spec_int ("autohide-hide-delay",
                      "Autohide Hide Delay",
                      "Delay between mouse leaving the panel and it hiding",
                      50, 10000, 1000,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_AUTOHIDE_POLL_DELAY,
    g_param_spec_int ("autohide-poll-delay",
                      "Autohide Poll Delay",
                      "Delay for mouse position polling",
                      40, 5000, 500,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_STYLE,
    g_param_spec_int ("style",
                      "Style",
                      "Style of the bar",
                      0, STYLE_LAST - 1, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_CLICKTHROUGH,
    g_param_spec_int ("clickthrough-type",
                      "Clickthrough type",
                      "Type of clickthrough action",
                      0, CLICKTHROUGH_LAST - 1, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  /* Add signals to the class */
  _panel_signals[SIZE_CHANGED] =
		g_signal_new ("size_changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, size_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT, 
			      G_TYPE_NONE,
			      1, G_TYPE_INT);
  
  _panel_signals[POS_CHANGED] =
		g_signal_new ("position_changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, position_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE,
			      1, G_TYPE_INT);

  _panel_signals[OFFSET_CHANGED] =
		g_signal_new ("offset_changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, offset_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT, 
			      G_TYPE_NONE,
			      1, G_TYPE_INT);

  _panel_signals[PROPERTY_CHANGED] =
		g_signal_new ("property-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, property_changed),
			      NULL, NULL,
			      awn_marshal_VOID__STRING_BOXED,
			      G_TYPE_NONE,
			      2, G_TYPE_STRING, G_TYPE_VALUE);

  _panel_signals[DESTROY_NOTIFY] =
		g_signal_new ("destroy_notify",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, destroy_notify),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

  _panel_signals[DESTROY_APPLET] =
		g_signal_new ("destroy_applet",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, destroy_applet),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1, G_TYPE_STRING);

  _panel_signals[AUTOHIDE_START] =
		g_signal_new ("autohide_start",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, autohide_start),
			      NULL, NULL,
			      awn_marshal_BOOLEAN__VOID,
			      G_TYPE_BOOLEAN,
			      0);

  _panel_signals[AUTOHIDE_END] =
		g_signal_new ("autohide_end",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, autohide_end),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

  g_type_class_add_private (obj_class, sizeof (AwnPanelPrivate));

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass), 
                                   &dbus_glib_awn_panel_object_info);
}

static gboolean
viewport_expose (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  gpointer bin_class = g_type_class_peek (GTK_TYPE_BIN);
  GTK_WIDGET_CLASS (bin_class)->expose_event (widget, event);

  return TRUE;
}

static void
viewport_size_req (GtkWidget *widget, GtkRequisition *req, gpointer data)
{
  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;

  guint t, b, l, r;
  gboolean arrows_visible = FALSE;
  GtkRequisition arrow_req;
  gtk_widget_size_request (priv->arrow1, &arrow_req);
  awn_background_padding_request (priv->bg, priv->position, &t, &b, &l, &r);

  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      if (req->width > priv->monitor->width - (gint)l - (gint)r)
      {
        arrows_visible = TRUE;
        req->width = priv->monitor->width - l - r - 2*arrow_req.width;
      }
      break;
    default:
      if (req->height > priv->monitor->height - (gint)t - (gint)b)
      {
        arrows_visible = TRUE;
        req->height = priv->monitor->height - t - b - 2*arrow_req.height;
      }
      break;
  }

  if (arrows_visible)
  {
    if (!gtk_widget_get_visible (GTK_WIDGET (priv->arrow1)))
    {
      gtk_widget_show (priv->arrow1);
      gtk_widget_show (priv->arrow2);
    }
    gtk_container_set_resize_mode (GTK_CONTAINER (priv->viewport),
                                   GTK_RESIZE_QUEUE);
  }
  else
  {
    if (gtk_widget_get_visible (GTK_WIDGET (priv->arrow1)))
    {
      gtk_widget_hide (priv->arrow1);
      gtk_widget_hide (priv->arrow2);
    }
    gtk_container_set_resize_mode (GTK_CONTAINER (priv->viewport),
                                   GTK_RESIZE_PARENT);
  }
}

static gboolean
awn_panel_scroll_timer (AwnPanel *panel)
{
  gdouble max, value;
  GtkAllocation alloc;
  GtkAdjustment *adj;
  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (panel);

  gpointer data = g_object_get_data (G_OBJECT (panel), "scrolled-icon");

  if (data == NULL)
  {
    priv->scroll_timer_id = 0;
    return FALSE;
  }

  gboolean inc = data != priv->arrow1;
  gint mult = inc ? 1 : -1;

  gtk_widget_get_allocation (priv->viewport, &alloc);

  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      adj = gtk_viewport_get_hadjustment (GTK_VIEWPORT (priv->viewport));
      max = gtk_adjustment_get_upper (adj) - alloc.width;
      break;
    default:
      adj = gtk_viewport_get_vadjustment (GTK_VIEWPORT (priv->viewport));
      max = gtk_adjustment_get_upper (adj) - alloc.height;
      break;
  }
  value = CLAMP (gtk_adjustment_get_value (adj) + 8 * mult, 0.0, max);
  gtk_adjustment_set_value (adj, value);

  // without this there'll be artifacts
  awn_applet_manager_redraw_throbbers (AWN_APPLET_MANAGER (priv->manager));

  if ((inc && value >= max) || (!inc && value <= 0.0))
  {
    priv->scroll_timer_id = 0;
    return FALSE;
  }

  return TRUE;
}

static gboolean
awn_panel_arrow_over (AwnPanel *panel, GdkEventCrossing *event,
                      GtkWidget *icon)
{
  AwnPanelPrivate *priv = panel->priv;

  g_object_set_data (G_OBJECT (panel), "scrolled-icon", icon);
  if (priv->scroll_timer_id == 0)
  {
    priv->scroll_timer_id =
      g_timeout_add (40, (GSourceFunc)awn_panel_scroll_timer, panel);
  }

  return FALSE;
}

static gboolean
awn_panel_arrow_out (AwnPanel *panel, GdkEventCrossing *event,
                     GtkWidget *icon)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->scroll_timer_id != 0)
  {
    g_source_remove (priv->scroll_timer_id);
    priv->scroll_timer_id = 0;
  }

  return FALSE;
}

static void
awn_panel_init (AwnPanel *panel)
{
  AwnPanelPrivate *priv;

  priv = panel->priv = AWN_PANEL_GET_PRIVATE (panel);

  priv->draw_width = 32;
  priv->draw_height = 32;

  priv->docklet_alpha = 1.0;
  priv->docklet_close_on_pos_change = TRUE;

  priv->inhibits = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                          NULL, free_inhibit_item);

  gtk_widget_set_app_paintable (GTK_WIDGET (panel), TRUE);

  /*
   * Create the window hierarchy:
   *
   * - AwnPanel
   *   - GtkEventBox                (priv->eventbox)
   *     - GtkAlignment             (priv->alignment)
   *       - AwnBox                 (priv->box)
   *         - AwnThrobber          (priv->arrow1)
   *         - GtkViewport          (priv->viewport)
   *           - AwnAppletManager   (priv->manager)
   *         - AwnThrobber          (priv->arrow2)
   */
  priv->eventbox = gtk_event_box_new ();
  gtk_widget_set_app_paintable (priv->eventbox, TRUE);
  GTK_CONTAINER_CLASS (awn_panel_parent_class)->add (GTK_CONTAINER (panel),
                                                     priv->eventbox);
  gtk_widget_show (priv->eventbox);
  
  priv->alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_container_add (GTK_CONTAINER (priv->eventbox), priv->alignment);
  gtk_widget_show (priv->alignment);

  priv->box = awn_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_container_add (GTK_CONTAINER (priv->alignment), priv->box);

  priv->arrow1 = awn_throbber_new_with_config (
      awn_config_get_default (0, NULL));
  awn_throbber_set_type (AWN_THROBBER (priv->arrow1),
                         AWN_THROBBER_TYPE_ARROW_1);
  g_signal_connect_swapped (priv->arrow1, "enter-notify-event",
                            G_CALLBACK (awn_panel_arrow_over), panel);
  g_signal_connect_swapped (priv->arrow1, "leave-notify-event",
                            G_CALLBACK (awn_panel_arrow_out), panel);
  gtk_box_pack_start (GTK_BOX (priv->box), priv->arrow1, FALSE, TRUE, 0);

  priv->arrow2 = awn_throbber_new_with_config (
      awn_config_get_default (0, NULL));
  awn_throbber_set_type (AWN_THROBBER (priv->arrow2),
                         AWN_THROBBER_TYPE_ARROW_2);
  g_signal_connect_swapped (priv->arrow2, "enter-notify-event",
                            G_CALLBACK (awn_panel_arrow_over), panel);
  g_signal_connect_swapped (priv->arrow2, "leave-notify-event",
                            G_CALLBACK (awn_panel_arrow_out), panel);
  gtk_box_pack_end (GTK_BOX (priv->box), priv->arrow2, FALSE, TRUE, 0);

  gtk_widget_show (priv->box);

  priv->viewport = gtk_viewport_new (NULL, NULL);
  g_signal_connect (priv->viewport, "expose-event",
                    G_CALLBACK (viewport_expose), NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (priv->viewport),
                                GTK_SHADOW_NONE);
  g_signal_connect (priv->viewport, "size-request", 
                    G_CALLBACK (viewport_size_req), panel);
  gtk_box_pack_start (GTK_BOX (priv->box), priv->viewport, TRUE, TRUE, 0);
  awn_utils_ensure_transparent_bg (priv->viewport);
  gtk_widget_show (priv->viewport);

  g_signal_connect (priv->eventbox, "expose-event",
                    G_CALLBACK (on_eb_expose), panel);
  awn_utils_ensure_transparent_bg (priv->eventbox);

  g_signal_connect (panel, "enter-notify-event", 
                    G_CALLBACK (on_mouse_over), NULL);
  g_signal_connect (panel, "leave-notify-event", 
                    G_CALLBACK (on_mouse_out), NULL);
  awn_utils_ensure_transparent_bg (GTK_WIDGET (panel));
  gtk_window_set_resizable (GTK_WINDOW (panel), FALSE);
}

//#define DEBUG_INVALIDATION
#ifdef DEBUG_INVALIDATION
static gboolean debug_invalidating (gpointer data)
{
  gdk_window_set_debug_updates (TRUE);

  return FALSE;
}
#endif

GtkWidget *
awn_panel_new_with_panel_id (gint panel_id)
{
  GtkWidget *window;
  DesktopAgnosticConfigClient *client;
  GError *error = NULL;

  client = awn_config_get_default (panel_id, &error);

  if (error)
  {
    g_warning ("Unable to retrieve the configuration client: %s",
               error->message);
    g_error_free (error);
    return NULL;
  }

  window = g_object_new (AWN_TYPE_PANEL,
                         "type", GTK_WINDOW_TOPLEVEL,
                         "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
                         "panel-id", panel_id,
                         "client", client,
                         NULL);
#ifdef DEBUG_INVALIDATION
  g_timeout_add (5000, debug_invalidating, NULL);
#endif

  return window;
}

/*
 * COMPOSITED STATE CODE
 */

static void
load_correct_colormap (GtkWidget *panel)
{
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (panel);
  GdkScreen       *screen;
  GdkColormap     *colormap;

  screen = gtk_widget_get_screen (panel);

  /* First try loading the RGBA colormap */
  colormap = gdk_screen_get_rgba_colormap (screen);
  if (!colormap)
  {
      /* Use the RGB colormap and set composited to FALSE */
      colormap = gdk_screen_get_rgb_colormap (screen);
      priv->composited = FALSE;
  }

  gtk_widget_set_colormap (panel, colormap);
}

/*
 * This weird function tries to workaround an X(?) bug where the composited
 * child windows do not repaint properly after calling gtk_widget_show().
 * Affects only fade-out autohide type.
 */
static gboolean
awn_panel_schedule_redraw (gpointer user_data)
{
  GtkWidget *widget = (GtkWidget*)user_data;

  g_return_val_if_fail (AWN_IS_PANEL (widget), FALSE);

  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (widget);

  gint x, y;
  gtk_widget_translate_coordinates (priv->viewport, widget, 
                                    0, 0, &x, &y);
  gtk_widget_queue_draw_area (widget, x, y, 
                              priv->viewport->allocation.width,
                              priv->viewport->allocation.height);

  // increase the timer in every step
  switch (priv->withdraw_redraw_timer)
  {
    case 0: priv->withdraw_redraw_timer = 50; break;
    case 50: priv->withdraw_redraw_timer = 150; break;
    case 150: priv->withdraw_redraw_timer = 350; break;
    case 350: priv->withdraw_redraw_timer = 850; break;
    default: priv->withdraw_redraw_timer = 0; break;
  }

  priv->withdraw_timer_id = priv->withdraw_redraw_timer == 0 ? 0 :
    g_timeout_add (priv->withdraw_redraw_timer, awn_panel_schedule_redraw,
                   widget);

  return FALSE;
}

/*
 Window state has changed...  awn does a lot of things can trigger unwanted
 state changes.
 */
static gboolean
on_window_state_event (GtkWidget *widget, GdkEventWindowState *event)
{
  /*
   Have we just lost sticky?
   */

  /*
   It would be nice to check event->changed_mask to see if STICKY changed but
   we don't get the initail state change signal when we set sticky in at least
   one WM (openbox).
   */
  if ( ! (GDK_WINDOW_STATE_STICKY & event->new_window_state) )
  {
    /*
     For whatever reason, sticky is gone.  We don't want that.
     */
    gtk_window_stick (GTK_WINDOW (widget));
  }

  if (GDK_WINDOW_STATE_WITHDRAWN & event->changed_mask)
  {
    if ( ! (GDK_WINDOW_STATE_WITHDRAWN & event->new_window_state))
    {
      AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (widget);
      if (priv->withdraw_timer_id == 0 && priv->composited)
      {
        priv->withdraw_timer_id =
          g_idle_add (awn_panel_schedule_redraw, widget);
      }
    }
  }
  return FALSE;
}

static void 
on_composited_changed (GtkWidget *widget, gpointer data)
{

  g_return_if_fail (AWN_IS_PANEL (widget));

  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE (widget);
  GdkWindow *win;
  win = gtk_widget_get_window (priv->eventbox);

  priv->composited = gtk_widget_is_composited (widget);
  priv->animated_resize = priv->composited;

  if (priv->composited)
  {
    gtk_widget_shape_combine_mask (widget, NULL, 0, 0);
  }
  else
  {
    gtk_widget_input_shape_combine_mask (widget, NULL, 0, 0);
  }
  gdk_window_set_composited (win, priv->composited);

  awn_panel_refresh_padding (AWN_PANEL (widget), NULL);

  awn_panel_reset_autohide (AWN_PANEL (widget));

  position_window (AWN_PANEL (widget));
}

/*
 * SIZING AND POSITIONING 
 */

/*
 * We set the shape of the window, so when in composited mode, we dont't 
 * receive events in the blank space above the main window
 */
static void
awn_panel_update_masks (GtkWidget *panel, 
                        gint       real_width, 
                        gint       real_height)
{
  AwnPanelPrivate *priv;
  GtkAllocation   alloc;
  GdkBitmap       *shaped_bitmap;
  cairo_t         *cr;

  g_return_if_fail (AWN_IS_PANEL (panel));
  priv = AWN_PANEL (panel)->priv;

  gtk_widget_get_allocation (GTK_WIDGET (panel), &alloc);

  if (!real_width) real_width = alloc.width;
  if (!real_height) real_height = alloc.height;

  if (priv->clickthrough && priv->composited)
  {
    GdkRegion *region = gdk_region_new ();
    GdkWindow *win = gtk_widget_get_window (GTK_WIDGET (panel));
    gdk_window_input_shape_combine_region (win, region, 0, 0);
    gdk_region_destroy (region);
  }
  else
  {
    shaped_bitmap = (GdkBitmap*)gdk_pixmap_new (NULL,
                                                real_width, real_height, 1);

    g_return_if_fail (shaped_bitmap);

    /* clear the bitmap */
    cr = gdk_cairo_create (shaped_bitmap);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    if (priv->bg)
    {
      GdkRectangle area;
      awn_panel_get_draw_rect (AWN_PANEL (panel), &area,
                               real_width, real_height);
      /* Set the input shape of the window if the window is composited */
      if (priv->composited)
      {
        awn_background_get_input_shape_mask (priv->bg, cr, priv->position, &area);
      }
      /* If window is not composited set shape of the window */
      else
      {
        awn_background_get_shape_mask (priv->bg, cr, priv->position, &area);
      }
    }

    /* combine with applet's eventbox (with proper dimensions) */
    cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
    cairo_set_source_rgb (cr, 1.0f, 1.0f, 1.0f);

    GdkRegion *region = awn_panel_get_mask (AWN_PANEL (panel));
    gdk_cairo_region (cr, region);
    cairo_fill (cr);
    gdk_region_destroy (region);
#if 0
    awn_panel_get_applet_rect (AWN_PANEL (panel), &applet_rect,
                               real_width, real_height);
    cairo_identity_matrix (cr);
    cairo_new_path (cr);
    cairo_rectangle (cr, applet_rect.x, applet_rect.y,
                     applet_rect.width, applet_rect.height);
    cairo_fill (cr);
#endif
    cairo_destroy (cr);

    if (priv->composited)
    {
      gtk_widget_input_shape_combine_mask (panel, shaped_bitmap, 0, 0);
    }
    else
    {
      gtk_widget_shape_combine_mask (panel, shaped_bitmap, 0, 0);
    }

    g_object_unref (shaped_bitmap);
  }
}

static gboolean
masks_update_scheduler (AwnPanel *panel)
{
  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);

  AwnPanelPrivate *priv = panel->priv;

  priv->masks_update_id = 0;

  awn_panel_update_masks (GTK_WIDGET (panel), 0, 0);

  return FALSE;
}

static void
awn_panel_queue_masks_update (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->masks_update_id == 0)
  {
    priv->masks_update_id = g_idle_add ((GSourceFunc)masks_update_scheduler,
                                        panel);
  }
}

/*
 * Position Awn's window 
 */
static gboolean
position_window (AwnPanel *panel)
{
  GtkWindow       *window  = GTK_WINDOW (panel);
  AwnPanelPrivate *priv    = panel->priv;
  AwnMonitor      *monitor = priv->monitor;
  gint             ww = 0, hh = 0, x = 0, y = 0;

  if (!gtk_widget_get_realized (GTK_WIDGET (panel)))
    return FALSE;

  gtk_window_get_size (GTK_WINDOW (window), &ww, &hh);
  
  switch (priv->position)
  {
    case GTK_POS_TOP:
      x = ROUND ((monitor->width - ww) * monitor->align) + monitor->x_offset;
      y = monitor->y_offset;
      break;

    case GTK_POS_RIGHT:
      x = monitor->width - ww + monitor->x_offset;
      y = ROUND ((monitor->height - hh) * monitor->align) + monitor->y_offset;
      break;

    case GTK_POS_BOTTOM:
      x = ROUND ((monitor->width - ww) * monitor->align) + monitor->x_offset;
      y = monitor->height - hh + monitor->y_offset;
      break;

    case GTK_POS_LEFT:
      x = monitor->x_offset;
      y = ROUND ((monitor->height - hh) * monitor->align) + monitor->y_offset;
      break;

    default:
      g_assert (0);
  }

  gtk_window_move (window, x, y);
  return FALSE;
}

/*
 * We get a configure event when the windows size changes. This is a good as
 * a time as any to update the input shape and the strut of the window.
 */
static gboolean
on_window_configure (GtkWidget          *panel,
                     GdkEventConfigure  *event)
{
  AwnPanelPrivate *priv;

  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);
  priv = AWN_PANEL (panel)->priv;

  if (priv->old_width != event->width || priv->old_height != event->height ||
      priv->old_position != priv->position)
  {
    priv->old_width = event->width;
    priv->old_height = event->height;
    priv->old_position = priv->position;
    priv->old_x = event->x;
    priv->old_y = event->y;

    awn_panel_update_masks (panel, event->width, event->height);

    /* Update position */
    position_window (AWN_PANEL (panel));

    /* Update the strut if the panel_mode is set */
    if (priv->panel_mode)
      awn_panel_queue_strut_update (AWN_PANEL (panel));

    return TRUE;
  }
  if (priv->old_x != event->x || priv->old_y != event->y)
  {
    priv->old_x = event->x;
    priv->old_y = event->y;

    /* Update the strut if the panel_mode is set */
    if (priv->panel_mode)
      awn_panel_queue_strut_update (AWN_PANEL (panel));

    return FALSE;
  }
  return FALSE;
}

static void   
on_geometry_changed   (AwnMonitor *monitor,
                       AwnPanel   *panel)
{
  AwnPanelPrivate *priv;

  g_return_if_fail (AWN_IS_PANEL (panel));
  priv = AWN_PANEL (panel)->priv;

  awn_panel_reset_autohide (panel);

  position_window (panel);
}

/*
 * PANEL BACKGROUND & EMBEDDING CODE
 */

static gboolean
on_eb_expose (GtkWidget *eb, GdkEventExpose *event, AwnPanel *panel)
{
  cairo_t         *cr;
  AwnPanelPrivate *priv = panel->priv;

  /* This method is only used in non-composited mode */
  if (priv->composited) return FALSE;

  /* Get our ctx */
  cr = gdk_cairo_create (gtk_widget_get_window (eb));
  g_return_val_if_fail (cr, FALSE);

  /* Clip */
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  if (priv->bg)
  {
    GdkRectangle area;
    awn_panel_get_draw_rect (panel, &area, 0, 0);
    awn_background_draw (priv->bg, cr, priv->position, &area);
  }

  return FALSE;
}

/*
 * Draw the panel 
 */
static gboolean
awn_panel_expose (GtkWidget *widget, GdkEventExpose *event)
{
  AwnPanelPrivate *priv;
  cairo_t         *cr;
  GtkWidget       *child;
  GdkWindow       *win;

  g_return_val_if_fail (AWN_IS_PANEL (widget), FALSE);
  priv = AWN_PANEL (widget)->priv;

  if (priv->composited == FALSE)
  {
    /* we dont need to paint anything, it will be overlayed by the eventbox */
    child = gtk_bin_get_child (GTK_BIN (widget));
    if (!GTK_IS_WIDGET (child))
      return TRUE;

    goto propagate;
  }

  win = gtk_widget_get_window (widget);

  /* Get our ctx */
  cr = gdk_cairo_create (win);
  g_return_val_if_fail (cr, FALSE);

  /* Clip */
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  /* The actual drawing of the background 
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr); */

  if (priv->bg)
  {
    GdkRectangle area;
    awn_panel_get_draw_rect (AWN_PANEL (widget), &area, 0, 0);
    awn_background_draw (priv->bg, cr, priv->position, &area);
  }

#if 0
  if (1)
  {
    // enable to paint the input shape masks of individual applets
    GList *list = gtk_container_get_children (GTK_CONTAINER (priv->manager));
    for (GList *iter = list; iter != NULL; iter = iter->next)
    {
      GtkWidget *s = (GtkWidget*)iter->data;
      if (GTK_IS_SOCKET (s) && gtk_widget_get_visible (s))
      {
        GdkWindow *plug_window = gtk_socket_get_plug_window(GTK_SOCKET (s));
        if (plug_window)
        {
          GtkAllocation alloc;

          gtk_widget_get_allocation (s, &alloc);

          cairo_save (cr);
          cairo_translate (cr, alloc.x, alloc.y);
          GdkRegion *region = xutils_get_input_shape (plug_window);
          cairo_set_source_rgba (cr, 1.0, 1.0, 0.0, 0.1);
          gdk_cairo_region (cr, region);
          cairo_fill (cr);
          gdk_region_destroy (region);
          cairo_restore (cr);
        }
      }
    }
    g_list_free (list);
  }
#endif

#if 0
  if (1)
  {
    // enable to see the mask AppletManager uses
    GdkRegion *region;
    region = awn_applet_manager_get_mask (AWN_APPLET_MANAGER (priv->manager),
                                          priv->path_type, priv->offset_mod);
    cairo_save (cr);
    cairo_set_source_rgba (cr, 0.0, 1.0, 0.3, 0.4);
    gdk_cairo_region (cr, region);
    cairo_fill (cr);
    cairo_restore (cr);
    gdk_region_destroy (region);
  }
#endif

  /* Pass on the expose event to the child */
  child = gtk_bin_get_child (GTK_BIN (widget));
  if (!GTK_IS_WIDGET (child))
    return TRUE;

  if (priv->composited)
  {
    GtkAllocation box_alloc;
    GdkRegion *region;
    cairo_t *window_cr = cr;

    // the applets must be always inside AppletManager, clipping to it's
    // allocation should make us perform better
    gtk_widget_get_allocation (priv->box, &box_alloc);

    // region is intersection of AppletManager allocation and event->region
    region = gdk_region_rectangle (&box_alloc);
    gdk_region_intersect (region, event->region);

    if (priv->docklet_alpha < 1.0)
    {
      // redirect painting to offscreen pixmap, so we can change its alpha
      cr = gdk_cairo_create (priv->tmp_pixmap);
      gdk_cairo_region (cr, region);
      cairo_clip (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
    }

    gdk_cairo_set_source_pixmap (cr, gtk_widget_get_window (child),
                                 child->allocation.x, child->allocation.y);

    gdk_cairo_region (cr, region);
    cairo_clip (cr);
    
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_paint (cr);

    if (window_cr != cr)
    {
      /* We need to be careful with clipping here - when painting dock_snapshot
       * it can be larger than AppletManager's allocation.
       */

      // decrease the alpha in temp pixmap
      cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OUT);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1 - priv->docklet_alpha);
      gdk_cairo_region (cr, region);
      cairo_fill (cr);
      cairo_destroy (cr);

      // copy back to window
      cr = window_cr;
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

      cairo_save (cr);
      gdk_cairo_rectangle (cr, &priv->snapshot_paint_size);
      cairo_clip (cr);
      gdk_cairo_set_source_pixmap (cr, priv->dock_snapshot, 0, 0);
      cairo_paint (cr);
      cairo_restore (cr);

      gdk_cairo_region (cr, region);
      cairo_clip (cr);
      gdk_cairo_set_source_pixmap (cr, priv->tmp_pixmap, 0, 0);
      cairo_paint (cr);
    }
    gdk_region_destroy (region);
  }

  cairo_destroy (cr);

#ifdef DEBUG_DRAW_AREA
  if (1)
  {
    /* Calling gdk_draw_rectangle (window, gc, FALSE, 0, 0, 20, 20) results
     * in an outlined rectangle with corners at (0, 0), (0, 20), (20, 20),
     * and (20, 0), which makes it 21 pixels wide and 21 pixels high.
     */
    GdkRectangle a;
    GdkColor color;

    awn_panel_get_draw_rect (AWN_PANEL (widget), &a, 0, 0);
    GdkGC *gc = gdk_gc_new (win);
    gdk_gc_set_line_attributes (gc, 1, GDK_LINE_ON_OFF_DASH, 
                                GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
    gdk_color_parse ("#0F0", &color);
    gdk_gc_set_rgb_fg_color (gc, &color);
    gdk_draw_rectangle (win, gc, FALSE, a.x, a.y,
                        a.width-1, a.height-1); /* minus 1 because ^^ */
    g_object_unref (gc);
  }
#endif

#ifdef DEBUG_APPLET_AREA
  if (1)
  {
    GdkRectangle a;
    GdkColor color;

    awn_panel_get_applet_rect (AWN_PANEL (widget), &a, 0, 0);
    GdkGC *gc = gdk_gc_new (win);
    gdk_gc_set_line_attributes (gc, 1, GDK_LINE_ON_OFF_DASH, 
                                GDK_CAP_NOT_LAST, GDK_JOIN_MITER);
    gdk_color_parse ("#00F", &color);
    gdk_gc_set_rgb_fg_color (gc, &color);
    gdk_draw_rectangle (win, gc, FALSE, a.x, a.y,
                        a.width-1, a.height-1); /* minus 1 because ^^ */
    g_object_unref (gc);
  }
#endif

  propagate:

  gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                  child,
                                  event);

  return TRUE;
}


static void
awn_panel_add (GtkContainer *window, GtkWidget *widget)
{
  AwnPanelPrivate *priv;
  GdkWindow *win;

  g_return_if_fail (AWN_IS_PANEL (window));
  priv = AWN_PANEL_GET_PRIVATE (window);

  /* Add the widget to the internal alignment */
  gtk_container_add (GTK_CONTAINER (priv->viewport), widget);
  
  /* Set up the eventbox for compositing (if necessary) */
  gtk_widget_realize (priv->eventbox);
  win = gtk_widget_get_window (priv->eventbox);
  if (priv->composited)
    gdk_window_set_composited (win, priv->composited);

  gtk_widget_show (widget);
}

static void
on_theme_changed (AwnBackground *bg, AwnPanel *panel)
{
  g_return_if_fail (AWN_IS_BACKGROUND (bg));

  gtk_widget_queue_draw (GTK_WIDGET (panel));
}

/*
 * PROPERTY SETTERS
 */
static void  
awn_panel_set_offset  (AwnPanel *panel, 
                       gint      offset)
{
  AwnPanelPrivate *priv = panel->priv;
  
  priv->offset = offset;

  //awn_panel_refresh_padding (panel, NULL);
  awn_icon_set_offset (AWN_ICON (priv->arrow1), offset);
  awn_icon_set_offset (AWN_ICON (priv->arrow2), offset);

  g_signal_emit (panel, _panel_signals[OFFSET_CHANGED], 0, priv->offset);

  gtk_widget_queue_resize (GTK_WIDGET (panel));
}

static void
awn_panel_set_pos_type (AwnPanel *panel, GtkPositionType position)
{
  AwnPanelPrivate *priv = panel->priv;

  priv->position = position;

  awn_box_set_orientation_from_pos_type (AWN_BOX (priv->box), position);

  awn_panel_refresh_alignment (panel);

  awn_panel_refresh_padding (panel, NULL);

  if (!gtk_widget_get_realized (GTK_WIDGET (panel)))
    return;

  if (priv->docklet && priv->docklet_close_on_pos_change)
  {
    awn_panel_docklet_destroy (panel);
  }
  if (priv->docklet_closer)
  {
    awn_icon_set_pos_type (AWN_ICON (priv->docklet_closer), position);
  }

  awn_icon_set_pos_type (AWN_ICON (priv->arrow1), position);
  awn_icon_set_pos_type (AWN_ICON (priv->arrow2), position);
  awn_throbber_set_size (AWN_THROBBER (priv->arrow1), priv->size);
  awn_throbber_set_size (AWN_THROBBER (priv->arrow2), priv->size);

  awn_panel_reset_autohide (panel);

  g_signal_emit (panel, _panel_signals[POS_CHANGED], 0, priv->position);
  
  position_window (panel);
  
  gtk_widget_queue_resize (GTK_WIDGET (panel));
}

static void
awn_panel_set_size (AwnPanel *panel, gint size)
{
  AwnPanelPrivate *priv = panel->priv;
  
  priv->size = size;

  if (!gtk_widget_get_realized (GTK_WIDGET (panel)))
    return;
  
  g_signal_emit (panel, _panel_signals[SIZE_CHANGED], 0, priv->size);

  position_window (panel);

  if (priv->docklet_closer)
  {
    awn_throbber_set_size (AWN_THROBBER (priv->docklet_closer), size);
    awn_icon_set_offset (AWN_ICON (priv->docklet_closer), size / 2 +
                                                          priv->offset);
  }

  awn_throbber_set_size (AWN_THROBBER (priv->arrow1), size);
  awn_throbber_set_size (AWN_THROBBER (priv->arrow2), size);

  gtk_widget_queue_resize (GTK_WIDGET (panel));
}

/* This will help autohide, but i'd work even without it */
static gboolean
on_mouse_over (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnPanel *panel = AWN_PANEL (widget);
  AwnPanelPrivate *priv = panel->priv;

  if (priv->autohide_start_timer_id)
  {
    g_source_remove (priv->autohide_start_timer_id);
    priv->autohide_start_timer_id = 0;
  }
  if (priv->autohide_started)
  {
    priv->autohide_started = FALSE;
    g_signal_emit (panel, _panel_signals[AUTOHIDE_END], 0);
  }

  if (priv->mouse_poll_timer_id == 0 && poll_mouse_position (panel))
  {
    priv->mouse_poll_timer_id =
      g_timeout_add (priv->autohide_mouse_poll_delay,
                     poll_mouse_position, panel);
  }

  return FALSE;
}

static gboolean
on_mouse_out (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnPanel *panel = AWN_PANEL (widget);
  AwnPanelPrivate *priv = panel->priv;

  if (priv->autohide_start_timer_id == 0  && !priv->autohide_started)
  {
    /* the timeout will emit autohide-start */
    priv->autohide_start_timer_id =
      g_timeout_add (priv->autohide_hide_delay,
                     autohide_start_timeout, panel);
  }

  return FALSE;
}

static void
awn_panel_reset_autohide (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->autohide_started)
  {
    priv->autohide_started = FALSE;
    g_signal_emit (panel, _panel_signals[AUTOHIDE_END], 0);
  }
}

static void
awn_panel_set_autohide_type (AwnPanel *panel, gint type)
{
  AwnPanelPrivate *priv = panel->priv;

  priv->autohide_type = type;

  awn_panel_reset_autohide (panel);

  if (priv->autohide_type != AUTOHIDE_TYPE_NONE
      && priv->mouse_poll_timer_id == 0)
  {
    priv->mouse_poll_timer_id = 
      g_timeout_add (priv->autohide_mouse_poll_delay,
                     poll_mouse_position, panel);
  }

  if (priv->autohide_start_handler_id)
  {
    g_signal_handler_disconnect (panel, priv->autohide_start_handler_id);
    priv->autohide_start_handler_id = 0;
  }
  if (priv->autohide_end_handler_id)
  {
    g_signal_handler_disconnect (panel, priv->autohide_end_handler_id);
    priv->autohide_end_handler_id = 0;
  }

  /* for all autohide types, just connect to autohide-start & end signals
   * and do what you want to do in the callbacks
   * The autohide-start should return FALSE if un-hiding requires to touch
   * the display edge, and TRUE if it should react on whole AWN window
   */
  switch (priv->autohide_type)
  {
    case AUTOHIDE_TYPE_KEEP_BELOW:
      priv->autohide_start_handler_id = g_signal_connect (
        panel, "autohide-start", G_CALLBACK (keep_below_start), NULL);
      priv->autohide_end_handler_id = g_signal_connect (
        panel, "autohide-end", G_CALLBACK (keep_below_end), NULL);
      break;
    case AUTOHIDE_TYPE_FADE_OUT:
      priv->autohide_start_handler_id = g_signal_connect (
        panel, "autohide-start", G_CALLBACK (alpha_blend_start), NULL);
      priv->autohide_end_handler_id = g_signal_connect (
        panel, "autohide-end", G_CALLBACK (alpha_blend_end), NULL);
      break;
    case AUTOHIDE_TYPE_TRANSPARENTIZE:
      priv->autohide_start_handler_id = g_signal_connect (
        panel, "autohide-start", G_CALLBACK (transparentize_start), NULL);
      priv->autohide_end_handler_id = g_signal_connect (
        panel, "autohide-end", G_CALLBACK (transparentize_end), NULL);
      break;
    default:
      break;
  }
}

static void
on_applet_embedded (AwnPanel *panel, GtkWidget *applet)
{
  g_return_if_fail (AWN_IS_PANEL (panel) && GTK_IS_SOCKET (applet));
}

static void
on_applet_removed (AwnPanel *panel, GtkWidget *applet)
{
  g_return_if_fail (AWN_IS_PANEL (panel) && GTK_IS_SOCKET (applet));

  // let the applet know it's about to be removed
  char *uid = NULL;
  g_object_get (applet, "uid", &uid, NULL);
  g_return_if_fail (uid);

  g_signal_emit (panel, _panel_signals[DESTROY_APPLET], 0, uid);
  g_free (uid);
}

static void
awn_panel_set_style (AwnPanel *panel, gint style)
{
  AwnPanelPrivate *priv = panel->priv;
  AwnBackground *old_bg = NULL;
  AwnPathType path = AWN_PATH_LINEAR;
  gfloat offset_mod = 1.0;

  /* if the style hasn't changed and the background exist everything is done. */
  if(priv->style == style && priv->bg) return;

  old_bg = priv->bg;
  priv->style = style;

  switch (priv->style)
  {
    case STYLE_NONE:
      priv->bg = NULL;
      break;
    case STYLE_FLAT:
      priv->bg = awn_background_flat_new (priv->client, panel);
      break;
    case STYLE_3D:
      priv->bg = awn_background_3d_new (priv->client, panel);
      break;
    case STYLE_CURVES:
      priv->bg = awn_background_curves_new (priv->client, panel);
      break;
    case STYLE_EDGY:
      priv->bg = awn_background_edgy_new (priv->client, panel);
      break;
    case STYLE_FLOATY:
      priv->bg = awn_background_floaty_new (priv->client, panel);
      break;
    default:
      g_assert_not_reached ();
  }

  if (old_bg) g_object_unref (old_bg);

  if (priv->bg)
  {
    g_signal_connect (priv->bg, "changed", G_CALLBACK (on_theme_changed),
                      panel);
    g_signal_connect_swapped (priv->bg, "padding-changed",
                              G_CALLBACK (awn_panel_refresh_padding), panel);
    path = awn_background_get_path_type (priv->bg, &offset_mod);
  }

  awn_panel_refresh_padding (panel, NULL);

  /* Emit offset-modifier & path-type properties via DBus */
  if (offset_mod != 1.0)
  {
    GValue mod_value = {0};
    g_value_init (&mod_value, G_TYPE_FLOAT);
    /* FIXME: we also need to calculate our dimensions based on offset_mod */
    g_value_set_float (&mod_value, offset_mod);

    priv->offset_mod = offset_mod;
    g_object_notify (G_OBJECT (panel), "offset-modifier");

    g_signal_emit (panel, _panel_signals[PROPERTY_CHANGED], 0,
                   "offset-modifier", &mod_value);
  }

  GValue value = {0};
  g_value_init (&value, G_TYPE_INT);

  g_value_set_int (&value, path);

  priv->path_type = path;
  g_object_notify (G_OBJECT (panel), "path-type");

  g_signal_emit (panel, _panel_signals[PROPERTY_CHANGED], 0,
                 "path-type", &value);
}

static void
awn_panel_set_panel_mode (AwnPanel *panel, gboolean  panel_mode)
{
  AwnPanelPrivate *priv = panel->priv;
  priv->panel_mode = panel_mode;

  if (priv->panel_mode)
  {
    /* Check if panel is already realized. So it has an GdkWindow.
     * If it's not, the strut will get set when the position and dimension get set. 
     */
    if (gtk_widget_get_realized (GTK_WIDGET (panel)))
      awn_panel_queue_strut_update (panel);
  }
  else
  {
    if (gtk_widget_get_realized (GTK_WIDGET (panel)))
      awn_panel_remove_strut (panel);
  }

}

static void
awn_panel_set_expand_mode (AwnPanel *panel, gboolean expand)
{
  AwnPanelPrivate *priv = panel->priv;

  priv->expand = expand;
  awn_panel_refresh_alignment (panel);
  gtk_widget_queue_resize (GTK_WIDGET (panel));

  if (priv->manager)
  {
    // we may be called in constructed, before AppletManager is created
    awn_panel_update_masks (GTK_WIDGET (panel), 0, 0);
  }
}

static void     
awn_panel_set_clickthrough_type(AwnPanel *panel, gint type)
{
  AwnPanelPrivate *priv = panel->priv;
  priv->clickthrough_type = type;

  if (priv->clickthrough_type != CLICKTHROUGH_NEVER
      && priv->mouse_poll_timer_id == 0 )
  {
    priv->mouse_poll_timer_id =
      g_timeout_add (priv->autohide_mouse_poll_delay,
                     poll_mouse_position, panel);
  }

  if (priv->clickthrough_type == CLICKTHROUGH_NEVER && priv->clickthrough)
  {
    GdkWindow *win;
    win = gtk_widget_get_window (GTK_WIDGET(panel));
    priv->clickthrough = FALSE;
    awn_panel_update_masks (GTK_WIDGET(panel),
                            priv->old_width, priv->old_height);
    gdk_window_set_opacity (win, 1.0);
  }
}

static void
on_manager_size_alloc (GtkWidget *manager, GtkAllocation *alloc,
                       AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->panel_mode && priv->animated_resize && !priv->expand)
  {
    awn_panel_queue_strut_update (panel);
  }
  awn_panel_queue_masks_update (panel);
}

static gboolean
check_monitor_intersects_area (GdkScreen * screen, gint x,gint y,gint width,gint height)
{
  gint num_monitors =  gdk_screen_get_n_monitors (screen);
  int i;
  GdkRectangle intersection;
  GdkRectangle search_area;
  search_area.x = x;
  search_area.y = y;
  search_area.width = width;
  search_area.height = height;
  
  for (i = 0; i<num_monitors; i++)
  {
    GdkRectangle monitor_geom;
    gdk_screen_get_monitor_geometry (screen,i,&monitor_geom);
    if ( gdk_rectangle_intersect (&search_area,&monitor_geom,&intersection) )
    {
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
strut_update_scheduler (AwnPanel *panel)
{
  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);

  AwnPanelPrivate *priv = panel->priv;

  priv->strut_update_id = 0;
  if (priv->panel_mode)
  {
    awn_panel_set_strut (panel);
  }

  return FALSE;
}

static void
awn_panel_queue_strut_update (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->strut_update_id == 0)
  {
    priv->strut_update_id = g_idle_add ((GSourceFunc)strut_update_scheduler,
                                        panel);
  }
}

static void
awn_panel_set_strut (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  gint strut, strut_start, strut_end;
  GdkWindow *win;
  GdkRectangle area;
  GdkScreen * screen = gtk_widget_get_screen (GTK_WIDGET(panel));
  gint root_x, root_y;
  gint screen_width, screen_height;
  GtkAllocation panel_alloc;
  gint monitor_number;
  GdkRectangle monitor_geom;
  gint adjust = 0;  /*adjustment used for monitors of different sizes*/
  gboolean on_shared_edge = FALSE;
  
  monitor_number = gdk_screen_get_monitor_at_window (screen,GTK_WIDGET(panel)->window);
  gdk_screen_get_monitor_geometry (screen,monitor_number,&monitor_geom);
  gtk_widget_get_allocation (GTK_WIDGET(panel), &panel_alloc);
  screen_width = gdk_screen_get_width (screen);
  screen_height = gdk_screen_get_height (screen);
  gtk_window_get_position (GTK_WINDOW (panel), &root_x, &root_y);
  if (priv->expand)
  {
    area.x = root_x;
    area.y = root_y;
    gtk_window_get_size (GTK_WINDOW (panel), &area.width, &area.height);
  }
  else
  {
    GtkAllocation box_alloc;

    gtk_widget_get_allocation (priv->box, &box_alloc);
    area.x = box_alloc.x + root_x;
    area.y = box_alloc.y + root_y;
    area.width = box_alloc.width;
    area.height = box_alloc.height;
  }

  strut = priv->offset + priv->size + priv->extra_padding;
  switch (priv->position)
  {
    case GTK_POS_TOP:
      if (root_y > 0)
      {
        if (check_monitor_intersects_area (screen,
                                           monitor_geom.x,
                                           0,
                                           monitor_geom.width,
                                           monitor_geom.y-1))
        {
          on_shared_edge = TRUE;
        }
        else
        {
          on_shared_edge = FALSE;
          adjust = root_y;
        }
        strut = strut +  adjust;
      }
      else
      {
        on_shared_edge = FALSE;
      }
      strut_start = area.x;
      strut_end = area.x + area.width - 1;
      break;
    case GTK_POS_RIGHT:
      /*possible monitors of different size
       This could alsbe a situation of being on the smaller of multiple monitors*/
      if (monitor_geom.x + monitor_geom.width < screen_width)
      {
        /*might actually be on a shared edge in which case... reset adjust
         see if there is any monitor existing anywhere to the right*/
        if (check_monitor_intersects_area (screen,
                                           monitor_geom.width + monitor_geom.x +1,
                                           monitor_geom.y,
                                           screen_width - (monitor_geom.width + monitor_geom.x),
                                           monitor_geom.height))
        {
          adjust =0;
        }
        else
        {
          adjust = screen_width - (monitor_geom.width + monitor_geom.x);
        }
        strut = strut +  adjust;
      }      
      on_shared_edge = (root_x + panel_alloc.width + adjust < screen_width);      
      /*different sized monitors... */
      strut_start = area.y;
      strut_end = area.y + area.height - 1;
      break;
    case GTK_POS_BOTTOM:
      if (monitor_geom.height + monitor_geom.y < screen_height)
      {
        if (check_monitor_intersects_area (screen,
                                           monitor_geom.x,
                                           monitor_geom.y+monitor_geom.height+1,
                                           monitor_geom.width,
                                           screen_height - ( monitor_geom.height + monitor_geom.y)))
        {
          adjust =0;
        }
        else
        {
          adjust = screen_height - (monitor_geom.height + monitor_geom.y);
        }
        strut = strut +  adjust;
      }
      on_shared_edge = (root_y + panel_alloc.height + adjust< screen_height);
      /*different sized monitors... */
      strut_start = area.x;
      strut_end = area.x + area.width - 1;
      break;
    case GTK_POS_LEFT:
      if (root_x > 0)
      {
        if (check_monitor_intersects_area (screen,
                                           monitor_geom.x,
                                           0,
                                           monitor_geom.width,
                                           monitor_geom.y-1))

        {
          on_shared_edge = TRUE;
        }
        else
        {
          on_shared_edge = FALSE;
          adjust = root_x;
        }
        strut = strut +  adjust;
      }
      else
      {
        on_shared_edge = FALSE;
      }
      on_shared_edge = (root_x >0);
      strut_start = area.y;
      strut_end = area.y + area.height - 1;
      break;
    default:
      g_assert_not_reached ();
  }

  if (on_shared_edge)
  {
    awn_panel_remove_strut (panel);
    return;
  }
 
  /* allow AwnBackground to change the strut */
  if (priv->bg != NULL)
    awn_background_get_strut_offsets (priv->bg, priv->position, &area,
                                      &strut, &strut_start, &strut_end);

  win = gtk_widget_get_window (GTK_WIDGET (panel));

  xutils_set_strut (win, priv->position, strut, strut_start, strut_end);
}

static void
awn_panel_remove_strut (AwnPanel *panel)
{
  GdkWindow *win;
  win = gtk_widget_get_window (GTK_WIDGET (panel));
  xutils_set_strut (win, 0, 0, 0, 0);
}

/*
 * DBUS METHODS
 */

gboolean
awn_panel_add_applet (AwnPanel *panel, gchar *desktop_file, GError **error)
{
  AwnPanelPrivate *priv;

  g_return_val_if_fail (AWN_IS_PANEL (panel), TRUE);
  priv = panel->priv;

  AwnAppletManager *manager = AWN_APPLET_MANAGER (priv->manager);
  gchar *uid = awn_applet_manager_generate_uid (manager);

  g_debug ("Adding applet \"%s\" with UID: %s", desktop_file, uid);

  GValueArray *applets = g_value_array_new (0);
  g_object_get (manager, "applet-list", &applets, NULL);

  GValue applet = { 0, };
  g_value_init (&applet, G_TYPE_STRING);
  g_value_take_string (&applet, g_strdup_printf ("%s::%s", desktop_file, uid));

  g_value_array_append (applets, &applet);

  desktop_agnostic_config_client_set_list (priv->client,
      AWN_GROUP_PANEL, AWN_PANEL_APPLET_LIST, applets, NULL);

  g_value_unset (&applet);
  g_value_array_free (applets);
  g_free (uid);

  return TRUE;
}

gboolean
awn_panel_delete_applet (AwnPanel  *panel,
                         gchar     *uid,
                         GError   **error)
{
  AwnPanelPrivate *priv;
  	
  g_return_val_if_fail (AWN_IS_PANEL (panel), TRUE);
  priv = panel->priv;

  return TRUE;
}

gboolean 
awn_panel_set_applet_flags (AwnPanel         *panel,
                            const gchar      *uid,
                            gint              flags,
                            GError          **error)
{
  AwnPanelPrivate *priv;

  g_return_val_if_fail (AWN_IS_PANEL (panel), TRUE);
  priv = panel->priv;

  g_print ("Applet [%s] flags: %d: ", uid, flags);
  
  if (flags & AWN_APPLET_FLAGS_NONE)
    g_print ("None ");
  if (flags & AWN_APPLET_EXPAND_MINOR)
    g_print ("Minor ");
  if (flags & AWN_APPLET_EXPAND_MAJOR)
    g_print ("Major ");
  if (flags & AWN_APPLET_IS_EXPANDER)
    g_print ("Expander ");
  if (flags & AWN_APPLET_IS_SEPARATOR)
    g_print ("Separator ");
  if (flags & AWN_APPLET_HAS_SHAPE_MASK)
    g_print ("ShapeMask ");
  if (flags & AWN_APPLET_DOCKLET_HANDLES_POSITION_CHANGE)
    g_print ("DockletHandlesPositionChange ");
  if (flags & AWN_APPLET_DOCKLET_CLOSE_ON_MOUSE_OUT)
    g_print ("DockletCloseOnMouseOut ");

  g_print ("\n");

  // we'll handle docklet flags ourselves, 
  //   other flags are handled by AppletManager
  if ((flags & (AWN_APPLET_DOCKLET_HANDLES_POSITION_CHANGE |
               AWN_APPLET_DOCKLET_CLOSE_ON_MOUSE_OUT)) != 0)
  {
    if (flags & AWN_APPLET_DOCKLET_HANDLES_POSITION_CHANGE)
    {
      priv->docklet_close_on_pos_change = FALSE;
    }
    priv->docklet_close_on_mouse_out = 
      (flags & AWN_APPLET_DOCKLET_CLOSE_ON_MOUSE_OUT) != 0;
    // FIXME: start mouse polling if not doing so...
  }
  else
  {
    awn_applet_manager_set_applet_flags (AWN_APPLET_MANAGER (priv->manager),
                                         uid, flags);
  }

  return TRUE;
}

static guint
awn_panel_disable_autohide (AwnPanel *panel,
                            const gchar *app_name,
                            const gchar *reason)
{
  AwnPanelPrivate *priv = panel->priv;

  static guint cookie = 0; // FIXME: use something different!
  if (cookie == 0) cookie++;

  AwnInhibitItem *item = g_new0 (AwnInhibitItem, 1);
  item->panel = panel;
  item->description = g_strdup_printf ("(%s): %s", app_name, reason);
  item->cookie = cookie;

  g_hash_table_insert (priv->inhibits,
                       GINT_TO_POINTER (cookie),
                       item);

  if (!priv->autohide_inhibited)
  {
    priv->autohide_inhibited = TRUE;
    awn_panel_reset_autohide (panel);
  }

  return cookie++;
}

static void
dbus_inhibitor_lost (AwnDBusWatcher *watcher, gchar *name,
                     AwnInhibitItem *item)
{
  g_debug ("AwnPanel: DBus object %s didn't call UninhibitAutohide!", name);
  awn_panel_uninhibit_autohide (item->panel, item->cookie);
}

void
awn_panel_inhibit_autohide (AwnPanel *panel,
                            const gchar *app_name,
                            const gchar *reason,
                            DBusGMethodInvocation *context)
{
  AwnPanelPrivate *priv = panel->priv;

  guint cookie = awn_panel_disable_autohide (panel, app_name, reason);

  // watch the sender on dbus and remove all its inhibits when it
  //   disappears (to be sure that we don't misbehave due to crashing app)
  gchar *sender = dbus_g_method_get_sender (context);
  gchar *detailed_signal = g_strdup_printf ("name-disappeared::%s", sender);
  g_signal_connect (awn_dbus_watcher_get_default (), detailed_signal,
                    G_CALLBACK (dbus_inhibitor_lost),
                    g_hash_table_lookup (priv->inhibits, 
                                         GINT_TO_POINTER (cookie)));

  g_free (detailed_signal);
  g_free (sender);

  dbus_g_method_return (context, cookie);
}

gboolean
awn_panel_uninhibit_autohide  (AwnPanel *panel, guint cookie)
{
  AwnPanelPrivate *priv = panel->priv;

  AwnInhibitItem *item = g_hash_table_lookup (priv->inhibits,
                                              GINT_TO_POINTER (cookie));

  if (!item) return TRUE; // we could set an error

  // remove the dbus watcher
  g_signal_handlers_disconnect_by_func (awn_dbus_watcher_get_default (),
                                        G_CALLBACK (dbus_inhibitor_lost),
                                        item);

  g_hash_table_remove (priv->inhibits, GINT_TO_POINTER (cookie));

  if (g_hash_table_size (priv->inhibits) == 0)
  {
    priv->autohide_inhibited = FALSE;

    // don't wait for the timer, start autohide immediately (in next loop)
    if (priv->autohide_start_timer_id)
      g_source_remove (priv->autohide_start_timer_id);

    priv->autohide_start_timer_id = g_idle_add (autohide_start_timeout, panel);
  }

  return TRUE;
}

gboolean
awn_panel_get_inhibitors (AwnPanel *panel, GStrv *reasons)
{
  AwnPanelPrivate *priv = panel->priv;
  GList *list, *l;

  *reasons = g_new0 (char*, g_hash_table_size (priv->inhibits) + 1);

  list = l = g_hash_table_get_values (priv->inhibits); // list should be freed
  int i=0;

  while (list)
  {
    AwnInhibitItem *item = list->data;
    (*reasons)[i++] = g_strdup (item->description);

    list = list->next;
  }

  g_list_free (l);

  return TRUE;
}

static void
docklet_size_request (GtkWidget *widget, GtkRequisition *req, gpointer data)
{
  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;

  switch (priv->position)
  {
    case GTK_POS_LEFT:
    case GTK_POS_RIGHT:
      req->height = MAX (req->height, priv->docklet_minsize);
      break;
    default:
      req->width = MAX (req->width, priv->docklet_minsize);
  }
}

static void
multiply_pixmap_alpha (GdkPixmap *pixmap, GdkRectangle *rect, gfloat mult)
{
  cairo_t *cr;

  cr = gdk_cairo_create (pixmap);
  gdk_cairo_rectangle (cr, rect);
  cairo_clip (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OUT);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1-mult); // alpha multiplier
  gdk_cairo_rectangle (cr, rect);
  cairo_fill (cr);
  cairo_destroy (cr);
}

static gboolean
docklet_appear_cb (AwnPanel *panel)
{
  gint x, y, width, height;
  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);

  AwnPanelPrivate *priv = panel->priv;

  gfloat old_alpha = 1 - priv->docklet_alpha;
  priv->docklet_alpha += 0.15;

  x = MIN (priv->snapshot_paint_size.x, priv->box->allocation.x);
  y = MIN (priv->snapshot_paint_size.y, priv->box->allocation.y);
  width = MAX (priv->snapshot_paint_size.width,
               priv->box->allocation.width);
  height = MAX (priv->snapshot_paint_size.height, 
                priv->box->allocation.height);

  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (panel));
  GdkRectangle rect = {.x = x, .y = y, .width = width, .height = height};
  gdk_window_invalidate_rect (window, &rect, FALSE);

  if (priv->docklet_alpha >= 1.0)
  {
    priv->docklet_appear_timer_id = 0;
    g_object_unref (priv->dock_snapshot);
    g_object_unref (priv->tmp_pixmap);
    priv->dock_snapshot = NULL;
    priv->tmp_pixmap = NULL;
    return FALSE;
  }

  // let's change alpha of the pixmap here, no need to wait for expose
  multiply_pixmap_alpha (priv->dock_snapshot,
                         (GdkRectangle*)&priv->snapshot_paint_size,
                         (1-priv->docklet_alpha) / old_alpha);

  return TRUE;
}

static GdkPixmap*
get_window_snapshot (GdkDrawable *drawable, gint width, gint height)
{
  GdkPixmap *pixmap = gdk_pixmap_new (drawable, width, height, -1);
  cairo_t *cr = gdk_cairo_create (pixmap);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  gdk_cairo_set_source_pixmap (cr, drawable, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_paint (cr);
  cairo_destroy (cr);

  return pixmap;
}

static void
awn_panel_prepare_crossfade (AwnPanel *panel, gint time_step)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->composited)
  {
    // copy a snapshot of the dock into a pixmap
    GdkDrawable *drawable = gtk_widget_get_window (priv->eventbox);
    gint width, height;

    if (priv->dock_snapshot)
    {
      g_object_unref (priv->dock_snapshot);
      g_object_unref (priv->tmp_pixmap);
    }

    width = priv->eventbox->allocation.width;
    height = priv->eventbox->allocation.height;

    priv->dock_snapshot = get_window_snapshot (drawable, width, height);
    priv->snapshot_paint_size = priv->box->allocation;
    priv->docklet_alpha = 0.2;
    multiply_pixmap_alpha (priv->dock_snapshot,
                           (GdkRectangle*)&priv->snapshot_paint_size,
                           1 - priv->docklet_alpha);

    priv->tmp_pixmap = gdk_pixmap_new (drawable, width, height, -1);
    
    gtk_widget_queue_draw_area (GTK_WIDGET (panel),
                                priv->snapshot_paint_size.x,
                                priv->snapshot_paint_size.y,
                                priv->snapshot_paint_size.width,
                                priv->snapshot_paint_size.height);

    if (priv->docklet_appear_timer_id != 0)
    {
      g_source_remove (priv->docklet_appear_timer_id);
    }
    priv->docklet_appear_timer_id =
        g_timeout_add_full (GDK_PRIORITY_REDRAW + 10, time_step,
                            (GSourceFunc)docklet_appear_cb, panel, NULL);
  }
}

static void
docklet_plug_added (GtkSocket *socket, AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  awn_panel_prepare_crossfade (panel, 40);

  awn_applet_manager_hide_applets (AWN_APPLET_MANAGER (priv->manager));
  gtk_widget_show (priv->docklet);
  gtk_widget_show (priv->docklet_closer);
}

static gboolean
docklet_plug_removed (GtkSocket *socket, AwnPanel *panel)
{
  awn_panel_docklet_destroy (panel);

  return FALSE;
}

static gboolean
docklet_closer_click (GtkWidget *widget, AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->docklet == NULL) return FALSE;

  awn_panel_docklet_destroy (panel);

  return FALSE;
}

static void
awn_panel_docklet_destroy (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  if (priv->docklet == NULL) return;

  // FIXME: we could optimize and not destroy the widget
  awn_panel_prepare_crossfade (panel, 50);
  
  awn_applet_manager_remove_widget (AWN_APPLET_MANAGER (priv->manager),
                                    priv->docklet);
  awn_applet_manager_show_applets (AWN_APPLET_MANAGER (priv->manager));
  priv->docklet = NULL;
  priv->docklet_minsize = 0;

  gtk_widget_hide (priv->docklet_closer);
}

void
awn_panel_docklet_request (AwnPanel *panel,
                           gint min_size,
                           gboolean shrink,
                           gboolean expand,
                           DBusGMethodInvocation *context)
{
  AwnPanelPrivate *priv = panel->priv;
  GtkAllocation alloc;
  gint64 window_id = 0;

  if (!priv->docklet_closer)
  {
    priv->docklet_closer = awn_throbber_new_with_config (priv->client);
    awn_throbber_set_type (AWN_THROBBER (priv->docklet_closer),
                           AWN_THROBBER_TYPE_CLOSE_BUTTON);
    awn_icon_set_hover_effects (AWN_ICON (priv->docklet_closer), TRUE);
    awn_icon_set_tooltip_text (AWN_ICON (priv->docklet_closer),
                               _("Close docklet"));

    awn_applet_manager_add_widget (AWN_APPLET_MANAGER (priv->manager),
                                   priv->docklet_closer, 1);

    g_signal_connect (priv->docklet_closer, "clicked",
                      G_CALLBACK (docklet_closer_click), panel);
  }

  if (!priv->docklet)
  {
    AwnThrobber *closer = AWN_THROBBER (priv->docklet_closer);

    awn_throbber_set_size (closer, priv->size);
    awn_icon_set_pos_type (AWN_ICON (closer), priv->position);
    awn_icon_set_offset (AWN_ICON (closer), priv->size / 2 + priv->offset);

    GtkRequisition closer_req;
    gtk_widget_size_request (priv->docklet_closer, &closer_req);

    gtk_widget_get_allocation (priv->box, &alloc);

    priv->docklet = gtk_socket_new ();
    priv->docklet_close_on_pos_change = TRUE;
    priv->docklet_close_on_mouse_out = FALSE;
    // if expand param is false the docklet will be restricted to this size
    switch (priv->position)
    {
      case GTK_POS_LEFT:
      case GTK_POS_RIGHT:
        priv->docklet_minsize = shrink ? min_size :
          MAX (min_size, alloc.height - closer_req.height);

        if (expand == FALSE)
        {
           gtk_widget_set_size_request (priv->docklet,
                                        -1, priv->docklet_minsize);
        }
        break;
      default:
        priv->docklet_minsize = shrink ? min_size :
          MAX (min_size, alloc.width - closer_req.width);

        if (expand == FALSE)
        {
           gtk_widget_set_size_request (priv->docklet,
                                        priv->docklet_minsize, -1);
        }
        break;
    }

    awn_utils_ensure_transparent_bg (priv->docklet);

    g_signal_connect_after (priv->docklet, "size-request",
                            G_CALLBACK (docklet_size_request), panel);
    g_signal_connect (priv->docklet, "plug-added",
                      G_CALLBACK (docklet_plug_added), panel);
    g_signal_connect (priv->docklet, "plug-removed",
                      G_CALLBACK (docklet_plug_removed), panel);

    awn_applet_manager_add_docklet (AWN_APPLET_MANAGER (priv->manager),
                                    priv->docklet);
    gtk_widget_realize (priv->docklet);
    gtk_widget_hide (priv->docklet);
  }
  else
  {
    // FIXME: set error
  }

  window_id = gtk_socket_get_id (GTK_SOCKET (priv->docklet));

  dbus_g_method_return (context, window_id);
}

static void
value_array_append_int (GValueArray *array, gint i)
{
  GValue *value = g_new0 (GValue, 1);

  g_value_init (value, G_TYPE_INT);
  g_value_set_int (value, i);

  g_value_array_append (array, value);
}

static void
value_array_append_bool (GValueArray *array, gboolean b)
{
  GValue *value = g_new0 (GValue, 1);

  g_value_init (value, G_TYPE_BOOLEAN);
  g_value_set_boolean (value, b);

  g_value_array_append (array, value);
}

static void
value_array_append_array (GValueArray *array, guchar *data, gsize data_len)
{
  GArray *byte_array;
  GValue *value = g_new0 (GValue, 1);

  byte_array = g_array_sized_new (FALSE, FALSE, sizeof(guchar), data_len);
  g_array_append_vals (byte_array, data, data_len);

  g_value_init (value, dbus_g_type_get_collection ("GArray", G_TYPE_CHAR));
  g_value_take_boxed (value, byte_array);

  g_value_array_append (array, value);
}

gboolean
awn_panel_get_snapshot (AwnPanel *panel, GValue *value, GError **error)
{
  GdkRectangle rect;
  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);

  awn_panel_get_draw_rect (panel, &rect, 0, 0);

  // get snapshot from root window, cause we'll loose alpha anyway
  GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (panel));

  // FIXME: incorrect width/height for curved dock
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         rect.width,
                                                         rect.height);
  cairo_t *cr = cairo_create (surface);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  gdk_cairo_set_source_pixmap (cr, window, -rect.x, -rect.y);
  cairo_paint (cr);

  cairo_destroy (cr);
  cairo_surface_flush (surface);  

  // stuff the pixbuf to our out param
  gint width = rect.width;
  gint height = rect.height;
  gint rowstride = cairo_image_surface_get_stride (surface);
  gint n_channels = 4;
  gint bits_per_sample = 8;
  //gint image_len = (height - 1) * rowstride + width *
  //  ((n_channels * bits_per_sample + 7) / 8);

  guchar *image = cairo_image_surface_get_data (surface);

  GValueArray *image_struct = g_value_array_new (1);

  value_array_append_int (image_struct, width);
  value_array_append_int (image_struct, height);
  value_array_append_int (image_struct, rowstride);
  value_array_append_bool (image_struct, TRUE);
  value_array_append_int (image_struct, bits_per_sample);
  value_array_append_int (image_struct, n_channels);
  value_array_append_array (image_struct, image, height * rowstride);

  g_value_init (value, G_TYPE_VALUE_ARRAY);
  g_value_take_boxed (value, image_struct);

  cairo_surface_destroy (surface);

  return TRUE;
}

gboolean
awn_panel_get_all_server_flags (AwnPanel *panel,
                                GHashTable **hash,
                                gchar     *name,
                                GError   **error)
{
  AwnPanelPrivate *priv;

  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);
  priv = panel->priv;

  return awn_ua_get_all_server_flags (AWN_APPLET_MANAGER (priv->manager),
                                      hash, name, error);
}

gboolean
awn_panel_ua_add_applet (AwnPanel *panel, gchar *name, glong xid,
                         gint width, gint height,
                         gchar *size_type,
                         GError **error)
{
  AwnPanelPrivate *priv;

  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);
  priv = panel->priv;

  return awn_ua_add_applet (AWN_APPLET_MANAGER (priv->manager), name, xid,
                            width, height, size_type, error);
}

