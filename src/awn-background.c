/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#include "config.h"

#include <gdk/gdkx.h>
#include <libawn/awn-config-client.h>

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

#include "awn-background.h"
#include "awn-x.h"

G_DEFINE_ABSTRACT_TYPE (AwnBackground, awn_background, G_TYPE_OBJECT)

enum 
{
  PROP_0,

  PROP_CLIENT,

  PROP_GSTEP1,
  PROP_GSTEP2,
  PROP_GHISTEP1,
  PROP_GHISTEP2,
  PROP_BORDER,
  PROP_HILIGHT,
  
  PROP_SHOW_SEP,
  PROP_SEP_COLOR,

  PROP_ENABLE_PATTERN,
  PROP_PATTERN_ALPHA,
  PROP_PATTERN_FILENAME,

  PROP_ROUNDED_CORNERS,
  PROP_CORNER_RADIUS,
  PROP_PANEL_ANGLE,
  PROP_CURVINESS,
  PROP_CURVES_SYMEMETRY
};

static void
awn_background_constructed (GObject *object)
{
  AwnBackground *bg = AWN_BACKGROUND (object);

  if (bg->client)
    g_print ("Hook this up to the client Holmes!\n");
}

static void
awn_background_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  g_return_if_fail (AWN_IS_BACKGROUND (object));

  switch (prop_id)
  {
    case PROP_GSTEP1:
    case PROP_GSTEP2:
    case PROP_GHISTEP1:
    case PROP_GHISTEP2:
    case PROP_BORDER:
    case PROP_HILIGHT:
    case PROP_SHOW_SEP:
    case PROP_SEP_COLOR:
      g_value_set_string (value, "FFFFFFFF");
      break;
  default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_background_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  AwnBackground *bg = AWN_BACKGROUND (object);

  g_return_if_fail (AWN_IS_BACKGROUND (object));

  switch (prop_id)
  {
    case PROP_CLIENT:
      bg->client = g_value_get_pointer (value);
      break;

    case PROP_GSTEP1:
      awn_cairo_string_to_color (g_value_get_string (value), &bg->g_step_1);
      break;
    case PROP_GSTEP2:
      awn_cairo_string_to_color (g_value_get_string (value), &bg->g_step_2);
      break;
    case PROP_GHISTEP1:
      awn_cairo_string_to_color (g_value_get_string (value), &bg->g_histep_1);
      break;
    case PROP_GHISTEP2:
      awn_cairo_string_to_color (g_value_get_string (value), &bg->g_histep_2);
      break;
    case PROP_BORDER:
      awn_cairo_string_to_color (g_value_get_string (value), &bg->border_color);
      break;
    case PROP_HILIGHT:
      awn_cairo_string_to_color (g_value_get_string (value),&bg->hilight_color);
      break;
    
    case PROP_SHOW_SEP:
      bg->show_sep = g_value_get_boolean (value);
      break;
    case PROP_SEP_COLOR:
      awn_cairo_string_to_color (g_value_get_string (value),&bg->sep_color);
      break;
    
    case PROP_ENABLE_PATTERN:
      bg->enable_pattern = g_value_get_boolean (value);
      break;
    case PROP_PATTERN_ALPHA:
      bg->pattern_alpha = g_value_get_float (value);
      break;
    case PROP_PATTERN_FILENAME:
      if (GDK_IS_PIXBUF (bg->pattern)) g_object_unref (bg->pattern);
      bg->pattern = gdk_pixbuf_new_from_file (
                               g_value_get_string (value), NULL);
      break;

    case PROP_ROUNDED_CORNERS:
      bg->rounded_corners = g_value_get_boolean (value);
      break;
    case PROP_CORNER_RADIUS:
      bg->corner_radius = g_value_get_float (value);
      break;
    case PROP_PANEL_ANGLE:
      bg->panel_angle = g_value_get_float (value);
      break;
    case PROP_CURVINESS:
      bg->curviness = g_value_get_float (value);
      break;
    case PROP_CURVES_SYMEMETRY:
      bg->curves_symmetry = g_value_get_float (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_background_class_init (AwnBackgroundClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructed   = awn_background_constructed;
  obj_class->get_property  = awn_background_get_property;
  obj_class->set_property  = awn_background_set_property;

  /* Object properties */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_pointer ("client",
                          "Client",
                          "Awn Config Client",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


  g_object_class_install_property (obj_class,
    PROP_GSTEP1,
    g_param_spec_string ("gstep1",
                         "GStep1",
                         "Gradient Step 1",
                         "00000033",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GSTEP2,
    g_param_spec_string ("gstep2",
                         "GStep2",
                         "Gradient Step 2",
                         "000000EE",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP1,
    g_param_spec_string ("ghistep1",
                         "GHiStep1",
                         "Hilight Gradient Step 1",
                         "FFFFFF44",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP2,
    g_param_spec_string ("ghistep2",
                         "GHiStep2",
                         "Hilight Gradient Step 2",
                         "FFFFFF11",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_BORDER,
    g_param_spec_string ("border",
                         "Border",
                         "Border color",
                         "00000055",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_HILIGHT,
    g_param_spec_string ("hilight",
                         "Hilight",
                         "Internal border color",
                         "FFFFFF11",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  
  g_object_class_install_property (obj_class,
    PROP_SHOW_SEP,
    g_param_spec_boolean ("show_sep",
                          "Show Separators",
                          "Show separators",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_SEP_COLOR,
    g_param_spec_string ("sep_color",
                         "Separator color",
                         "Separator color",
                         "00000000",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


  g_object_class_install_property (obj_class,
    PROP_ENABLE_PATTERN,
    g_param_spec_boolean ("draw_pattern",
                          "Draw pattern",
                          "Enable drawing of pattern",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_PATTERN_ALPHA,
    g_param_spec_float ("pattern_alpha",
                        "Pattern alpha",
                        "Pattern Alpha",
                        0.0, 1.0, 0.5,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_PATTERN_FILENAME,
    g_param_spec_string ("pattern_filename",
                         "Pattern filename",
                         "Pattern Filename",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  
  g_object_class_install_property (obj_class,
    PROP_ROUNDED_CORNERS,
    g_param_spec_boolean ("rounded_corners",
                          "Rounded Corners",
                          "Enable drawing of rounded corners",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_CORNER_RADIUS,
    g_param_spec_float ("corner_radius",
                        "Corner Radius",
                        "Corner Radius",
                        0.0, G_MAXFLOAT, 10.0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_PANEL_ANGLE,
    g_param_spec_float ("panel_angle",
                        "Panel Angle",
                        "The angle of the panel in 3D mode",
                        0.0, 90.0, 45.0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_CURVINESS,
    g_param_spec_float ("curviness",
                        "Curviness",
                        "Curviness",
                        0.0, G_MAXFLOAT, 10.0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_CURVES_SYMEMETRY,
    g_param_spec_float ("curves_symmetry",
                        "Curves Symmetry",
                        "The symmetry of the curve",
                        0.0, 1.0, 0.5,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}


static void
awn_background_init (AwnBackground *bg)
{
  ;
}

void 
awn_background_draw (AwnBackground  *bg,
                     cairo_t        *cr, 
                     AwnOrientation  orient,
                     gdouble         x,
                     gdouble         y,
                     gint            width,
                     gint            height)
{
  AwnBackgroundClass *klass;

  g_return_if_fail (AWN_IS_BACKGROUND (bg));
  
  klass = AWN_BACKGROUND_GET_CLASS (bg);
  g_return_if_fail (klass->draw != NULL);

  klass->draw (bg, cr, orient, x, y, width, height);
}

/* vim: set et ts=2 sts=2 sw=2 : */
