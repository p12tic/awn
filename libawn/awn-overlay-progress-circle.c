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
 * SECTION: awn-overlay-progress-circle
 * @short_description: Circle Progress overlay for use with #AwnIcon.
 * @see_also: #AwnEffects, #AwnOverlay, #AwnOverlayThemedIcon, #AwnOverlayThrobber,
 * #AwnOverlayPixbuf, #AwnOverlayPixbufFile, #AwnIcon, #AwnOverlayProgress
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Circle Progress overlay used with #AwnIcon.
 */

/**
 * AwnOverlayProgressCircle:
 *
 * Circle Progress overlay used with #AwnIcon.
 */

 /* awn-overlay-progress-circle.c */

#include <math.h>
#include "awn-cairo-utils.h"
#include "awn-overlay-progress-circle.h"

G_DEFINE_TYPE (AwnOverlayProgressCircle, awn_overlay_progress_circle, AWN_TYPE_OVERLAY_PROGRESS)

#define AWN_OVERLAY_PROGRESS_CIRCLE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_PROGRESS_CIRCLE, AwnOverlayProgressCirclePrivate))

typedef struct _AwnOverlayProgressCirclePrivate AwnOverlayProgressCirclePrivate;

struct _AwnOverlayProgressCirclePrivate 
{
  DesktopAgnosticColor * bg_color;
  DesktopAgnosticColor * fg_color;
  DesktopAgnosticColor * outline_color;  
  
  gdouble scale;
};

enum
{
  PROP_0,
  PROP_SCALE,
  PROP_BACKGROUND_COLOR,
  PROP_FOREGROUND_COLOR,
  PROP_OUTLINE_COLOR
};

static void 
_awn_overlay_progress_circle_render (AwnOverlay* _overlay,
                          GtkWidget *widget,
                          cairo_t * cr,
                          gint width,
                          gint height);

static void
awn_overlay_progress_circle_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayProgressCirclePrivate * priv = AWN_OVERLAY_PROGRESS_CIRCLE_GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_SCALE:
      g_value_set_double (value,priv->scale);
      break;       
    case PROP_FOREGROUND_COLOR:
      g_value_set_object (value,priv->fg_color);
      break;   
    case PROP_BACKGROUND_COLOR:
      g_value_set_object (value,priv->bg_color);
      break;   
    case PROP_OUTLINE_COLOR:
      g_value_set_object (value,priv->outline_color);
      break;         
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_progress_circle_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayProgressCirclePrivate * priv = AWN_OVERLAY_PROGRESS_CIRCLE_GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_SCALE:
      priv->scale = g_value_get_double (value);
      break;    
    case PROP_FOREGROUND_COLOR:
      if (priv->fg_color)
      {
        g_object_unref (priv->fg_color);
      }
      priv->fg_color = g_value_get_object (value);
      break;
    case PROP_BACKGROUND_COLOR:
      if (priv->bg_color)
      {
        g_object_unref (priv->bg_color);
      }
      priv->bg_color = g_value_get_object (value);
      break;
    case PROP_OUTLINE_COLOR:
      if (priv->outline_color)
      {
        g_object_unref (priv->outline_color);
      }
      priv->outline_color = g_value_get_object (value);
      break;    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_progress_circle_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_progress_circle_parent_class)->dispose (object);
}

static void
awn_overlay_progress_circle_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_progress_circle_parent_class)->finalize (object);
}

