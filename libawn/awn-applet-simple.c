/*
 * Copyright (c) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "awn-applet-simple.h"

#include "awn-icon.h"
#include "awn-themed-icon.h"

G_DEFINE_TYPE (AwnAppletSimple, awn_applet_simple, AWN_TYPE_APPLET)

#define AWN_APPLET_SIMPLE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
    AWN_TYPE_APPLET_SIMPLE, \
    AwnAppletSimplePrivate))

struct _AwnAppletSimplePrivate
{
  GtkWidget *icon;
};

/* GObject stuff */
static void
awn_applet_simple_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_applet_simple_parent_class)->dispose (object);
}

static void
awn_applet_simple_class_init (AwnAppletSimpleClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
    
  obj_class->dispose = awn_applet_simple_dispose;
  
  g_type_class_add_private (obj_class, sizeof (AwnAppletSimplePrivate));
}
static void
awn_applet_simple_init (AwnAppletSimple *simple)
{
  AwnAppletSimplePrivate *priv;
  
  priv = simple->priv = AWN_APPLET_SIMPLE_GET_PRIVATE(simple);
}

/**
 * awn_applet_simple_new:
 * @uid: The unique identifier of the instance of the applet on the dock.
 * @orient: The orientation of the applet - see #AwnOrientation.
 * @height: The height of the applet.
 *
 * Creates a new #AwnAppletSimple object.  This applet will have awn-effects
 * effects applied to its icon automatically if awn_applet_simple_set_icon() or
 * awn_applet_simple_set_temp_icon() are used to specify the applet icon.
 * Returns: a new instance of an applet.
 */
GtkWidget*
awn_applet_simple_new (const gchar *uid, gint orient, gint size)
{
  AwnAppletSimple *simple;

  simple = g_object_new(AWN_TYPE_APPLET_SIMPLE,
                        "uid", uid,
                        "orient", orient,
                        "size", size,
                        NULL);

  return GTK_WIDGET(simple);
}
