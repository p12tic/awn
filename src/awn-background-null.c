/*
 *  Copyright (C) 2010 Alberto Aldegheri <albyrock87+dev@gmail.com>
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
 *  Author : Alberto Aldegheri <albyrock87+dev@gmail.com>
 *
 */

#include "config.h"

#include "awn-background-null.h"

G_DEFINE_TYPE (AwnBackgroundNull, awn_background_null, AWN_TYPE_BACKGROUND)

static void
awn_background_null_constructed (GObject *object)
{
  G_OBJECT_CLASS (awn_background_null_parent_class)->constructed (object);
}

static void
awn_background_null_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_background_null_parent_class)->dispose (object);
}

static void
awn_background_null_class_init (AwnBackgroundNullClass *klass)
{
  GObjectClass       *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructed  = awn_background_null_constructed;
  obj_class->dispose = awn_background_null_dispose;
}


static void
awn_background_null_init (AwnBackgroundNull *bg)
{

}

AwnBackground * 
awn_background_null_new (DesktopAgnosticConfigClient *client, AwnPanel *panel)
{
  AwnBackground *bg;

  bg = g_object_new (AWN_TYPE_BACKGROUND_NULL,
                     "client", client,
                     "panel", panel,
                     NULL);
  return bg;
}

/* vim: set et ts=2 sts=2 sw=2 : */
