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

#include "awn-panel.h"

#include "awn-applet-manager.h"
#include "awn-background.h"
#include "awn-background-flat.h"
#include "awn-defines.h"
#include "awn-config-bridge.h"
#include "awn-monitor.h"
#include "awn-x.h"

G_DEFINE_TYPE (AwnPanel, awn_panel, GTK_TYPE_WINDOW) 

#define AWN_PANEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_PANEL, AwnPanelPrivate))

struct _AwnPanelPrivate
{
  AwnConfigClient *client;
  AwnMonitor *monitor;

  AwnBackground *bg;

  GtkWidget *alignment;
  GtkWidget *eventbox;
  GtkWidget *manager;
  
  gboolean composited;
  gboolean panel_mode;

  gint size;
  gint offset;
  gint orient;

  gint old_width;
  gint old_height;
};

enum 
{
  PROP_0,

  PROP_CLIENT,
  PROP_COMPOSITED,
  PROP_PANEL_MODE,
  PROP_OFFSET,
  PROP_ORIENT,
  PROP_SIZE
};

static const GtkTargetEntry drop_types[] = 
{
  { "STRING", 0, 0 },
  { "text/plain", 0, 0},
  { "text/uri-list", 0, 0}
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);

enum
{
  SIZE_CHANGED,
  ORIENT_CHANGED,
  DESTROY_NOTIFY,
  DESTROY_APPLET,

  LAST_SIGNAL
};
static guint _panel_signals[LAST_SIGNAL] = { 0 };

/* 
 * FORWARDS
 */
static void     load_correct_colormap (GtkWidget *panel);
static void     on_composited_changed (GdkScreen *screen, 
                                       AwnPanel  *panel);
static void     on_screen_changed     (GtkWidget *widget, 
                                       GdkScreen *screen,
                                       AwnPanel  *panel);

static gboolean on_window_configure   (GtkWidget         *panel,
                                       GdkEventConfigure *event);
static gboolean position_window       (AwnPanel *panel);

static gboolean on_eb_expose          (GtkWidget      *eb, 
                                       GdkEventExpose *event,
                                       GtkWidget      *child);
static gboolean awn_panel_expose      (GtkWidget      *widget, 
                                       GdkEventExpose *event);
static void     awn_panel_add         (GtkContainer   *window, 
                                       GtkWidget      *widget);

static void     awn_panel_set_offset  (AwnPanel *panel, 
                                       gint      offset);
static void     awn_panel_set_orient  (AwnPanel *panel,
                                       gint      orient);
