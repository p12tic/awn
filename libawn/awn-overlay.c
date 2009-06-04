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


/* awn-overlay.c */

#include "awn-overlay.h"
#include "awn-defines.h"
#include "awn-overlaid-icon.h"

#include <gdk/gdk.h>
#include <glib-object.h>



G_DEFINE_TYPE (AwnOverlay, awn_overlay, G_TYPE_OBJECT)

#define AWN_OVERLAY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY, AwnOverlayPrivate))

typedef struct _AwnOverlayPrivate AwnOverlayPrivate;

struct _AwnOverlayPrivate 
{
  GdkGravity  gravity;
  AwnOverlayAlign align;

  double      x_adj;
  double      y_adj;
  double      x_per;      
  double      y_per;  
};

enum
{
  PROP_0,
  PROP_GRAVITY,
  PROP_ALIGN,
  PROP_X_ADJUST,
  PROP_Y_ADJUST
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
  
  pspec = g_param_spec_enum ("gravity",
                               "Gravity",
                               "Gravity",
                               GDK_TYPE_GRAVITY,
                               GDK_GRAVITY_CENTER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_GRAVITY, pspec);  

  pspec = g_param_spec_int ("align",
                               "Align",
                               "Align",
                               AWN_OVERLAY_ALIGN_CENTRE,
                               AWN_OVERLAY_ALIGN_RIGHT,
                               AWN_OVERLAY_ALIGN_CENTRE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_ALIGN, pspec);  
  
  pspec = g_param_spec_double ("x_adj",
                               "X adjust",
                               "X adjust",
                               -1000.0,
                               1000.0,
                               0.0,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_X_ADJUST, pspec);  

  pspec = g_param_spec_double ("y_adj",
                               "Y adjust",
                               "Y adjust",
                               -1000.0,
                               1000.0,
                               0.0,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_Y_ADJUST, pspec);  
    
  g_type_class_add_private (klass, sizeof (AwnOverlayPrivate));
  
}

static void
awn_overlay_init (AwnOverlay *self)
{
}

AwnOverlay*
awn_overlay_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY, NULL);
}

GdkGravity 
awn_overlay_get_gravity (AwnOverlay * overlay)
{
  AwnOverlayPrivate * priv;
  priv = AWN_OVERLAY_GET_PRIVATE (overlay);
  return priv->gravity;
}  

__attribute__((noreturn))  static void 
_awn_overlay_render_overlay (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height)
{
  
  g_assert_not_reached ();
}

void awn_overlay_render_overlay (AwnOverlay* overlay,
                                        AwnThemedIcon * icon,
                                        cairo_t * cr,                                 
                                        gint width,
                                        gint height)
{
  AwnOverlayClass *klass;

  klass = AWN_OVERLAY_GET_CLASS (overlay);

  klass->render_overlay (overlay,icon,cr,width,height);
}

void
awn_overlay_move_to (cairo_t * cr,
                           AwnOverlay* overlay,
                           gint   icon_width,
                           gint   icon_height,
                           gint   overlay_width,
                           gint   overlay_height
                           )
{
  gint yoffset = 0;
  gdouble xoffset;
  gint align;
  GdkGravity gravity;
  gdouble x_adj;
  gdouble y_adj;

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
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0 + xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);  
      break;
    case GDK_GRAVITY_NORTH:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0 + xoffset, 1 + icon_height / 20 + yoffset);  
      break;      
    case GDK_GRAVITY_NORTH_EAST:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, 1 + icon_height / 20 + yoffset);  
      break;
    case GDK_GRAVITY_EAST:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);
      break;      
    case GDK_GRAVITY_SOUTH_EAST:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, icon_height - overlay_height -1+ yoffset);      
      break;
    case GDK_GRAVITY_SOUTH:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0+ xoffset, icon_height - overlay_height -1+ yoffset);
      break;
    case GDK_GRAVITY_SOUTH_WEST:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, icon_height - overlay_height -1+ yoffset);
      break;
    case GDK_GRAVITY_WEST:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);
      break;
    case GDK_GRAVITY_NORTH_WEST:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, 1 + icon_height / 20 + yoffset);  
      break;
    default:
      g_assert_not_reached();   
  }
}
