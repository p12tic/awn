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
 *
 *  Notes : Thanks to MacSlow (macslow.thepimp.net) for the transparent & shaped
 *	        monitor code.
*/

#include "config.h"

#include <gdk/gdkx.h>
#include <libawn/awn-config-bridge.h>
#include <libawn/awn-config-client.h>

#include "awn-monitor.h"

#include "awn-defines.h"

G_DEFINE_TYPE (AwnMonitor, awn_monitor, G_TYPE_OBJECT) 

#define AWN_MONITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_MONITOR, AwnMonitorPrivate))

struct _AwnMonitorPrivate
{
  AwnConfigClient *client;

  GdkScreen *screen;
  guint      tag;

  /* Monitor Geometry stuff */
  gboolean force_monitor;

  gint config_width, config_height;
};

enum 
{
  PROP_0,

  PROP_CLIENT,
  PROP_FORCE_MONITOR,
  PROP_HEIGHT,
  PROP_WIDTH,
  PROP_OFFSET,
  PROP_ALIGN
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

/* GObject stuff */
static void
awn_monitor_constructed (GObject *object)
{
  AwnMonitor        *monitor = AWN_MONITOR (object);
  AwnMonitorPrivate *priv = monitor->priv;
  AwnConfigBridge   *bridge = awn_config_bridge_get_default ();

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_MONITOR_FORCE,
                          object, "monitor_force");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_MONITOR_WIDTH,
                          object, "monitor_width");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_MONITOR_HEIGHT,
                          object, "monitor_height");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_MONITOR_FORCE,
                          object, "monitor_force");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_MONITOR_OFFSET,
                          object, "monitor_offset");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_MONITOR_ALIGN,
                          object, "monitor_align");
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
      g_value_set_pointer (value, priv->client);
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
    case PROP_OFFSET:
      g_value_set_int (value, monitor->offset);
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
    case PROP_CLIENT:
      priv->client = g_value_get_pointer (value);
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
    case PROP_OFFSET:
      monitor->offset = g_value_get_int (value);
      break;   
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
  
  g_signal_emit (object, _monitor_signals[GEOMETRY_CHANGED], 0);
}

static void
awn_monitor_class_init (AwnMonitorClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  
  obj_class->get_property = awn_monitor_get_property;
  obj_class->set_property = awn_monitor_set_property;
  obj_class->constructed  = awn_monitor_constructed;

  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_pointer ("client",
                          "Client",
                          "AwnConfigClient",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_FORCE_MONITOR,
    g_param_spec_boolean ("monitor_force",
                          "Monitor Force",
                          "Force the monitor geometry",
                          FALSE,
                          G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
    PROP_WIDTH,
    g_param_spec_int ("monitor_width",
                      "Monitor Width",
                      "Monitor Width",
                      0, G_MAXINT, 1024,
                      G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
    PROP_HEIGHT,
    g_param_spec_int ("monitor_height",
                      "Monitor Height",
                      "Monitor Height",
                      0, G_MAXINT, 768,
                      G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
    PROP_OFFSET,
    g_param_spec_int ("monitor_offset",
                      "Monitor Offset",
                      "An optional offset (for displays > 1)",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_ALIGN,
    g_param_spec_float ("monitor_align",
                        "Monitor Align",
                        "The alignment of the panel on the monitor",
                        0.0, 1.0, 0.5,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

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

  priv->screen = gdk_screen_get_default ();
  priv->tag = 0;
}

AwnMonitor *
awn_monitor_new_from_config (AwnConfigClient *client)
{
  AwnMonitor *monitor = g_object_new (AWN_TYPE_MONITOR,
                                      "client", client,
                                      NULL);
  return monitor;
}

static void
on_screen_size_changed (GdkScreen *screen, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  if (!priv->force_monitor)
    return;

  monitor->width = gdk_screen_get_width (screen);
  monitor->height = gdk_screen_get_height (screen);

  g_signal_emit (monitor, _monitor_signals[GEOMETRY_CHANGED], 0);
}

static void 
awn_monitor_set_force_monitor (AwnMonitor *monitor,
                               gboolean    force_monitor)
{
  AwnMonitorPrivate *priv = monitor->priv;

  if (force_monitor)
  {
    if (priv->tag)
      g_signal_handler_disconnect (priv->screen, priv->tag);

    g_object_set (monitor,
                  "monitor-width", priv->config_width,
                  "monitor-height", priv->config_height,
                  NULL);
  }
  else
  {
    priv->tag = g_signal_connect (priv->screen, "size-changed",
                                  G_CALLBACK (on_screen_size_changed), monitor);

    monitor->width = gdk_screen_get_width (priv->screen);
    monitor->height = gdk_screen_get_height (priv->screen);

    g_signal_emit (monitor, _monitor_signals[GEOMETRY_CHANGED], 0);
  }
}


/* vim: set et ts=2 sts=2 sw=2 : */
