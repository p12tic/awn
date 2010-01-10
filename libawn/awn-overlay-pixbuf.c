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
 
/* awn-overlay-pixbuf.c */

/**
 * SECTION: awn-overlay-pixbuf
 * @short_description: An #AwnOverlay subclass to overlay a #GdkPixbuf
 * @see_also: #AwnOverlay, #AwnOverlayText, #AwnOverlayThemedIcon, #AwnOverlaidIcon
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Overlay a #GdkPixbuf on an #AwnOverlaidIcon.  You probably should _NOT_ be 
 * using this object.  You almost certainly should be using #AwnOverlayThemedIcon
 * or failing that #AwnOverlayPixbufFile.  Unecessary use of this object is grounds
 * to be pilloried.
 * 
 */

/**
 * AwnOverlayPixbuf:
 *
 * Overlay a #GdkPixbuf on an #AwnOverlaidIcon.  You probably should _NOT_ be 
 * using this object.  You almost certainly should be using #AwnOverlayThemedIcon
 * or failing that #AwnOverlayPixbufFile.  Unecessary use of this object is grounds
 * to be pilloried.
 * 
 */

#include <math.h>

#include "awn-overlay-pixbuf.h"

enum
{
  PROP_0,
  PROP_PIXBUF,
  PROP_SCALE,
  PROP_ALPHA
};

G_DEFINE_TYPE (AwnOverlayPixbuf, awn_overlay_pixbuf, AWN_TYPE_OVERLAY)

#define AWN_OVERLAY_PIXBUF_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_PIXBUF, AwnOverlayPixbufPrivate))

typedef struct _AwnOverlayPixbufPrivate AwnOverlayPixbufPrivate;

struct _AwnOverlayPixbufPrivate {
    GdkPixbuf * pixbuf;
    GdkPixbuf * scaled_pixbuf;  
    gdouble scale;  
    gdouble alpha;
};


static void
_awn_overlay_pixbuf_render (AwnOverlay* _overlay,
                            GtkWidget *widget,
                            cairo_t * cr,
                            gint icon_width,
                            gint icon_height);

static void
awn_overlay_pixbuf_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayPixbufPrivate *priv = AWN_OVERLAY_PIXBUF_GET_PRIVATE (object);  
  switch (property_id) 
  {
    case PROP_PIXBUF:
      g_value_set_object (value,priv->pixbuf);
      break;
    case PROP_SCALE:
      g_value_set_double (value,priv->scale);
      break;      
    case PROP_ALPHA:
      g_value_set_double (value,priv->alpha);
      break;            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_pixbuf_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayPixbufPrivate *priv = AWN_OVERLAY_PIXBUF_GET_PRIVATE (object);    
  switch (property_id) 
  {
    case PROP_PIXBUF:
      if (priv->pixbuf)
      {
        g_object_unref (priv->pixbuf);
      }
      priv->pixbuf = g_value_dup_object (value);
      if (priv->scaled_pixbuf)
      {
        g_object_unref (priv->scaled_pixbuf);
        priv->scaled_pixbuf = NULL;
      }
      break;    
    case PROP_SCALE:
      priv->scale = g_value_get_double (value);
      break;      
    case PROP_ALPHA:
      priv->alpha = g_value_get_double (value);
      break;            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_pixbuf_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_pixbuf_parent_class)->dispose (object);
}

static void
awn_overlay_pixbuf_finalize (GObject *object)
{
  AwnOverlayPixbufPrivate *priv = AWN_OVERLAY_PIXBUF_GET_PRIVATE (object);    
  if (priv->pixbuf)
  {
    g_object_unref (priv->pixbuf);
  }
  if (priv->scaled_pixbuf)
  {
    g_object_unref (priv->scaled_pixbuf);
  }  
  G_OBJECT_CLASS (awn_overlay_pixbuf_parent_class)->finalize (object);  
}

