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
#include <libdesktop-agnostic/desktop-agnostic.h>

#include "awn-background.h"

#include "awn-defines.h"
#include "libawn/gseal-transition.h"
#include "libawn/awn-effects-ops-helpers.h"

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
  PROP_DIALOG_GTK_MODE,
  PROP_ROUNDED_CORNERS,
  PROP_CORNER_RADIUS,
  PROP_PANEL_ANGLE,
  PROP_CURVINESS,
  PROP_CURVES_SYMEMETRY,
  PROP_FLOATY_OFFSET,
  PROP_THICKNESS
};

enum 
{
  CHANGED,
  PADDING_CHANGED,

  LAST_SIGNAL
};
static guint _bg_signals[LAST_SIGNAL] = { 0 };

static void awn_background_set_gtk_theme_mode (AwnBackground *bg, 
                                               gboolean       gtk_mode);

static void awn_background_set_dialog_gtk_mode (AwnBackground *bg, 
                                                gboolean       gtk_mode);

static void awn_background_refresh_pattern (AwnBackground *bg);

static void awn_background_padding_zero (AwnBackground *bg,
                                         GtkPositionType position,
                                         guint *padding_top,
                                         guint *padding_bottom,
                                         guint *padding_left,
                                         guint *padding_right);

static void
awn_background_draw_none   (AwnBackground  *bg,
                            cairo_t        *cr,
                            GtkPositionType  position,
                            GdkRectangle   *area);

static void awn_background_mask_none (AwnBackground  *bg,
                                      cairo_t        *cr,
                                      GtkPositionType  position,
                                      GdkRectangle   *area);

static gboolean awn_background_get_needs_redraw (AwnBackground *bg,
                                                 GtkPositionType position,
                                                 GdkRectangle *area);

static AwnPathType awn_background_path_default (AwnBackground *bg,
                                                gfloat *offset_mod);

static void
awn_background_constructed (GObject *object)
{
  AwnBackground   *bg = AWN_BACKGROUND (object);

  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_GSTEP1,
                                       object, "gstep1", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_GSTEP2,
                                       object, "gstep2", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_GHISTEP1,
                                       object, "ghistep1", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_GHISTEP2,
                                       object, "ghistep2", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_BORDER,
                                       object, "border", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_HILIGHT,
                                       object, "hilight", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_SHOW_SEP,
                                       object, "show-sep", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_SEP_COLOR,
                                       object, "sep-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_DRAW_PATTERN,
                                       object, "draw-pattern", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_PATTERN_ALPHA,
                                       object, "pattern-alpha", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_PATTERN_FILENAME,
                                       object, "pattern-filename", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);

  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_GTK_THEME_MODE,
                                       object, "gtk-theme-mode", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_DLG_GTK_MODE,
                                       object, "dialog-gtk-mode", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_CORNER_RADIUS,
                                       object, "corner-radius", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_PANEL_ANGLE,
                                       object, "panel-angle", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_CURVINESS,
                                       object, "curviness", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_FLOATY_OFFSET,
                                       object, "floaty-offset", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_CURVES_SYMMETRY,
                                       object, "curves-symmetry", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (bg->client,
                                       AWN_GROUP_THEME, AWN_THEME_THICKNESS,
                                       object, "thickness", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
}

