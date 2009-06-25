/*
 * Copyright (C) 2009 Michal Hruby
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

#include "awn-throbber.h"
#include <libawn/awn-utils.h>
#include <libawn/awn-cairo-utils.h>

#define APPLY_SIZE_MULTIPLIER(x)	(x)*6/5

G_DEFINE_TYPE (AwnThrobber, awn_throbber, GTK_TYPE_DRAWING_AREA)

#define AWN_THROBBER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_THROBBER, \
  AwnThrobberPrivate))

struct _AwnThrobberPrivate
{
  AwnEffects     *effects;
  GtkWidget      *tooltip;
  AwnOrientation  orient;
  AwnThrobberType type;
  gint            offset;
  gint            size;

  gint        counter;
  guint       timer_id;
};

/* GObject stuff */
static void
awn_throbber_dispose (GObject *object)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE (object);

  if (priv->tooltip)
  {
    gtk_widget_destroy (priv->tooltip);
    priv->tooltip = NULL;
  }

  if (priv->effects)
  {
    g_object_unref (priv->effects);
    priv->effects = NULL;
  }

  if (priv->timer_id)
  {
    g_source_remove(priv->timer_id);
    priv->timer_id = 0;
  }

  G_OBJECT_CLASS (awn_throbber_parent_class)->dispose (object);
}

static gboolean
awn_throbber_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  AwnThrobberPrivate *priv = AWN_THROBBER (widget)->priv;
  cairo_t *cr;

  /* clip the drawing region, nvidia likes it */
  cr = awn_effects_cairo_create_clipped (priv->effects, event->region);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  /* TODO: translate to event->area
   * we'll paint to [0,0] - [1,1], so scale's needed
   */
  cairo_scale(cr, priv->size, priv->size);

  switch (priv->type)
  {
    case AWN_THROBBER_TYPE_NORMAL:
    {
      const gdouble RADIUS = 0.0625;
      const gdouble DIST = 0.3;
      const gdouble OTHER = DIST * 0.707106781; /* sqrt(2)/2 */
      const gint COUNT = 8;
      const gint counter = priv->counter;

      cairo_translate(cr, 0.5, 0.5);
      cairo_scale(cr, 1, -1);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+0) % COUNT) / (float)COUNT);
      cairo_arc(cr, 0, DIST, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+1) % COUNT) / (float)COUNT);
      cairo_arc(cr, OTHER, OTHER, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+2) % COUNT) / (float)COUNT);
      cairo_arc(cr, DIST, 0, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+3) % COUNT) / (float)COUNT);
      cairo_arc(cr, OTHER, -OTHER, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+4) % COUNT) / (float)COUNT);
      cairo_arc(cr, 0, -DIST, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+5) % COUNT) / (float)COUNT);
      cairo_arc(cr, -OTHER, -OTHER, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+6) % COUNT) / (float)COUNT);
      cairo_arc(cr, -DIST, 0, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);

      cairo_set_source_rgba(cr, 1, 1, 1, ((counter+7) % COUNT) / (float)COUNT);
      cairo_arc(cr, -OTHER, OTHER, RADIUS, 0, 2*M_PI);
      cairo_fill(cr);
      break;
    }
    case AWN_THROBBER_TYPE_SAD_FACE:
    {
      const gfloat EYE_SIZE = 0.04;

      GdkColor c = gtk_widget_get_style (widget)->fg[GTK_STATE_NORMAL];
      double r = c.red / 65535.0;
      double g = c.green / 65535.0;
      double b = c.blue / 65535.0;

      gfloat EYE_POS_X, EYE_POS_Y;
      /* sad face */
      cairo_set_source_rgb(cr, r, g, b);
      cairo_set_line_width(cr, 0.03);

      awn_cairo_rounded_rect(cr, 0.05, 0.05, 0.9, 0.9, 0.05, ROUND_ALL);
      cairo_stroke(cr);

      EYE_POS_X = 0.25;
      EYE_POS_Y = 0.30;
      /* left eye */
      cairo_rectangle(cr, EYE_POS_X, EYE_POS_Y, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X, EYE_POS_Y + 2*EYE_SIZE, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X + EYE_SIZE, EYE_POS_Y + EYE_SIZE, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y + 2*EYE_SIZE, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);

      EYE_POS_X = 0.65;
      /* right eye */
      cairo_rectangle(cr, EYE_POS_X, EYE_POS_Y, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X, EYE_POS_Y + 2*EYE_SIZE, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X + EYE_SIZE, EYE_POS_Y + EYE_SIZE, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y, EYE_SIZE, EYE_SIZE);;
      cairo_fill(cr);
      cairo_rectangle(cr, EYE_POS_X + 2*EYE_SIZE, EYE_POS_Y + 2*EYE_SIZE, EYE_SIZE, EYE_SIZE);
      cairo_fill(cr);

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
      break;
    }
    case AWN_THROBBER_TYPE_CLOSE_BUTTON:
    {
      GdkColor c = gtk_widget_get_style (widget)->fg[GTK_STATE_NORMAL];
      double r = c.red / 65535.0;
      double g = c.green / 65535.0;
      double b = c.blue / 65535.0;

      cairo_set_source_rgb (cr, r, g, b);
      cairo_set_line_width (cr, 0.03);

      // FIXME: paint something nice!

      cairo_move_to (cr, 0.0, 0.0);
      cairo_line_to (cr, 1.0, 1.0);
      cairo_stroke (cr);

      cairo_move_to (cr, 1.0, 0.0);
      cairo_line_to (cr, 0.0, 1.0);
      cairo_stroke (cr);
      break;
    }
    default:
      break;
  }

  /* let effects know we're finished */
  awn_effects_cairo_destroy(priv->effects);

  return TRUE;
}

