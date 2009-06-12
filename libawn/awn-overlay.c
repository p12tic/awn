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
 * SECTION:AwnOverlay
 * @short_description: Base object for overlays used with #AwnOverlaidIcon.
 * @see_also: #AwnOverlaidIcon, #AwnOverlayText, #AwnOverlayIcon, #AwnOverlayThrobber
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Base object for overlays used with #AwnOverlaidIcon.  This object is only useful
 * as a base class from which others are derived.
 */



/* awn-overlay.c */

#include "awn-overlay.h"
#include "awn-defines.h"
#include "awn-overlaid-icon.h"

#include <gdk/gdk.h>
#include <glib-object.h>



G_DEFINE_ABSTRACT_TYPE (AwnOverlay, awn_overlay, G_TYPE_OBJECT)

#define AWN_OVERLAY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY, AwnOverlayPrivate))

typedef struct _AwnOverlayPrivate AwnOverlayPrivate;

struct _AwnOverlayPrivate 
{
  GdkGravity  gravity;
  AwnOverlayAlign align;

  double      x_adj;
  double      y_adj;

  gboolean    active;  /*if false then the overlay_render will not run*/
  
  gdouble     x_override;
  gdouble     y_override;
};

enum
{
  PROP_0,
  PROP_GRAVITY,
  PROP_ALIGN,
  PROP_X_ADJUST,
  PROP_Y_ADJUST,
  PROP_ACTIVE,
  PROP_X_OVERRIDE,
  PROP_Y_OVERRIDE
};

static void
_awn_overlay_render_overlay (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height);

static void
awn_overlay_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayPrivate * priv;
  priv = AWN_OVERLAY_GET_PRIVATE (object);
  switch (property_id) {
    case PROP_GRAVITY:
        g_value_set_enum (value,priv->gravity);
        break;
    case PROP_ALIGN:
        g_value_set_int (value,priv->align);
        break;
    case PROP_X_ADJUST:
        g_value_set_double (value, priv->x_adj);
        break;
    case PROP_Y_ADJUST:
        g_value_set_double (value, priv->y_adj);
        break;
    case PROP_ACTIVE:
        g_value_set_boolean (value, priv->active);
        break;
    case PROP_X_OVERRIDE:
        g_value_set_double (value, priv->x_override);
        break;      
    case PROP_Y_OVERRIDE:
        g_value_set_double (value, priv->y_override);
        break;      
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayPrivate * priv;
  priv = AWN_OVERLAY_GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_GRAVITY:
      priv->gravity = g_value_get_enum (value);
      break;
    case PROP_ALIGN:
      priv->align = g_value_get_int (value);
      break;
    case PROP_X_ADJUST:
      priv->x_adj = g_value_get_double (value);
      break;
    case PROP_Y_ADJUST:
      priv->y_adj = g_value_get_double (value);
      break;
    case PROP_ACTIVE:
      priv->active = g_value_get_boolean (value);
      break;      
    case PROP_X_OVERRIDE:
      priv->x_override = g_value_get_double (value);
      break;
    case PROP_Y_OVERRIDE:
      priv->y_override = g_value_get_double (value);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_parent_class)->dispose (object);
}

static void
awn_overlay_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_parent_class)->finalize (object);
}