static void
awn_background_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  AwnBackground *bg;
  g_return_if_fail (AWN_IS_BACKGROUND (object));

  bg = AWN_BACKGROUND (object);

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
    {
      DesktopAgnosticColor *color;

      g_warning ("Background property get unimplemented!");
      color = desktop_agnostic_color_new_from_string ("white", NULL);
      g_value_take_object (value, color);
      break;
    }
    case PROP_FLOATY_OFFSET:
      g_value_set_int (value, bg->floaty_offset);
      break;
    case PROP_THICKNESS:
      g_value_set_float (value, bg->thickness);
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
  GError *error = NULL;

  g_return_if_fail (AWN_IS_BACKGROUND (object));

  switch (prop_id)
  {
    case PROP_CLIENT:
      bg->client = g_value_get_object (value);
      break;
    case PROP_PANEL:
      bg->panel = g_value_get_pointer (value);
      break;

    case PROP_GSTEP1:
      if (bg->g_step_1)
      {
        bg->g_step_1 = (g_object_unref (bg->g_step_1), NULL);
      }
      bg->g_step_1 = (DesktopAgnosticColor*)g_value_dup_object (value);
      break;
    case PROP_GSTEP2:
      if (bg->g_step_2)
      {
        bg->g_step_2 = (g_object_unref (bg->g_step_2), NULL);
      }
      bg->g_step_2 = (DesktopAgnosticColor*)g_value_dup_object (value);
      break;
    case PROP_GHISTEP1:
      if (bg->g_histep_1)
      {
        bg->g_histep_1 = (g_object_unref (bg->g_histep_1), NULL);
      }
      bg->g_histep_1 = (DesktopAgnosticColor*)g_value_dup_object (value);
      break;
    case PROP_GHISTEP2:
      if (bg->g_histep_2)
      {
        bg->g_histep_2 = (g_object_unref (bg->g_histep_2), NULL);
      }
      bg->g_histep_2 = (DesktopAgnosticColor*)g_value_dup_object (value);
      break;
    case PROP_BORDER:
      if (bg->border_color)
      {
        bg->border_color = (g_object_unref (bg->border_color), NULL);
      }
      bg->border_color = (DesktopAgnosticColor*)g_value_dup_object (value);
      break;
    case PROP_HILIGHT:
      if (bg->hilight_color)
      {
        bg->hilight_color = (g_object_unref (bg->hilight_color), NULL);
      }
      bg->hilight_color = (DesktopAgnosticColor*)g_value_dup_object (value);
      break;
    case PROP_SHOW_SEP:
      bg->show_sep = g_value_get_boolean (value);
      break;
    case PROP_SEP_COLOR:
      if (bg->sep_color)
      {
        bg->sep_color = (g_object_unref (bg->sep_color), NULL);
      }
      bg->sep_color = (DesktopAgnosticColor*)g_value_dup_object (value);
      break;
    
    case PROP_ENABLE_PATTERN:
      bg->enable_pattern = g_value_get_boolean (value);
      break;
    case PROP_PATTERN_ALPHA:
      bg->pattern_alpha = g_value_get_float (value);
      awn_background_refresh_pattern (bg);
      break;
    case PROP_PATTERN_FILENAME:
      if (bg->pattern_original != NULL)
      {
        g_object_unref  (bg->pattern_original);
        bg->pattern_original = NULL;
      }
      bg->pattern_original = gdk_pixbuf_new_from_file (
          g_value_get_string (value), &error);

      if (error != NULL)
      {
        if (bg->enable_pattern)
        {
          g_warning ("Unable to load \"%s\". Error: %s.",
                     g_value_get_string (value), error->message);
        }
        bg->pattern_original = NULL;
        g_error_free (error);
      }
      awn_background_refresh_pattern (bg);
      break;

    case PROP_GTK_THEME_MODE:
      awn_background_set_gtk_theme_mode (bg, g_value_get_boolean (value));
      break;
    case PROP_DIALOG_GTK_MODE:
      awn_background_set_dialog_gtk_mode (bg, g_value_get_boolean (value));
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
    case PROP_THICKNESS:
      bg->thickness = g_value_get_float (value);
      break;
    case PROP_FLOATY_OFFSET:
      bg->floaty_offset = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      return;
  }
  awn_background_invalidate (bg);
  g_signal_emit (object, _bg_signals[CHANGED], 0);
}

