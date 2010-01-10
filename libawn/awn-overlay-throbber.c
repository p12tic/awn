/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 
 /* awn-overlay-throbber.c */

#include <math.h>

#include "awn-overlay-throbber.h"

/**
 * SECTION: awn-overlay-throbber
 * @short_description: Throbber overlay.
 * @see_also: #AwnOverlaidIcon, #AwnOverlay, #AwnOverlayIcon, #AwnOverlayText,
 * #AwnOverlayPixbuf, #AwnOverlayPixbufFile
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Throbber overlay used with #AwnOverlaidIcon.
 */

/**
 * AwnOverlayThrobber:
 *
 * Throbber overlay used with #AwnOverlaidIcon.
 */

G_DEFINE_TYPE (AwnOverlayThrobber, awn_overlay_throbber, AWN_TYPE_OVERLAY)

#define AWN_OVERLAY_THROBBER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_THROBBER, AwnOverlayThrobberPrivate))

typedef struct _AwnOverlayThrobberPrivate AwnOverlayThrobberPrivate;

struct _AwnOverlayThrobberPrivate 
{
  gint        counter;
  guint       timer_id;  
  guint       timeout;
  gdouble     scale;  
};

enum
{
  PROP_0,
  PROP_TIMEOUT,
  PROP_SCALE
};

static void 
_awn_overlay_throbber_render (AwnOverlay* overlay,
                              GtkWidget *widget,
                              cairo_t * cr,
                              gint icon_width,
                              gint icon_height);

