/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
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
 *
 */

#include "config.h"

#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libawn/libawn.h>
#include <libawn/awn-utils.h>

#include "awn-panel.h"

#include "awn-applet-manager.h"
#include "awn-background.h"
#include "awn-background-flat.h"
#include "awn-background-3d.h"
#include "awn-background-curves.h"
#include "awn-background-edgy.h"
#include "awn-dbus-watcher.h"
#include "awn-defines.h"
#include "awn-marshal.h"
#include "awn-monitor.h"
#include "awn-x.h"

#include "gseal-transition.h"
#include "xutils.h"

G_DEFINE_TYPE (AwnPanel, awn_panel, GTK_TYPE_WINDOW)

#define AWN_PANEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_PANEL, AwnPanelPrivate))

struct _AwnPanelPrivate
{
  AwnConfigClient *client;
  AwnMonitor *monitor;

  GHashTable *inhibits;

  AwnBackground *bg;

  GtkWidget *alignment;
  GtkWidget *eventbox;
  GtkWidget *manager;
  GtkWidget *docklet;
  gint docklet_minsize;

  gboolean composited;
  gboolean panel_mode;
  gboolean expand;
  gboolean animated_resize;

  gint size;
  gint offset;
  gfloat offset_mod;
  gint orient;
  gint path_type;
  gint style;

  gint autohide_type;

  /* for masks/strut updating */
  gint old_width;
  gint old_height;
  gint old_orient;

  /* for strut updating */
  gint old_x;
  gint old_y;

  /* animated resizing */
  gint draw_width;
  gint draw_height;
  guint resize_timer_id;

  guint extra_padding;

  gint hide_counter;
  guint hiding_timer_id;

  guint mouse_poll_timer_id;

  guint autohide_start_timer_id;
  gboolean autohide_started;
  gboolean autohide_always_visible;
  gboolean autohide_inhibited;

  gboolean clickthrough;
  gint clickthrough_type;
  gint last_clickthrough_type;
};

typedef struct _AwnInhibitItem
{
  AwnPanel *panel;

  gchar *description;
  guint cookie;
} AwnInhibitItem;

/* FIXME: this timeout should be configurable I guess */
#define AUTOHIDE_DELAY 1000
#define MOUSE_POLL_TIMER_DELAY 500

#define CLICKTHROUGH_OPACITY 0.3

// padding for active_rect, yea it really isn't nice but so far it seems to
// be the only feasible solution
#define ACTIVE_RECT_PADDING 3

#define ROUND(x) (x < 0 ? x - 0.5 : x + 0.5)

enum 
{
  PROP_0,

  PROP_CLIENT,
  PROP_MONITOR,
  PROP_APPLET_MANAGER,
  PROP_COMPOSITED,
  PROP_PANEL_MODE,
  PROP_EXPAND,
  PROP_OFFSET,
  PROP_ORIENT,
  PROP_SIZE,
  PROP_MAX_SIZE,
  PROP_AUTOHIDE_TYPE,
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

  STYLE_LAST
};

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
  ORIENT_CHANGED,
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
static gboolean poll_mouse_position         (gpointer data);
static gboolean awn_panel_expose            (GtkWidget      *widget, 
                                             GdkEventExpose *event);
static void     awn_panel_size_request      (GtkWidget *widget,
                                             GtkRequisition *requisition);
static gboolean awn_panel_resize_timeout    (gpointer data);

static void     awn_panel_add               (GtkContainer   *window, 
                                             GtkWidget      *widget);

static void     awn_panel_set_offset        (AwnPanel *panel,
                                             gint      offset);
static void     awn_panel_set_orient        (AwnPanel *panel,
                                             gint      orient);
static void     awn_panel_set_size          (AwnPanel *panel,
                                             gint      size);
static void     awn_panel_set_autohide_type (AwnPanel *panel,
                                             gint      type);
static void     awn_panel_set_style         (AwnPanel *panel,
                                             gint      style);
static void     awn_panel_set_panel_mode    (AwnPanel *panel,
                                             gboolean  panel_mode);
static void     awn_panel_set_clickthrough_type(AwnPanel *panel,
                                             gint      type);

static void     awn_panel_reset_autohide    (AwnPanel *panel);

static void     on_geometry_changed         (AwnMonitor    *monitor,
                                             AwnPanel      *panel);
static void     on_theme_changed            (AwnBackground *bg,
                                             AwnPanel      *panel);

static gboolean awn_panel_check_mouse_pos   (AwnPanel *panel,
                                             gboolean whole_window);

static void     awn_panel_get_draw_rect     (AwnPanel *panel,
                                             GdkRectangle *area,
                                             gint width, gint height);

static void     awn_panel_get_applet_rect   (AwnPanel *panel,
                                             GdkRectangle *area,
                                             gint width, gint height);

static void     awn_panel_refresh_alignment (AwnPanel *panel);

static void     awn_panel_refresh_padding   (AwnPanel *panel,
                                             gpointer user_data);

static void     awn_panel_update_masks      (GtkWidget *panel, 
                                             gint real_width, 
                                             gint real_height);

