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

#include "awn-label.h"
#include "awn-config-client.h"
#include "awn-config-bridge.h"
#include "awn-cairo-utils.h"

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

static void
awn_label_constructed (GObject *object)
{
  if (G_OBJECT_CLASS (awn_label_parent_class)->constructed )
  {
    G_OBJECT_CLASS (awn_label_parent_class)->constructed (object);
  }

  AwnConfigClient *client = awn_config_client_new ();
  AwnConfigBridge *bridge = awn_config_bridge_get_default ();

  awn_config_bridge_bind (bridge, client,
                          "theme", "icon_text_color",
                          object, "text-color");
  awn_config_bridge_bind (bridge, client,
                          "theme", "icon_text_outline_color",
                          object, "text-outline-color");
  awn_config_bridge_bind (bridge, client,
                          "theme", "icon_font_mode",
                          object, "font-mode");
  awn_config_bridge_bind (bridge, client,
                          "theme", "icon_text_outline_width",
                          object, "text-outline-width");
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
      g_value_take_string (value, priv->text_color ?
         desktop_agnostic_color_to_string (priv->text_color) : NULL);
      break;
    case PROP_TEXT_OUTLINE_COLOR:
      g_value_take_string (value, priv->text_outline_color ?
         desktop_agnostic_color_to_string (priv->text_outline_color) : NULL);
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
  GError *err = NULL;

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
      priv->text_color =
        desktop_agnostic_color_new_from_string (g_value_get_string (value),
                                                &err);
      if (err)
      {
        g_error_free (err);
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
      priv->text_outline_color =
        desktop_agnostic_color_new_from_string (g_value_get_string (value),
                                                &err);
      if (err)
      {
        g_error_free (err);
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
  AwnLabelPrivate *priv = AWN_LABEL_GET_PRIVATE (object);

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

  cairo_move_to (cr, x, y);
  cairo_set_line_width (cr, priv->text_outline_width);
  g_return_val_if_fail (priv->text_outline_color && priv->text_color, FALSE);

  awn_cairo_set_source_color (cr, priv->font_mode == FONT_MODE_OUTLINE ?
                              priv->text_outline_color : priv->text_color);
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

  GParamSpec *pspec;

  pspec = g_param_spec_string ("text-color",
                               "Text color",
                               "Text color",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_TEXT_COLOR, pspec);

  pspec = g_param_spec_string ("text-outline-color",
                               "Text Outline Color",
                               "Text outline color",
                               "",
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class,
                                   PROP_TEXT_OUTLINE_COLOR, pspec);

  pspec = g_param_spec_int ("font-mode",
                            "Font Mode",
                            "Font Mode",
                            FONT_MODE_SOLID,
                            FONT_MODE_OUTLINE_REVERSED,
                            FONT_MODE_SOLID,
                            G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_FONT_MODE, pspec);

  pspec = g_param_spec_double ("text-outline-width",
                               "Text Outline Width",
                               "Text Outline Width",
                               0.0,
                               10.0,
                               2.5,
                               G_PARAM_READWRITE);
  g_object_class_install_property (object_class, 
                                   PROP_TEXT_OUTLINE_WIDTH, pspec);

  g_type_class_add_private (klass, sizeof (AwnLabelPrivate));
}

static void
awn_label_init (AwnLabel *self)
{
  g_signal_connect (G_OBJECT (self), "expose-event", 
                    G_CALLBACK (awn_label_expose), NULL);
}

AwnLabel*
awn_label_new (void)
{
  return g_object_new (AWN_TYPE_LABEL, NULL);
}

