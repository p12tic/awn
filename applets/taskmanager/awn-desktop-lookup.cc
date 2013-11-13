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
/* awn-desktop-lookup.c */

#include "awn-desktop-lookup.h"

G_DEFINE_TYPE (AwnDesktopLookup, awn_desktop_lookup, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_DESKTOP_LOOKUP, AwnDesktopLookupPrivate))

typedef struct _AwnDesktopLookupPrivate AwnDesktopLookupPrivate;

struct _AwnDesktopLookupPrivate {
  int dummy;
};

static void
awn_desktop_lookup_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->dispose (object);
}

static void
awn_desktop_lookup_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->finalize (object);
}

static void
awn_desktop_lookup_constructed (GObject *object)
{
  if ( G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->constructed (object);
  }
}

static void
awn_desktop_lookup_class_init (AwnDesktopLookupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnDesktopLookupPrivate));

  object_class->get_property = awn_desktop_lookup_get_property;
  object_class->set_property = awn_desktop_lookup_set_property;
  object_class->dispose = awn_desktop_lookup_dispose;
  object_class->finalize = awn_desktop_lookup_finalize;
  object_class->constructed = awn_desktop_lookup_constructed;
}

static void
awn_desktop_lookup_init (AwnDesktopLookup *self)
{
//  AwnDesktopLookupPrivate * priv = GET_PRIVATE(self);
}

AwnDesktopLookup*
awn_desktop_lookup_new (void)
{
  return g_object_new (AWN_TYPE_DESKTOP_LOOKUP, NULL);
}

/*
 If other attempts to find a desktop file fail then this function will
 typically get called which use tables of data to match problem apps to 
 desktop file names (refer to util.c)
 */
gchar *
awn_desktop_lookup_special_case (gchar * cmd, 
                                 gchar *res_name, 
                                 gchar * class_name,
                                 gchar * wm_icon_name,
                                 gchar * wm_name,
                                 gchar * window_role)
{
  gchar * result = NULL;
  return result;
}

gchar *
awn_desktop_lookup_search_for_desktop (gulong xid)
{
  gchar * found_desktop = NULL;
  return found_desktop;
}
