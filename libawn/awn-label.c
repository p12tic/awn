/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 * Authored by Michal Hruby <michal.mhr@gmail.com>
 *
 */

/**
 * AwnLabel:
 *
 * Widget derived from #GtkLabel, which should be used to display text
 * on the panel, as it provides various modes of drawing the text - with
 * or without an outline, and is therefore easily readable on non-solid 
 * colored background.
 */

#include "awn-label.h"
#include "awn-cairo-utils.h"
#include "awn-config.h"

G_DEFINE_TYPE (AwnLabel, awn_label, GTK_TYPE_LABEL)

#define AWN_LABEL_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_LABEL, AwnLabelPrivate))

typedef struct _AwnLabelPrivate AwnLabelPrivate;

struct _AwnLabelPrivate
{
  gint                  font_mode;
  DesktopAgnosticColor *text_color;
  DesktopAgnosticColor *text_outline_color;
  gdouble               text_outline_width;
};

enum
{
  FONT_MODE_SOLID,
  FONT_MODE_OUTLINE,
  FONT_MODE_OUTLINE_REVERSED
};

enum
{
  PROP_0,
  PROP_FONT_MODE,
  PROP_TEXT_OUTLINE_WIDTH,
  PROP_TEXT_COLOR,
  PROP_TEXT_OUTLINE_COLOR
};

static gboolean
do_bind_property (DesktopAgnosticConfigClient *client, const gchar *key,
                  GObject *object, const gchar *property)
{
  GError *error = NULL;

  desktop_agnostic_config_client_bind (client,
                                       "theme", key,
                                       object, property, TRUE, 
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       &error);
  if (error)
  {
    g_critical ("Could not bind property to config key: %s", error->message);
    g_error_free (error);
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}

static void
awn_label_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (awn_label_parent_class)->constructed )
  {
    G_OBJECT_CLASS (awn_label_parent_class)->constructed (object);
  }

  GError *error = NULL;
  DesktopAgnosticConfigClient *client;

  client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, &error);

  if (error)
  {
    g_critical ("Cannot obtain config object: %s", error->message);
    g_error_free (error);
    return;
  }

  if (!do_bind_property (client, "icon_text_color", object, "text-color"))
  {
    return;
  }
  if (!do_bind_property (client, "icon_text_outline_color", object,
                         "text-outline-color"))
  {
    return;
  }
  if (!do_bind_property (client, "icon_font_mode", object, "font-mode"))
  {
    return;
  }
  if (!do_bind_property (client, "icon_text_outline_width", object,
                         "text-outline-width"))
  {
    return;
  }
}

static void
awn_label_get_property (GObject *object, guint property_id,
                        GValue *value, GParamSpec *pspec)
{
  AwnLabelPrivate *priv = AWN_LABEL_GET_PRIVATE (object);

  switch (property_id)
  {
    case PROP_FONT_MODE:
      g_value_set_int (value, priv->font_mode);
      break;
    case PROP_TEXT_OUTLINE_WIDTH:
      g_value_set_double (value, priv->text_outline_width);
      break;
    case PROP_TEXT_COLOR:
      g_value_set_object (value, priv->text_color);
      break;
    case PROP_TEXT_OUTLINE_COLOR:
      g_value_set_object (value, priv->text_outline_color);
      break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_label_set_property (GObject *object, guint property_id,
                        const GValue *value, GParamSpec *pspec)
{
  AwnLabelPrivate *priv = AWN_LABEL_GET_PRIVATE (object);
  GtkWidget *widget = GTK_WIDGET (object);

  switch (property_id)
  {
    case PROP_FONT_MODE:
      priv->font_mode = g_value_get_int (value);
      break;

    case PROP_TEXT_OUTLINE_WIDTH:
      priv->text_outline_width = g_value_get_double (value);
      break;

    case PROP_TEXT_COLOR:
      if (priv->text_color)
      {
        g_object_unref (priv->text_color);
        priv->text_color = NULL;
      }
      priv->text_color = g_value_dup_object (value);
      if (!priv->text_color)
      {
        gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, NULL);
        priv->text_color =
          desktop_agnostic_color_new (&widget->style->fg[GTK_STATE_NORMAL],
                                      G_MAXUSHORT);
      }
      break;

    case PROP_TEXT_OUTLINE_COLOR:
      if (priv->text_outline_color)
      {
        g_object_unref (priv->text_outline_color);
        priv->text_outline_color = NULL;
      }
      priv->text_outline_color = g_value_dup_object (value);
      if (!priv->text_outline_color)
      {
        priv->text_outline_color =
          desktop_agnostic_color_new (&widget->style->bg[GTK_STATE_NORMAL],
                                      G_MAXUSHORT);
      }
      break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    return;
  }

  GdkColor color;
  DesktopAgnosticColor *da_color;

  // makes sure that text_color affects the actual label color
  da_color = priv->font_mode == FONT_MODE_OUTLINE_REVERSED ? 
    priv->text_outline_color : priv->text_color;
  if (da_color)
  {
    desktop_agnostic_color_get_color (da_color, &color);
    gtk_widget_modify_fg (widget, GTK_STATE_NORMAL, &color);
  }

  gtk_widget_queue_draw (GTK_WIDGET (object));
}

static void
awn_label_finalize (GObject *object)
{
  DesktopAgnosticConfigClient *client;
  AwnLabelPrivate *priv = AWN_LABEL_GET_PRIVATE (object);

  client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, NULL);
  desktop_agnostic_config_client_unbind_all_for_object (client, object, NULL);
  
  if (priv->text_color)
  {
    g_object_unref (priv->text_color);
    priv->text_color = NULL;
  }

  if (priv->text_outline_color)
  {
    g_object_unref (priv->text_outline_color);
    priv->text_outline_color = NULL;
  }

  G_OBJECT_CLASS (awn_label_parent_class)->finalize (object);
}

