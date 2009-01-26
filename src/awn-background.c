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

#include <glib/gprintf.h>
#include <libdesktop-agnostic/color.h>
#include <libawn/awn-config-client.h>
#include <libawn/awn-config-bridge.h>

#include "awn-background.h"

#include "awn-defines.h"

G_DEFINE_ABSTRACT_TYPE (AwnBackground, awn_background, G_TYPE_OBJECT)

enum 
{
  PROP_0,

  PROP_CLIENT,
  PROP_PANEL,

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

  PROP_GTK_THEME_MODE,
  PROP_ROUNDED_CORNERS,
  PROP_CORNER_RADIUS,
  PROP_PANEL_ANGLE,
  PROP_CURVINESS,
  PROP_CURVES_SYMEMETRY
};

enum 
{
  CHANGED,

  LAST_SIGNAL
};
static guint _bg_signals[LAST_SIGNAL] = { 0 };

static void awn_background_set_gtk_theme_mode (AwnBackground *bg, 
                                               gboolean       gtk_mode);


static void
awn_background_constructed (GObject *object)
{
  AwnBackground   *bg = AWN_BACKGROUND (object);
  AwnConfigBridge *bridge;

  bridge = awn_config_bridge_get_default ();

  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_GSTEP1,
                          object, "gstep1");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_GSTEP2,
                          object, "gstep2");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_GHISTEP1,
                          object, "ghistep1");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_GHISTEP2,
                          object, "ghistep2");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_BORDER,
                          object, "border");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_HILIGHT,
                          object, "hilight");
  
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_SHOW_SEP,
                          object, "show_sep");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_SEP_COLOR,
                          object, "sep_color");
  
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_DRAW_PATTERN,
                          object, "draw_pattern");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_PATTERN_ALPHA,
                          object, "pattern_alpha");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_PATTERN_FILENAME,
                          object, "pattern_filename");

  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_GTK_THEME_MODE,
                          object, "gtk_theme_mode");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_CORNER_RADIUS,
                          object, "corner_radius");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_PANEL_ANGLE,
                          object, "panel_angle");
  awn_config_bridge_bind (bridge, bg->client, 
                          AWN_GROUP_THEME, AWN_THEME_CURVINESS, 
                          object, "curviness");
  awn_config_bridge_bind (bridge, bg->client,
                          AWN_GROUP_THEME, AWN_THEME_CURVES_SYMMETRY,
                          object, "curves_symmetry");
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
    case PROP_PANEL:
      bg->panel = g_value_get_pointer (value);
      break;

    case PROP_GSTEP1:
      bg->g_step_1 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      break;
    case PROP_GSTEP2:
      bg->g_step_2 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      break;
    case PROP_GHISTEP1:
      bg->g_histep_1 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      break;
    case PROP_GHISTEP2:
      bg->g_histep_2 = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      break;
    case PROP_BORDER:
      bg->border_color = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      break;
    case PROP_HILIGHT:
      bg->hilight_color = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
      break;
    
    case PROP_SHOW_SEP:
      bg->show_sep = g_value_get_boolean (value);
      break;
    case PROP_SEP_COLOR:
      bg->sep_color = desktop_agnostic_color_new_from_string (g_value_get_string (value), NULL);
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

    case PROP_GTK_THEME_MODE:
      awn_background_set_gtk_theme_mode (bg, g_value_get_boolean (value));
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

  g_signal_emit (object, _bg_signals[CHANGED], 0);
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
    PROP_PANEL,
    g_param_spec_pointer ("panel",
                          "panel",
                          "AwnPanel associated with this background",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GSTEP1,
    g_param_spec_string ("gstep1",
                         "GStep1",
                         "Gradient Step 1",
                         "FF0000FF",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_GSTEP2,
    g_param_spec_string ("gstep2",
                         "GStep2",
                         "Gradient Step 2",
                         "00FF00FF",
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
                         "000000FF",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_HILIGHT,
    g_param_spec_string ("hilight",
                         "Hilight",
                         "Internal border color",
                         "FFFFFFff",
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
    PROP_GTK_THEME_MODE,
    g_param_spec_boolean ("gtk_theme_mode",
                          "Gtk theme mode",
                          "Use colours from the current Gtk theme",
                          TRUE,
                          G_PARAM_READWRITE));
  
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

  /* Add signals to the class */
  _bg_signals[CHANGED] = 
    g_signal_new ("changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (AwnBackgroundClass, changed),
                  NULL, NULL, 
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
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

/*
 * If we're in gtk theme mode, then load up the colours from the current gtk
 * theme
 */

/**
 * alpha: between 0 and 255. Multiplied by 256 to get the "proper" alpha value.
 */
static void
set_cfg_from_theme (GdkColor        *color,
                    gushort          alpha,
                    AwnConfigClient *client,
                    const gchar     *key)
{
  DesktopAgnosticColor *da_color;
  gchar *hex_value;
  da_color = desktop_agnostic_color_new (color, alpha * 256);
  hex_value = desktop_agnostic_color_to_string (da_color);
  awn_config_client_set_string (client,
                                AWN_GROUP_THEME, key,
                                hex_value, NULL);
  g_free (hex_value);
  g_object_unref (da_color);
}

static void
load_colours_from_widget (AwnBackground *bg, GtkWidget *widget)
{
  AwnConfigClient *client = bg->client;
  GtkStyle        *style;

  style = gtk_widget_get_style (widget);

  g_debug ("Updating gtk theme colours");

  /* main colours */
  set_cfg_from_theme (&style->bg[GTK_STATE_NORMAL], 155,
                      client, AWN_THEME_GSTEP1);
  set_cfg_from_theme (&style->bg[GTK_STATE_NORMAL], 200,
                      client, AWN_THEME_GSTEP2);

  awn_config_client_set_string (client,
                                AWN_GROUP_THEME, AWN_THEME_GHISTEP1,
                                "#FFFFFF0C", NULL);
  awn_config_client_set_string (client,
                                AWN_GROUP_THEME, AWN_THEME_GHISTEP2,
                                "#FFFFFF0B", NULL);

  set_cfg_from_theme (&style->dark[GTK_STATE_ACTIVE], 200,
                      client, AWN_THEME_BORDER);
  set_cfg_from_theme (&style->light[GTK_STATE_ACTIVE], 100,
                      client, AWN_THEME_HILIGHT);

  /* Don't draw patterns */
  awn_config_client_set_bool (client,
                              AWN_GROUP_THEME, AWN_THEME_DRAW_PATTERN, 
                              FALSE, NULL);

  /* Set up seperators to draw in the standard way */
  awn_config_client_set_bool (client, 
                              AWN_GROUP_THEME, AWN_THEME_SHOW_SEP,
                              TRUE, NULL);
  awn_config_client_set_string (client, 
                                AWN_GROUP_THEME, AWN_THEME_SEP_COLOR,
                                "#FFFFFF00", NULL);

  /* Misc settings */
}

static void
on_widget_realized (GtkWidget *widget, AwnBackground *bg)
{
  load_colours_from_widget (bg, widget);
}


static void
on_style_set (GtkWidget *widget, GtkStyle *old, AwnBackground *bg)
{
  load_colours_from_widget (bg, widget);
}

static void 
awn_background_set_gtk_theme_mode (AwnBackground *bg, 
                                   gboolean       gtk_mode)
{
  if (gtk_mode)
  {
    GtkWidget *widget = GTK_WIDGET (bg->panel);
    
    g_signal_connect (widget, "realize", G_CALLBACK (on_widget_realized), bg);

    bg->changed = g_signal_connect (widget, "style-set", 
                                     G_CALLBACK (on_style_set), bg);
    
  }
  else
  {
    if (bg->changed)
      g_signal_handler_disconnect (bg->panel, bg->changed);
    bg->changed = 0;
  }
}


/* vim: set et ts=2 sts=2 sw=2 : */
