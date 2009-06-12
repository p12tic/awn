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

/* awn-overlay-pixbuf-file.c */

#include "awn-overlay-pixbuf-file.h"

G_DEFINE_TYPE (AwnOverlayPixbufFile, awn_overlay_pixbuf_file, AWN_TYPE_OVERLAY_PIXBUF)

#define AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_PIXBUF_FILE, AwnOverlayPixbufFilePrivate))

typedef struct _AwnOverlayPixbufFilePrivate AwnOverlayPixbufFilePrivate;

struct _AwnOverlayPixbufFilePrivate {
    int dummy;
};

static void
awn_overlay_pixbuf_file_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_pixbuf_file_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_pixbuf_file_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_pixbuf_file_parent_class)->dispose (object);
}

static void
awn_overlay_pixbuf_file_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_pixbuf_file_parent_class)->finalize (object);
}

static void
awn_overlay_pixbuf_file_class_init (AwnOverlayPixbufFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnOverlayPixbufFilePrivate));

  object_class->get_property = awn_overlay_pixbuf_file_get_property;
  object_class->set_property = awn_overlay_pixbuf_file_set_property;
  object_class->dispose = awn_overlay_pixbuf_file_dispose;
  object_class->finalize = awn_overlay_pixbuf_file_finalize;
}

static void
awn_overlay_pixbuf_file_init (AwnOverlayPixbufFile *self)
{
}

AwnOverlayPixbufFile*
awn_overlay_pixbuf_file_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_PIXBUF_FILE, NULL);
}

