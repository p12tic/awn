/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */
 

/**
 * SECTION: awn-overlay-progress
 * @short_description: Progress overlay for use with #AwnIcon.
 * @see_also: #AwnEffects, #AwnOverlay, #AwnOverlayThemedIcon, #AwnOverlayThrobber,
 * #AwnOverlayPixbuf, #AwnOverlayPixbufFile, #AwnIcon
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Progress overlay used with #AwnIcon.  This is an abstract type.
 */

/**
 * AwnOverlayProgress:
 *
 * Progress overlay used with #AwnIcon.  This is an abstract type.
 */

 /* awn-overlay-progress.c */

#include "awn-overlay-progress.h"

G_DEFINE_ABSTRACT_TYPE (AwnOverlayProgress, awn_overlay_progress, AWN_TYPE_OVERLAY)

#define AWN_OVERLAY_PROGRESS_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_PROGRESS, AwnOverlayProgressPrivate))

typedef struct _AwnOverlayProgressPrivate AwnOverlayProgressPrivate;

struct _AwnOverlayProgressPrivate {
    gdouble percent_complete;
};

enum
{
  PROP_0,
  PROP_PERCENT_COMPLETE
};

static void
awn_overlay_progress_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayProgressPrivate * priv = AWN_OVERLAY_PROGRESS_GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_PERCENT_COMPLETE:
      g_value_set_double (value,priv->percent_complete);
      break;    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_progress_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayProgressPrivate * priv = AWN_OVERLAY_PROGRESS_GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_PERCENT_COMPLETE:
      priv->percent_complete = g_value_get_double (value);
      break;    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_progress_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_progress_parent_class)->dispose (object);
}

static void
awn_overlay_progress_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_progress_parent_class)->finalize (object);
}

static void
awn_overlay_progress_class_init (AwnOverlayProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = awn_overlay_progress_get_property;
  object_class->set_property = awn_overlay_progress_set_property;
  object_class->dispose = awn_overlay_progress_dispose;
  object_class->finalize = awn_overlay_progress_finalize;

/**
 * AwnOverlayProgress:percent-complete:
 *
 * A property of type double.  Set to the completion percentage for the overlay.
 */    
  
  g_object_class_install_property (object_class,
    PROP_PERCENT_COMPLETE,
    g_param_spec_double ("percent-complete",
                         "Percent Complete",
                         "Percent Complete",
                         0.0, 100.0, 0.0,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));
  
  g_type_class_add_private (klass, sizeof (AwnOverlayProgressPrivate));  
}

static void
awn_overlay_progress_init (AwnOverlayProgress *self)
{
}

/**
 * awn_overlay_progress_new:
 *
 * Creates a new instance of #AwnOverlayProgress.
 * Returns: an instance of #AwnOverlayProgress.
 */

AwnOverlayProgress*
awn_overlay_progress_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_PROGRESS, NULL);
}