static void awn_panel_set_strut              (AwnPanel *panel);
static void awn_panel_remove_strut           (AwnPanel *panel);

/*
 * GOBJECT CODE 
 */
static void
awn_panel_constructed (GObject *object)
{
  AwnPanelPrivate *priv;
  AwnConfigBridge *bridge;
  GtkWidget       *panel;
  GdkScreen       *screen;

  priv = AWN_PANEL_GET_PRIVATE (object);
  panel = GTK_WIDGET (object);

  priv->monitor = awn_monitor_new_from_config (priv->client);
  g_signal_connect (priv->monitor, "geometry_changed",
                    G_CALLBACK (on_geometry_changed), panel);

  g_signal_connect_swapped (priv->monitor, "notify::monitor-align",
                            G_CALLBACK (awn_panel_refresh_alignment), panel);

  /* Composited checks/setup */
  screen = gtk_widget_get_screen (panel);
  priv->composited = gdk_screen_is_composited (screen);
  priv->animated_resize = priv->composited;
  g_print ("Screen %s composited\n", priv->composited ? "is" : "isn't");
  load_correct_colormap (panel);

  g_signal_connect (panel, "composited-changed",
                    G_CALLBACK (on_composited_changed), NULL);

  /* Contents */
  priv->manager = awn_applet_manager_new_from_config (priv->client);
  g_signal_connect_swapped (priv->manager, "applet-embedded",
                            G_CALLBACK (on_applet_embedded), panel);
  g_signal_connect_swapped (priv->manager, "notify::expands",
                            G_CALLBACK (awn_panel_refresh_alignment), panel);
  gtk_container_add (GTK_CONTAINER (panel), priv->manager);
  gtk_widget_show_all (priv->manager);
  
  /* FIXME: Now is the time to hook our properties into priv->client */
  bridge = awn_config_bridge_get_default ();
  
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_PANEL_MODE,
                          object, "panel_mode");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_EXPAND,
                          object, "expand");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_ORIENT,
                          object, "orient");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_OFFSET,
                          object, "offset");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_SIZE,
                          object, "size");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_AUTOHIDE,
                          object, "autohide-type");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_STYLE,
                          object, "style");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_CLICKTHROUGH,
                          object, "clickthrough_type");

  /* Background drawing */
  awn_panel_set_style(AWN_PANEL (panel), priv->style);

  /* Size and position */
  g_signal_connect (panel, "configure-event",
                    G_CALLBACK (on_window_configure), NULL);

  position_window (AWN_PANEL (panel));
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

  switch (prop_id)
  {
    case PROP_CLIENT:
      g_value_set_pointer (value, priv->client);
      break;
    case PROP_MONITOR:
      g_value_set_pointer (value, priv->monitor);
      break;
    case PROP_APPLET_MANAGER:
      g_value_set_pointer (value, priv->manager);
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
    case PROP_ORIENT:
      g_value_set_int (value, priv->orient);
      break;
    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;
    case PROP_MAX_SIZE:
      // FIXME: probably not OK for non-composited
      switch (priv->orient)
      {
        case AWN_ORIENTATION_LEFT:
        case AWN_ORIENTATION_RIGHT:
          g_value_set_int (value, priv->manager->allocation.width);
          break;
        default:
          g_value_set_int (value, priv->manager->allocation.height);
          break;
      }
      break;
    case PROP_AUTOHIDE_TYPE:
      g_value_set_int (value, priv->autohide_type);
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
    case PROP_CLIENT:
      priv->client =  g_value_get_pointer (value);
      break;
    case PROP_COMPOSITED:
      priv->composited = g_value_get_boolean (value);
      break;
    case PROP_PANEL_MODE:
      awn_panel_set_panel_mode(panel, g_value_get_boolean (value));
      break;
    case PROP_EXPAND:
      priv->expand = g_value_get_boolean (value);
      gtk_widget_queue_resize (GTK_WIDGET (panel));
      break;
    case PROP_OFFSET:
      awn_panel_set_offset (panel, g_value_get_int (value));
      break;
    case PROP_ORIENT:
      awn_panel_set_orient (panel, g_value_get_int (value));
      break;
    case PROP_SIZE:
      awn_panel_set_size (panel, g_value_get_int (value));
      break;
    case PROP_AUTOHIDE_TYPE:
      awn_panel_set_autohide_type (panel, g_value_get_int (value));
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
  AwnPanelPrivate *priv = AWN_PANEL_GET_PRIVATE(widget);

  GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));
  if (!GTK_IS_WIDGET (child))
    return;

  gtk_widget_size_request (child, requisition);

  gint size = priv->size + priv->offset + priv->extra_padding;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      if (priv->expand) requisition->width = priv->monitor->width;
      requisition->height = priv->composited ? size + priv->size : size;

      // if we're animating resizes, we can't just request the size applets
      // require, because the animation must have extra space to draw to
      if (priv->animated_resize && !priv->expand)
      {
        if (requisition->width != priv->draw_width && !priv->resize_timer_id)
        {
          priv->resize_timer_id = g_timeout_add (40, awn_panel_resize_timeout,
                                                 widget);
        }
        requisition->width = MAX (requisition->width, priv->draw_width);
      }
      break;
    case AWN_ORIENTATION_LEFT:
    case AWN_ORIENTATION_RIGHT:
    default:
      requisition->width = priv->composited ? size + priv->size : size;
      if (priv->expand) requisition->height = priv->monitor->height;

      if (priv->animated_resize && !priv->expand)
      {
        if (requisition->height != priv->draw_height && !priv->resize_timer_id)
          priv->resize_timer_id = g_timeout_add (40, awn_panel_resize_timeout,
                                                 widget);
        requisition->height = MAX (requisition->height, priv->draw_height);
      }
      break;
  }
}

