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
 
 /* awn-overlay-progress-circle.c */

#include "awn-overlay-progress-circle.h"

G_DEFINE_TYPE (AwnOverlayProgressCircle, awn_overlay_progress_circle, AWN_TYPE_OVERLAY_PROGRESS)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_PROGRESS_CIRCLE, AwnOverlayProgressCirclePrivate))

typedef struct _AwnOverlayProgressCirclePrivate AwnOverlayProgressCirclePrivate;

struct _AwnOverlayProgressCirclePrivate {
    int dummy;
};

static void
awn_overlay_progress_circle_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_progress_circle_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_progress_circle_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_progress_circle_parent_class)->dispose (object);
}

static void
awn_overlay_progress_circle_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_progress_circle_parent_class)->finalize (object);
}

static void
awn_overlay_progress_circle_class_init (AwnOverlayProgressCircleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnOverlayProgressCirclePrivate));

  object_class->get_property = awn_overlay_progress_circle_get_property;
  object_class->set_property = awn_overlay_progress_circle_set_property;
  object_class->dispose = awn_overlay_progress_circle_dispose;
  object_class->finalize = awn_overlay_progress_circle_finalize;
}

static void
awn_overlay_progress_circle_init (AwnOverlayProgressCircle *self)
{
}

AwnOverlayProgressCircle*
awn_overlay_progress_circle_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_PROGRESS_CIRCLE, NULL);
}

