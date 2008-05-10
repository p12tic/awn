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
#include <libawn/awn-config-client.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include "awn-monitor.h"
#include "awn-x.h"

G_DEFINE_TYPE (AwnMonitor, awn_monitor, G_TYPE_OBJECT) 

#define AWN_MONITOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_MONITOR, AwnMonitorPrivate))

#define FORCE_MONITOR   "force_monitor"
#define MONITOR_WIDTH   "monitor_width"
#define MONITOR_HEIGHT  "monitor_height"
#define MONITOR_XOFFSET "monitor_xoffset"
#define MONITOR_XALIGN  "monitor_xalign"

struct _AwnMonitorPrivate
{
  AwnConfigClient *client;

  /* Monitor Geometry stuff */
  gboolean force_monitor;

  gint forced_width;
  gint forced_height;
  gint forced_xoffset;
};

enum 
{
  PROP_0,

  PROP_CLIENT,
};

enum
{
  GEOMETRY_CHANGED,

  LAST_SIGNAL
};
static guint _monitor_signals[LAST_SIGNAL] = { 0 };

/* Main function that keeps things in check */
static void
update (AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_MONITOR (monitor));
  priv = monitor->priv;

  if (priv->force_monitor)
  {
    monitor->width = priv->forced_width;
    monitor->height = priv->forced_height;
    monitor->xoffset = priv->forced_xoffset;
  }
  else
  {
    GdkScreen *screen = gdk_screen_get_default ();
    monitor->width = gdk_screen_get_width (screen);
    monitor->height = gdk_screen_get_height (screen);
    monitor->xoffset = 0;
  }

  g_debug ("Monitor changed: width=%d height=%d xoffset=%d xalign=%f\n", 
           monitor->width, monitor->height, monitor->xoffset, monitor->xalign);

  g_signal_emit (monitor, _monitor_signals[GEOMETRY_CHANGED], 0);
}

/* Callbacks */
static void
on_screen_size_changed (GdkScreen *screen, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  if (priv->force_monitor)
    return;
  
  update (monitor);
}

static void
on_force_changed (AwnConfigClientNotifyEntry *entry, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  priv->force_monitor = entry->value.bool_val;

  update (monitor);
}

static void
on_width_changed (AwnConfigClientNotifyEntry *entry, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  priv->forced_width = entry->value.int_val;

  update (monitor);
}

static void
on_height_changed (AwnConfigClientNotifyEntry *entry, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  priv->forced_height = entry->value.int_val;

  update (monitor);
}

static void
on_xoffset_changed (AwnConfigClientNotifyEntry *entry, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  priv->forced_xoffset = entry->value.int_val;

  update (monitor);
}

static void
on_xalign_changed (AwnConfigClientNotifyEntry *entry, AwnMonitor *monitor)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  priv = monitor->priv;

  monitor->xalign = entry->value.int_val;

  g_signal_emit (monitor, _monitor_signals[GEOMETRY_CHANGED], 0);
}

static void
awn_monitor_set_client (AwnMonitor *monitor, AwnConfigClient *client)
{
  AwnMonitorPrivate *priv;
  GdkScreen *screen = gdk_screen_get_default ();

  g_return_if_fail (AWN_IS_MONITOR (monitor));
  g_return_if_fail (client);
  priv = monitor->priv;

  awn_config_client_ensure_group (client, AWN_CONFIG_CLIENT_DEFAULT_GROUP);

  /* Lets grab the initial values */  
  priv->force_monitor = awn_config_client_get_bool (client,
                                               AWN_CONFIG_CLIENT_DEFAULT_GROUP,
                                                    FORCE_MONITOR, NULL);
  priv->forced_width = awn_config_client_get_int (client,
                                               AWN_CONFIG_CLIENT_DEFAULT_GROUP,
                                                   MONITOR_WIDTH, NULL);
  priv->forced_height = awn_config_client_get_int (client,
                                               AWN_CONFIG_CLIENT_DEFAULT_GROUP,
                                                   MONITOR_HEIGHT, NULL);
  priv->forced_xoffset = awn_config_client_get_int (client,
                                               AWN_CONFIG_CLIENT_DEFAULT_GROUP,
                                                   MONITOR_XOFFSET, NULL);
  monitor->xalign = awn_config_client_get_float (client,
                                                AWN_CONFIG_CLIENT_DEFAULT_GROUP,
                                                 MONITOR_XALIGN, NULL);
  /* 
   * Hook up to the relevent config-change signals, and the gdk-screen
   * signals
   */
  g_signal_connect (screen, "size-changed",
                    G_CALLBACK (on_screen_size_changed), monitor);
  awn_config_client_notify_add (client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, 
                                FORCE_MONITOR,
                                (AwnConfigClientNotifyFunc)on_force_changed,
                                monitor);
  awn_config_client_notify_add (client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, 
                                MONITOR_WIDTH,
                                (AwnConfigClientNotifyFunc)on_width_changed,
                                monitor);
  awn_config_client_notify_add (client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, 
                                MONITOR_HEIGHT,
                                (AwnConfigClientNotifyFunc)on_height_changed,
                                monitor);
  awn_config_client_notify_add (client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, 
                                MONITOR_XOFFSET,
                                (AwnConfigClientNotifyFunc)on_xoffset_changed,
                                monitor);
  awn_config_client_notify_add (client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, 
                                MONITOR_XALIGN,
                                (AwnConfigClientNotifyFunc)on_xalign_changed,
                                monitor);

  update (monitor);
}

/* GObject stuff */
static void
awn_monitor_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (object));
  priv = AWN_MONITOR (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      g_value_set_pointer (value, priv->client);
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
  AwnMonitorPrivate *priv;

  g_return_if_fail (AWN_IS_MONITOR (object));
  priv = AWN_MONITOR (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      awn_monitor_set_client (AWN_MONITOR (object),
                              g_value_get_pointer (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_monitor_class_init (AwnMonitorClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->get_property = awn_monitor_get_property;
  obj_class->set_property = awn_monitor_set_property;

  /* Add properties to the class */
 g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_pointer ("client",
                          "Client",
                          "AwnConfigClient",
                          G_PARAM_READWRITE));
    
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
awn_monitor_new_from_config (AwnConfigClient *client)
{
  AwnMonitor *monitor = g_object_new (AWN_TYPE_MONITOR,
                                      "client", client,
                                      NULL);
  return monitor;
}
/* vim: set et ts=2 sts=2 sw=2 : */

