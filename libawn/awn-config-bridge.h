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
*/

#ifndef	_AWN_CONFIG_BRIDGE_H
#define	_AWN_CONFIG_BRIDGE_H

#include <glib.h>
#include <glib-object.h>

#include <libawn/awn-config-client.h>

G_BEGIN_DECLS

#define AWN_TYPE_CONFIG_BRIDGE (awn_config_bridge_get_type())

#define AWN_CONFIG_BRIDGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_CONFIG_BRIDGE, \
  AwnConfigBridge))

#define AWN_CONFIG_BRIDGE_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_TYPE_CONFIG_BRIDGE, \
  AwnConfigBridgeClass))

#define AWN_IS_CONFIG_BRIDGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_CONFIG_BRIDGE))

#define AWN_IS_CONFIG_BRIDGE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
  AWN_TYPE_CONFIG_BRIDGE))

#define AWN_CONFIG_BRIDGE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
  AWN_TYPE_CONFIG_BRIDGE, AwnConfigBridgeClass))

typedef struct _AwnConfigBridge AwnConfigBridge;
typedef struct _AwnConfigBridgeClass AwnConfigBridgeClass;
typedef struct _AwnConfigBridgePrivate AwnConfigBridgePrivate;

struct _AwnConfigBridge 
{
  GObject  parent;

  AwnConfigBridgePrivate *priv;
};

struct _AwnConfigBridgeClass 
{
  GObjectClass parent_class;
};

GType             awn_config_bridge_get_type (void) G_GNUC_CONST;

AwnConfigBridge * awn_config_bridge_get_default (void);

void              awn_config_bridge_bind        (AwnConfigBridge *bridge,
                                                 AwnConfigClient *client,
                                                 const gchar     *group,
                                                 const gchar     *key,
                                                 GObject         *object,
                                                 const gchar     *prop_name);

void              awn_config_bridge_bind_list   (AwnConfigBridge   *bridge,
                                                 AwnConfigClient   *client,
                                                 const gchar       *group,
                                                 const gchar       *key,
                                                 AwnConfigListType  list_type,
                                                 GObject           *object,
                                                 const gchar       *prop_name);


G_END_DECLS

#endif /* _AWN_CONFIG_BRIDGE_H */