static void
awn_overlay_class_init (AwnOverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;  
  
  object_class->get_property = awn_overlay_get_property;
  object_class->set_property = awn_overlay_set_property;
  object_class->dispose = awn_overlay_dispose;
  object_class->finalize = awn_overlay_finalize;
  
  klass->render_overlay = _awn_overlay_render_overlay;  
  
/**
 * AwnOverlay:gravity:
 *
 * A property that controls placement of the overlay of type #GdkGravity.  
 * GDK_GRAVITY_STATIC is NOT a valid value.
 */  
  pspec = g_param_spec_enum ("gravity",
                               "Gravity",
                               "Gravity",
                               GDK_TYPE_GRAVITY,
                               GDK_GRAVITY_CENTER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_GRAVITY, pspec);  

/**
 * AwnOverlay:align:
 *
 * An #AwnOverlayAlign property that controls horizontal alignment of the overlay 
 * relative to it's position as specified by the gravity property.  Often used 
 * with #AwnOverlayText overlays.  Setting to AWN_OVERLAY_ALIGN_RIGHT or 
 * AWN_OVERLAY_ALIGN_LEFT will allow for a fixed right or left position for the 
 * overlay.
 */    
  pspec = g_param_spec_int ("align",
                               "Align",
                               "Align",
                               AWN_OVERLAY_ALIGN_CENTRE,
                               AWN_OVERLAY_ALIGN_RIGHT,
                               AWN_OVERLAY_ALIGN_CENTRE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ALIGN, pspec);  

/**
 * AwnOverlay:x-adj:
 *
 * An property of type #gdouble that allows the adjustment of the horizontal
 * position of the #AwnOverlay.  Range of -1.0...1.0.  The amount of adjustment is
 * this x-adj * width of the #AwnIcon.  A value of 0.0 indicates that gravity 
 * and align will solely determine the x position.
 */      
  pspec = g_param_spec_double ("x-adj",
                               "X adjust",
                               "X adjust",
                               -1.0,
                               1.0,
                               0.0,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_X_ADJUST, pspec);  

/**
 * AwnOverlay:y-adj:
 *
 * An property of type #gdouble that allows the adjustment of the vertical
 * position of the #AwnOverlay.  Range of -1.0...1.0.  The amount of adjustment is
 * this y-adj * height of the #AwnIcon.  A value of 0.0 indicates that gravity 
 * and align will solely determine the y position.
 */        
  pspec = g_param_spec_double ("y-adj",
                               "Y adjust",
                               "Y adjust",
                               -1.0,
                               1.0,
                               0.0,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_Y_ADJUST, pspec);  

/**
 * AwnOverlay:active:
 *
 * The active property controls if the render_overlay virtual method of
 * #AwnOverlayClass is invoked when awn_overlay_render_overlay() .  If set to 
 * FALSE the overlay is not rendered.  Subclass implementors should monitor this_effect
 * property for changes if it is appropriate to disengage timers etc when set to
 * FALSE.
 */        
  pspec = g_param_spec_boolean ("active",
                               "Active",
                               "Active",
                               TRUE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ACTIVE, pspec);  

/**
 * AwnOverlay:x-override:
 *
 * Overrides the x coordinates.  In most cases if you're using this then you 
 * are probably doing something wrong.
 */          
  pspec = g_param_spec_double ("x-override",
                               "X Override",
                               "X Override",
                               -10000.0,
                               1000.0,
                               -10000.0,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_X_OVERRIDE, pspec);  

/**
 * AwnOverlay:y-override:
 *
 * Overrides the y coordinates.  In most cases if you're using this then you 
 * are probably doing something wrong.
 */            
  pspec = g_param_spec_double ("y-override",
                               "Y Override",
                               "Y Override",
                               -10000.0,
                               1000.0,
                               -10000.0,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_Y_OVERRIDE, pspec);  
  
  g_type_class_add_private (klass, sizeof (AwnOverlayPrivate));
  
}

static void
awn_overlay_init (AwnOverlay *self)
{
}

/**
 * awn_overlay_new:
 *
 * Creates a new instance of #AwnOverlay.
 * Returns: an instance of #AwnOverlay.
 */
AwnOverlay*
awn_overlay_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY, NULL);
}

static void 
_awn_overlay_render_overlay (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height)
{
  g_warning ("Overlay has not overriden render_overlay member in base (AwnOverlay) \n");
}

/**
 * awn_overlay_render_overlay:
 * @overlay: An pointer to an #AwnOverlay (or subclass) object.
 * @icon: The #AwnThemedIcon that is being overlaid.
 * @cr: Pointer to cairo context ( #cairo_t ) for the surface being overlaid. 
 * @width: The width of the #AwnThemedIcon as #gint.
 * @height: The height of the #AwnThemedIcon as #gint.
 *
 * A virtual function invoked by #AwnOverlaidIcon for each overlay it contains, 
 * on #AwnOverlaidIcon::expose.  This should be implemented by subclasses of 
 * #AwnOverlay.  
 * 
 */
void 
awn_overlay_render_overlay (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height)
{
  AwnOverlayClass *klass;
  AwnOverlayPrivate * priv;
  
  priv = AWN_OVERLAY_GET_PRIVATE (overlay);
  klass = AWN_OVERLAY_GET_CLASS (overlay);

  if (priv->active)
  {
    klass->render_overlay (overlay,icon,cr,width,height);
  }
}