static void
awn_overlay_throbber_get_property (GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE(object);
  switch (property_id) 
  {
    case PROP_TIMEOUT:
      g_value_set_uint (value,priv->timeout);
      break;
    case PROP_SCALE:
      g_value_set_double (value,priv->scale);
      break;            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_throbber_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE(object);  
  switch (property_id) {
    case PROP_TIMEOUT:
      priv->timeout = g_value_get_uint (value);
      break;
    case PROP_SCALE:
      priv->scale = g_value_get_double (value);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_throbber_dispose (GObject *object)
{
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE(object);
  
  if (priv->timer_id)
  {
    g_source_remove (priv->timer_id);
    priv->timer_id = 0;
  }

  G_OBJECT_CLASS (awn_overlay_throbber_parent_class)->dispose (object);
}

static void
awn_overlay_throbber_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_throbber_parent_class)->finalize (object);
}

static gboolean
_awn_overlay_throbber_timeout (gpointer overlay)
{
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE(overlay);

  priv->counter = (priv->counter - 1) % 8 + 8;
  g_object_notify (overlay, "active");
  return TRUE;
}

static void
_awn_overlay_throbber_active_changed (GObject    *pspec,
                                      GParamSpec *gobject,
                                      gpointer * throbber)
{
  gboolean active_val;
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE (throbber);
  
  g_object_get (G_OBJECT(throbber),
                "active", &active_val,
                NULL);
  if (active_val)
  {
    if (!priv->timer_id)
    {
      priv->timer_id = g_timeout_add(priv->timeout, _awn_overlay_throbber_timeout, throbber);      
    }
  }
  else
  {
    if (priv->timer_id)
    {
      g_source_remove(priv->timer_id);
      priv->timer_id = 0;
    }
  }     
}

static void
_awn_overlay_throbber_timeout_changed (GObject    *pspec,
                                      GParamSpec *gobject,
                                      gpointer * throbber)
{
  gboolean active_val;

  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE (throbber);
  
  g_object_get (G_OBJECT(throbber),
                "active", &active_val,
                NULL);
  if (active_val)
  {
    if (priv->timer_id)
    {
      g_source_remove(priv->timer_id);
    }
    priv->timer_id = g_timeout_add(priv->timeout, _awn_overlay_throbber_timeout, throbber);          
  }
}

static void
awn_overlay_throbber_constructed (GObject * object)
{  

  if (G_OBJECT_CLASS (awn_overlay_throbber_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_overlay_throbber_parent_class)->constructed (object);
  }
    
  g_signal_connect (object, "notify::active",
                  G_CALLBACK(_awn_overlay_throbber_active_changed),
                  object);
  g_signal_connect (object, "notify::timeout",
                  G_CALLBACK(_awn_overlay_throbber_timeout_changed),
                  object);  
}

static void
awn_overlay_throbber_class_init (AwnOverlayThrobberClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = awn_overlay_throbber_get_property;
  object_class->set_property = awn_overlay_throbber_set_property;
  object_class->dispose = awn_overlay_throbber_dispose;
  object_class->finalize = awn_overlay_throbber_finalize;
  object_class->constructed = awn_overlay_throbber_constructed;
  
  AWN_OVERLAY_CLASS(klass)->render = _awn_overlay_throbber_render;

/**
 * AwnOverlayThrobber:timeout:
 *
 * The time in milliseconds between throbber updates.
 */

  g_object_class_install_property (object_class,
    PROP_TIMEOUT,
    g_param_spec_uint ("timeout",
                       "Timeout",
                       "Timeout",
                       50, 10000, 100,
                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                       G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayThrobber:scale:
 *
 * Determines the size of the #AwnOverlayThrobber scaled against the dimensions
 * of the #AwnIcon.   A scale of 0.5 would result in an overlay that covers 
 * 25% of the Icon.
 */

  g_object_class_install_property (object_class,
    PROP_SCALE,
    g_param_spec_double ("scale",
                         "scale",
                         "Scale",
                         0.01, 1.0, 0.6,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (AwnOverlayThrobberPrivate));  
}


static void
awn_overlay_throbber_init (AwnOverlayThrobber *self)
{
}



/**
 * awn_overlay_throbber_new:
 * @icon: The #AwnOverlaidIcon onto which the #AwnOverlayThrobber is rendered.
 *
 * Creates a new instance of #AwnOverlayThrobber.
 * Returns: an instance of #AwnOverlayThrobber.
 */
GtkWidget*
awn_overlay_throbber_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_THROBBER,
                       "active", FALSE,
                       NULL);
}

static void 
_awn_overlay_throbber_render (AwnOverlay* overlay,
                              GtkWidget *widget,
                              cairo_t * cr,
                              gint icon_width,
                              gint icon_height)
{
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE (overlay);

  const gdouble RADIUS = 0.0625;
  const gdouble DIST = 0.3;
  const gdouble OTHER = DIST * 0.707106781; /* sqrt(2)/2 */
  const gint COUNT = 8;
  const gint counter = priv->counter;
  gdouble scale;
  AwnOverlayCoord coord;
  gdouble scaled_height;
  gdouble scaled_width;

  g_object_get (overlay,
                "scale", &scale,
                NULL);
  
  scaled_height = icon_height * scale;
  scaled_width = icon_width * scale;
  cairo_save (cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_save (cr);
  awn_overlay_move_to (overlay, 
                       cr, 
                       icon_width,
                       icon_height,
                       scaled_width,
                       scaled_height,
                       &coord);
  cairo_restore(cr);
  cairo_translate(cr, coord.x  , coord.y  );
  cairo_scale(cr, scaled_width, scaled_height);
  
  cairo_translate(cr, 0.50, 0.50 );
  cairo_scale(cr, 1, -1);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+0) % COUNT) / (float)COUNT);
  cairo_arc(cr, 0, DIST, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+1) % COUNT) / (float)COUNT);
  cairo_arc(cr, OTHER, OTHER, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+2) % COUNT) / (float)COUNT);
  cairo_arc(cr, DIST, 0, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+3) % COUNT) / (float)COUNT);
  cairo_arc(cr, OTHER, -OTHER, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+4) % COUNT) / (float)COUNT);
  cairo_arc(cr, 0, -DIST, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+5) % COUNT) / (float)COUNT);
  cairo_arc(cr, -OTHER, -OTHER, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+6) % COUNT) / (float)COUNT);
  cairo_arc(cr, -DIST, 0, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_set_source_rgba(cr, 1, 1, 1, ((counter+7) % COUNT) / (float)COUNT);
  cairo_arc(cr, -OTHER, OTHER, RADIUS, 0, 2*M_PI);
  cairo_fill(cr);

  cairo_restore (cr);
}
