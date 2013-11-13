/*
 * Copyright (C) 2009-2010 Michal Hruby <michal.mhr@gmail.com>
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
 * Authored by Michal Hruby <michal.mhr@gmail.com>
 *
 */
#include <gdk/gdkx.h>
#include <math.h>

#include <libawn/awn-utils.h>
#include <libawn/awn-cairo-utils.h>
#include <libawn/awn-overlayable.h>

#include "awn-defines.h"
#include "awn-throbber.h"

#include "libawn/gseal-transition.h"

G_DEFINE_TYPE (AwnThrobber, awn_throbber, AWN_TYPE_ICON)

#define AWN_THROBBER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_THROBBER, \
  AwnThrobberPrivate))

struct _AwnThrobberPrivate
{
  AwnThrobberType type;
  gint size;

  gint        counter;
  guint       timer_id;

  DesktopAgnosticConfigClient *client;
  DesktopAgnosticColor *fill_color;
  DesktopAgnosticColor *outline_color;
};

enum
{
  PROP_0,

  PROP_CLIENT,
  PROP_FILL_COLOR,
  PROP_OUTLINE_COLOR
};

/* GObject stuff */
static void
awn_throbber_dispose (GObject *object)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE (object);

  if (priv->timer_id)
  {
    g_source_remove(priv->timer_id);
    priv->timer_id = 0;
  }

  G_OBJECT_CLASS (awn_throbber_parent_class)->dispose (object);
}

static void
awn_throbber_finalize (GObject *object)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE (object);

  if (priv->client)
  {
    desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                          object, NULL);
  }

  if (priv->fill_color)
  {
    g_object_unref (priv->fill_color);
  }

  if (priv->outline_color)
  {
    g_object_unref (priv->outline_color);
  }

  G_OBJECT_CLASS (awn_throbber_parent_class)->finalize (object);
}

static void
awn_throbber_constructed (GObject *object)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE (object);

  g_return_if_fail (priv->client);

  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_THEME, AWN_THEME_TEXT_COLOR,
                                       object, "fill-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);

  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_THEME, AWN_THEME_OUTLINE_COLOR,
                                       object, "outline-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
}