static gboolean
awn_panel_resize_timeout (gpointer data)
{
  gboolean resize_done;
  gint inc, step;
  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;

  // find size we are resizing to
  const gint target_width = priv->alignment->requisition.width;
  const gint target_height = priv->alignment->requisition.height;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_LEFT:
    case AWN_ORIENTATION_RIGHT:
      inc = abs (target_height - priv->draw_height);
      step = inc / 7 + 2; // makes the resize shiny
      inc = MIN (inc, step);

      priv->draw_height += priv->draw_height < target_height ? inc : -inc;

      resize_done = priv->draw_height == target_height;
      break;
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
    default:
      inc = abs (target_width - priv->draw_width);
      step = inc / 7 + 2; // makes the resize shiny
      inc = MIN (inc, step);

      priv->draw_width += priv->draw_width < target_width ? inc : -inc;

      resize_done = priv->draw_width == target_width;
      break;
  }

#if 0
  g_debug ("dw: %d..%d, dh: %d..%d", priv->draw_width, target_width,
                                     priv->draw_height, target_height);
#endif

  gtk_widget_queue_draw (GTK_WIDGET (panel));

  if (resize_done)
  {
    gtk_widget_queue_resize (GTK_WIDGET (panel));
    priv->resize_timer_id = 0;
  }

  return !resize_done;
}

static
void awn_panel_refresh_alignment (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;
  gfloat align = priv->monitor ? priv->monitor->align : 0.5;
  gfloat expand = priv->expand && priv->manager &&
    awn_applet_manager_get_expands (AWN_APPLET_MANAGER (priv->manager)) ?
      1.0 : 0.0;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      gtk_alignment_set (GTK_ALIGNMENT (priv->alignment),
                         align, 0.5, expand, 1.0);
      break;
    case AWN_ORIENTATION_LEFT:
    case AWN_ORIENTATION_RIGHT:
    default:
      gtk_alignment_set (GTK_ALIGNMENT (priv->alignment),
                         0.5, align, 1.0, expand);
      break;
  }
}

static
void awn_panel_refresh_padding (AwnPanel *panel, gpointer user_data)
{
  AwnPanelPrivate *priv = panel->priv;
  guint top, left, bottom, right;

  if (!priv->bg || !AWN_IS_BACKGROUND (priv->bg))
  {
    gtk_alignment_set_padding (GTK_ALIGNMENT (priv->alignment), 0, 0, 0, 0);
    priv->extra_padding = ACTIVE_RECT_PADDING;

    gtk_widget_queue_draw (GTK_WIDGET (panel));
    return;
  }

  /* refresh the padding */
  awn_background_padding_request (priv->bg, priv->orient,
                                  &top, &bottom, &left, &right);

  /* never actually set the top padding, its only internal constant */
  /* well actually non-composited env could use also the top padding */
  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
      priv->extra_padding = bottom + top;
      if (priv->composited) bottom = 0;
      break;
    case AWN_ORIENTATION_BOTTOM:
      priv->extra_padding = bottom + top;
      if (priv->composited) top = 0;
      break;
    case AWN_ORIENTATION_LEFT:
      priv->extra_padding = left + right;
      if (priv->composited) right = 0;
      break;
    case AWN_ORIENTATION_RIGHT:
      priv->extra_padding = left + right;
      if (priv->composited) left = 0;
      break;
  }

  priv->extra_padding += ACTIVE_RECT_PADDING;

  gtk_alignment_set_padding (GTK_ALIGNMENT (priv->alignment),
                             top, bottom, left, right);

  gtk_widget_queue_draw (GTK_WIDGET (panel));
}

