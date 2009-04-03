/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 *
 */

#include <glib/gstdio.h>
#include <string.h>
#include <gio/gio.h>

#include "awn-overlaid-icon.h"

G_DEFINE_TYPE (AwnOverlaidIcon, awn_overlaid_icon, AWN_TYPE_THEMED_ICON)

#define AWN_OVERLAID_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_OVERLAID_ICON, \
  AwnOverlaidIconPrivate))

struct _AwnOverlaidIconPrivate
{
  gpointer * padding;
};


/* Forwards */


/* GObject stuff */
static void
awn_overlaid_icon_dispose (GObject *object)
{
  AwnOverlaidIconPrivate *priv;

  g_return_if_fail (AWN_IS_OVERLAID_ICON (object));
  priv = AWN_OVERLAID_ICON (object)->priv;

  G_OBJECT_CLASS (awn_overlaid_icon_parent_class)->dispose (object);
}

static void
awn_overlaid_icon_class_init (AwnOverlaidIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
  
  obj_class->dispose = awn_overlaid_icon_dispose;
  
  g_type_class_add_private (obj_class, sizeof (AwnOverlaidIconPrivate));
}


static void
awn_overlaid_icon_init (AwnOverlaidIcon *icon)
{
  AwnOverlaidIconPrivate *priv;

  priv = icon->priv = AWN_OVERLAID_ICON_GET_PRIVATE (icon);

}

GtkWidget *
awn_overlaid_icon_new (void)

{
  GtkWidget *overlaid_icon = NULL;

  overlaid_icon = g_object_new (AWN_TYPE_OVERLAID_ICON, 
                              NULL);

  return overlaid_icon;
}




/*
 * Public functions
 */