static void
awn_background_finalize (GObject *object)
{
  g_return_if_fail (AWN_IS_BACKGROUND (object));

  AwnBackground *bg = AWN_BACKGROUND (object);

  if (bg->client)
  {
    desktop_agnostic_config_client_unbind_all_for_object (bg->client,
                                                          object, NULL);
  }

  if (bg->changed)
  {
    g_signal_handler_disconnect (bg->panel, bg->changed);
  }

  if (bg->pattern_original) g_object_unref (bg->pattern_original);
  if (bg->pattern) cairo_surface_destroy (bg->pattern);

  if (bg->g_step_1) g_object_unref (bg->g_step_1);
  if (bg->g_step_2) g_object_unref (bg->g_step_2);
  if (bg->g_histep_1) g_object_unref (bg->g_histep_1);
  if (bg->g_histep_2) g_object_unref (bg->g_histep_2);
  if (bg->border_color) g_object_unref (bg->border_color);
  if (bg->hilight_color) g_object_unref (bg->hilight_color);
  if (bg->sep_color) g_object_unref (bg->sep_color);

  if (bg->helper_surface != NULL)
  {
    cairo_surface_finish (bg->helper_surface);
    cairo_surface_destroy (bg->helper_surface);
  }

  G_OBJECT_CLASS (awn_background_parent_class)->finalize (object);
}