static
void awn_panel_get_applet_rect (AwnPanel *panel,
                                GdkRectangle *area,
                                gint width, gint height)
{
  AwnPanelPrivate *priv = panel->priv;

  /*
   * We provide a param for width & height, cause for example
   * in the configure event callback allocation field is not yet updated.
   * Otherwise zeroes can be used for width & height
   */
  if (!width) width = GTK_WIDGET (panel)->allocation.width;
  if (!height) height = GTK_WIDGET (panel)->allocation.height;

  guint top, bottom, left, right;
  gtk_alignment_get_padding (GTK_ALIGNMENT (priv->alignment),
                             &top, &bottom, &left, &right);

  gint paintable_size = priv->offset + priv->size;

  /* this should work for both composited and non-composited */
  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
      area->x = left;
      area->y = top;
      area->width = width - left - right;
      area->height = paintable_size;
      break;

    case AWN_ORIENTATION_BOTTOM:
      area->x = left;
      area->y = height - paintable_size - bottom;
      area->width = width - left - right;
      area->height = paintable_size;
      break;

    case AWN_ORIENTATION_RIGHT:
      area->x = width - paintable_size - right;
      area->y = top;
      area->width = paintable_size;
      area->height = height - top - bottom;
      break;

    case AWN_ORIENTATION_LEFT:
    default:
      area->x = left;
      area->y = top;
      area->width = paintable_size;
      area->height = height - top - bottom;
  }
}


