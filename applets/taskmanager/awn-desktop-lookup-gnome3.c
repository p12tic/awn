/*
 * Copyright (C) 2010 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 *
 * Authored by Rodney Cryderman <rcryderman@gmail.com>
 */
/* awn-desktop-lookup-gnome3.c */

#include "awn-desktop-lookup-gnome3.h"

G_DEFINE_TYPE (AwnDesktopLookupGnome3, awn_desktop_lookup_gnome3, AWN_TYPE_DESKTOP_LOOKUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_DESKTOP_LOOKUP_GNOME3, AwnDesktopLookupGnome3Private))

typedef struct _AwnDesktopLookupGnome3Private AwnDesktopLookupGnome3Private;

struct _AwnDesktopLookupGnome3Private {
    int dummy;
};

static void
awn_desktop_lookup_gnome3_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_gnome3_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_gnome3_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_gnome3_parent_class)->dispose (object);
}

static void
awn_desktop_lookup_gnome3_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_gnome3_parent_class)->finalize (object);
}

static void
awn_desktop_lookup_gnome3_class_init (AwnDesktopLookupGnome3Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnDesktopLookupGnome3Private));

  object_class->get_property = awn_desktop_lookup_gnome3_get_property;
  object_class->set_property = awn_desktop_lookup_gnome3_set_property;
  object_class->dispose = awn_desktop_lookup_gnome3_dispose;
  object_class->finalize = awn_desktop_lookup_gnome3_finalize;
}

static void
awn_desktop_lookup_gnome3_init (AwnDesktopLookupGnome3 *self)
{
}

AwnDesktopLookupGnome3*
awn_desktop_lookup_gnome3_new (void)
{
  return g_object_new (AWN_TYPE_DESKTOP_LOOKUP_GNOME3, NULL);
}