static void
awn_overlay_progress_circle_class_init (AwnOverlayProgressCircleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = awn_overlay_progress_circle_get_property;
  object_class->set_property = awn_overlay_progress_circle_set_property;
  object_class->dispose = awn_overlay_progress_circle_dispose;
  object_class->finalize = awn_overlay_progress_circle_finalize;
  
  AWN_OVERLAY_CLASS(klass)->render = _awn_overlay_progress_circle_render;  
  
/**
 * AwnOverlayProgressCircle:scale:
 *
 * The desired size of the overlay scaled to the Icon.  Range 0.0...1.0. Default
 * value of 0.9
 */        
  
  g_object_class_install_property (object_class,
    PROP_SCALE,
    g_param_spec_double ("scale",
                         "Scale",
                         "Scale",
                         0.0, 1.0, 0.9,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayProgressCircle:background-color:
 *
 * Overlay background color of type #DesktopAgnosticColor.
 */        
  
  g_object_class_install_property (object_class,
    PROP_BACKGROUND_COLOR,
    g_param_spec_object ("background-color",
                         "Background Color",
                         "Background Color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayProgressCircle:foreground-color:
 *
 * Overlay foregroundground color of type #DesktopAgnosticColor.
 */        
  
  g_object_class_install_property (object_class,
    PROP_FOREGROUND_COLOR,
    g_param_spec_object ("foreground-color",
                         "Foreground Color",
                         "Foreground Color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

/**
 * AwnOverlayProgressCircle:outline-color:
 *
 * Overlay outline color of type #DesktopAgnosticColor.
 */        
  
  g_object_class_install_property (object_class,
    PROP_OUTLINE_COLOR,
    g_param_spec_object ("outline-color",
                         "Outline Color",
                         "Outline Color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));
  
  g_type_class_add_private (klass, sizeof (AwnOverlayProgressCirclePrivate));  
}

static void
awn_overlay_progress_circle_init (AwnOverlayProgressCircle *self)
{
}

/**
 * awn_overlay_progress_circle_new:
 *
 * Creates a new instance of #AwnOverlayProgressCircle.
 * Returns: an instance of #AwnOverlayProgressCircle.
 */
AwnOverlayProgressCircle*
awn_overlay_progress_circle_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_PROGRESS_CIRCLE, NULL);
}

static void 
_awn_overlay_progress_circle_render (AwnOverlay* _overlay,
                          GtkWidget *widget,
                          cairo_t * cr,
                          gint width,
                          gint height)
{
  AwnOverlayProgressCircle * overlay = AWN_OVERLAY_PROGRESS_CIRCLE(_overlay);
  AwnOverlayProgressCirclePrivate *priv;
  DesktopAgnosticColor * fg_color;
  DesktopAgnosticColor * bg_color;
  DesktopAgnosticColor * outline_color;  
  gdouble percent_complete, x_pos, y_pos;
  AwnOverlayCoord coord;

  priv =  AWN_OVERLAY_PROGRESS_CIRCLE_GET_PRIVATE (overlay); 

  g_object_get (overlay,
                "percent-complete",&percent_complete,
                NULL);
  if (priv->fg_color)
  {
    fg_color = priv->fg_color;
    g_object_ref (fg_color);
  }
  else
  {
    fg_color = desktop_agnostic_color_new(&widget->style->bg[GTK_STATE_ACTIVE], 
                                          0.7*G_MAXUSHORT);
  }
  if (priv->bg_color)
  {
    bg_color = priv->bg_color;
    g_object_ref (bg_color);
  }
  else
  {
    bg_color = desktop_agnostic_color_new(&widget->style->fg[GTK_STATE_ACTIVE],
                                          0.2 * G_MAXUSHORT);
  }
  if (priv->outline_color)
  {
    outline_color = priv->outline_color;
    g_object_ref (outline_color);
  }
  else
  {
    outline_color = desktop_agnostic_color_new(&widget->style->fg[GTK_STATE_ACTIVE], 
                                               G_MAXUSHORT);
  }

  awn_overlay_move_to (_overlay, cr,  width, height,
                       width * priv->scale, height * priv->scale,
                       &coord);
  x_pos = coord.x / width + priv->scale / 2.0;
  y_pos = coord.y / height + priv->scale / 2.0;

  cairo_save (cr);
  cairo_scale (cr, width, height);
  cairo_set_line_width (cr, 2. / (width + height));

  awn_cairo_set_source_color (cr,bg_color);
  cairo_arc (cr, x_pos, y_pos, priv->scale/2.0, 0, 2 * M_PI);
  cairo_fill (cr);

  cairo_arc (cr, x_pos, y_pos, priv->scale/2.0, -0.5 * M_PI, 
             2 * (percent_complete/100.0) * M_PI -0.5 * M_PI );
  cairo_line_to (cr, x_pos, y_pos);
  cairo_close_path (cr);
  awn_cairo_set_source_color (cr,fg_color);
  cairo_fill (cr);

  if (percent_complete > 0)
  {
    cairo_arc (cr, x_pos, y_pos, priv->scale/2.0, -0.5 * M_PI,
               2 * (percent_complete/100.0) * M_PI -0.5 * M_PI );

    if (percent_complete < 100)
    {
      cairo_line_to (cr, x_pos, y_pos);
      cairo_close_path (cr);
    }
    awn_cairo_set_source_color (cr, outline_color);
    cairo_stroke (cr);
  }

  cairo_restore (cr);
  g_object_unref (fg_color);
  g_object_unref (bg_color);
  g_object_unref (outline_color);
}