static
void awn_panel_get_draw_rect (AwnPanel *panel,
                              GdkRectangle *area,
                              gint width, gint height)
{
  AwnPanelPrivate *priv = panel->priv;

  /* 
   * We provide a param for width & height, cause for example
   * in the configure event callback allocation field is not yet updated.
   * Otherwise zeroes can be used for width & height
   */
  if (!width) width = GTK_WIDGET (panel)->allocation.width;
  if (!height) height = GTK_WIDGET (panel)->allocation.height;

  /* if we're not composited the whole window is drawable */
  if (priv->composited == FALSE)
  {
    area->x = 0; area->y = 0;
    area->width = width; area->height = height;
    return;
  }

  gint paintable_size = priv->offset + priv->size + priv->extra_padding;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
      area->y = 0;
      area->height = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->x = ROUND ((width - priv->draw_width) * priv->monitor->align);
        area->width = priv->draw_width;
      }
      else
      {
        area->x = 0;
        area->width = width;
      }
      break;

    case AWN_ORIENTATION_BOTTOM:
      area->y = height - paintable_size;
      area->height = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->x = ROUND ((width - priv->draw_width) * priv->monitor->align);
        area->width = priv->draw_width;
      }
      else
      {
        area->x = 0;
        area->width = width;
      }
      break;

    case AWN_ORIENTATION_RIGHT:
      area->x = width - paintable_size;
      area->width = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->y = ROUND ((height - priv->draw_height) * priv->monitor->align);
        area->height = priv->draw_height;
      }
      else
      {
        area->y = 0;
        area->height = height;
      }
      break;

    case AWN_ORIENTATION_LEFT:
    default:
      area->x = 0;
      area->width = paintable_size;

      if (priv->animated_resize && !priv->expand)
      {
        area->y = ROUND ((height - priv->draw_height) * priv->monitor->align);
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

static gboolean awn_panel_check_mouse_pos (AwnPanel *panel,
                                           gboolean whole_window)
{
  /* returns TRUE if mouse is in the window / or on the edge (if !whole_window) */
  g_return_val_if_fail(AWN_IS_PANEL(panel), FALSE);

  GtkWidget *widget = GTK_WIDGET (panel);
  AwnPanelPrivate *priv = panel->priv;
  GdkWindow *panel_win;

  gint x, y, window_x, window_y;
  /* FIXME: probably needs some love to work on multiple monitors */
  gdk_display_get_pointer (gdk_display_get_default (), NULL, &x, &y, NULL);
  panel_win = gtk_widget_get_window (widget);
  gdk_window_get_root_origin (panel_win, &window_x, &window_y);

#if 0
  g_debug ("mouse: %d,%d; window: %d,%d, %dx%d", x, y, window_x, window_y,
    widget->allocation.width, widget->allocation.height);
#endif

  if (!whole_window)
  {
    /* edge detection */
    switch (priv->orient)
    {
      case AWN_ORIENTATION_TOP:
        return x >= window_x && x < window_x + widget->allocation.width &&
               y == window_y;
      case AWN_ORIENTATION_BOTTOM:
        return x >= window_x && x < window_x + widget->allocation.width &&
               y == window_y + widget->allocation.height - 1;
      case AWN_ORIENTATION_LEFT:
        return y >= window_y && y < window_y + widget->allocation.height &&
               x == window_x;
      case AWN_ORIENTATION_RIGHT:
        return y >= window_y && y < window_y + widget->allocation.height &&
               x == window_x + widget->allocation.width - 1;
    }
  }
  else
  {
    if (!priv->composited)
    {
      return (x >= window_x && x < window_x + widget->allocation.width &&
              y >= window_y && y < window_y + widget->allocation.height);
    }
    else
    {
      gint k = priv->size;
      switch (priv->orient)
      {
        case AWN_ORIENTATION_TOP:
          return x >= window_x && x < window_x + widget->allocation.width &&
                 y >= window_y && y < window_y + widget->allocation.height - k;
        case AWN_ORIENTATION_BOTTOM:
          return x >= window_x && x < window_x + widget->allocation.width &&
                 y >= window_y + k && y < window_y + widget->allocation.height;
        case AWN_ORIENTATION_LEFT:
          return y >= window_y && y < window_y + widget->allocation.height &&
                 x >= window_x && x < window_x + widget->allocation.width - k;
        case AWN_ORIENTATION_RIGHT:
          return y >= window_y && y < window_y + widget->allocation.height &&
                 x >= window_x + k && x < window_x + widget->allocation.width;
      }
    }
  }
  return FALSE;
}

/* Auto-hide fade out method */
static gboolean 
alpha_blend_hide (gpointer data)
{
  g_return_val_if_fail(AWN_IS_PANEL(data), FALSE);

  AwnPanel *panel = AWN_PANEL (data);
  AwnPanelPrivate *priv = panel->priv;
  GdkWindow *win;

  priv->hide_counter++;

  win = gtk_widget_get_window (GTK_WIDGET (panel));

  gdk_window_set_opacity (win, 1 - 0.05 * priv->hide_counter);

  if (priv->hide_counter == 20)
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
  gtk_window_stick (GTK_WINDOW (panel));

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
      awn_panel_check_mouse_pos (panel, TRUE))
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
          !priv->autohide_started || priv->autohide_always_visible))
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
    else if (GTK_WIDGET_MAPPED (widget))
    {
      /* mouse is away, panel should start hiding */
      if (priv->autohide_start_timer_id == 0  && !priv->autohide_started)
      {
        /* the timeout will emit autohide-start */
        priv->autohide_start_timer_id =
          g_timeout_add (AUTOHIDE_DELAY, autohide_start_timeout, panel);
      }
    }
  }

  /* Clickthrough on CTRL */
  GdkModifierType mask; 
  gdk_display_get_pointer (gdk_display_get_default (), NULL, NULL, NULL, &mask);

  /* specialstate is TRUE whenever someone hovers Awn while holding
   *  the ctrl-key
   */
  gboolean specialstate = (mask & GDK_CONTROL_MASK) &&
                          awn_panel_check_mouse_pos (panel, TRUE);

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
  if (awn_panel_check_mouse_pos (panel, TRUE) &&
      priv->clickthrough_type == CLICKTHROUGH_ON_CTRL)
    return TRUE;

  /* In other cases, the polling may end */
  priv->mouse_poll_timer_id = 0;
  return FALSE;
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

  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_pointer ("client",
                          "Client",
                          "The AwnConfigClient",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_MONITOR,
    g_param_spec_pointer ("monitor",
                          "Monitor",
                          "The AwnMonitor",
                          G_PARAM_READABLE));

  g_object_class_install_property (obj_class,
    PROP_APPLET_MANAGER,
    g_param_spec_pointer ("applet-manager",
                          "Applet manager",
                          "The AwnAppletManager",
                          G_PARAM_READABLE));

  g_object_class_install_property (obj_class,
    PROP_COMPOSITED,
    g_param_spec_boolean ("composited",
                          "Composited",
                          "The window is composited",
                          TRUE,
                          G_PARAM_READWRITE));
  g_object_class_install_property (obj_class,
    PROP_PANEL_MODE,
    g_param_spec_boolean ("panel_mode",
                          "Panel Mode",
                          "The window sets the appropriete panel size hints",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (obj_class,
    PROP_EXPAND,
    g_param_spec_boolean ("expand",
                          "Expand",
                          "The panel will expand to fill the entire "
                          "width/height of the screen",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
 g_object_class_install_property (obj_class,
    PROP_ORIENT,
    g_param_spec_int ("orient",
                      "Orient",
                      "The orientation of the panel",
                      0, 3, AWN_ORIENTATION_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
   
  g_object_class_install_property (obj_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "Size",
                      "The size of the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (obj_class,
    PROP_MAX_SIZE,
    g_param_spec_int ("max-size",
                      "Max Size",
                      "Maximum size for drawing on the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READABLE));
  
  g_object_class_install_property (obj_class,
    PROP_OFFSET,
    g_param_spec_int ("offset",
                      "Offset",
                      "An offset for applets in the panel",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_AUTOHIDE_TYPE,
    g_param_spec_int ("autohide-type",
                      "Autohide type",
                      "Type of used autohide",
                      0, AUTOHIDE_TYPE_LAST - 1, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
 
  g_object_class_install_property (obj_class,
    PROP_STYLE,
    g_param_spec_int ("style",
                      "Style",
                      "Style of the bar",
                      0, STYLE_LAST - 1, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_CLICKTHROUGH,
    g_param_spec_int ("clickthrough-type",
                      "Clickthrough type",
                      "Type of clickthrough action",
                      0, CLICKTHROUGH_LAST - 1, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

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
  
  _panel_signals[ORIENT_CHANGED] =
		g_signal_new ("orient_changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnPanelClass, orient_changed),
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
			      awn_marshal_VOID__STRING_STRING_BOXED,
			      G_TYPE_NONE,
			      3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_VALUE);

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

static void
awn_panel_init (AwnPanel *panel)
{
  AwnPanelPrivate *priv;

  priv = panel->priv = AWN_PANEL_GET_PRIVATE (panel);

  priv->path_type = AWN_PATH_LINEAR;
  priv->offset_mod = 1.0;

  priv->inhibits = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                          NULL, free_inhibit_item);

  gtk_widget_set_app_paintable (GTK_WIDGET (panel), TRUE);

  priv->eventbox = gtk_event_box_new ();
  gtk_widget_set_app_paintable (priv->eventbox, TRUE);
  GTK_CONTAINER_CLASS (awn_panel_parent_class)->add (GTK_CONTAINER (panel),
                                                     priv->eventbox);
  gtk_widget_show (priv->eventbox);
  
  priv->alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_container_add (GTK_CONTAINER (priv->eventbox), priv->alignment);
  gtk_widget_show (priv->alignment);

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

GtkWidget *
awn_panel_new_from_config (AwnConfigClient *client)
{
  GtkWidget *window;
  
  window = g_object_new (AWN_TYPE_PANEL,
                         "type", GTK_WINDOW_TOPLEVEL,
                         "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
                         "client", client,
                         NULL);
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
  GdkBitmap       *shaped_bitmap;
  cairo_t         *cr;
  GdkRectangle     applet_rect;

  g_return_if_fail (AWN_IS_PANEL (panel));
  priv = AWN_PANEL (panel)->priv;
    
  if (priv->clickthrough && priv->composited)
  {
    GdkRegion *region = gdk_region_new ();
    GdkWindow *win;
    win = gtk_widget_get_window (GTK_WIDGET (panel));
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
        awn_background_get_input_shape_mask (priv->bg, cr, priv->orient, &area);
      }
      /* If window is not composited set shape of the window */
      else
      {
        awn_background_get_shape_mask (priv->bg, cr, priv->orient, &area);
      }
    }

    /* combine with applet's eventbox (with proper dimensions) */
    cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
    cairo_set_source_rgb (cr, 1.0f, 1.0f, 1.0f);
    awn_panel_get_applet_rect (AWN_PANEL (panel), &applet_rect,
                               real_width, real_height);
    cairo_identity_matrix (cr);
    cairo_new_path (cr);
    cairo_rectangle (cr, applet_rect.x, applet_rect.y,
                     applet_rect.width, applet_rect.height);
    cairo_fill (cr);

    cairo_destroy (cr);

    if (priv->composited)
    {
      gtk_widget_input_shape_combine_mask (panel, NULL, 0, 0);
      gtk_widget_input_shape_combine_mask (panel, shaped_bitmap, 0, 0);
    }
    else
    {
      gtk_widget_shape_combine_mask (panel, NULL, 0, 0);
      gtk_widget_shape_combine_mask (panel, shaped_bitmap, 0, 0);
    }

    g_object_unref (shaped_bitmap);
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

  if (!GTK_WIDGET_REALIZED (panel))
    return FALSE;

  gtk_window_get_size (GTK_WINDOW (window), &ww, &hh);
  
  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
      x = ROUND ((monitor->width - ww) * monitor->align) + monitor->offset;
      y = 0;
      break;

    case AWN_ORIENTATION_RIGHT:
      x = monitor->width - ww;
      y = ROUND ((monitor->height - hh) * monitor->align) + monitor->offset;
      break;

    case AWN_ORIENTATION_BOTTOM:
      x = ROUND ((monitor->width - ww) * monitor->align) + monitor->offset;
      y = monitor->height - hh;
      break;

    case AWN_ORIENTATION_LEFT:
      x = 0;
      y = ROUND ((monitor->height - hh) * monitor->align) + monitor->offset;
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
      priv->old_orient != priv->orient)
  {
    priv->old_width = event->width;
    priv->old_height = event->height;
    priv->old_orient = priv->orient;
    priv->old_x = event->x;
    priv->old_y = event->y;

    awn_panel_update_masks (panel, event->width, event->height);

    /* Update position */
    position_window (AWN_PANEL (panel));

    /* Update the strut if the panel_mode is set */
    if (priv->panel_mode)
      awn_panel_set_strut (AWN_PANEL (panel));

    return TRUE;
  }
  if (priv->old_x != event->x || priv->old_y != event->y)
  {
    priv->old_x = event->x;
    priv->old_y = event->y;

    /* Update the strut if the panel_mode is set */
    if (priv->panel_mode)
      awn_panel_set_strut (AWN_PANEL (panel));

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
    awn_background_draw (priv->bg, cr, priv->orient, &area);
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
    awn_background_draw (priv->bg, cr, priv->orient, &area);
  }
 
  /* Pass on the expose event to the child */
  child = gtk_bin_get_child (GTK_BIN (widget));
  if (!GTK_IS_WIDGET (child))
    return TRUE;

  if (priv->composited)
  {
    GdkRegion *region;
    gdk_cairo_set_source_pixmap (cr,
                                 gtk_widget_get_window (child),
                                 child->allocation.x, 
                                 child->allocation.y);

    region = gdk_region_rectangle (&child->allocation);
    gdk_region_intersect (region, event->region);
    gdk_cairo_region (cr, region);
    cairo_clip (cr);
    gdk_region_destroy (region);
    
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_paint (cr);
  }

  cairo_destroy (cr);

/*#define DEBUG_DRAW_AREA
#define DEBUG_APPLET_AREA*/

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
  gtk_container_add (GTK_CONTAINER (priv->alignment), widget);
  
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

  g_signal_emit (panel, _panel_signals[OFFSET_CHANGED], 0, priv->offset);

  gtk_widget_queue_resize (GTK_WIDGET (panel));
}

static void
awn_panel_set_orient (AwnPanel *panel, gint orient)
{
  AwnPanelPrivate *priv = panel->priv;

  priv->orient = orient;

  awn_panel_refresh_alignment (panel);

  awn_panel_refresh_padding (panel, NULL);

  if (!GTK_WIDGET_REALIZED (panel))
    return;

  awn_panel_reset_autohide (panel);

  g_signal_emit (panel, _panel_signals[ORIENT_CHANGED], 0, priv->orient);
  
  position_window (panel);
  
  gtk_widget_queue_resize (GTK_WIDGET (panel));
}

static void
awn_panel_set_size (AwnPanel *panel, gint size)
{
  AwnPanelPrivate *priv = panel->priv;
  
  priv->size = size;

  if (!GTK_WIDGET_REALIZED (panel))
    return;
  
  g_signal_emit (panel, _panel_signals[SIZE_CHANGED], 0, priv->size);

  position_window (panel);

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
    priv->mouse_poll_timer_id = g_timeout_add (MOUSE_POLL_TIMER_DELAY,
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
      g_timeout_add (AUTOHIDE_DELAY, autohide_start_timeout, panel);
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
    priv->mouse_poll_timer_id = g_timeout_add (MOUSE_POLL_TIMER_DELAY,
                                               poll_mouse_position, panel);

  static gulong start_handler_id = 0;
  static gulong end_handler_id = 0;
  if (start_handler_id)
  {
    g_signal_handler_disconnect (panel, start_handler_id);
    start_handler_id = 0;
  }
  if (end_handler_id)
  {
    g_signal_handler_disconnect (panel, end_handler_id);
    end_handler_id = 0;
  }

  /* for all autohide types, just connect to autohide-start & end signals
   * and do what you want to do in the callbacks
   * The autohide-start should return FALSE if un-hiding requires to touch
   * the display edge, and TRUE if it should react on whole AWN window
   */
  switch (priv->autohide_type)
  {
    case AUTOHIDE_TYPE_KEEP_BELOW:
      start_handler_id = g_signal_connect (
        panel, "autohide-start", G_CALLBACK (keep_below_start), NULL);
      end_handler_id = g_signal_connect (
        panel, "autohide-end", G_CALLBACK (keep_below_end), NULL);
      break;
    case AUTOHIDE_TYPE_FADE_OUT:
      start_handler_id = g_signal_connect (
        panel, "autohide-start", G_CALLBACK (alpha_blend_start), NULL);
      end_handler_id = g_signal_connect (
        panel, "autohide-end", G_CALLBACK (alpha_blend_end), NULL);
      break;
    case AUTOHIDE_TYPE_TRANSPARENTIZE:
      start_handler_id = g_signal_connect (
        panel, "autohide-start", G_CALLBACK (transparentize_start), NULL);
      end_handler_id = g_signal_connect (
        panel, "autohide-end", G_CALLBACK (transparentize_end), NULL);
      break;
    default:
      break;
  }
}

static void
on_applet_embedded (AwnPanel  *panel, GtkWidget *applet)
{
  g_return_if_fail (AWN_IS_PANEL (panel) && GTK_IS_SOCKET (applet));

  AwnPanelPrivate *priv = panel->priv;

  if (priv->path_type != AWN_PATH_LINEAR)
  {
    /* the applet is most likely AwnAppletProxy - get uid */
    gchar *uid = NULL;
    g_object_get (applet, "uid", &uid, NULL);

    g_return_if_fail (uid);

    /* update applet's offset-modifier */
    GValue mod_value = {0};
    g_value_init (&mod_value, G_TYPE_FLOAT);
    g_value_set_float (&mod_value, priv->offset_mod);
    
    g_signal_emit (panel, _panel_signals[PROPERTY_CHANGED], 0, uid,
                   "offset-modifier", &mod_value);

    /* update applet's path-type */
    GValue value = {0};
    g_value_init (&value, G_TYPE_INT);

    g_value_set_int (&value, priv->path_type);
    g_signal_emit (panel, _panel_signals[PROPERTY_CHANGED], 0, uid,
                   "path-type", &value);

    g_free (uid);
  }
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
    default:
      g_assert_not_reached();
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
    
    g_signal_emit (panel, _panel_signals[PROPERTY_CHANGED], 0, "",
                   "offset-modifier", &mod_value);
  }

  GValue value = {0};
  g_value_init (&value, G_TYPE_INT);

  g_value_set_int (&value, path);

  priv->path_type = path;

  g_signal_emit (panel, _panel_signals[PROPERTY_CHANGED], 0, "",
                 "path-type", &value);
}

static void
awn_panel_set_panel_mode (AwnPanel *panel, gboolean  panel_mode)
{
  AwnPanelPrivate *priv = panel->priv;
  priv->panel_mode = panel_mode;

  if(priv->panel_mode)
  {
    /* Check if panel is already realized. So it has an GdkWindow.
     * If it's not, the strut will get set when the position and dimension get set. 
     */
    if(GTK_WIDGET_REALIZED(panel))
      awn_panel_set_strut(panel);
  }
  else
  {
    if(GTK_WIDGET_REALIZED(panel))
      awn_panel_remove_strut(panel);
  }

}

static void     
awn_panel_set_clickthrough_type(AwnPanel *panel, gint type)
{
  AwnPanelPrivate *priv = panel->priv;
  priv->clickthrough_type = type;

  if (priv->clickthrough_type != CLICKTHROUGH_NEVER
      && priv->mouse_poll_timer_id == 0 )
    priv->mouse_poll_timer_id = g_timeout_add (MOUSE_POLL_TIMER_DELAY,
                                               poll_mouse_position, panel);

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
awn_panel_set_strut (AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  gint strut, strut_start, strut_end;
  GdkWindow *win;
  GdkRectangle area;

  gtk_window_get_size (GTK_WINDOW (panel), &area.width, &area.height);
  gtk_window_get_position (GTK_WINDOW (panel), &area.x, &area.y);

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
      strut_start = area.x;
      strut_end = area.x + area.width - 1;
      break;
    case AWN_ORIENTATION_RIGHT:
      strut_start = area.y;
      strut_end = area.y + area.height - 1;
      break;
    case AWN_ORIENTATION_BOTTOM:
      strut_start = area.x;
      strut_end = area.x + area.width - 1;
      break;
    case AWN_ORIENTATION_LEFT:
      strut_start = area.y;
      strut_end = area.y + area.height - 1;
      break;
    default:
      g_assert_not_reached ();
  }
  strut = priv->offset + priv->size + priv->extra_padding;

  /* allow AwnBackground to change the strut */
  if (priv->bg != NULL)
    awn_background_get_strut_offsets (priv->bg, priv->orient, &area,
                                      &strut, &strut_start, &strut_end);

  win = gtk_widget_get_window (GTK_WIDGET (panel));

  xutils_set_strut (win, priv->orient, strut, strut_start, strut_end);
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

  g_print ("\n");

  awn_applet_manager_set_applet_flags (AWN_APPLET_MANAGER (priv->manager),
                                       uid, flags);

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
  gchar *detailed_signal = g_strdup_printf ("connection-closed::%s", sender);
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
    priv->autohide_inhibited = FALSE;

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

  switch (priv->orient)
  {
    case AWN_ORIENTATION_LEFT:
    case AWN_ORIENTATION_RIGHT:
      req->height = MAX (req->height, priv->docklet_minsize);
      break;
    default:
      req->width = MAX (req->width, priv->docklet_minsize);
  }
}

static void
docklet_plug_added (GtkSocket *socket, AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  // FIXME: an animation?!
  awn_applet_manager_hide_applets (AWN_APPLET_MANAGER (priv->manager));
  gtk_widget_show (priv->docklet);
}

static gboolean
docklet_plug_removed (GtkSocket *socket, AwnPanel *panel)
{
  AwnPanelPrivate *priv = panel->priv;

  // FIXME: an animation?! we could also optimize and not destroy the widget
  awn_applet_manager_remove_widget (AWN_APPLET_MANAGER (priv->manager),
                                    priv->docklet);
  awn_applet_manager_show_applets (AWN_APPLET_MANAGER (priv->manager));
  priv->docklet = NULL;
  priv->docklet_minsize = 0;

  return FALSE;
}

void
awn_panel_docklet_request (AwnPanel *panel,
                           gint min_size,
                           gboolean shrink,
                           DBusGMethodInvocation *context)
{
  AwnPanelPrivate *priv = panel->priv;
  guint32 window_id = 0;

  if (!priv->docklet)
  {
    // FIXME: perhaps the min-size shouldn't be min-size but the actual size
    //  and the docklet would be restricted to this size (set_size_request).
    if (!shrink) 
    {
      switch (priv->orient)
      {
        case AWN_ORIENTATION_LEFT:
        case AWN_ORIENTATION_RIGHT:
          priv->docklet_minsize =
            MAX (min_size, priv->manager->allocation.height);
          break;
        default:
          priv->docklet_minsize =
            MAX (min_size, priv->manager->allocation.width);
          break;
      }
    }
    else
    {
      priv->docklet_minsize = min_size;
    }

    priv->docklet = gtk_socket_new ();
    awn_utils_ensure_transparent_bg (priv->docklet);

    g_signal_connect_after (priv->docklet, "size-request",
                            G_CALLBACK (docklet_size_request), panel);
    g_signal_connect (priv->docklet, "plug-added",
                      G_CALLBACK (docklet_plug_added), panel);
    g_signal_connect (priv->docklet, "plug-removed",
                      G_CALLBACK (docklet_plug_removed), panel);

    awn_applet_manager_add_widget (AWN_APPLET_MANAGER (priv->manager),
                                   priv->docklet, 0);
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

