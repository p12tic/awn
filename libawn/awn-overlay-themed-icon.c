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
 * SECTION: awn-overlay-themed-icon
 * @short_description: Themed Icon overlay for use with #AwnThemedIcon.
 * @see_also: #AwnEffects, #AwnOverlay, #AwnOverlayPixbuf, #AwnOverlayPixbufFile, 
 * #AwnOverlayThrobber, #AwnOverlayText, #AwnThemedIcon.
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Themed Icon overlay used with an #AwnThemedIcon.
 */

/**
 * AwnOverlayThemedIcon:
 *
 * Themed Icon overlay used with an #AwnThemedIcon.
 */

/* awn-overlay-themed-icon.c */

/*
 FIXME
 hook up to theme changes... clear pixbufs when this happens.
 
 the caching of pixbufs may be a bit excessive if scale,width,height are going crazy.
 
 */
#include "libawn/libawn.h"
#include "awn-overlay-themed-icon.h"


#if !GTK_CHECK_VERSION(2,14,0)
#define GTK_ICON_LOOKUP_FORCE_SIZE 0
#endif

G_DEFINE_TYPE (AwnOverlayThemedIcon, awn_overlay_themed_icon, AWN_TYPE_OVERLAY)

 #define AWN_OVERLAY_THEMED_ICON_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_THEMED_ICON, AwnOverlayThemedIconPrivate))

typedef struct _AwnOverlayThemedIconPrivate AwnOverlayThemedIconPrivate;

struct _AwnOverlayThemedIconPrivate 
{
    gdouble alpha;
    gchar * icon_name;
    gdouble scale;
    AwnPixbufCache *pixbufs;
};

enum
{
  PROP_0,
  PROP_ALPHA,
  PROP_ICON_NAME,
  PROP_SCALE
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

  g_signal_connect_swapped (gtk_icon_theme_get_default(), "changed",
                    G_CALLBACK (_awn_overlay_themed_icon_theme_change),
                    object);
}

static void
awn_overlay_themed_icon_class_init (AwnOverlayThemedIconClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

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
  
  g_object_class_install_property (object_class,
    PROP_ALPHA,
    g_param_spec_double ("alpha",
                         "alpha",
                         "Alpha",
                         0.0, 1.0, 0.9,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayThemedIcon:scale:
 *
 * The desired size of the overlay scaled to the Icon.  Range 0.0...1.0. Default
 * value of 0.3
 */        
  
  g_object_class_install_property (object_class,
    PROP_SCALE,
    g_param_spec_double ("scale",
                         "scale",
                         "Scale",
                         0.0, 1.0, 0.3,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayThemedIcon:icon-name:
 *
 * The themed icon name of the icon to be overlayed.
 */        
  
  g_object_class_install_property (object_class,
    PROP_ICON_NAME,
    g_param_spec_string ("icon-name",
                         "Icon name",
                         "Icon gtk theme name",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));
  
  g_type_class_add_private (klass, sizeof (AwnOverlayThemedIconPrivate));  
}

static void
awn_overlay_themed_icon_init (AwnOverlayThemedIcon *self)
{
  AwnOverlayThemedIconPrivate * priv;
  priv = AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (self);

  priv->pixbufs = awn_pixbuf_cache_get_default ();
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
awn_overlay_themed_icon_new (const gchar *icon_name)
{
  AwnOverlayThemedIcon * ret;
  
  g_return_val_if_fail (icon_name,NULL);
  
  ret = g_object_new (AWN_TYPE_OVERLAY_THEMED_ICON, 
                      "icon-name", icon_name,
                      "gravity", GDK_GRAVITY_SOUTH_EAST,
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
  GtkIconTheme * gtk_theme = gtk_icon_theme_get_default ();
  GError * err = NULL;
  gint size;
  
  priv =  AWN_OVERLAY_THEMED_ICON_GET_PRIVATE (overlay); 

  size = width > height ? width : height * priv->scale;
  /*
    TODO:  Add support of awn theme
   */
  pixbuf = awn_pixbuf_cache_lookup (priv->pixbufs,
                                    NULL,
                                    awn_utils_get_gtk_icon_theme_name ( gtk_theme ),
                                    priv->icon_name,
                                    -1,
                                    size,
                                    NULL);
 
  if (! pixbuf)
  {
    pixbuf = gtk_icon_theme_load_icon ( gtk_theme,
                                       priv->icon_name,
                                       size,
                                       GTK_ICON_LOOKUP_FORCE_SIZE ,
                                       &err);
    if (err)
    {
      g_warning ("%s: error loading icon %s, %s",__func__,priv->icon_name,err->message);
      g_error_free (err);
      err = NULL;
    }

    if (!pixbuf)
    {
      g_warning ("%s: Failed to load icon %s.  Falling back to %s",__func__,priv->icon_name,GTK_STOCK_MISSING_IMAGE);
      pixbuf = gtk_icon_theme_load_icon ( gtk_theme,
                                       GTK_STOCK_MISSING_IMAGE,
                                       size,
                                       GTK_ICON_LOOKUP_FORCE_SIZE ,
                                       NULL);
    }
    if (!pixbuf )
    {
      g_warning ("%s: Failed to load %s",__func__,GTK_STOCK_MISSING_IMAGE);
      pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, size, size);
      gdk_pixbuf_fill (pixbuf, 0xee221100);
    }
    awn_pixbuf_cache_insert_pixbuf (priv->pixbufs,
                                    pixbuf,
                                    NULL,
                                    awn_utils_get_gtk_icon_theme_name (gtk_theme),
                                    priv->icon_name);
  }
                                       
  awn_overlay_move_to (AWN_OVERLAY(overlay),cr,width,height,width *priv->scale,height *priv->scale,&coord);  
  gdk_cairo_set_source_pixbuf (cr,pixbuf,coord.x,coord.y);  
  cairo_paint_with_alpha (cr,priv->alpha);
}

