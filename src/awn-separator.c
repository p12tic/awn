/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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

#include <libawn/libawn.h>

#include "awn-defines.h"
#include "awn-separator.h"

G_DEFINE_TYPE (AwnSeparator, awn_separator, GTK_TYPE_IMAGE)

#define AWN_SEPARATOR_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_SEPARATOR, AwnSeparatorPrivate))

struct _AwnSeparatorPrivate
{
  DesktopAgnosticConfigClient *client;

  GtkPositionType  position;
  gint             offset;
  gint             size;

  DesktopAgnosticColor *sep_color;
};

enum
{
  PROP_0,

  PROP_CLIENT,
  PROP_POSITION,
  PROP_OFFSET,
  PROP_SIZE,

  PROP_SEP_COLOR
};

static void
awn_separator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  AwnSeparatorPrivate *priv;

  g_return_if_fail (AWN_IS_SEPARATOR (object));
  priv = AWN_SEPARATOR (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      g_value_set_object (value, priv->client);
      break;
    case PROP_POSITION:
      g_value_set_int (value, priv->position);
      break;
    case PROP_OFFSET:
      g_value_set_int (value, priv->offset);
      break;
    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;
    case PROP_SEP_COLOR:
      g_value_set_object (value, priv->sep_color);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_separator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  AwnSeparatorPrivate *priv;
  gint temp;

  g_return_if_fail (AWN_IS_SEPARATOR (object));
  priv = AWN_SEPARATOR (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      priv->client = g_value_get_object (value);
      break;
    case PROP_POSITION:
      temp = g_value_get_int (value);
      if ((GtkPositionType)temp == priv->position) break;
      priv->position = temp;
      gtk_widget_queue_resize (GTK_WIDGET (object));
      break;
    case PROP_OFFSET:
      temp = g_value_get_int (value);
      if (temp == priv->offset) break;
      priv->offset = temp;
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    case PROP_SIZE:
      temp = g_value_get_int (value);
      if (temp == priv->size) break;
      priv->size = temp;
      gtk_widget_queue_resize (GTK_WIDGET (object));
      break;
    case PROP_SEP_COLOR:
      if (priv->sep_color)
      {
        g_object_unref (priv->sep_color);
      }
      priv->sep_color = g_value_dup_object (value);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_separator_constructed (GObject *object)
{
  AwnSeparatorPrivate *priv = AWN_SEPARATOR_GET_PRIVATE (object);

  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_THEME, AWN_THEME_SEP_COLOR,
                                       object, "separator-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
}

static void
awn_separator_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_separator_parent_class)->dispose (object);
}

static void
awn_separator_finalize (GObject *object)
{
  AwnSeparatorPrivate *priv = AWN_SEPARATOR_GET_PRIVATE (object);

  if (priv->client)
  {
    desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                          object, NULL);
  }

  if (priv->sep_color)
  {
    g_object_unref (priv->sep_color);
    priv->sep_color = NULL;
  }

  G_OBJECT_CLASS (awn_separator_parent_class)->finalize (object);
}

static void
awn_separator_size_request (GtkWidget *widget, GtkRequisition *req)
{
  AwnSeparatorPrivate *priv = AWN_SEPARATOR_GET_PRIVATE (widget);
  /*
  GTK_WIDGET_CLASS (awn_separator_parent_class)->size_request (widget, req);
  */

  switch (priv->position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      req->width = 10;
      req->height = priv->size + priv->offset;
      break;
    case GTK_POS_LEFT:
    case GTK_POS_RIGHT:
      req->width = priv->size + priv->offset;
      req->height = 10;
      break;
    default:
      break;
  }
}