static gboolean
awn_label_expose (GtkWidget *widget, GdkEventExpose *event)
{
  /* FIXME: one way to do this correctly is to create a subclass of
   *  GdkPangoRenderer and override its draw_glyphs method, where it'll 
   *  paint the outline as well as the text itself.
   *  After this is done, this class can support all GtkLabel's features
   *  (ie. angle etc). Of course the expose-event will have to be rewritten,
   *  so it can use this new PangoRenderer.
   *  But perhaps it'd be enough to take the PangoMatrix and give it to cairo?
   */
  AwnLabelPrivate *priv = AWN_LABEL_GET_PRIVATE (widget);
  GtkLabel *label = GTK_LABEL (widget);
  gint x, y;

  if (priv->font_mode == FONT_MODE_SOLID) return FALSE;

  PangoLayout *layout = gtk_label_get_layout (label);
  gtk_label_get_layout_offsets (label, &x, &y);

  /* Paint outline */
  cairo_t *cr = gdk_cairo_create (GDK_DRAWABLE (event->window));

  g_return_val_if_fail (cr, FALSE);

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  cairo_set_line_width (cr, priv->text_outline_width);

  PangoContext *context = pango_layout_get_context (layout);
  const PangoMatrix *matrix = pango_context_get_matrix (context);
  if (matrix)
  {
    PangoRectangle rect;
    pango_layout_get_pixel_extents (layout, NULL, &rect);
    pango_matrix_transform_rectangle (matrix, &rect);

    cairo_matrix_t cairo_matrix;
    cairo_matrix_init (&cairo_matrix,
                       matrix->xx, matrix->yx,
                       matrix->xy, matrix->yy,
                       abs(rect.x) + x, abs(rect.y) + y);
    cairo_set_matrix (cr, &cairo_matrix);
  }
  else
  {
    cairo_move_to (cr, x, y);
  }

  g_return_val_if_fail (priv->text_outline_color && priv->text_color, FALSE);

  awn_cairo_set_source_color (cr, priv->font_mode == FONT_MODE_OUTLINE ?
                              priv->text_outline_color : priv->text_color);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
  pango_cairo_layout_path (cr, layout);
  cairo_stroke (cr);

  cairo_destroy (cr);

  return FALSE;
}

static void
awn_label_class_init (AwnLabelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = awn_label_constructed;
  object_class->get_property = awn_label_get_property;
  object_class->set_property = awn_label_set_property;
  object_class->finalize = awn_label_finalize;

  g_object_class_install_property (object_class, 
    PROP_TEXT_COLOR,
    g_param_spec_object ("text-color",
                         "Text color",
                         "Text color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_TEXT_OUTLINE_COLOR,
    g_param_spec_object ("text-outline-color",
                         "Text Outline Color",
                         "Text outline color",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, 
    PROP_FONT_MODE,
    g_param_spec_int ("font-mode",
                      "Font Mode",
                      "Font Mode",
                      FONT_MODE_SOLID,
                      FONT_MODE_OUTLINE_REVERSED,
                      FONT_MODE_SOLID,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, 
    PROP_TEXT_OUTLINE_WIDTH,
    g_param_spec_double ("text-outline-width",
                         "Text Outline Width",
                         "Text Outline Width",
                         0.0, 10.0, 2.5,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (AwnLabelPrivate));
}

static void
awn_label_init (AwnLabel *self)
{
  g_signal_connect (G_OBJECT (self), "expose-event", 
                    G_CALLBACK (awn_label_expose), NULL);
}

/**
 * awn_label_new:
 *
 * Creates new #AwnLabel. Automatically applies user configured colors and
 * style.
 *
 * Returns: An instance of #AwnLabel.
 */
AwnLabel*
awn_label_new (void)
{
  return g_object_new (AWN_TYPE_LABEL, NULL);
}

