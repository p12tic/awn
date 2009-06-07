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
 
/* awn-overlay-icon.c */

/*
 FIXME
 hook up to theme changes... clear pixbufs when this happens.
 
 the caching of pixbufs may be a bit excessive if scale,width,height are going crazy.
 
 */

#include "awn-overlay-icon.h"

G_DEFINE_TYPE (AwnOverlayIcon, awn_overlay_icon, AWN_TYPE_OVERLAY)

 #define AWN_OVERLAY_ICON_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_ICON, AwnOverlayIconPrivate))

typedef struct _AwnOverlayIconPrivate AwnOverlayIconPrivate;

struct _AwnOverlayIconPrivate 
{
    gdouble alpha;
    gchar * icon_name;
    gchar * icon_state;
    gdouble scale;
  
    GHashTable *pixbufs;
};

enum
{
  PROP_0,
  PROP_ALPHA,
  PROP_ICON_NAME,
  PROP_ICON_STATE,
  PROP_SCALE
};

static void 
_awn_overlay_icon_render ( AwnOverlay* _overlay,
                               AwnThemedIcon * icon,
                               cairo_t * cr,                                 
                               gint width,
                               gint height);

static void
awn_overlay_icon_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayIconPrivate * priv;
  priv = AWN_OVERLAY_ICON_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_ALPHA:
      g_value_set_double (value,priv->alpha);
      break;
    case PROP_SCALE:
      g_value_set_double (value,priv->scale);
      break;      
    case PROP_ICON_NAME:
      g_value_set_string (value,priv->icon_name);
      break;
    case PROP_ICON_STATE:
      g_value_set_string (value,priv->icon_state);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_icon_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayIconPrivate * priv;
  priv = AWN_OVERLAY_ICON_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_ALPHA:
      priv->alpha = g_value_get_double (value);
      break;
    case PROP_SCALE:
      priv->scale = g_value_get_double (value);
      break;      
    case PROP_ICON_NAME:
      g_free(priv->icon_name);
      priv->icon_name = g_value_dup_string (value);
      break;
    case PROP_ICON_STATE:
      g_free(priv->icon_state);
      priv->icon_state = g_value_dup_string (value);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_icon_dispose (GObject *object)
{
  AwnOverlayIconPrivate * priv;
  priv = AWN_OVERLAY_ICON_GET_PRIVATE (object);
  
  G_OBJECT_CLASS (awn_overlay_icon_parent_class)->dispose (object);
  
  g_hash_table_destroy (priv->pixbufs);
}

static void
awn_overlay_icon_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_icon_parent_class)->finalize (object);
}

static void
awn_overlay_icon_class_init (AwnOverlayIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;      

  object_class->get_property = awn_overlay_icon_get_property;
  object_class->set_property = awn_overlay_icon_set_property;
  object_class->dispose = awn_overlay_icon_dispose;
  object_class->finalize = awn_overlay_icon_finalize;
  
  AWN_OVERLAY_CLASS(klass)->render_overlay = _awn_overlay_icon_render;
  
  pspec = g_param_spec_double ("alpha",
                               "alpha",
                               "Alpha",
                               0.0,
                               1.0,
                               0.9,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ALPHA, pspec);   

  pspec = g_param_spec_double ("scale",
                               "scale",
                               "Scale",
                               0.0,
                               1.0,
                               0.3,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_SCALE, pspec);   
  
  pspec = g_param_spec_string ("icon-name",
                               "Icon name",
                               "Icon gtk theme name",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ICON_NAME, pspec);   

  pspec = g_param_spec_string ("icon-state",
                               "Icon state",
                               "Icon state",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ICON_STATE, pspec);   
  
  g_type_class_add_private (klass, sizeof (AwnOverlayIconPrivate));  
}

static void
awn_overlay_icon_init (AwnOverlayIcon *self)
{
  AwnOverlayIconPrivate * priv;
  priv = AWN_OVERLAY_ICON_GET_PRIVATE (self);

  priv->pixbufs = g_hash_table_new_full (g_str_hash,g_str_equal, g_free, g_object_unref);
}

AwnOverlayIcon*
awn_overlay_icon_new (AwnThemedIcon * icon, const gchar * icon_name, const gchar * state)
{
  AwnOverlayIcon * ret;
  gchar * created_state = NULL;
  
  g_return_val_if_fail (icon_name,NULL);
  g_return_val_if_fail (icon,NULL);
  
  if (!state)
  {
    created_state = g_strdup_printf ("::invisible::__AWN_OVERLAY_ICON_STATE_NAME_FOR_%s",icon_name);
    state = created_state;
  }
  
  ret = g_object_new (AWN_TYPE_OVERLAY_ICON, 
                      "icon-name",icon_name,
                      "icon-state",state,
                      "gravity",GDK_GRAVITY_SOUTH_EAST,
                      NULL);
/* some of this probably should be done in constructed() FIXME*/
  awn_themed_icon_set_info_append (icon,icon_name,state);
    
  if (created_state)
  {
    g_free (created_state);
  }
  return ret;
}

static void 
_awn_overlay_icon_render ( AwnOverlay* _overlay,
                               AwnThemedIcon * icon,
                               cairo_t * cr,                                 
                               gint width,
                               gint height)
{
  AwnOverlayIcon *overlay = AWN_OVERLAY_ICON(_overlay);
  AwnOverlayIconPrivate *priv;
  AwnOverlayCoord coord;
  GdkPixbuf * pixbuf = NULL;
  
  priv =  AWN_OVERLAY_ICON_GET_PRIVATE (overlay); 
  
  gchar * key = g_strdup_printf ("%dx%d@%lf",width,height,priv->scale);
 
  pixbuf = g_hash_table_lookup (priv->pixbufs,key);
  
  if (! pixbuf)
  {
    pixbuf = awn_themed_icon_get_icon_at_size (icon, 
                                                   width>height?width:height * priv->scale,
                                                   priv->icon_state);
    g_hash_table_insert (priv->pixbufs, key, pixbuf);
  }
  else
  {
    g_free (key);
  }
  awn_overlay_move_to (cr,AWN_OVERLAY(overlay),width,height,width *priv->scale,height *priv->scale,&coord);  
  gdk_cairo_set_source_pixbuf (cr,pixbuf,coord.x,coord.y);  
  cairo_paint_with_alpha (cr,priv->alpha);
}