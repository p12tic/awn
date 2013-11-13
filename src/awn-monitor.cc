/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *                2010 Michal Hruby <michal.mhr@gmail.com>
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
 *  Notes : Thanks to MacSlow (macslow.thepimp.net) for the transparent & shaped
 *	        monitor code.
 */

#include "config.h"

#include <gdk/gdkx.h>

#include "awn-monitor.h"

#include "awn-defines.h"

G_DEFINE_TYPE (AwnMonitor, awn_monitor, G_TYPE_OBJECT) 

#define AWN_MONITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_MONITOR, AwnMonitorPrivate))

struct _AwnMonitorPrivate
{
  DesktopAgnosticConfigClient *client;

  GdkScreen *screen;
  guint      size_signal_id;
  guint      monitors_signal_id;

  gboolean construction_done;
  gboolean warning_shown;
  /* Monitor Geometry stuff */
  gboolean force_monitor;

  gint config_width, config_height;
  gint config_x_offset, config_y_offset;
  gint monitor_number;
};

enum 
{
  PROP_0,

  PROP_SCREEN,
  PROP_CLIENT,
  PROP_FORCE_MONITOR,
  PROP_HEIGHT,
  PROP_WIDTH,
  PROP_X_OFFSET,
  PROP_Y_OFFSET,
  PROP_ALIGN,
  PROP_MONITOR_NUMBER
};

enum
{
  GEOMETRY_CHANGED,

  LAST_SIGNAL
};
static guint _monitor_signals[LAST_SIGNAL] = { 0 };

/* FORWARDS */
static void awn_monitor_set_force_monitor (AwnMonitor *monitor,
                                           gboolean    force_monitor);

static void awn_monitor_update_fields     (AwnMonitor *monitor);

/* GObject stuff */
static void
awn_monitor_constructed (GObject *object)
{
  AwnMonitor        *monitor = AWN_MONITOR (object);
  AwnMonitorPrivate *priv = monitor->priv;

  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_MONITOR_NUM,
                                       object, "monitor-number", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_MONITOR_FORCE,
                                       object, "monitor-force", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_MONITOR_WIDTH,
                                       object, "monitor-width", FALSE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_MONITOR_HEIGHT,
                                       object, "monitor-height", FALSE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_MONITOR_X_OFFSET,
                                       object, "monitor-x-offset", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_MONITOR_Y_OFFSET,
                                       object, "monitor-y-offset", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_MONITOR_ALIGN,
                                       object, "monitor-align", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);

  priv->construction_done = TRUE;
}

static void
awn_monitor_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  AwnMonitor *monitor = AWN_MONITOR (object);
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (object));
  priv = AWN_MONITOR (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      g_value_set_object (value, priv->client);
      break;
    case PROP_MONITOR_NUMBER:
      g_value_set_int (value, priv->monitor_number);
      break;
    case PROP_FORCE_MONITOR:
      g_value_set_boolean (value, priv->force_monitor);
      break;
    case PROP_ALIGN:
      g_value_set_float (value, monitor->align);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, monitor->height);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, monitor->width);
      break;
    case PROP_X_OFFSET:
      g_value_set_int (value, monitor->x_offset);
      break;
    case PROP_Y_OFFSET:
      g_value_set_int (value, monitor->y_offset);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_monitor_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  AwnMonitor *monitor = AWN_MONITOR (object);
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (object));
  priv = AWN_MONITOR (object)->priv;

  switch (prop_id)
  {
    case PROP_SCREEN:
      priv->screen = g_value_get_object (value);
      priv->warning_shown = FALSE;
      if (!priv->force_monitor && priv->construction_done)
      {
        awn_monitor_update_fields (monitor);
      }
      break;
    case PROP_CLIENT:
      priv->client = g_value_get_object (value);
      break;
    case PROP_MONITOR_NUMBER:
      priv->monitor_number = g_value_get_int (value);
      if (!priv->force_monitor) awn_monitor_update_fields (monitor);
      break;
    case PROP_FORCE_MONITOR:
      priv->force_monitor = g_value_get_boolean (value);
      awn_monitor_set_force_monitor (monitor, priv->force_monitor);
      break;    
    case PROP_ALIGN:
      monitor->align = g_value_get_float (value);
      break;
    case PROP_WIDTH:
      priv->config_width = g_value_get_int (value);
      if (priv->force_monitor) monitor->width = priv->config_width;
      else return; // no signal emit
      break;
    case PROP_HEIGHT:
      priv->config_height = g_value_get_int (value);
      if (priv->force_monitor) monitor->height = priv->config_height;
      else return; // no signal emit
      break;
    case PROP_X_OFFSET:
      priv->config_x_offset = g_value_get_int (value);
      if (priv->force_monitor) monitor->x_offset = priv->config_x_offset;
      else return; // no signal emit
      break;
    case PROP_Y_OFFSET:
      priv->config_y_offset = g_value_get_int (value);
      if (priv->force_monitor) monitor->y_offset = priv->config_y_offset;
      else return; // no signal emit
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
  
  g_signal_emit (object, _monitor_signals[GEOMETRY_CHANGED], 0);
}

static void
awn_monitor_dispose (GObject *object)
{
  AwnMonitorPrivate *priv;

  priv = AWN_MONITOR_GET_PRIVATE (object);

  if (priv->size_signal_id)
  {
    g_signal_handler_disconnect (priv->screen, priv->size_signal_id);
    priv->size_signal_id = 0;
  }

  if (priv->monitors_signal_id)
  {
    g_signal_handler_disconnect (priv->screen, priv->monitors_signal_id);
    priv->monitors_signal_id = 0;
  }

  desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                        object, NULL);

  G_OBJECT_CLASS (awn_monitor_parent_class)->dispose (object);
}

