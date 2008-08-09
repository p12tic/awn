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

static void
awn_config_bridge_class_init (AwnConfigBridgeClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);


}


static void
awn_config_bridge_init (AwnConfigBridge *bg)
{
  ;
}

AwnConfigBridge * 
awn_bridge_get_default (AwnConfigClient *client)
{
  
}