static void
awn_overlay_pixbuf_class_init (AwnOverlayPixbufClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = awn_overlay_pixbuf_get_property;
  object_class->set_property = awn_overlay_pixbuf_set_property;
  object_class->dispose = awn_overlay_pixbuf_dispose;
  object_class->finalize = awn_overlay_pixbuf_finalize;
 
  AWN_OVERLAY_CLASS(klass)->render = _awn_overlay_pixbuf_render;

/**
 * AwnOverlayPixbuf:pixbuf:
 *
 * A #GdkPixbuf to overlay.
 */        
  
  g_object_class_install_property (object_class,
    PROP_PIXBUF,
    g_param_spec_object ("pixbuf",
                         "Pixbuf",
                         "GdkPixbuf object",
                         GDK_TYPE_PIXBUF,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayPixbuf:scale:
 *
 * The desired size of the overlay scaled to the Icon.  Range 0.0...1.0. Default
 * value of 0.5
 */        
  
  g_object_class_install_property (object_class,
    PROP_SCALE,
    g_param_spec_double ("scale",
                         "scale",
                         "Scale",
                         0.0, 1.0, 0.5,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayPixbuf:alpha:
 *
 * An alpha value to apply to the Overlay.  Range of 0.0...1.0.  Default is 0.9.
 */        
  
  g_object_class_install_property (object_class,
    PROP_ALPHA,
    g_param_spec_double ("alpha",
                         "Alpha",
                         "Alpha",
                         0.0, 1.0, 0.9,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));
  
  g_type_class_add_private (klass, sizeof (AwnOverlayPixbufPrivate));  
}

static void
awn_overlay_pixbuf_init (AwnOverlayPixbuf *self)
{
  AwnOverlayPixbufPrivate *priv = AWN_OVERLAY_PIXBUF_GET_PRIVATE (self);
  
  priv->scaled_pixbuf = NULL;
}

/**
 * awn_overlay_pixbuf_new:
 *
 * Creates a new instance of #AwnOverlayPixbuf.
 * Returns: an instance of #AwnOverlayPixbuf.
 */
AwnOverlayPixbuf*
awn_overlay_pixbuf_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_PIXBUF, NULL);
}


AwnOverlayPixbuf*
awn_overlay_pixbuf_new_with_pixbuf (GdkPixbuf * pixbuf)
{
  AwnOverlayPixbuf* ret;
  if (!pixbuf)
  {
    ret = awn_overlay_pixbuf_new ();
  }
  else
  {
    ret = g_object_new (AWN_TYPE_OVERLAY_PIXBUF, 
                       "pixbuf", pixbuf,
                       NULL);
  }
  return ret;
}

static void
_awn_overlay_pixbuf_render (AwnOverlay* _overlay,
                            GtkWidget *widget,
                            cairo_t * cr,
                            gint icon_width,
                            gint icon_height)
{
  AwnOverlayPixbuf *overlay = AWN_OVERLAY_PIXBUF(_overlay);
  AwnOverlayPixbufPrivate *priv;
  gdouble pixbuf_width;
  gdouble pixbuf_height;
  gint scaled_width;
  gint scaled_height;
  AwnOverlayCoord coord;  
  
  priv = AWN_OVERLAY_PIXBUF_GET_PRIVATE (overlay);
  g_return_if_fail (priv->pixbuf);
 
  cairo_save (cr);
  pixbuf_width = gdk_pixbuf_get_width (priv->pixbuf);
  pixbuf_height = gdk_pixbuf_get_height (priv->pixbuf);

  scaled_width = lround (icon_width * priv->scale);  
  scaled_height = lround (pixbuf_height * (scaled_width / (gdouble) pixbuf_width) );

  
  if ( (scaled_height / (gdouble) icon_height) > priv->scale)
  {
    scaled_height = lround (icon_height * priv->scale);
    scaled_width = lround (pixbuf_width * (scaled_height / (gdouble) pixbuf_height));
  }
  
  /* Why do we do this?  Well the gdk pixbuf scaling gives a better result than
   the cairo scaling when dealing with a source pixbuf */
  if ( !priv->scaled_pixbuf || (scaled_width != gdk_pixbuf_get_width (priv->scaled_pixbuf) ) || 
        (scaled_height != gdk_pixbuf_get_height (priv->scaled_pixbuf) ) )
  {
    if (priv->scaled_pixbuf)
    {
      g_object_unref (priv->scaled_pixbuf);
      priv->scaled_pixbuf = NULL;
    }
    if ( (scaled_width == pixbuf_width) && (scaled_height==pixbuf_height))
    {
      g_object_ref (priv->pixbuf);
      priv->scaled_pixbuf = priv->pixbuf;
    }
    else
    {
      priv->scaled_pixbuf = gdk_pixbuf_scale_simple  (priv->pixbuf,
                                                    scaled_width,
                                                    scaled_height,
                                                    GDK_INTERP_BILINEAR);
    }
  }
    
  awn_overlay_move_to (AWN_OVERLAY (overlay), cr,
                       icon_width, icon_height, 
                       scaled_width, scaled_height, &coord);

  if (awn_overlay_get_use_source_op (AWN_OVERLAY (overlay)))
  {
    // there's some issue in gdk + cairo, using source operator itself
    // produces some artifacts
    //cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_rectangle (cr, coord.x, coord.y, scaled_width, scaled_height);
    cairo_fill (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  }

  gdk_cairo_set_source_pixbuf (cr,priv->scaled_pixbuf,coord.x,coord.y);
  cairo_paint_with_alpha (cr,priv->alpha);
  cairo_restore (cr);
}