static void
awn_throbber_size_request (GtkWidget *widget, GtkRequisition *req)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE (widget);

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      req->width = APPLY_SIZE_MULTIPLIER(priv->size);
      req->height = priv->size + priv->effects->icon_offset;
      break;

    default:
      req->width = priv->size + priv->effects->icon_offset;
      req->height = APPLY_SIZE_MULTIPLIER(priv->size);
      break;
  }
}

static void
awn_throbber_update_effects (GtkWidget *widget, gpointer data)
{
  AwnThrobberPrivate *priv = AWN_THROBBER (widget)->priv;

  if (gtk_widget_is_composited(widget))
  {
    /* optimize the render speed */
    g_object_set(priv->effects,
                 "indirect-paint", FALSE, NULL);
  }
  else
  {
    g_object_set(priv->effects,
                 "indirect-paint", TRUE, NULL);
  }
}

static gboolean
awn_throbber_timeout (gpointer user_data)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE(user_data);

  priv->counter = (priv->counter - 1) % 8 + 8;

  gtk_widget_queue_draw (GTK_WIDGET (user_data));

  return TRUE;
}

static void
awn_throbber_show (GtkWidget *widget, gpointer user_data)
{
  AwnThrobberPrivate *priv = AWN_THROBBER_GET_PRIVATE(widget);

  if (!priv->timer_id && priv->type == AWN_THROBBER_TYPE_NORMAL)
  {
    priv->timer_id = g_timeout_add(100, awn_throbber_timeout, widget);
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
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->dispose      = awn_throbber_dispose;

  wid_class->size_request       = awn_throbber_size_request;
  wid_class->expose_event       = awn_throbber_expose_event;

  g_type_class_add_private (obj_class, sizeof (AwnThrobberPrivate));
}

static void
awn_throbber_init (AwnThrobber *throbber)
{
  AwnThrobberPrivate *priv;

  priv = throbber->priv = AWN_THROBBER_GET_PRIVATE (throbber);

  priv->orient = AWN_ORIENTATION_BOTTOM;
  priv->size = 50;
  priv->counter = 0;
  priv->type = AWN_THROBBER_TYPE_NORMAL;
  priv->tooltip = awn_tooltip_new_for_widget (GTK_WIDGET (throbber));

  priv->effects = awn_effects_new_for_widget (GTK_WIDGET (throbber));
  /* FIXME: move the prop binding from AwnEffects to AwnIcon, we don't want
   * any notifications from the ConfigClient (orient and size is changed when
   * properties of AwnAppletProxy change), well except offset
   */
  g_object_set (priv->effects, "effects", 0,
                "reflection-visible", FALSE, NULL);

  gtk_widget_add_events (GTK_WIDGET (throbber), GDK_ALL_EVENTS_MASK);

  awn_utils_ensure_transparent_bg (GTK_WIDGET (throbber));
  g_signal_connect (throbber, "realize",
                    G_CALLBACK (awn_throbber_update_effects), NULL);
  g_signal_connect (throbber, "composited-changed",
                    G_CALLBACK (awn_throbber_update_effects), NULL);
  g_signal_connect (throbber, "show",
                    G_CALLBACK (awn_throbber_show), NULL);
  g_signal_connect (throbber, "hide",
                    G_CALLBACK (awn_throbber_hide), NULL);
}

GtkWidget *
awn_throbber_new (void)
{
  GtkWidget *throbber = NULL;

  throbber = g_object_new (AWN_TYPE_THROBBER, NULL);

  return throbber;
}

static void
awn_throbber_update_tooltip_pos (AwnThrobber *throbber)
{
  AwnThrobberPrivate *priv;

  g_return_if_fail (AWN_IS_THROBBER (throbber));
  priv = throbber->priv;

  awn_tooltip_set_position_hint (AWN_TOOLTIP (priv->tooltip),
                                 priv->orient, priv->size + priv->offset);
}

void 
awn_throbber_set_orientation (AwnThrobber *throbber,
                              AwnOrientation  orient)
{
  AwnThrobberPrivate *priv;

  g_return_if_fail (AWN_IS_THROBBER (throbber));
  priv = throbber->priv;

  priv->orient = orient;

  g_object_set (priv->effects, "orientation", orient, NULL);

  gtk_widget_queue_resize (GTK_WIDGET (throbber));

  awn_throbber_update_tooltip_pos (throbber);
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
      if (!priv->timer_id && GTK_WIDGET_MAPPED (GTK_WIDGET (throbber)))
      {
        priv->timer_id = g_timeout_add (100, awn_throbber_timeout, throbber);
      }
      break;
    default:
      if (priv->timer_id)
      {
        g_source_remove (priv->timer_id);
        priv->timer_id = 0;
      }
      break;
  }

  if (needs_redraw)
    gtk_widget_queue_draw (GTK_WIDGET (throbber));
}