static void     awn_panel_set_size    (AwnPanel *panel, 
                                       gint      size);

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

  /* FIXME: Now is the time to hook our properties into priv->client */
  bridge = awn_config_bridge_get_default ();
  
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_PANEL_MODE,
                          object, "panel_mode");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_ORIENT,
                          object, "orient");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_OFFSET,
                          object, "offset");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_SIZE,
                          object, "size");

  /* Background drawing */
  priv->bg = awn_background_flat_new (priv->client);

  /* Composited checks/setup */
  screen = gtk_widget_get_screen (panel);
  priv->composited = gdk_screen_is_composited (screen);
  load_correct_colormap (panel);
  g_signal_connect (screen, "composited-changed", 
                    G_CALLBACK (on_composited_changed), panel);
  g_signal_connect (panel, "screen-changed",
                    G_CALLBACK (on_screen_changed), panel);

  /* Size and position */
  g_signal_connect (panel, "configure-event",
                    G_CALLBACK (on_window_configure), NULL);

  switch (priv->orient)
  {
    case AWN_ORIENT_TOP:
    case AWN_ORIENT_BOTTOM:
      gtk_window_resize (GTK_WINDOW (panel),
                         48,
                         priv->offset +
                           (priv->composited ? 2 * priv->size : priv->size));
      break;

    case AWN_ORIENT_RIGHT:
    case AWN_ORIENT_LEFT:
      gtk_window_resize (GTK_WINDOW (panel),
                         priv->offset +
                           (priv->composited ? 2 * priv->size : priv->size),
                           48);  
      break;
    default:
      g_assert (0);
  }
  position_window (AWN_PANEL (panel));

  /* Contents */
  priv->manager = awn_applet_manager_new_from_config (priv->client);
  gtk_container_add (GTK_CONTAINER (panel), priv->manager);
  gtk_widget_show_all (priv->manager);
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
    case PROP_COMPOSITED:
      g_value_set_boolean (value, priv->composited);
      break;
    case PROP_PANEL_MODE:
      g_value_set_boolean (value, priv->panel_mode);
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
      priv->panel_mode = g_value_get_boolean (value);
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
awn_panel_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_panel_parent_class)->dispose (object);
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
  obj_class->get_property  = awn_panel_get_property;
  obj_class->set_property  = awn_panel_set_property;
    
  cont_class->add          = awn_panel_add;
  
  wid_class->expose_event  = awn_panel_expose;
  wid_class->show          = awn_panel_show;

  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_pointer ("client",
                          "Client",
                          "The AwnConfigClient",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

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
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
 g_object_class_install_property (obj_class,
    PROP_ORIENT,
    g_param_spec_int ("orient",
                      "Orient",
                      "The orientation of the panel",
                      0, 3, AWN_ORIENT_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
   
  g_object_class_install_property (obj_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "Size",
                      "The size of the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (obj_class,
    PROP_OFFSET,
    g_param_spec_int ("offset",
                      "Offset",
                      "An offset for applets in the panel",
                      0, G_MAXINT, 0,
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

  g_type_class_add_private (obj_class, sizeof (AwnPanelPrivate));

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass), 
                                   &dbus_glib_awn_panel_object_info);
}

static void
awn_panel_init (AwnPanel *panel)
{
  AwnPanelPrivate *priv;

  priv = panel->priv = AWN_PANEL_GET_PRIVATE (panel);

  priv->eventbox = gtk_event_box_new ();
  gtk_widget_set_app_paintable (priv->eventbox, TRUE);
  GTK_CONTAINER_CLASS (awn_panel_parent_class)->add (GTK_CONTAINER (panel),
                                                     priv->eventbox);
  gtk_widget_show (priv->eventbox);
  
  priv->alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_container_add (GTK_CONTAINER (priv->eventbox), priv->alignment);
  gtk_widget_show (priv->alignment);

  g_signal_connect (priv->eventbox, "expose-event",
                    G_CALLBACK (on_eb_expose), priv->alignment);
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

  if (priv->composited)
  {
    /* First try loading the RGBA colormap */
    colormap = gdk_screen_get_rgba_colormap (screen);

    if (!colormap)
    {
      /* Use the RGB colormap and set composited to FALSE */
      colormap = gdk_screen_get_rgb_colormap (screen);
      priv->composited = FALSE;
    }
  }
  else
  {
    colormap = gdk_screen_get_rgb_colormap (screen);
  }
  gtk_widget_set_colormap (panel, colormap);
}

/*
 * At the moment, we just want to restart the panel on composited state changes
 */
static void 
on_composited_changed (GdkScreen *screen, 
                       AwnPanel  *panel)
{
  AwnConfigClient *client;

  g_return_if_fail (AWN_IS_PANEL (panel));

  if (gdk_screen_is_composited (screen) == panel->priv->composited)
    return;

  client = panel->priv->client;

  gtk_widget_destroy (GTK_WIDGET (panel));

  awn_panel_new_from_config (client);
}

static void 
on_screen_changed (GtkWidget *widget, 
                   GdkScreen *screen,
                   AwnPanel  *panel)
{
  AwnPanelPrivate *priv;
  GdkScreen       *new_screen;
  gboolean         new_is_composited;
  
  g_return_if_fail (AWN_IS_PANEL (panel));
  priv = panel->priv;
  
  new_screen = gtk_widget_get_screen (GTK_WIDGET (panel));
  new_is_composited = gdk_screen_is_composited (screen);

  /* If they are the same, then return */
  if (new_is_composited == priv->composited)
  {
    g_signal_handlers_disconnect_by_func (screen,
                                          on_composited_changed,
                                          panel);
    g_signal_connect (screen, "composited-changed", 
                      G_CALLBACK (on_composited_changed), panel);
    return;
  }
  else
  {
    on_composited_changed (new_screen, panel);
  }
}

/*
 * SIZING AND POSITIONING 
 */

/*
 * We set the shape of the window, so when in composited mode, we dont't 
 * receive events in the blank space above the main window
 */
static void
awn_panel_update_input_shape (GtkWidget *panel, 
                              gint       real_width, 
                              gint       real_height)
{
  AwnPanelPrivate *priv;
  GdkBitmap       *shaped_bitmap;
  cairo_t         *cr;
  gint             x, y, width, height;

  g_return_if_fail (AWN_IS_PANEL (panel));
  priv = AWN_PANEL (panel)->priv;
  
  shaped_bitmap = (GdkBitmap*)gdk_pixmap_new (NULL, real_width, real_height, 1);

  if (!shaped_bitmap)
    return;

  switch (priv->orient)
  {
    case AWN_ORIENT_TOP:
      x = 0;
      y = 0;
      width = real_width;
      height = priv->offset + priv->size;
      break;
    
    case AWN_ORIENT_RIGHT:
      x = priv->size;
      y = 0;
      width = priv->offset + priv->size;
      height = real_height;
      break;
    
    case AWN_ORIENT_BOTTOM:
      x = 0;
      y = priv->size;
      width = real_width;
      height = priv->size + priv->offset;
      break;

    case AWN_ORIENT_LEFT:
      x = 0;
      y = 0;
      width = priv->offset + priv->size;
      height = real_height;
      break;
    
    default:
      g_assert (0);
  }

  cr = gdk_cairo_create (shaped_bitmap);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgb (cr, 1.0f, 1.0f, 1.0f);

  cairo_rectangle (cr, x, y, width, height);
 
  cairo_fill (cr);
  cairo_destroy (cr);

  gtk_widget_input_shape_combine_mask (panel, NULL, 0, 0);
  gtk_widget_input_shape_combine_mask (panel, shaped_bitmap, 0, 0);

  if (shaped_bitmap)
    g_object_unref (shaped_bitmap);
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

  /* FIXME: This has no idea about auto-hide */

  switch (priv->orient)
  {
    case AWN_ORIENT_TOP:
      x = ((monitor->width - ww) * monitor->align) + monitor->offset;
      y = 0;
      break;

    case AWN_ORIENT_RIGHT:
      x = monitor->width - ww;
      y = ((monitor->height - hh) * monitor->align) + monitor->offset;
      break;

    case AWN_ORIENT_BOTTOM:
      x = ((monitor->width - ww) * monitor->align) + monitor->offset;
      y = monitor->height - hh;
      break;

    case AWN_ORIENT_LEFT:
      x = 0;
      y = ((monitor->height - hh) * monitor->align) + monitor->offset;
      break;

    default:
      g_assert (0);
  }

  gtk_window_move (window, x, y);

  g_debug ("%d %d %d %d %d %d", x, y, ww, hh, monitor->width, monitor->height);

  return FALSE;
}

/*
 * We get a configure event when the windows size changes. This is a good as
 * a time as any to update the input shape of the window.
 */
static gboolean
on_window_configure (GtkWidget          *panel,
                     GdkEventConfigure  *event)
{
  AwnPanelPrivate *priv;

  g_return_val_if_fail (AWN_IS_PANEL (panel), FALSE);
  priv = AWN_PANEL (panel)->priv;

  if (priv->old_width == event->width && priv->old_height == event->height)
    return FALSE;

  priv->old_width = event->width;
  priv->old_height = event->height;

  /* Only set the shape of the window if the window is composited */
  if (priv->composited)
    awn_panel_update_input_shape (panel, event->width, event->height);

  /* Update the size hints if the panel_mode is set */
  if (priv->panel_mode)
    awn_x_set_strut (GTK_WINDOW (panel));
  
  /* Update position */
  position_window (AWN_PANEL (panel));

  //gtk_widget_queue_draw (GTK_WIDGET (panel));

  return TRUE;
}

/*
 * PANEL BACKGROUND & EMBEDDING CODE
 */

/*
 * Clear the eventboxes background
 */
static gboolean 
on_eb_expose (GtkWidget      *widget, 
              GdkEventExpose *event,
              GtkWidget      *child)
{
  cairo_t *cr;

  if (!GDK_IS_DRAWABLE (widget->window))
  {
    g_debug ("!GDK_IS_DRAWABLE (widget->window) failed");
    return FALSE;
  }

  /* Get our ctx */
  cr = gdk_cairo_create (widget->window);
  if (!cr)
  {
    g_debug ("Unable to create cairo context\n");
    return FALSE;
  }

  /* The actual drawing of the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_destroy (cr);

  gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                  child,
                                  event);
  return TRUE;
}

/*
 * Draw the panel 
 */
static gboolean
awn_panel_expose (GtkWidget *widget, GdkEventExpose *event)
{
  AwnPanelPrivate *priv;
  gfloat           x=0, y=0;
  gint             width = 0, height = 0;
  cairo_t         *cr;
  GtkWidget       *child;

  g_return_val_if_fail (AWN_IS_PANEL (widget), FALSE);
  priv = AWN_PANEL (widget)->priv;

  if (!GDK_IS_DRAWABLE (widget->window))
  {
    g_debug ("!GDK_IS_DRAWABLE (widget->window) failed");
    return FALSE;
  }

  gtk_window_get_size (GTK_WINDOW (widget), &width, &height);

  /* Calculate correct values */
  switch (priv->orient)
  {
    case AWN_ORIENT_TOP:
      x = 0;
      y = 0;
      width = width;
      height = height - (priv->composited ? priv->size : 0);
      break;

    case AWN_ORIENT_RIGHT:
      x = (priv->composited ? priv->size : 0);
      y = 0;
      width = width - (priv->composited ? priv->size : 0);
      height = height;
      break;

    case AWN_ORIENT_BOTTOM:
      x = 0;
      y = (priv->composited ? priv->size : 0);
      width = width;
      height = height - (priv->composited ? priv->size : 0);
      break;

    case AWN_ORIENT_LEFT:
    default:
      x = 0;
      y = 0;
      width = width - (priv->composited ? priv->size : 0);
      height = height;
  }

  /* Get our ctx */
  cr = gdk_cairo_create (widget->window);
  if (!cr)
  {
    g_debug ("Unable to create cairo context\n");
    return FALSE;
  }
  
  /* The actual drawing of the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  awn_background_draw (priv->bg, cr, priv->orient, x, y, width, height);
 
  /* Pass on the expose event to the child */
  child = gtk_bin_get_child (GTK_BIN (widget));
  if (!GTK_IS_WIDGET (child))
    return TRUE;

  if (priv->composited)
  {
    GdkRegion *region;
    
    gdk_cairo_set_source_pixmap (cr, child->window,
                                 child->allocation.x, 
                                 child->allocation.y);

    region = gdk_region_rectangle (&child->allocation);
    gdk_region_intersect (region, event->region);
    gdk_cairo_region (cr, region);
    cairo_clip (cr);
    
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_paint_with_alpha (cr, 0.5);
  }

  cairo_destroy (cr);

  gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                  child,
                                  event);
  priv->old_width = width;
  priv->old_height = height;

  return TRUE;
}


static void
awn_panel_add (GtkContainer *window, GtkWidget *widget)
{
  AwnPanelPrivate *priv;

  g_return_if_fail (AWN_IS_PANEL (window));
  priv = AWN_PANEL_GET_PRIVATE (window);

  /* Add the widget to the internal alignment */
  gtk_container_add (GTK_CONTAINER (priv->alignment), widget);
  
  /* Set up the eventbox for compositing (if necessary) */
  gtk_widget_realize (priv->eventbox);
  if (priv->composited)
    gdk_window_set_composited (priv->eventbox->window, priv->composited);

  gtk_widget_show (widget);
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
  
  switch (priv->orient)
  {
    case AWN_ORIENT_TOP:
    case AWN_ORIENT_BOTTOM:
      gtk_window_resize (GTK_WINDOW (panel),
                         GTK_WIDGET (panel)->allocation.width, 
                         offset 
                          + (priv->composited ? 2 * priv->size : priv->size));
      break;
    case AWN_ORIENT_RIGHT:
    case AWN_ORIENT_LEFT:
    default:
      gtk_window_resize (GTK_WINDOW (panel),
                         offset 
                          + (priv->composited ? 2 * priv->size : priv->size),
                         GTK_WIDGET (panel)->allocation.height);
   }
}

static void
awn_panel_set_orient (AwnPanel *panel, gint orient)
{
  AwnPanelPrivate *priv = panel->priv;

  priv->orient = orient;
  g_signal_emit (panel, _panel_signals[ORIENT_CHANGED], 0, priv->orient);
  position_window (panel);
  gtk_widget_queue_draw (GTK_WIDGET (panel));
}

static void
awn_panel_set_size (AwnPanel *panel, gint size)
{
  AwnPanelPrivate *priv = panel->priv;
  
  priv->size = size;
  
  switch (priv->orient)
  {
    case AWN_ORIENT_TOP:
    case AWN_ORIENT_BOTTOM:
      gtk_window_resize (GTK_WINDOW (panel),
                         GTK_WIDGET (panel)->allocation.width, 
                         priv->offset 
                          + (priv->composited ? 2 * priv->size : priv->size));
      break;
    case AWN_ORIENT_RIGHT:
    case AWN_ORIENT_LEFT:
    default:
      gtk_window_resize (GTK_WINDOW (panel),
                         priv->offset 
                          + (priv->composited ? 2 * priv->size : priv->size),
                         GTK_WIDGET (panel)->allocation.height);
   }

  g_signal_emit (panel, _panel_signals[SIZE_CHANGED], 0, priv->size);
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