static void
awn_background_class_init (AwnBackgroundClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->constructed      = awn_background_constructed;
  obj_class->get_property     = awn_background_get_property;
  obj_class->set_property     = awn_background_set_property;
  obj_class->finalize         = awn_background_finalize;

  klass->padding_request      = awn_background_padding_zero;
  klass->get_shape_mask       = awn_background_mask_none;
  klass->get_input_shape_mask = awn_background_mask_none;
  klass->get_path_type        = awn_background_path_default;
  klass->get_strut_offsets    = NULL;
  klass->draw                 = awn_background_draw_none;
  klass->get_needs_redraw     = awn_background_get_needs_redraw;

  /* Object properties */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_object ("client",
                         "Client",
                         "Awn Config Client",
                         DESKTOP_AGNOSTIC_CONFIG_TYPE_CLIENT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_PANEL,
    g_param_spec_pointer ("panel",
                          "panel",
                          "AwnPanel associated with this background",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_GSTEP1,
    g_param_spec_object ("gstep1",
                         "GStep1",
                         "Gradient Step 1",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_GSTEP2,
    g_param_spec_object ("gstep2",
                         "GStep2",
                         "Gradient Step 2",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP1,
    g_param_spec_object ("ghistep1",
                         "GHiStep1",
                         "Hilight Gradient Step 1",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_GHISTEP2,
    g_param_spec_object ("ghistep2",
                         "GHiStep2",
                         "Hilight Gradient Step 2",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_BORDER,
    g_param_spec_object ("border",
                         "Border",
                         "Border color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_HILIGHT,
    g_param_spec_object ("hilight",
                         "Hilight",
                         "Internal border color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));
  
  g_object_class_install_property (obj_class,
    PROP_SHOW_SEP,
    g_param_spec_boolean ("show-sep",
                          "Show Separators",
                          "Show separators",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_SEP_COLOR,
    g_param_spec_object ("sep-color",
                         "Separator color",
                         "Separator color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_ENABLE_PATTERN,
    g_param_spec_boolean ("draw-pattern",
                          "Draw pattern",
                          "Enable drawing of pattern",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_PATTERN_ALPHA,
    g_param_spec_float ("pattern-alpha",
                        "Pattern alpha",
                        "Pattern Alpha",
                        0.0, 1.0, 0.5,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_PATTERN_FILENAME,
    g_param_spec_string ("pattern-filename",
                         "Pattern filename",
                         "Pattern Filename",
                         "",
                         G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_GTK_THEME_MODE,
    g_param_spec_boolean ("gtk-theme-mode",
                          "Gtk theme mode",
                          "Use colours from the current Gtk theme",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_DIALOG_GTK_MODE,
    g_param_spec_boolean ("dialog-gtk-mode",
                          "Dialog Gtk theme mode",
                          "Use colours from the current Gtk theme on dialog",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_ROUNDED_CORNERS,
    g_param_spec_boolean ("rounded-corners",
                          "Rounded Corners",
                          "Enable drawing of rounded corners",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_CORNER_RADIUS,
    g_param_spec_float ("corner-radius",
                        "Corner Radius",
                        "Corner Radius",
                        0.0, G_MAXFLOAT, 10.0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_PANEL_ANGLE,
    g_param_spec_float ("panel-angle",
                        "Panel Angle",
                        "The angle of the panel in 3D mode",
                        0.0, 90.0, 45.0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_FLOATY_OFFSET,
    g_param_spec_int ("floaty-offset",
                      "Floaty offset",
                      "The offset of the panel in Floaty mode",
                      0, G_MAXINT, 10,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_CURVINESS,
    g_param_spec_float ("curviness",
                        "Curviness",
                        "Curviness",
                        0.0, 1.0, 1.0,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_CURVES_SYMEMETRY,
    g_param_spec_float ("curves-symmetry",
                        "Curves Symmetry",
                        "The symmetry of the curve",
                        0.0, 1.0, 0.5,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (obj_class,
    PROP_THICKNESS,
    g_param_spec_float ("thickness",
                        "Thickness",
                        "The thickness in 3D mode",
                        0.0, 1.0, 0.6,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));
                        
  /* Add signals to the class */
  _bg_signals[CHANGED] = 
    g_signal_new ("changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (AwnBackgroundClass, changed),
                  NULL, NULL, 
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  _bg_signals[PADDING_CHANGED] =
    g_signal_new ("padding-changed",
                  G_OBJECT_CLASS_TYPE (obj_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (AwnBackgroundClass, padding_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
awn_background_refresh_pattern (AwnBackground *bg)
{
  if (bg->pattern != NULL)
  {
    cairo_surface_destroy (bg->pattern);
    bg->pattern = NULL;
  }

  if (bg->pattern_original)
  {
    gint w, h;
    w = gdk_pixbuf_get_width (bg->pattern_original);
    h = gdk_pixbuf_get_height (bg->pattern_original);

    if (FALSE) //if (bg->panel && GTK_WIDGET (bg->panel)->window)
    {
      // mhr3: using server-side pixmap seems to slow things down quite a lot
      // (especially on 3d and curved) - nvidia-only issue???
      GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (bg->panel));

      cairo_t *temp_cr = gdk_cairo_create (window);

      bg->pattern = cairo_surface_create_similar (cairo_get_target (temp_cr),
                                                  CAIRO_CONTENT_COLOR_ALPHA,
                                                  w, h);

      cairo_destroy (temp_cr);
    }
    else
    {
      bg->pattern = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
    }
    // copy the pixbuf to cairo surface
    cairo_t *cr = cairo_create (bg->pattern);

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    gdk_cairo_set_source_pixbuf (cr, bg->pattern_original, 0.0, 0.0);
    cairo_paint_with_alpha (cr, bg->pattern_alpha);

    cairo_destroy (cr);
  }
}

static void
awn_background_init (AwnBackground *bg)
{
  bg->g_step_1 = NULL;
  bg->g_step_2 = NULL;
  bg->g_histep_1 = NULL;
  bg->g_histep_2 = NULL;
  bg->border_color = NULL;
  bg->hilight_color = NULL;
  bg->sep_color = NULL;
  bg->needs_redraw = TRUE;
  bg->helper_surface = NULL;
  bg->cache_enabled = TRUE;
  bg->draw_glow = FALSE;
}

static void
awn_background_draw_glow (AwnBackground *bg, cairo_t *cr, 
                          GdkRectangle *area, gint rad,
                          GtkPositionType  position)
{
  gfloat x, y, width, height;
  gboolean non_null_draw;

  x = area->x - rad;
  y = area->y - rad;
  width = area->width + rad * 2.;
  height = area->height + rad * 2.;
  non_null_draw =
    AWN_BACKGROUND_GET_CLASS (bg)->draw != awn_background_draw_none;

  cairo_save (cr);
  /* Create a surface to apply the glow */
  cairo_surface_t *blur_srfc = cairo_image_surface_create
                                          (CAIRO_FORMAT_ARGB32,
                                           width,
                                           height);
  /* paint the shape mask */
  cairo_t *blur_ctx = cairo_create (blur_srfc);
  cairo_push_group (blur_ctx);
  if (non_null_draw)
  {
    cairo_set_source_surface (blur_ctx, cairo_get_target (cr), -x, -y);
    cairo_paint (blur_ctx);
  }
  else
  {
    cairo_translate (blur_ctx, -area->x + rad, -area->y + rad);
    awn_background_get_input_shape_mask (bg, blur_ctx, position, area);
  }
  cairo_pattern_t *pat = cairo_pop_group (blur_ctx);

  cairo_set_source (blur_ctx, pat);
  cairo_set_operator (blur_ctx, CAIRO_OPERATOR_SOURCE);
  cairo_paint (blur_ctx);
  GdkColor bg_color =
    gtk_widget_get_style (GTK_WIDGET (bg->panel))->bg[GTK_STATE_SELECTED];
  blur_surface_shadow_rgba (blur_srfc, width, height, MAX (1, rad),
                            bg_color.red / 256,
                            bg_color.green / 256,
                            bg_color.blue / 256,
                            2.5);
  if (non_null_draw)
  {
    cairo_set_source (blur_ctx, pat);
    cairo_set_operator (blur_ctx, CAIRO_OPERATOR_DEST_OUT);
    cairo_paint (blur_ctx);
  }
  cairo_pattern_destroy (pat);
  cairo_destroy (blur_ctx);

  cairo_set_source_surface (cr, blur_srfc, x, y);
  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
  /* paint the blur on original surface */
  cairo_paint (cr);
  cairo_surface_destroy (blur_srfc);
  cairo_restore (cr);
}

void 
awn_background_draw (AwnBackground  *bg,
                     cairo_t        *cr,
                     GtkPositionType  position,
                     GdkRectangle   *area)
{
  AwnBackgroundClass *klass;

  g_return_if_fail (AWN_IS_BACKGROUND (bg));
  
  klass = AWN_BACKGROUND_GET_CLASS (bg);
  g_return_if_fail (klass->draw != NULL);
  
  /* Check if background caching is enabled - TRUE by default */
  if (bg->cache_enabled)
  {
    g_return_if_fail (klass->get_needs_redraw != NULL);
    cairo_save (cr);
    
    /* Check if background needs to be redrawn */
    if (klass->get_needs_redraw (bg, position, area))
    {
      cairo_t *temp_cr;
      gint rad = awn_panel_get_glow_size (bg->panel);
      gint full_width = area->x + area->width + rad;
      gint full_height = area->y + area->height + rad;

      gboolean realloc_needed = bg->helper_surface == NULL ||
        cairo_image_surface_get_width (bg->helper_surface) != full_width ||
        cairo_image_surface_get_height (bg->helper_surface) != full_height;
      if (realloc_needed)
      {
        /* Free last surface */
        if (bg->helper_surface != NULL)
        {
          cairo_surface_destroy (bg->helper_surface);
        }
        /* Create new surface */
        bg->helper_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                         full_width,
                                                         full_height);
        temp_cr = cairo_create (bg->helper_surface);
      }
      else
      {
        temp_cr = cairo_create (bg->helper_surface);
        cairo_set_operator (temp_cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint (temp_cr);
        cairo_set_operator (temp_cr, CAIRO_OPERATOR_OVER);
      }
      /* Draw background on temp cairo_t */
      klass->draw (bg, temp_cr, position, area);
      if (bg->draw_glow && awn_panel_get_composited (bg->panel))
      {
        awn_background_draw_glow (bg, temp_cr, area, rad, position);
      }
      cairo_destroy (temp_cr);
    }
    /* Paint saved surface */
    cairo_set_source_surface (cr, bg->helper_surface, 0., 0.);
    cairo_paint(cr);
    cairo_restore (cr);
  }
  else
  {
    klass->draw (bg, cr, position, area);
  }
}

void 
awn_background_padding_request (AwnBackground *bg,
                                GtkPositionType position,
                                guint *padding_top,
                                guint *padding_bottom,
                                guint *padding_left,
                                guint *padding_right)
{
  AwnBackgroundClass *klass;

  g_return_if_fail (AWN_IS_BACKGROUND (bg) || bg == NULL);

  if (bg)
  {
    klass = AWN_BACKGROUND_GET_CLASS (bg);
    g_return_if_fail (klass->padding_request != NULL);

    klass->padding_request (bg, position, padding_top, padding_bottom,
                            padding_left, padding_right);
  }
  else
  {
    *padding_top = 0;
    *padding_bottom = 0;
    *padding_right = 0;
    *padding_left = 0;
  }
}

void 
awn_background_get_shape_mask (AwnBackground *bg,
                               cairo_t        *cr,
                               GtkPositionType  position,
                               GdkRectangle   *area)
{
  AwnBackgroundClass *klass;

  g_return_if_fail (AWN_IS_BACKGROUND (bg));
  
  klass = AWN_BACKGROUND_GET_CLASS (bg);
  g_return_if_fail (klass->get_shape_mask != NULL);

  klass->get_shape_mask (bg, cr, position, area);
}

void 
awn_background_get_input_shape_mask (AwnBackground *bg,
                                     cairo_t        *cr,
                                     GtkPositionType  position,
                                     GdkRectangle   *area)
{
  AwnBackgroundClass *klass;

  g_return_if_fail (AWN_IS_BACKGROUND (bg));
  
  klass = AWN_BACKGROUND_GET_CLASS (bg);
  g_return_if_fail (klass->get_input_shape_mask != NULL);

  klass->get_input_shape_mask (bg, cr, position, area);
}

AwnPathType
awn_background_get_path_type (AwnBackground *bg,
                              gfloat *offset_mod)
{
  AwnBackgroundClass *klass;

  g_return_val_if_fail (AWN_IS_BACKGROUND (bg), AWN_PATH_LINEAR);
  
  klass = AWN_BACKGROUND_GET_CLASS (bg);
  g_return_val_if_fail (klass->get_path_type, AWN_PATH_LINEAR);

  return klass->get_path_type (bg, offset_mod);
}

void
awn_background_get_strut_offsets (AwnBackground *bg,
                                  GtkPositionType position,
                                  GdkRectangle *area,
                                  gint *strut,
                                  gint *strut_start, gint *strut_end)
{
  AwnBackgroundClass *klass;

  g_return_if_fail (AWN_IS_BACKGROUND (bg));

  klass = AWN_BACKGROUND_GET_CLASS (bg);
  if (klass->get_strut_offsets == NULL) return;

  klass->get_strut_offsets (bg, position, area, strut, strut_start, strut_end);
}

void
awn_background_emit_padding_changed (AwnBackground *bg)
{
  g_return_if_fail (AWN_IS_BACKGROUND (bg));
  awn_background_invalidate (bg);
  g_signal_emit (bg, _bg_signals[PADDING_CHANGED], 0);
}

void
awn_background_emit_changed (AwnBackground *bg)
{
  g_return_if_fail (AWN_IS_BACKGROUND (bg));
  awn_background_invalidate (bg);
  g_signal_emit (bg, _bg_signals[CHANGED], 0);
}

gboolean
awn_background_do_rtl_swap (AwnBackground *bg)
{
  g_return_val_if_fail (AWN_IS_BACKGROUND (bg) && bg->panel, FALSE);

  if (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL)
  {
    GtkPositionType position;
    g_object_get (bg->panel, "position", &position, NULL);
    if (position == GTK_POS_TOP || position == GTK_POS_BOTTOM)
    {
      return TRUE;
    }
  }

  return FALSE;
}

gfloat
awn_background_get_panel_alignment (AwnBackground *bg)
{
  gfloat alignment = 0.5;
  g_return_val_if_fail (AWN_IS_BACKGROUND (bg) && bg->panel, alignment);

  gpointer monitor = NULL;
  g_object_get (bg->panel, "monitor", &monitor, NULL);
  g_return_val_if_fail (monitor, alignment);

  g_object_get (monitor, "monitor_align", &alignment, NULL);

  if (awn_background_do_rtl_swap (bg))
  {
    alignment = 1.0 - alignment;
  }

  return alignment;
}

/*
 * If we're in gtk theme mode, then load up the colours from the current gtk
 * theme
 */

/**
 * alpha: between 0 and 255. Multiplied by 256 to get the "proper" alpha value.
 */
static void
set_cfg_from_theme (GdkColor                    *color,
                    gushort                      alpha,
                    DesktopAgnosticConfigClient *client,
                    const gchar                 *group,
                    const gchar                 *key)
{
  DesktopAgnosticColor *da_color;
  GValue val = {0};

  da_color = desktop_agnostic_color_new (color, alpha * 256);
  g_value_init (&val, DESKTOP_AGNOSTIC_TYPE_COLOR);
  g_value_set_object (&val, da_color);
  desktop_agnostic_config_client_set_value (client,
                                            group, key,
                                            &val, NULL);
  g_value_unset (&val);
  g_object_unref (da_color);
}

static void
load_colours_from_widget (AwnBackground *bg, GtkWidget *widget)
{
  DesktopAgnosticConfigClient *client = bg->client;
  GtkStyle        *style;

  // try to get values which are set for the Panel, so we look like panel
  GtkSettings *settings = gtk_settings_get_default ();
  style = gtk_rc_get_style_by_paths (settings, "PanelWidget", "", G_TYPE_NONE);

  if (style == NULL) style = gtk_widget_get_style (widget);

  g_debug ("Updating gtk theme colours");

  /* main colours */
  set_cfg_from_theme (&style->bg[GTK_STATE_NORMAL], 224,
                      client, AWN_GROUP_THEME, AWN_THEME_GSTEP1);
  set_cfg_from_theme (&style->bg[GTK_STATE_NORMAL], 200,
                      client, AWN_GROUP_THEME, AWN_THEME_GSTEP2);

  set_cfg_from_theme (&style->bg[GTK_STATE_ACTIVE], 180,
                      client, AWN_GROUP_THEME, AWN_THEME_GHISTEP1);
  set_cfg_from_theme (&style->bg[GTK_STATE_NORMAL], 32,
                      client, AWN_GROUP_THEME, AWN_THEME_GHISTEP2);

  set_cfg_from_theme (&style->dark[GTK_STATE_ACTIVE], 200,
                      client, AWN_GROUP_THEME, AWN_THEME_BORDER);
  set_cfg_from_theme (&style->base[GTK_STATE_ACTIVE], 100,
                      client, AWN_GROUP_THEME, AWN_THEME_HILIGHT);

  set_cfg_from_theme (&style->base[GTK_STATE_ACTIVE], 164,
                      client, AWN_GROUP_THEME, AWN_THEME_SEP_COLOR);

  set_cfg_from_theme (&style->fg[GTK_STATE_NORMAL], 255,
                      client, AWN_GROUP_THEME, AWN_THEME_TEXT_COLOR);
  set_cfg_from_theme (&style->bg[GTK_STATE_NORMAL], 255,
                      client, AWN_GROUP_THEME, AWN_THEME_OUTLINE_COLOR);

  /* now colors from standard (non-panel) theme */
  style = gtk_widget_get_style (widget);

  set_cfg_from_theme (&style->light[GTK_STATE_SELECTED], 255,
                      client, AWN_GROUP_EFFECTS, AWN_EFFECTS_DOT_COLOR);
  set_cfg_from_theme (&style->bg[GTK_STATE_SELECTED], 127,
                      client, AWN_GROUP_EFFECTS, AWN_EFFECTS_RECT_COLOR);
  set_cfg_from_theme (&style->light[GTK_STATE_SELECTED], 0,
                      client, AWN_GROUP_EFFECTS, AWN_EFFECTS_RECT_OUTLINE);

  /* Don't draw patterns */
  desktop_agnostic_config_client_set_bool (client,
                                           AWN_GROUP_THEME,
                                           AWN_THEME_DRAW_PATTERN,
                                           FALSE, NULL);

  /* Set up separators to draw in the standard way */
  desktop_agnostic_config_client_set_bool (client,
                                           AWN_GROUP_THEME,
                                           AWN_THEME_SHOW_SEP,
                                           TRUE, NULL);

  /* Misc settings */
}

static void
load_dlg_colours_from_widget (AwnBackground *bg, GtkWidget *widget)
{
  DesktopAgnosticConfigClient *client = bg->client;
  GtkStyle        *style;

  GtkSettings *settings = gtk_settings_get_default ();
  style = gtk_rc_get_style_by_paths (settings, "AwnDialog", "", G_TYPE_NONE);

  if (style == NULL) style = gtk_widget_get_style (widget);

  g_debug ("Updating dialog colours");

  /* Set colors for AwnDialog */
  set_cfg_from_theme (&style->bg[GTK_STATE_NORMAL], 255,
                      client, AWN_GROUP_THEME, AWN_THEME_DLG_BG);
  set_cfg_from_theme (&style->bg[GTK_STATE_PRELIGHT], 255,
                      client, AWN_GROUP_THEME, AWN_THEME_DLG_TITLE_BG);
}

static void
update_widget_colors (GtkWidget *widget, AwnBackground *bg)
{
  if (bg->gtk_theme_mode)
  {
    load_colours_from_widget (bg, widget);
  }
  if (bg->dialog_gtk_mode)
  {
    load_dlg_colours_from_widget (bg, widget);
  }
}

static void
on_style_set (GtkWidget *widget, GtkStyle *old, AwnBackground *bg)
{
  update_widget_colors (widget, bg);
  awn_background_invalidate (bg);
}

static void
awn_background_set_dialog_gtk_mode (AwnBackground *bg, gboolean gtk_mode)
{
  bg->dialog_gtk_mode = gtk_mode;
  if (gtk_mode)
  {
    load_dlg_colours_from_widget (bg, GTK_WIDGET (bg->panel));
  }
}

static void 
awn_background_set_gtk_theme_mode (AwnBackground *bg, gboolean gtk_mode)
{
  bg->gtk_theme_mode = gtk_mode;

  GtkWidget *widget = GTK_WIDGET (bg->panel);

  if (gtk_mode)
  {
    if (gtk_widget_get_realized (GTK_WIDGET (widget)))
    {
      load_colours_from_widget (bg, widget);
    }
    else
    {
      g_signal_connect (widget, "realize",
                        G_CALLBACK (update_widget_colors), bg);
    }
  }

  if (bg->changed == 0)
  {
    bg->changed = g_signal_connect (widget, "style-set",
                                    G_CALLBACK (on_style_set), bg);
  }
}

static void awn_background_padding_zero(AwnBackground *bg,
                                        GtkPositionType position,
                                        guint *padding_top,
                                        guint *padding_bottom,
                                        guint *padding_left,
                                        guint *padding_right)
{
  *padding_top  = 0; *padding_bottom = 0;
  *padding_left = 0; *padding_right  = 0;
}

static void awn_background_mask_none (AwnBackground *bg,
                                      cairo_t        *cr,
                                      GtkPositionType  position,
                                      GdkRectangle   *area)
{
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, area->x, area->y, area->width, area->height);
  cairo_fill (cr);
}

static void
awn_background_draw_none   (AwnBackground  *bg,
                            cairo_t        *cr,
                            GtkPositionType  position,
                            GdkRectangle   *area)
{

}

gboolean awn_background_get_glow (AwnBackground *bg)
{
  g_return_val_if_fail (AWN_IS_BACKGROUND (bg), FALSE);

  return bg->draw_glow;
}

void awn_background_set_glow (AwnBackground *bg, gboolean activate)
{
  g_return_if_fail (AWN_IS_BACKGROUND (bg));

  if (bg->draw_glow != activate)
  {
    bg->draw_glow = activate;
    awn_background_invalidate (bg);
    awn_background_emit_changed (bg);
  }
}

static AwnPathType awn_background_path_default (AwnBackground *bg,
                                                gfloat *offset_mod)
{
  return AWN_PATH_LINEAR;
}

static gboolean awn_background_get_needs_redraw (AwnBackground *bg,
                                                 GtkPositionType position,
                                                 GdkRectangle *area)
{
  if (bg->needs_redraw)
  {
    bg->needs_redraw = 0;
    return TRUE;
  }
  return FALSE;
}

void awn_background_invalidate (AwnBackground  *bg)
{
  bg->needs_redraw = 1;
}

/* vim: set et ts=2 sts=2 sw=2 : */