static void
awn_throbber_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  AwnThrobberPrivate *priv;

  g_return_if_fail (AWN_IS_THROBBER (object));
  priv = AWN_THROBBER (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      g_value_set_object (value, priv->client);
      break;
    case PROP_FILL_COLOR:
      g_value_set_object (value, priv->fill_color);
      break;
    case PROP_OUTLINE_COLOR:
      g_value_set_object (value, priv->outline_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_throbber_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  AwnThrobberPrivate *priv;

  g_return_if_fail (AWN_IS_THROBBER (object));
  priv = AWN_THROBBER (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      priv->client = g_value_get_object (value);
      break;
    case PROP_FILL_COLOR:
      if (priv->fill_color)
      {
        g_object_unref (priv->fill_color);
      }
      priv->fill_color = g_value_dup_object (value);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_OUTLINE_COLOR:
      if (priv->outline_color)
      {
        g_object_unref (priv->outline_color);
      }
      priv->outline_color = g_value_dup_object (value);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
paint_sad_face (cairo_t *cr)
{
  /* sad face */
  const gfloat EYE_SIZE = 0.04;
  gfloat EYE_POS_X, EYE_POS_Y;

  awn_cairo_rounded_rect(cr, 0.05, 0.05, 0.9, 0.9, 0.05, ROUND_ALL);
  cairo_stroke(cr);

  #define MINI_RECT(cr, x, y) \
    cairo_rectangle (cr, x, y, EYE_SIZE, EYE_SIZE); \
    cairo_fill (cr);

  EYE_POS_X = 0.25;
  EYE_POS_Y = 0.30;

  /* left eye */
  MINI_RECT(cr, EYE_POS_X, EYE_POS_Y);
  MINI_RECT(cr, EYE_POS_X, EYE_POS_Y + 2*EYE_SIZE);
  MINI_RECT(cr, EYE_POS_X + EYE_SIZE, EYE_POS_Y + EYE_SIZE);
  MINI_RECT(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y);
  MINI_RECT(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y + 2*EYE_SIZE);

  /* right eye */
  EYE_POS_X = 0.65;

  MINI_RECT(cr, EYE_POS_X, EYE_POS_Y);
  MINI_RECT(cr, EYE_POS_X, EYE_POS_Y + 2*EYE_SIZE);
  MINI_RECT(cr, EYE_POS_X + EYE_SIZE, EYE_POS_Y + EYE_SIZE);
  MINI_RECT(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y);
  MINI_RECT(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y + 2*EYE_SIZE);

  #undef MINI_RECT

  /* nose */
  cairo_curve_to(cr, 0.45, 0.48,
                     0.5, 0.53,
                     0.55, 0.48);
  cairo_stroke(cr);

  /* mouth */
  cairo_curve_to(cr, 0.25, 0.73,
                     0.5, 0.62,
                     0.77, 0.77);
  cairo_stroke(cr);

}

static gboolean
awn_throbber_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  AwnThrobberPrivate *priv = AWN_THROBBER (widget)->priv;
  cairo_t *cr;
  gint w, h;
  GtkPositionType pos_type;

  /* clip the drawing region, nvidia likes it */
  AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (widget));
  cr = awn_effects_cairo_create_clipped (fx, event);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // we'll paint to [0,0] - [1,1], so scale's needed
  g_object_get (G_OBJECT (widget), "icon-width", &w, "icon-height", &h, NULL);
  cairo_scale (cr, w, h);

  switch (priv->type)
  {
    case AWN_THROBBER_TYPE_NORMAL:
    {
      const gdouble RADIUS = 0.0625;
      const gdouble DIST = 0.3;
      const gdouble OTHER = DIST * 0.707106781; /* sqrt(2)/2 */
      const gint COUNT = 8;
      const gint counter = priv->counter;

      cairo_set_line_width (cr, 1. / priv->size);
      cairo_translate (cr, 0.5, 0.5);
      cairo_scale (cr, 1, -1);

      #define PAINT_CIRCLE(cr, x, y, cnt) \
        awn_cairo_set_source_color_with_alpha_multiplier (cr, \
            priv->fill_color, ((counter+cnt) % COUNT) / (float)COUNT); \
        cairo_arc (cr, x, y, RADIUS, 0, 2*M_PI); \
        cairo_fill_preserve (cr); \
        awn_cairo_set_source_color_with_alpha_multiplier (cr, \
            priv->outline_color, ((counter+cnt) % COUNT) / (float)COUNT); \
        cairo_stroke (cr);

      PAINT_CIRCLE (cr, 0, DIST, 0);
      PAINT_CIRCLE (cr, OTHER, OTHER, 1);
      PAINT_CIRCLE (cr, DIST, 0, 2);
      PAINT_CIRCLE (cr, OTHER, -OTHER, 3);
      PAINT_CIRCLE (cr, 0, -DIST, 4);
      PAINT_CIRCLE (cr, -OTHER, -OTHER, 5);
      PAINT_CIRCLE (cr, -DIST, 0, 6);
      PAINT_CIRCLE (cr, -OTHER, OTHER, 7);

      #undef PAINT_CIRCLE

      break;
    }
    case AWN_THROBBER_TYPE_SAD_FACE:
    {
      cairo_set_line_width (cr, 0.03);

      if (priv->fill_color == NULL)
      {
        GdkColor c;
        double r, g, b;

        c = gtk_widget_get_style (widget)->fg[GTK_STATE_NORMAL];
        r = c.red / 65535.0;
        g = c.green / 65535.0;
        b = c.blue / 65535.0;

        cairo_set_source_rgb (cr, r, g, b);
      }
      else
      {
        awn_cairo_set_source_color_with_alpha_multiplier (cr,
                                                          priv->fill_color,
                                                          0.75);
      }
      
      paint_sad_face (cr);

      break;
    }
    case AWN_THROBBER_TYPE_ARROW_1:
      cairo_rotate (cr, M_PI);
      cairo_translate (cr, -1.0, -1.0);
      // no break
    case AWN_THROBBER_TYPE_ARROW_2:
    {
      pos_type = awn_icon_get_pos_type (AWN_ICON (widget));

      if (pos_type == GTK_POS_LEFT || pos_type == GTK_POS_RIGHT)
      {
        cairo_rotate (cr, 0.5 * M_PI);
        cairo_translate (cr, 0.0, -1.0);
      }

      if (priv->outline_color)
      {
        cairo_set_line_width (cr, 3.5 / priv->size);
        awn_cairo_set_source_color (cr, priv->outline_color);

        cairo_move_to (cr, 0.125, 0.375);
        cairo_line_to (cr, 0.875, 0.5);
        cairo_line_to (cr, 0.125, 0.625);
        cairo_stroke (cr);
      }

      cairo_set_line_width (cr, 1.75 / priv->size);
      
      if (priv->fill_color == NULL)
      {
        GdkColor c = gtk_widget_get_style (widget)->fg[GTK_STATE_NORMAL];
        double r = c.red / 65535.0;
        double g = c.green / 65535.0;
        double b = c.blue / 65535.0;

        cairo_set_source_rgb (cr, r, g, b);
      }
      else
      {
        awn_cairo_set_source_color (cr, priv->fill_color);
      }

      cairo_move_to (cr, 0.125, 0.375);
      cairo_line_to (cr, 0.875, 0.5);
      cairo_line_to (cr, 0.125, 0.625);
      cairo_stroke (cr);

      break;
    }
    case AWN_THROBBER_TYPE_CLOSE_BUTTON:
    {
      cairo_set_line_width (cr, 1. / priv->size);

      cairo_pattern_t *pat = cairo_pattern_create_linear (0.0, 0.0, 0.5, 1.0);
      cairo_pattern_add_color_stop_rgb (pat, 0.0, 0.97254, 0.643137, 0.403921);
      //cairo_pattern_add_color_stop_rgb (pat, 0.0, 0.0, 0.0, 0.0); // test
      cairo_pattern_add_color_stop_rgb (pat, 0.7, 0.98823, 0.4, 0.0);
      cairo_pattern_add_color_stop_rgb (pat, 1.0, 0.98823, 0.4, 0.0);

      cairo_set_source (cr, pat);
      //cairo_arc (cr, 0.5, 0.5, 0.4, 0.0, 2 * M_PI);
      awn_cairo_rounded_rect (cr, 0.1, 0.1, 0.8, 0.8, 0.15, ROUND_ALL);
      cairo_fill_preserve (cr);

      cairo_pattern_destroy (pat);

      cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
      cairo_stroke (cr);

      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_set_line_width (cr, 2./priv->size);

      /*  \  starting with the upper point  */
      cairo_move_to (cr, 0.3, 0.25);
      cairo_line_to (cr, 0.7, 0.75);
      cairo_stroke (cr);
      /*  /  starting with the upper point  */
      cairo_move_to (cr, 0.7, 0.25);
      cairo_line_to (cr, 0.3, 0.75);
      cairo_stroke (cr);
      break;
    }
    default:
      break;
  }

  /* let effects know we're finished */
  awn_effects_cairo_destroy (fx);

  return TRUE;
}

static gboolean
awn_throbber_timeout (gpointer user_data)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE (user_data);

  priv->counter = (priv->counter - 1) % 8 + 8;

  AwnOverlayable *overlayable = AWN_OVERLAYABLE (user_data);
  awn_effects_redraw (awn_overlayable_get_effects (overlayable));

  return TRUE;
}

