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

#include "awn-config-bridge.h"

G_DEFINE_TYPE (AwnConfigBridge, awn_config_bridge, G_TYPE_OBJECT)

#define AWN_CONFIG_BRIDGE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_CONFIG_BRIDGE, AwnConfigBridgePrivate))

struct _AwnConfigBridgePrivate
{
  GList *binds;
};

typedef struct
{
  GObject *object;
  gchar   *property_name;

} AwnConfigBind;

static void
awn_config_bridge_dispose (GObject *object)
{
  AwnConfigBridgePrivate *priv = AWN_CONFIG_BRIDGE_GET_PRIVATE (object);
  GList                   *b;

  for (b = priv->binds; b; b = b->next)
  {
    AwnConfigBind *bind = b->data;

    g_free (bind->property_name);
    g_slice_free (AwnConfigBind, bind);
  }
  g_list_free (priv->binds);

  G_OBJECT_CLASS (awn_config_bridge_parent_class)->dispose (object);
}

static void
awn_config_bridge_class_init (AwnConfigBridgeClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->dispose = awn_config_bridge_dispose;

  g_type_class_add_private (obj_class, sizeof (AwnConfigBridgePrivate));
}


static void
awn_config_bridge_init (AwnConfigBridge *bridge)
{
  AwnConfigBridgePrivate *priv;

  priv = bridge->priv = AWN_CONFIG_BRIDGE_GET_PRIVATE (bridge);

  priv->binds = NULL;
}

AwnConfigBridge * 
awn_config_bridge_get_default (void)
{
  AwnConfigBridge *bridge = NULL;

  if (!bridge)
    bridge = g_object_new (AWN_TYPE_CONFIG_BRIDGE, NULL);

  return bridge;
}

/*
 * Binding function for most types 
 */
void  
awn_config_bridge_bind (AwnConfigBridge *bridge,
                        AwnConfigClient *client,
                        const gchar     *group,
                        const gchar     *key,
                        GObject         *object,
                        const gchar     *property_name)
{
  AwnConfigBridgePrivate *priv;
  AwnConfigBind          *bind;
  GParamSpec             *spec;

  g_return_if_fail (AWN_IS_CONFIG_BRIDGE (bridge));
  g_return_if_fail (client);
  g_return_if_fail (group);
  g_return_if_fail (key);
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name);
  priv = bridge->priv;

  bind = g_slice_new0 (AwnConfigBind);
  bind->object = object;
  bind->property_name = g_strdup (property_name);

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                       property_name);
  
  switch (G_PARAM_SPEC_VALUE_TYPE (spec))
  {
    case G_TYPE_BOOLEAN:
      g_print ("boolean: %s\n", property_name);
      break;
    
    case G_TYPE_FLOAT:
    case G_TYPE_DOUBLE:
      g_print ("Float/double: %s\n", property_name);
      break;
    
    case G_TYPE_INT:
      g_print ("Int: %s\n", property_name);
      break;

    case G_TYPE_STRING:
      g_print ("String: %s\n", property_name);
      break;

    default:
      g_print ("Unable to handle type for %s\n", property_name);
      g_free (bind->property_name);
      g_slice_free (AwnConfigBind, bind);
      return;
  }

  priv->binds = g_list_append (priv->binds, bind);
}
