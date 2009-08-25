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
 

/**
 * SECTION: awn-overlay-themed-icon
 * @short_description: Themed Icon overlay for use with #AwnOverlaidIcon.
 * @see_also: #AwnEffects, #AwnOverlay, #AwnOverlayPixbuf, #AwnOverlayPixbufFile, 
 * #AwnOverlayThrobber, #AwnOverlayText, #AwnThemedIcon.
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Themed Icon overlay used with an #AwnThemedIcon.
 */


/* awn-overlay-themed-icon.c */

/*
 FIXME
 hook up to theme changes... clear pixbufs when this happens.
 
 the caching of pixbufs may be a bit excessive if scale,width,height are going crazy.
 
 */

#include "awn-overlay-themed-icon.h"

G_DEFINE_TYPE (AwnOverlayThemedIcon, awn_overlay_themed_icon, AWN_TYPE_OVERLAY)

 #define AWN_OVERLAY_THEMED_ICON_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_THEMED_ICON, AwnOverlayThemedIconPrivate))

typedef struct _AwnOverlayThemedIconPrivate AwnOverlayThemedIconPrivate;

struct _AwnOverlayThemedIconPrivate 
{
    AwnThemedIcon *themed_icon;
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
  PROP_SCALE,
  PROP_ICON
};

static void 
_awn_overlay_themed_icon_render (AwnOverlay* _overlay,
                                 GtkWidget *widget,
                                 cairo_t * cr,
                                 gint width,
                                 gint height);
static void
_awn_overlay_themed_icon_theme_change (AwnOverlayThemedIcon * overlay);


static void
awn_overlay_themed_icon_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayThemedIconPrivate * priv;
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (object);

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
    case PROP_ICON:
      g_value_set_object (value,priv->themed_icon);
      break;            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_themed_icon_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayThemedIconPrivate * priv;
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (object);

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
      if ( ! priv->icon_state )
      {
        priv->icon_state = g_strdup_printf ("::no_drop::__AWN_OVERLAY_THEMED_ICON_STATE_NAME_FOR_%s",priv->icon_name);        
      }
      break;      
    case PROP_ICON:
      priv->themed_icon = g_value_get_object (value);
      break;            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_themed_icon_dispose (GObject *object)
{
  AwnOverlayThemedIconPrivate * priv;
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (object);
  
  G_OBJECT_CLASS (awn_overlay_themed_icon_parent_class)->dispose (object);
  

}

static void
awn_overlay_themed_icon_finalize (GObject *object)
{
  AwnOverlayThemedIconPrivate * priv;
  g_return_if_fail (AWN_IS_OVERLAY_THEMED_ICON(object));  
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (object);

  g_free (priv->icon_name);
  g_free (priv->icon_state);
  g_hash_table_destroy (priv->pixbufs);  
  g_signal_handlers_disconnect_by_func (gtk_icon_theme_get_default(),
                                        _awn_overlay_themed_icon_theme_change,
                                        object);
  G_OBJECT_CLASS (awn_overlay_themed_icon_parent_class)->finalize (object);  
  
}

static void
_awn_overlay_themed_icon_theme_change (AwnOverlayThemedIcon * overlay)
{
  /* a change in props will cause effects to queue a draw... however
   a theme change does not cause a change in a prop... we don't have the 
   applet here... so we can't queue a draw on it... so trigger the 
   signal which will invoke g_hash_table_remove_all () and
   signal effects to queue a draw.  I think I should be able to use 
   g_signal_emit_by_name here.... but it's late.  FIXME*/
  AwnOverlayThemedIconPrivate * priv;
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (overlay);
  g_return_if_fail (priv->icon_name);
  gchar * tmp = g_strdup (priv->icon_name);
  g_object_set (overlay,
                "icon_name",tmp,
                NULL);
  g_free (tmp);
}

static void
awn_overlay_themed_icon_constructed (GObject *object)
{
  AwnOverlayThemedIconPrivate * priv;
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (object);
  
  if ( G_OBJECT_CLASS( awn_overlay_themed_icon_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_overlay_themed_icon_parent_class)->constructed (object);
  }

  awn_themed_icon_set_info_append (priv->themed_icon, priv->icon_state, priv->icon_name);


  /*FIXME need to clear the hash table on theme changes also */
  g_signal_connect_swapped (object, "notify::icon-name",
                            G_CALLBACK (g_hash_table_remove_all),
                            priv->pixbufs);
  g_signal_connect_swapped (gtk_icon_theme_get_default(), "changed",
                    G_CALLBACK (_awn_overlay_themed_icon_theme_change),
                    object);
}