static void
awn_throbber_show (GtkWidget *widget, gpointer user_data)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE(widget);

  if (!priv->timer_id && priv->type == AWN_THROBBER_TYPE_NORMAL)
  {
    // we want lower prio than HIGH_IDLE
    priv->timer_id = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 
                                         100, awn_throbber_timeout, 
                                         widget, NULL);
  }
}

static void
awn_throbber_hide (GtkWidget *widget, gpointer user_data)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE(widget);

  if (priv->timer_id)
  {
    g_source_remove(priv->timer_id);
    priv->timer_id = 0;
  }
}

static void
awn_throbber_class_init (AwnThrobberClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = awn_throbber_dispose;
  object_class->finalize     = awn_throbber_finalize;
  object_class->constructed  = awn_throbber_constructed;
  object_class->get_property = awn_throbber_get_property;
  object_class->set_property = awn_throbber_set_property;

  wid_class->expose_event = awn_throbber_expose_event;

  /* Add properties to the class */
  g_object_class_install_property (object_class,
    PROP_CLIENT,
    g_param_spec_object ("client",
                         "Client",
                         "The configuration client",
                         DESKTOP_AGNOSTIC_CONFIG_TYPE_CLIENT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_FILL_COLOR,
    g_param_spec_object ("fill-color",
                         "Fill Color",
                         "The fill color for the throbber",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_OUTLINE_COLOR,
    g_param_spec_object ("outline-color",
                         "Outline Color",
                         "The outline color for the throbber",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (object_class, sizeof (AwnThrobberPrivate));
}

static void
awn_throbber_init (AwnThrobber *throbber)
{
  AwnThrobberPrivate *priv;

  priv = throbber->priv = AWN_THROBBER_GET_PRIVATE (throbber);

  priv->size = 50;
  priv->counter = 0;
  priv->type = AWN_THROBBER_TYPE_NORMAL;

  g_object_set (awn_overlayable_get_effects (AWN_OVERLAYABLE (throbber)),
                "effects", 0,
                "reflection-visible", FALSE,
                NULL);

  gtk_widget_add_events (GTK_WIDGET (throbber), GDK_ALL_EVENTS_MASK);

  g_signal_connect (throbber, "show",
                    G_CALLBACK (awn_throbber_show), NULL);
  g_signal_connect (throbber, "hide",
                    G_CALLBACK (awn_throbber_hide), NULL);
}

GtkWidget *
awn_throbber_new (void)
{
  GtkWidget *throbber = NULL;

  throbber = g_object_new (AWN_TYPE_THROBBER, "bind-effects", FALSE, NULL);

  return throbber;
}

GtkWidget*
awn_throbber_new_with_config (DesktopAgnosticConfigClient *client)
{
  GtkWidget *throbber = NULL;

  throbber = g_object_new (AWN_TYPE_THROBBER, 
                           "bind-effects", FALSE,
                           "client", client, NULL);

  return throbber;
}

void
awn_throbber_set_type (AwnThrobber *throbber, AwnThrobberType type)
{
  g_return_if_fail (AWN_IS_THROBBER (throbber));
  gboolean needs_redraw = FALSE;

  AwnThrobberPrivate *priv = throbber->priv;

  if (type != priv->type) needs_redraw = TRUE;

  priv->type = type;
  switch (type)
  {
    case AWN_THROBBER_TYPE_NORMAL:
      if (!priv->timer_id && gtk_widget_get_mapped (GTK_WIDGET (throbber)))
      {
        // we want lower prio than HIGH_IDLE
        priv->timer_id = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 
                                             100, awn_throbber_timeout, 
                                             throbber, NULL);
      }
      break;
    case AWN_THROBBER_TYPE_CLOSE_BUTTON:
      g_object_set (awn_overlayable_get_effects (AWN_OVERLAYABLE (throbber)),
                    "make-shadow", TRUE, NULL);
      // no break;
    default:
      if (priv->timer_id)
      {
        g_source_remove (priv->timer_id);
        priv->timer_id = 0;
      }
      break;
  }

  if (needs_redraw)
  {
    gtk_widget_queue_draw (GTK_WIDGET (throbber));
  }
}

void
awn_throbber_set_size (AwnThrobber *throbber, gint size)
{
  gboolean is_horizontal;
  GtkPositionType pos_type;
  g_return_if_fail (AWN_IS_THROBBER (throbber));

  AwnThrobberPrivate *priv = throbber->priv;

  switch (priv->type)
  {
    case AWN_THROBBER_TYPE_ARROW_1:
    case AWN_THROBBER_TYPE_ARROW_2:
      pos_type = awn_icon_get_pos_type (AWN_ICON (throbber));
      is_horizontal = pos_type == GTK_POS_TOP || pos_type == GTK_POS_BOTTOM;
      if (is_horizontal)
      {
        awn_icon_set_custom_paint (AWN_ICON (throbber), size / 4, size);
      }
      else
      {
        awn_icon_set_custom_paint (AWN_ICON (throbber), size, size / 4);
      }
      priv->size = size;
      break;
    case AWN_THROBBER_TYPE_CLOSE_BUTTON:
      awn_icon_set_custom_paint (AWN_ICON (throbber), size / 2, size / 2);
      priv->size = size / 2;
      break;
    default:
      awn_icon_set_custom_paint (AWN_ICON (throbber), size, size);
      priv->size = size;
      break;
  }

  gtk_widget_queue_resize (GTK_WIDGET (throbber));
}