static void
awn_monitor_class_init (AwnMonitorClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  
  obj_class->get_property = awn_monitor_get_property;
  obj_class->set_property = awn_monitor_set_property;
  obj_class->constructed  = awn_monitor_constructed;
  obj_class->dispose      = awn_monitor_dispose;

  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_SCREEN,
    g_param_spec_object ("screen",
                         "Screen",
                         "Managed screen",
                         GDK_TYPE_SCREEN,
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_object ("client",
                         "Config Client",
                         "Configuration client",
                         DESKTOP_AGNOSTIC_CONFIG_TYPE_CLIENT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_MONITOR_NUMBER,
    g_param_spec_int ("monitor-number",
                      "Monitor Number",
                      "Monitor number the panel is on",
                      -1, G_MAXINT, -1,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_FORCE_MONITOR,
    g_param_spec_boolean ("monitor-force",
                          "Monitor Force",
                          "Force the monitor geometry",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_WIDTH,
    g_param_spec_int ("monitor-width",
                      "Monitor Width",
                      "Monitor Width",
                      0, G_MAXINT, 1024,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_HEIGHT,
    g_param_spec_int ("monitor-height",
                      "Monitor Height",
                      "Monitor Height",
                      0, G_MAXINT, 768,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_X_OFFSET,
    g_param_spec_int ("monitor-x-offset",
                      "Monitor X-offset",
                      "An optional offset (for displays > 1)",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_Y_OFFSET,
    g_param_spec_int ("monitor-y-offset",
                      "Monitor Y-offset",
                      "An optional offset (for displays > 1)",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_ALIGN,
    g_param_spec_float ("monitor-align",
                        "Monitor Align",
                        "The alignment of the panel on the monitor",
                        0.0, 1.0, 0.5,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  /* Add signals to the class */
  _monitor_signals[GEOMETRY_CHANGED] = 
    g_signal_new ("geometry-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (AwnMonitorClass, geometry_changed),
                  NULL, NULL, 
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (AwnMonitorPrivate));
}

static void
awn_monitor_init (AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  priv = monitor->priv = AWN_MONITOR_GET_PRIVATE (monitor);
}

AwnMonitor *
awn_monitor_new_for_screen (GdkScreen *screen,
                            DesktopAgnosticConfigClient *client)
{
  AwnMonitor *monitor = g_object_new (AWN_TYPE_MONITOR,
                                      "screen", screen,
                                      "client", client,
                                      NULL);
  return monitor;
}

static void
awn_monitor_update_fields (AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv = monitor->priv;

  GdkRectangle geometry;
  gint monitor_number = priv->monitor_number;
  gint n_monitors = gdk_screen_get_n_monitors (priv->screen);
  // make sure there is monitor with this number, if not get last one
  if (n_monitors <= monitor_number)
  {
    if (!priv->warning_shown)
    {
      g_warning ("Unable to position Awn on monitor number %d,"
                 " using monitor %d instead", monitor_number, n_monitors-1);
      priv->warning_shown = TRUE;
    }
    monitor_number = n_monitors - 1;
  }
  // for monitor_number == -1, we'll get monitor with position [0, 0]
  if (monitor_number < 0)
  {
    monitor_number = gdk_screen_get_monitor_at_point (priv->screen, 0, 0);
  }

  gdk_screen_get_monitor_geometry (priv->screen, monitor_number, &geometry);
  monitor->width = geometry.width;
  monitor->height = geometry.height;
  monitor->x_offset = geometry.x;
  monitor->y_offset = geometry.y;
}

static void
on_screen_size_changed (GdkScreen *screen, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  if (priv->force_monitor) return;

  awn_monitor_update_fields (monitor);

  g_signal_emit (monitor, _monitor_signals[GEOMETRY_CHANGED], 0);
}

static void
on_screen_monitors_changed (GdkScreen *screen, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  priv->warning_shown = FALSE;

  if (priv->force_monitor) return;

  awn_monitor_update_fields (monitor);

  g_signal_emit (monitor, _monitor_signals[GEOMETRY_CHANGED], 0);
}

static void 
awn_monitor_set_force_monitor (AwnMonitor *monitor,
                               gboolean    force_monitor)
{
  AwnMonitorPrivate *priv = monitor->priv;

  if (force_monitor)
  {
    if (priv->size_signal_id)
    {
      g_signal_handler_disconnect (priv->screen, priv->size_signal_id);
      priv->size_signal_id = 0;
    }

    if (priv->monitors_signal_id)
    {
      g_signal_handler_disconnect (priv->screen, priv->monitors_signal_id);
      priv->monitors_signal_id = 0;
    }

    g_object_set (monitor,
                  "monitor-width", priv->config_width,
                  "monitor-height", priv->config_height,
                  "monitor-x-offset", priv->config_x_offset,
                  "monitor-y-offset", priv->config_y_offset,
                  NULL);
  }
  else
  {
    priv->size_signal_id = g_signal_connect (priv->screen, "size-changed",
                                  G_CALLBACK (on_screen_size_changed), monitor);
    priv->monitors_signal_id = g_signal_connect (priv->screen, 
          "monitors-changed", G_CALLBACK (on_screen_monitors_changed), monitor);

    awn_monitor_update_fields (monitor);

    g_signal_emit (monitor, _monitor_signals[GEOMETRY_CHANGED], 0);
  }
}


/* vim: set et ts=2 sts=2 sw=2 : */