static void
awn_overlay_themed_icon_class_init (AwnOverlayThemedIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;      

  object_class->get_property = awn_overlay_themed_icon_get_property;
  object_class->set_property = awn_overlay_themed_icon_set_property;
  object_class->dispose = awn_overlay_themed_icon_dispose;
  object_class->finalize = awn_overlay_themed_icon_finalize;
  object_class->constructed = awn_overlay_themed_icon_constructed;
  
  AWN_OVERLAY_CLASS(klass)->render = _awn_overlay_themed_icon_render;

/**
 * AwnOverlayThemedIcon:alpha:
 *
 * The alpha value to apply to the icon overlay.  Range 0.0...1.0.
 * Defaults to 0.9
 */        
  
  pspec = g_param_spec_double ("alpha",
                               "alpha",
                               "Alpha",
                               0.0,
                               1.0,
                               0.9,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ALPHA, pspec);   

/**
 * AwnOverlayThemedIcon:scale:
 *
 * The desired size of the overlay scaled to the Icon.  Range 0.0...1.0. Default
 * value of 0.3
 */        
  
  pspec = g_param_spec_double ("scale",
                               "scale",
                               "Scale",
                               0.0,
                               1.0,
                               0.3,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_SCALE, pspec);   

/**
 * AwnOverlayThemedIcon:icon-name:
 *
 * The themed icon name of the icon to be overlayed.
 */        
  
  pspec = g_param_spec_string ("icon-name",
                               "Icon name",
                               "Icon gtk theme name",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ICON_NAME, pspec);   

/**
 * AwnOverlayThemedIcon:icon-name:
 *
 * The icon-state to be associated with the themed icon for access through
 * #AwnThemedIcon.  Or NULL if it will not need to be directly accessed.
 */        
  
  pspec = g_param_spec_string ("icon-state",
                               "Icon state",
                               "Icon state",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ICON_STATE, pspec);   

  pspec = g_param_spec_object ("icon",
                               "Icon",
                               "Icon",
                               AWN_TYPE_THEMED_ICON,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_ICON, pspec);   
  
  
  g_type_class_add_private (klass, sizeof (AwnOverlayThemedIconPrivate));  
}

static void
awn_overlay_themed_icon_init (AwnOverlayThemedIcon *self)
{
  AwnOverlayThemedIconPrivate * priv;
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (self);

  priv->pixbufs = g_hash_table_new_full (g_str_hash,g_str_equal, g_free, g_object_unref);
}

/**
 * awn_overlay_themed_icon_new:
 * @icon: an #AwnThemedIcon.
 * @icon_name: A themed icon icon name.
 * @state: A icon state for the icon or NULL.
 * Creates a new instance of #AwnOverlayThemedIcon.
 * Returns: an instance of #AwnOverlayThemedIcon.
 */

AwnOverlayThemedIcon*
awn_overlay_themed_icon_new (AwnThemedIcon * icon,
                             const gchar *icon_name, const gchar *state)
{
  AwnOverlayThemedIcon * ret;
  
  g_return_val_if_fail (icon_name,NULL);
  g_return_val_if_fail (icon,NULL);
  g_return_val_if_fail (AWN_IS_THEMED_ICON (icon), NULL);
  
  ret = g_object_new (AWN_TYPE_OVERLAY_THEMED_ICON, 
                      "icon-name", icon_name,
                      "icon-state", state,
                      "gravity", GDK_GRAVITY_SOUTH_EAST,
                      "icon", icon,
                      NULL);
  return ret;
}

static void 
_awn_overlay_themed_icon_render (AwnOverlay* _overlay,
                                 GtkWidget * widget,
                                 cairo_t * cr,
                                 gint width,
                                 gint height)
{
  AwnOverlayThemedIcon *overlay = AWN_OVERLAY_THEMED_ICON(_overlay);
  AwnOverlayThemedIconPrivate *priv;
  AwnOverlayCoord coord;
  GdkPixbuf * pixbuf = NULL;
  
  priv =  AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (overlay); 
  
  gchar * key = g_strdup_printf ("%dx%d@%lf",width,height,priv->scale);
 
  pixbuf = g_hash_table_lookup (priv->pixbufs,key);
  
  if (! pixbuf)
  {
    pixbuf = awn_themed_icon_get_icon_at_size (priv->themed_icon,
                                               width > height ?
                                                 width : height * priv->scale,
                                               priv->icon_state);
    g_hash_table_insert (priv->pixbufs, key, pixbuf);
  }
  else
  {
    g_free (key);
  }
  awn_overlay_move_to (AWN_OVERLAY(overlay),cr,width,height,width *priv->scale,height *priv->scale,&coord);  
  gdk_cairo_set_source_pixbuf (cr,pixbuf,coord.x,coord.y);  
  cairo_paint_with_alpha (cr,priv->alpha);
}

