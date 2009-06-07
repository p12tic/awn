/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 
 /* awn-overlay-throbber.c */

#include <math.h>

#include "awn-overlay-throbber.h"

G_DEFINE_TYPE (AwnOverlayThrobber, awn_overlay_throbber, AWN_TYPE_OVERLAY)

#define AWN_OVERLAY_THROBBER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_THROBBER, AwnOverlayThrobberPrivate))

typedef struct _AwnOverlayThrobberPrivate AwnOverlayThrobberPrivate;

struct _AwnOverlayThrobberPrivate 
{
  AwnIcon *   icon;
  gint counter;
  guint       timer_id;  
  guint       timeout;
  
};

enum
{
  PROP_0,
  PROP_ICON,
  PROP_TIMEOUT
};

static void 
_awn_overlay_throbber_render (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height);


static void
awn_overlay_throbber_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE(object);
  switch (property_id) 
  {
    case PROP_ICON:
      g_value_set_object (value,priv->icon);
      break;    
    case PROP_TIMEOUT:
      g_value_set_uint (value,priv->timeout);
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
    case PROP_ICON:
      if (priv->icon)
      {
        g_object_unref (priv->icon);
      }
      priv->icon = g_value_get_object (value);
      break;    
    case PROP_TIMEOUT:
      priv->timeout = g_value_get_uint (value);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_throbber_dispose (GObject *object)
{
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

  gtk_widget_queue_draw (GTK_WIDGET (priv->icon));

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
  GParamSpec   *pspec;    

  object_class->get_property = awn_overlay_throbber_get_property;
  object_class->set_property = awn_overlay_throbber_set_property;
  object_class->dispose = awn_overlay_throbber_dispose;
  object_class->finalize = awn_overlay_throbber_finalize;
  object_class->constructed = awn_overlay_throbber_constructed;
  
  AWN_OVERLAY_CLASS(klass)->render_overlay = _awn_overlay_throbber_render;  
  
  pspec = g_param_spec_object ("icon",
                               "Icon",
                               "Icon",
                               AWN_TYPE_ICON,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ICON, pspec);   

  pspec = g_param_spec_uint ("timeout",
                               "Timeout",
                               "Timeout",
                               50,
                               10000,
                               100,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TIMEOUT, pspec);   

  g_type_class_add_private (klass, sizeof (AwnOverlayThrobberPrivate));
  
}


static void
awn_overlay_throbber_init (AwnOverlayThrobber *self)
{
}

GtkWidget*
awn_overlay_throbber_new (AwnIcon * icon)
{
  return g_object_new (AWN_TYPE_OVERLAY_THROBBER, 
                       "icon", icon,
                       "active",FALSE,
                       NULL);
}

static void 
_awn_overlay_throbber_render (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height)
{
  AwnOverlayThrobberPrivate *priv = AWN_OVERLAY_THROBBER_GET_PRIVATE (overlay);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  /* TODO: translate to event->area
   * we'll paint to [0,0] - [1,1], so scale's needed
 
   */
  cairo_save (cr);
  cairo_scale(cr, width, height);

  const gdouble RADIUS = 0.0625;
  const gdouble DIST = 0.3;
  const gdouble OTHER = DIST * 0.707106781; /* sqrt(2)/2 */
  const gint COUNT = 8;
  const gint counter = priv->counter;

  cairo_translate(cr, 0.5, 0.5);
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