/**
 * awn_overlay_move_to:
 * @cr: Pointer to Cairo context ( #cairo_t) for the surface being overlaid.  Poi
 * @overlay: An pointer to an #AwnOverlay (or subclass) object.
 * @icon_width: The width of the #AwnIcon as #gint.
 * @icon_height: The height of the #AwnIcon as #gint.
 * @overlay_width: The width of the #AwnOverlay as #gint.
 * @overlay_height: The height of the #AwnOverlay as #gint.
 * @coord_req: Address of a #AwnOverlayCoord structure or NULL.  The x,y coords
 * will be returned in the structure if one is provided so they can be used
 * a later time if needed.
 *
 * A convenience function for subclasses of #AwnOverlay.  For most cases will
 * provide correct placement of the overlay within the surface.  Only of 
 * interest for those implementing #AwnOverlay subclass.
 * 
 */
void
awn_overlay_move_to (      AwnOverlay* overlay,
                           cairo_t * cr,
                           gint   icon_width,
                           gint   icon_height,
                           gint   overlay_width,
                           gint   overlay_height,
                           AwnOverlayCoord * coord_req
                           )
{
  gint yoffset = 0;
  gdouble xoffset;
  gint align;
  GdkGravity gravity;
  gdouble x_adj;
  gdouble y_adj;
  AwnOverlayCoord  coord;
  AwnOverlayPrivate * priv;
  
  priv = AWN_OVERLAY_GET_PRIVATE (overlay);

  g_object_get (overlay,
               "align", &align,
               "gravity", &gravity,
                "x_adj", &x_adj,
                "y_adj", &y_adj,
               NULL);
  
  switch (align)
  {
    case AWN_OVERLAY_ALIGN_CENTRE:
      xoffset = 0;
      break;
    case AWN_OVERLAY_ALIGN_LEFT:
      xoffset = overlay_width / 2.0;
      break;
    case AWN_OVERLAY_ALIGN_RIGHT:
      xoffset = -1 * overlay_width / 2.0;      
      break;
    default:
      g_assert_not_reached();
  }
  xoffset = xoffset + (x_adj * icon_width);
  yoffset = yoffset + (y_adj * icon_height);
  switch (gravity)
  {
    case GDK_GRAVITY_CENTER:
      coord.x = icon_width/2.0 - overlay_width / 2.0 + xoffset;
      coord.y = icon_height / 2.0 - overlay_height/2.0 + yoffset;
      break;
    case GDK_GRAVITY_NORTH:
      coord.x = icon_width/2.0 - overlay_width / 2.0 + xoffset; 
      coord.y = yoffset;  
      break;      
    case GDK_GRAVITY_NORTH_WEST:
      coord.x = xoffset;
      coord.y = yoffset;
      break;
    case GDK_GRAVITY_WEST:
      coord.x = xoffset;
      coord.y = icon_height / 2.0 - overlay_height/2.0 + yoffset;
      break;      
    case GDK_GRAVITY_SOUTH_WEST:
      coord.x = xoffset;
      coord.y = icon_height - overlay_height + yoffset;
      break;
    case GDK_GRAVITY_SOUTH:
      coord.x = icon_width/2.0 - overlay_width / 2.0+ xoffset;
      coord.y = icon_height - overlay_height + yoffset;
      break;
    case GDK_GRAVITY_SOUTH_EAST:
      coord.x = icon_width - overlay_width+ xoffset;
      coord.y = icon_height - overlay_height + yoffset;
      break;
    case GDK_GRAVITY_EAST:
      coord.x = icon_width - overlay_width+ xoffset;
      coord.y = icon_height / 2.0 - overlay_height/2.0 + yoffset;
      break;
    case GDK_GRAVITY_NORTH_EAST:
      coord.x = icon_width - overlay_width+ xoffset; 
      coord.y = yoffset;  
      break;
    default:
      g_assert_not_reached();   
  }
  if (priv->x_override > -1000.0)
  {
    coord.x = priv->x_override * icon_width / 48.0;
  }
  if (priv->y_override > -1000.)
  {
    coord.y = priv->y_override * icon_width / 48.0;
  }
  cairo_move_to (cr, coord.x, coord.y);  
  if (coord_req)
  {
    *coord_req = coord;
  }
}