void
awn_throbber_set_size (AwnThrobber *throbber, gint size)
{
  g_return_if_fail (AWN_IS_THROBBER (throbber));

  AwnThrobberPrivate *priv = throbber->priv;

  priv->size = size;
  awn_effects_set_icon_size(priv->effects, size, size, FALSE);

  gtk_widget_queue_resize (GTK_WIDGET(throbber));

  awn_throbber_update_tooltip_pos (throbber);
}

void
awn_throbber_set_offset (AwnThrobber *throbber, gint o)
{
  g_return_if_fail (AWN_IS_THROBBER (throbber));

  AwnThrobberPrivate *priv = throbber->priv;

  priv->offset = o;
  g_object_set (priv->effects, "icon-offset", o, NULL);
}

void
awn_throbber_set_text (AwnThrobber *throbber, const gchar* text)
{
  g_return_if_fail (AWN_IS_THROBBER (throbber));

  AwnThrobberPrivate *priv = throbber->priv;

  awn_tooltip_set_text (AWN_TOOLTIP (priv->tooltip), text);
}

AwnEffects *
awn_throbber_get_effects (AwnThrobber *throbber)
{
  g_return_val_if_fail (AWN_IS_THROBBER (throbber), NULL);

  return throbber->priv->effects;
}

AwnTooltip *
awn_throbber_get_tooltip (AwnThrobber *throbber)
{
  g_return_val_if_fail (AWN_IS_THROBBER (throbber), NULL);

  return AWN_TOOLTIP (throbber->priv->tooltip);
}

