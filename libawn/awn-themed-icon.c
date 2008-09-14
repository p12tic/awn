/*
 * Copyright (C) 2008 Neil Jagdish Patel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */
#include "awn-themed-icon.h"

G_DEFINE_TYPE (AwnThemedIcon, awn_themed_icon, AWN_TYPE_ICON);

#define AWN_THEMED_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_THEMED_ICON, \
  AwnThemedIconPrivate))

struct _AwnThemedIconPrivate
{
  gint null;
};

/* GObject stuff */
static void
awn_themed_icon_dispose (GObject *object)
{
  AwnThemedIconPrivate *priv;

  g_return_if_fail (AWN_IS_THEMED_ICON (object));
  priv = AWN_THEMED_ICON (object)->priv;

  G_OBJECT_CLASS (awn_themed_icon_parent_class)->dispose (object);
}

static void
awn_themed_icon_class_init (AwnThemedIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  
  obj_class->dispose = awn_themed_icon_dispose;

  g_type_class_add_private (obj_class, sizeof (AwnThemedIconPrivate));
}

static void
awn_themed_icon_init (AwnThemedIcon *icon)
{
  AwnThemedIconPrivate *priv;

  priv = icon->priv = AWN_THEMED_ICON_GET_PRIVATE (icon);
}

GtkWidget *
awn_themed_icon_new (void)

{
  GtkWidget *themed_icon = NULL;

  themed_icon = g_object_new (AWN_TYPE_THEMED_ICON, 
                              NULL);

  return themed_icon;
}
