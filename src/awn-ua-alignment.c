/*
 *  Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 *
 */

/* awn-ua-alignment.c */

#include "awn-ua-alignment.h"

G_DEFINE_TYPE (AwnUAAlignment, awn_ua_alignment, AWN_TYPE_UA_ALIGNMENT)

#define AWN_UA_ALIGNMENT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_UA_ALIGNMENT, AwnUAAlignmentPrivate))

typedef struct _AwnUAAlignmentPrivate AwnUAAlignmentPrivate;

struct _AwnUAAlignmentPrivate {
    GtkWidget * socket;
};

enum
{
  PROP_0
};

static void
awn_ua_alignment_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnUAAlignmentPrivate * priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (object);
  
  switch (property_id) 
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_ua_alignment_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnUAAlignmentPrivate * priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (object);

  switch (property_id) 
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_ua_alignment_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_ua_alignment_parent_class)->dispose (object);
}

static void
awn_ua_alignment_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_ua_alignment_parent_class)->finalize (object);
}

static void
awn_ua_alignment_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (awn_ua_alignment_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_ua_alignment_parent_class)->constructed (object);
  }
  
  
}

static void
awn_ua_alignment_class_init (AwnUAAlignmentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnUAAlignmentPrivate));

  object_class->get_property = awn_ua_alignment_get_property;
  object_class->set_property = awn_ua_alignment_set_property;
  object_class->dispose = awn_ua_alignment_dispose;
  object_class->finalize = awn_ua_alignment_finalize;
  object_class->constructed = awn_ua_alignment_constructed;  
}

static void
awn_ua_alignment_init (AwnUAAlignment *self)
{
  AwnUAAlignmentPrivate *priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (self); 

  priv->socket = gtk_socket_new ();  
}

AwnUAAlignment*
awn_ua_alignment_new (void)
{
  return g_object_new (AWN_TYPE_UA_ALIGNMENT,
                       NULL);
}