static gboolean
awn_separator_expose (GtkWidget *widget, GdkEventExpose *event)
{
  AwnSeparatorPrivate *priv = AWN_SEPARATOR (widget)->priv;
  cairo_t *cr;
  cairo_path_t *path;
  GtkOrientation orient;
  cairo_pattern_t *pat = NULL, *shadow_pat = NULL;
  gdouble r, g, b, a;

  cr = gdk_cairo_create (widget->window);
  g_return_val_if_fail (cr, FALSE);

  cairo_set_line_width (cr, 1.0);

  // translate to correct position
  switch (priv->position)
  {
    case GTK_POS_LEFT:
      cairo_translate (cr, event->area.x,
                       event->area.y);
      orient = GTK_ORIENTATION_HORIZONTAL;
      break;
    case GTK_POS_RIGHT:
      cairo_translate (cr, widget->allocation.width - priv->size - priv->offset,
                       event->area.y);
      orient = GTK_ORIENTATION_HORIZONTAL;
      break;
    case GTK_POS_TOP:
      cairo_translate (cr, event->area.x,
                       event->area.y);
      orient = GTK_ORIENTATION_VERTICAL;
      break;
    case GTK_POS_BOTTOM:
    default:
      cairo_translate (cr, event->area.x,
                       widget->allocation.height - priv->size - priv->offset);
      orient = GTK_ORIENTATION_VERTICAL;
      break;
  }

  // pixel perfection
  cairo_translate (cr, 0.5, 0.5);

  // TODO: switch (style)
  if (orient == GTK_ORIENTATION_HORIZONTAL)
  {
    pat = cairo_pattern_create_linear (0.0, 0.0, 
                                       priv->size + priv->offset / 2, 0.0);
    shadow_pat = cairo_pattern_create_linear (0.0, 0.0,
                                       priv->size + priv->offset / 2, 0.0);

    cairo_move_to (cr, 0.0, widget->allocation.height / 2);
    cairo_rel_line_to (cr, priv->size + priv->offset, 0.0);
  }
  else
  {
    pat = cairo_pattern_create_linear (0.0, 0.0, 
                                       0.0, priv->size + priv->offset / 2);
    shadow_pat = cairo_pattern_create_linear (0.0, 0.0,
                                       0.0, priv->size + priv->offset / 2);

    cairo_move_to (cr, widget->allocation.width / 2, 0.0);
    cairo_rel_line_to (cr, 0.0, priv->size + priv->offset);
  }

  // set color
  awn_cairo_pattern_add_color_stop_color_with_alpha_multiplier (
      pat, 0.0, priv->sep_color, 0.0);
  awn_cairo_pattern_add_color_stop_color (pat, 0.4, priv->sep_color);
  awn_cairo_pattern_add_color_stop_color (pat, 0.6, priv->sep_color);
  awn_cairo_pattern_add_color_stop_color_with_alpha_multiplier (
      pat, 1.0, priv->sep_color, 0.0);
  cairo_set_source (cr, pat);

  path = cairo_copy_path (cr);
  cairo_stroke (cr);

  desktop_agnostic_color_get_cairo_color (priv->sep_color, &r, &g, &b, &a);

  #define INVERT_CHANNEL(x) x = x >= 0.5 ? x / 4 : 1.0 - x / 4;
  INVERT_CHANNEL (r);
  INVERT_CHANNEL (g);
  INVERT_CHANNEL (b);
  #undef INVERT_CHANNEL

  cairo_pattern_add_color_stop_rgba (shadow_pat, 0.0, r, g, b, 0.0);
  cairo_pattern_add_color_stop_rgba (shadow_pat, 0.4, r, g, b, a);
  cairo_pattern_add_color_stop_rgba (shadow_pat, 0.6, r, g, b, a);
  cairo_pattern_add_color_stop_rgba (shadow_pat, 1.0, r, g, b, 0.0);

  cairo_set_source (cr, shadow_pat);

  if (orient == GTK_ORIENTATION_HORIZONTAL)
  {
    cairo_translate (cr, 0.0, 1.0);
  }
  else
  {
    cairo_translate (cr, 1.0, 0.0);
  }
  cairo_append_path (cr, path);
  cairo_stroke (cr);

  cairo_pattern_destroy (shadow_pat);
  cairo_pattern_destroy (pat);
  cairo_path_destroy (path);
  cairo_destroy (cr);

  return TRUE;
}

static void
awn_separator_class_init (AwnSeparatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = awn_separator_dispose;
  object_class->finalize     = awn_separator_finalize;
  object_class->constructed  = awn_separator_constructed;
  object_class->get_property = awn_separator_get_property;
  object_class->set_property = awn_separator_set_property;

  widget_class->size_request = awn_separator_size_request;
  widget_class->expose_event = awn_separator_expose;

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
    PROP_POSITION,
    g_param_spec_int ("position",
                      "Position",
                      "The position of the panel",
                      0, 3, GTK_POS_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_OFFSET,
    g_param_spec_int ("offset",
                      "Offset",
                      "The icon offset of the panel",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "Size",
                      "The size of the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_SEP_COLOR,
    g_param_spec_object ("separator-color",
                         "Separator Color",
                         "The color of the separator",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (AwnSeparatorPrivate));
}

static void
awn_separator_init (AwnSeparator *self)
{
  AwnSeparatorPrivate *priv;
  priv = self->priv = AWN_SEPARATOR_GET_PRIVATE (self);

  priv->sep_color = NULL;
}

/**
 * awn_separator_new_from_config:
 * @client: A config client.
 *
 * Creates new instance of #AwnSeparator.
 *
 * Returns: An instance of #AwnSeparator.
 */
GtkWidget*
awn_separator_new_from_config (DesktopAgnosticConfigClient *client)
{
  return g_object_new (AWN_TYPE_SEPARATOR, "client", client, NULL);
}

GtkWidget*
awn_separator_new_from_config_with_values (DesktopAgnosticConfigClient *client,
                                           GtkPositionType pos,
                                           gint size, gint offset)
{
  return g_object_new (AWN_TYPE_SEPARATOR,
                       "client", client,
                       "position", pos,
                       "size", size,
                       "offset", offset,
                       NULL);
}

