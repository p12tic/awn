/*
 * Copyright (C) 2007 Neil J. Patel <njpatel@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors: Neil J. Patel <njpatel@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <libdesktop-agnostic/desktop-agnostic.h>

#include "awn-tooltip.h"

#include "awn-cairo-utils.h"
#include "awn-config-bridge.h"
#include "awn-config-client.h"

#include "gseal-transition.h"

G_DEFINE_TYPE (AwnTooltip, awn_tooltip, GTK_TYPE_WINDOW)

#define AWN_TOOLTIP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                      AWN_TYPE_TOOLTIP, AwnTooltipPrivate))

#define AWN_THEME_GROUP "theme"
#define FONT_NAME       "tooltip_font_name"
#define FONT_COLOR      "tooltip_font_color"
#define BACKGROUND      "tooltip_bg_color"

#define AWN_PANEL_GROUP "panel"
#define ICON_OFFSET     "offset"

#define TOOLTIP_ROUND_RADIUS 7.0

struct _AwnTooltipPrivate
{
  AwnConfigClient *client;
  
  GtkWidget *focus;
  GtkWidget *label;

  DesktopAgnosticColor  *bg;
  gchar    *font_name;
  gchar    *font_color;
  gint      icon_offset;

  gboolean  smart_behavior, toggle_on_click;
  gboolean  inhibit_show;

  gint      delay;
  guint     show_timer_id;
  guint     hide_timer_id;

  AwnOrientation orient;
  gint           size;

  gchar    *text;

  gulong enter_id, leave_id, press_id;
  gint old_w, old_h;
};

enum
{
  PROP_0,

  PROP_FOCUS,  
  PROP_BG,
  PROP_FONT_NAME,
  PROP_FONT_COLOR,
  PROP_ICON_OFFSET,
  PROP_DELAY,

  PROP_SMART_BEHAVIOUR,
  PROP_TOGGLE_ON_CLICK
};

/* Forwards */
static gboolean awn_tooltip_show (AwnTooltip *tooltip,
                                  GdkEventCrossing *event,
                                  GtkWidget *widget);
static gboolean awn_tooltip_hide (AwnTooltip *tooltip,
                                  GdkEventCrossing *event,
                                  GtkWidget *widget);

/* GObject Stuff */
static gboolean
awn_tooltip_expose_event (GtkWidget *widget, GdkEventExpose *expose)
{
  AwnTooltipPrivate *priv;
  cairo_t           *cr;
  gint               width, height;

  priv = AWN_TOOLTIP (widget)->priv;

  width = widget->allocation.width;
  height = widget->allocation.height;

  cr = gdk_cairo_create (gtk_widget_get_window (widget));

  if (!cr)
  {
    return FALSE;
  }

  /* Clear the background to transparent */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  
  /* Draw */
  awn_cairo_set_source_color (cr, priv->bg);

  awn_cairo_rounded_rect (cr, 0, 0, width, height,
                          TOOLTIP_ROUND_RADIUS, ROUND_ALL);
  cairo_fill (cr);

  /* Clean up */
  cairo_destroy (cr);

  gtk_container_propagate_expose (GTK_CONTAINER (widget), 
                                  gtk_bin_get_child (GTK_BIN (widget)),
                                  expose);

  return TRUE;
}

static void
awn_tooltip_set_mask (AwnTooltip *tooltip, gint width, gint height)
{
  GtkWidget *widget = GTK_WIDGET (tooltip);

  if (gtk_widget_is_composited (widget) == FALSE)
  {
    GdkBitmap *shaped_bitmap;
    shaped_bitmap = (GdkBitmap*) gdk_pixmap_new (NULL, width, height, 1);

    if (shaped_bitmap)
    {
      cairo_t *cr = gdk_cairo_create (shaped_bitmap);

      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);

      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_translate (cr, 0.5, 0.5);

      
      awn_cairo_rounded_rect (cr, 0, 0, width, height,
                              TOOLTIP_ROUND_RADIUS, ROUND_ALL);
      cairo_fill (cr);

      cairo_destroy (cr);

      gtk_widget_shape_combine_mask (widget, NULL, 0, 0);
      gtk_widget_shape_combine_mask (widget, shaped_bitmap, 0, 0);

      g_object_unref (shaped_bitmap);
    }
  }
}

static gboolean
_on_configure_event (AwnTooltip *tooltip, GdkEventConfigure *event)
{
  AwnTooltipPrivate *priv = AWN_TOOLTIP_GET_PRIVATE (tooltip);

  if (event->width == priv->old_w && event->height == priv->old_h)
  {
    return FALSE;
  }

  priv->old_w = event->width; priv->old_h = event->height;

  awn_tooltip_set_mask (tooltip, event->width, event->height);

  return FALSE;
}

static void
_on_composited_changed (GtkWidget *widget)
{
  if (gtk_widget_is_composited (widget) == FALSE)
  {
    GtkAllocation *a = &widget->allocation;
    awn_tooltip_set_mask (AWN_TOOLTIP (widget), a->width, a->height);
  }
  else
  {
    gtk_widget_shape_combine_mask (widget, NULL, 0, 0);
  }
}

static void
awn_tooltip_constructed (GObject *object)
{
  AwnTooltip        *tooltip = AWN_TOOLTIP (object);
  AwnTooltipPrivate *priv = tooltip->priv;
  AwnConfigBridge   *bridge = awn_config_bridge_get_default ();
  GtkWidget         *align;

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_THEME_GROUP, FONT_NAME,
                          object, FONT_NAME);

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_THEME_GROUP, FONT_COLOR,
                          object, FONT_COLOR);

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_THEME_GROUP, BACKGROUND,
                          object, BACKGROUND);

  /* Setup internal widgets */
  align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 5, 3, 8, 8);
  gtk_container_add (GTK_CONTAINER (tooltip), align);
  gtk_widget_show (align);

  priv->label = gtk_label_new (" ");
  gtk_label_set_line_wrap (GTK_LABEL (priv->label), FALSE);
  gtk_label_set_ellipsize (GTK_LABEL (priv->label), PANGO_ELLIPSIZE_NONE);
  gtk_container_add (GTK_CONTAINER (align), priv->label);
  gtk_widget_show (priv->label);

  gtk_window_set_resizable (GTK_WINDOW (tooltip), FALSE);

  g_signal_connect (tooltip, "leave-notify-event",
                    G_CALLBACK (awn_tooltip_hide), NULL);
}

static void
awn_tooltip_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail (AWN_IS_TOOLTIP (object));
  priv = AWN_TOOLTIP (object)->priv;

  switch (prop_id)
  {
    case PROP_FOCUS:
      g_value_set_object (value, priv->focus);
      break;

    case PROP_FONT_NAME:
      g_value_set_string (value, priv->font_name);
      break;

    case PROP_FONT_COLOR:
      g_value_set_string (value, priv->font_color);
      break;

    case PROP_BG:
      g_value_set_string (value, "#FFFFFFFF");
      break;

    case PROP_ICON_OFFSET:
      g_value_set_int (value, priv->icon_offset);
      break;

    case PROP_DELAY:
      g_value_set_int (value, priv->delay);
      break;

    case PROP_SMART_BEHAVIOUR:
      g_value_set_boolean (value, priv->smart_behavior);
      break;

    case PROP_TOGGLE_ON_CLICK:
      g_value_set_boolean (value, priv->toggle_on_click);
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_tooltip_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  AwnTooltip *tooltip = AWN_TOOLTIP (object);
  AwnTooltipPrivate *priv;

  g_return_if_fail (AWN_IS_TOOLTIP (object));
  priv = AWN_TOOLTIP (object)->priv;

  switch (prop_id)
  {
    case PROP_FOCUS:
      awn_tooltip_set_focus_widget (tooltip, g_value_get_object (value));
      break;

    case PROP_FONT_NAME:
      awn_tooltip_set_font_name (tooltip, g_value_get_string (value));
      break;

    case PROP_FONT_COLOR:
      awn_tooltip_set_font_color (tooltip, g_value_get_string (value));
      break;

    case PROP_BG:
      awn_tooltip_set_background_color (tooltip, g_value_get_string (value));
      break;

    case PROP_ICON_OFFSET:
      priv->icon_offset = g_value_get_int (value);
      break;

    case PROP_DELAY:
      priv->delay = g_value_get_int (value);
      break;

    case PROP_SMART_BEHAVIOUR:
      priv->smart_behavior = g_value_get_boolean (value);
      break;

    case PROP_TOGGLE_ON_CLICK:
      priv->toggle_on_click = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_tooltip_dispose(GObject *obj)
{
  AwnTooltipPrivate *priv = AWN_TOOLTIP_GET_PRIVATE (obj);

  if (priv->focus)
  {
    g_signal_handler_disconnect (priv->focus, priv->enter_id);
    g_signal_handler_disconnect (priv->focus, priv->leave_id);
    g_signal_handler_disconnect (priv->focus, priv->press_id);
    priv->focus = NULL;
  }

  if (priv->show_timer_id)
  {
    g_source_remove (priv->show_timer_id);
    priv->show_timer_id = 0;
  }

  if (priv->hide_timer_id)
  {
    g_source_remove (priv->hide_timer_id);
    priv->hide_timer_id = 0;
  }

  G_OBJECT_CLASS (awn_tooltip_parent_class)->dispose (obj);
}

static void
awn_tooltip_finalize(GObject *obj)
{
  AwnTooltipPrivate *priv = AWN_TOOLTIP_GET_PRIVATE (obj);
 
  if (priv->text)
  {
    g_free (priv->text);
    priv->text = NULL;
  }
  if (priv->font_name)
  {
    g_free (priv->font_name);
    priv->font_name = NULL;
  }
  if (priv->font_color)
  {
    g_free (priv->font_color);
    priv->font_color = NULL;
  }

  G_OBJECT_CLASS (awn_tooltip_parent_class)->finalize (obj);
}

static void
awn_tooltip_class_init(AwnTooltipClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->dispose      = awn_tooltip_dispose;
  obj_class->finalize     = awn_tooltip_finalize;
  obj_class->get_property = awn_tooltip_get_property;
  obj_class->set_property = awn_tooltip_set_property;
  obj_class->constructed  = awn_tooltip_constructed;

  wid_class->expose_event = awn_tooltip_expose_event;

  /* Class property */
  g_object_class_install_property (obj_class,
    PROP_FOCUS,
    g_param_spec_object ("focus",
                         "Focus",
                         "Widget to focus on",
                         GTK_TYPE_WIDGET,
                         G_PARAM_READWRITE));

  g_object_class_install_property (obj_class,
    PROP_FONT_NAME,
    g_param_spec_string ("tooltip_font_name",
                         "tooltip-font-name",
                         "Tooltip Font Name",
                         "Sans 8",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  
  g_object_class_install_property (obj_class,
    PROP_FONT_COLOR,
    g_param_spec_string ("tooltip_font_color",
                         "tooltip-font-color",
                         "Tooltip Font Color",
                         "#FFFFFF00",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_BG,
    g_param_spec_string ("tooltip_bg_color",
                         "tooltip-bg-color",
                         "Tooltip Background Color",
                         "#000000B3",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_ICON_OFFSET,
    g_param_spec_int ("offset",
                      "Icon Offset",
                      "Icon Offset",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_DELAY,
    g_param_spec_int ("delay",
                      "delay",
                      "Delay",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT)); 

  g_object_class_install_property (obj_class,
    PROP_SMART_BEHAVIOUR,
    g_param_spec_boolean ("smart-behavior",
                          "Smart behavior",
                          "Will show the tooltip on enter-notify-event and "
                          "hide on leave-notify-event",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_TOGGLE_ON_CLICK,
    g_param_spec_boolean ("toggle-on-click",
                          "Toggle on click",
                          "Toggles tooltip visibility on click",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_type_class_add_private (obj_class, sizeof (AwnTooltipPrivate));
}

static void
awn_tooltip_init (AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;
  GdkScreen *screen;
  
  priv = tooltip->priv = AWN_TOOLTIP_GET_PRIVATE (tooltip);

  priv->client = awn_config_client_new ();
  priv->text = NULL;
  priv->font_name = NULL;
  priv->font_color = NULL;
  priv->orient = AWN_ORIENTATION_BOTTOM;
  priv->size = 50;
  priv->show_timer_id = 0;
  priv->hide_timer_id = 0;

  gtk_widget_set_app_paintable (GTK_WIDGET (tooltip), TRUE);

  g_signal_connect (tooltip, "configure-event",
                    G_CALLBACK (_on_configure_event), NULL);
  g_signal_connect (tooltip, "composited-changed",
                    G_CALLBACK (_on_composited_changed), NULL);

  screen = gdk_screen_get_default ();
  if (gdk_screen_get_rgba_colormap (screen))
    gtk_widget_set_colormap (GTK_WIDGET (tooltip),
                             gdk_screen_get_rgba_colormap (screen));
}
 
GtkWidget *
awn_tooltip_new_for_widget (GtkWidget *widget)
{
  GtkWidget *tooltip;

  tooltip = g_object_new(AWN_TYPE_TOOLTIP,
                         "type", GTK_WINDOW_POPUP,
                         "decorated", FALSE,
                         "skip-pager-hint", TRUE,
                         "skip-taskbar-hint", TRUE,
                         "focus", widget,
                         NULL);

  return tooltip;
}

/* POSITIONING */
void
awn_tooltip_update_position (AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;
  GtkRequisition req;
  gint x = 0, y = 0, w, h;
  gint fx, fy, fw, fh;
  GdkWindow *focus_win;

  g_return_if_fail (AWN_IS_TOOLTIP (tooltip));
  priv = tooltip->priv;

  /* Get our dimensions */
  gtk_widget_size_request (GTK_WIDGET (tooltip), &req);

  w = req.width;
  h = req.height;

  /* Get the dimesions of the widget we are focusing on */
  focus_win = gtk_widget_get_window (priv->focus);
  gdk_window_get_origin (focus_win, &fx, &fy);
  gdk_drawable_get_size (GDK_DRAWABLE (focus_win), &fw, &fh);
  
  /* Find and set our position */
  #define TOOLTIP_OFFSET 16
  switch (priv->orient) {
    case AWN_ORIENTATION_TOP:
      x = fx + (fw / 2) - (w / 2);
      y = fy + priv->size + priv->icon_offset + TOOLTIP_OFFSET;
      break;
    case AWN_ORIENTATION_BOTTOM:
      x = fx + (fw / 2) - (w / 2);
      y = fy + fh - priv->size - priv->icon_offset - TOOLTIP_OFFSET - h;
      break;
    case AWN_ORIENTATION_RIGHT:
      x = fx + fw - priv->size - priv->icon_offset - TOOLTIP_OFFSET - w;
      y = fy + (fh / 2) - h / 2;
      break;
    case AWN_ORIENTATION_LEFT:
      x = fx + priv->size + priv->icon_offset + TOOLTIP_OFFSET;
      y = fy + (fh / 2) - h / 2;
      break;
  }

  if (gtk_widget_has_screen (GTK_WIDGET (tooltip)))
  {
    GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (tooltip));
    gint screen_w = gdk_screen_get_width (screen);
    if (x + w > screen_w) x -= x + w - screen_w;
  }

  if (x < 0) x = 0;
  if (y < 0) y = 0;

  gtk_window_move (GTK_WINDOW (tooltip), x, y);
}

static void
awn_tooltip_position_and_show (AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail (AWN_IS_TOOLTIP (tooltip));
  priv = tooltip->priv;

  if (!GTK_IS_WIDGET (priv->focus))
  {
    gtk_widget_hide (GTK_WIDGET (tooltip));
    return;
  }

  awn_tooltip_update_position (tooltip);
  gtk_widget_show_all (GTK_WIDGET (tooltip));
}


/* PUBLIC FUNCS */
static void
ensure_tooltip (AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv = tooltip->priv;
  gchar *normal = NULL;
  gchar *markup = NULL;

  if (priv->text == NULL)
    return;

  normal = g_markup_escape_text (priv->text, -1);
  markup = g_strdup_printf ("<span foreground='%s' font_desc='%s'>%s</span>",
                            priv->font_color,
                            priv->font_name,
                            normal);

  gtk_label_set_max_width_chars (GTK_LABEL (priv->label), 120);
  gtk_label_set_ellipsize (GTK_LABEL (priv->label), PANGO_ELLIPSIZE_END);
  gtk_label_set_markup (GTK_LABEL (priv->label), markup);

  g_free (normal);
  g_free (markup);

  if (GTK_WIDGET_MAPPED (tooltip) && GTK_IS_WIDGET (priv->focus))
  {
    awn_tooltip_update_position (tooltip);
  }
}

void  
awn_tooltip_set_text (AwnTooltip  *tooltip,
                      const gchar *text)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail (AWN_IS_TOOLTIP (tooltip));
  priv = tooltip->priv;

  if (priv->text)
    g_free (priv->text);

  priv->text = g_strdup (text);

  ensure_tooltip (tooltip);
}

gchar *
awn_tooltip_get_text (AwnTooltip  *tooltip)
{
  g_return_val_if_fail (AWN_IS_TOOLTIP (tooltip), NULL);

  return tooltip->priv->text ? g_strdup(tooltip->priv->text) : NULL;
}

static gboolean
awn_tooltip_show_timer(gpointer data)
{
  g_return_val_if_fail (AWN_IS_TOOLTIP (data), FALSE);
  AwnTooltip *tooltip = (AwnTooltip*)data;
  
  tooltip->priv->show_timer_id = 0;

  if (tooltip->priv->inhibit_show) return FALSE;

  awn_tooltip_position_and_show(tooltip);

  return FALSE;
}

static gboolean
awn_tooltip_show (AwnTooltip *tooltip,
                  GdkEventCrossing *event,
                  GtkWidget *focus)
{
  g_return_val_if_fail (AWN_IS_TOOLTIP (tooltip), FALSE);

  AwnTooltipPrivate *priv = tooltip->priv;

  if (!priv->text || priv->show_timer_id || !priv->smart_behavior
      || priv->inhibit_show)
    return FALSE;

  if (priv->hide_timer_id)
  {
    g_source_remove (priv->hide_timer_id);
    priv->hide_timer_id = 0;
    return FALSE;
  }

  if (GTK_WIDGET_VISIBLE (tooltip)) return FALSE;

  /* always use timer to show the widget, because there's a show/hide race
   * condition when mouse moves on the tooltip, leave-notify-event is generated
   * -> tooltip hides, then enter-notify-event from the widget is generated,
   * tooltip shows, therefore looping in an infinite loop
   *  with the timer X-server at least doesn't stall
   */
  if (!priv->show_timer_id)
  {
    gint delay = priv->delay > 0 ? priv->delay : 10;
    priv->show_timer_id = g_timeout_add(delay, awn_tooltip_show_timer, tooltip);
  }

  return FALSE;
}

static gboolean
awn_tooltip_hide_timer(gpointer data)
{
  g_return_val_if_fail (AWN_IS_TOOLTIP (data), FALSE);
  AwnTooltip *tooltip = (AwnTooltip*)data;

  tooltip->priv->hide_timer_id = 0;

  gtk_widget_hide (GTK_WIDGET (tooltip));

  return FALSE;
}

static gboolean
awn_tooltip_hide (AwnTooltip *tooltip,
                  GdkEventCrossing *event,
                  GtkWidget *focus)
{
  g_return_val_if_fail (AWN_IS_TOOLTIP (tooltip), FALSE);

  AwnTooltipPrivate *priv = tooltip->priv;

  if (priv->inhibit_show) priv->inhibit_show = FALSE;

  if (!priv->smart_behavior) return FALSE;

  /* remove show timer */
  if (priv->show_timer_id) {
    g_source_remove(priv->show_timer_id);
    priv->show_timer_id = 0;
    return FALSE;
  }

  /* start hide timer */
  if (!priv->hide_timer_id)
  {
    priv->hide_timer_id = g_timeout_add (50, awn_tooltip_hide_timer, tooltip);
  }

  return FALSE;
}

static gboolean
on_button_press (GtkWidget *widget, GdkEventCrossing *event, AwnTooltip *tooltip)
{
  g_return_val_if_fail (AWN_IS_TOOLTIP (tooltip), FALSE);

  AwnTooltipPrivate *priv = tooltip->priv;

  if (!priv->toggle_on_click) return FALSE;

  priv->inhibit_show = !priv->inhibit_show;

  /* 
   * This if is a bit of voodoo, will hide the tooltip on first click and show
   * it again on second click. Yay it was damn hard to make it work this way!
   */
  if (priv->show_timer_id)
  {
    g_source_remove (priv->show_timer_id);
    priv->show_timer_id = 0;
  }
  else if (GTK_WIDGET_VISIBLE (tooltip))
  {
    gtk_widget_hide (GTK_WIDGET (tooltip));
  }
  else if (priv->text)
  {
    awn_tooltip_position_and_show(tooltip);
  }

  return FALSE;
}

void   
awn_tooltip_set_focus_widget (AwnTooltip *tooltip,
                              GtkWidget  *widget)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail (AWN_IS_TOOLTIP (tooltip));
  priv = tooltip->priv;

  if (priv->focus)
  {
    g_signal_handler_disconnect (priv->focus, priv->enter_id);
    g_signal_handler_disconnect (priv->focus, priv->leave_id);
    g_signal_handler_disconnect (priv->focus, priv->press_id);
  }

  if (!GTK_IS_WIDGET (widget))
    return;

  priv->focus = widget;

  priv->enter_id = g_signal_connect_swapped (widget, "enter-notify-event",
                                     G_CALLBACK (awn_tooltip_show), tooltip);
  priv->leave_id = g_signal_connect_swapped (widget, "leave-notify-event",
                                     G_CALLBACK (awn_tooltip_hide), tooltip);
  priv->press_id = g_signal_connect (widget, "button-press-event",
                                     G_CALLBACK (on_button_press), tooltip);
}

void  
awn_tooltip_set_font_name (AwnTooltip *tooltip,
                           const gchar *font_name)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail (AWN_TOOLTIP (tooltip));
  g_return_if_fail (font_name);
  priv = tooltip->priv;
  
  if (priv->font_name)
    g_free (priv->font_name);

  priv->font_name = g_strdup (font_name);

  ensure_tooltip (tooltip);
}

void   
awn_tooltip_set_font_color (AwnTooltip  *tooltip,
                            const gchar *font_color)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail (AWN_TOOLTIP (tooltip));
  g_return_if_fail (font_color);
  priv = tooltip->priv;
  
  if (priv->font_color)
    g_free (priv->font_color);

  priv->font_color = g_strndup (font_color, 7);

  ensure_tooltip (tooltip);
}

void     
awn_tooltip_set_background_color (AwnTooltip  *tooltip,
                                  const gchar *bg_color)
{
  AwnTooltipPrivate *priv;
  GError *err = NULL;

  g_return_if_fail (AWN_TOOLTIP (tooltip));
  g_return_if_fail (bg_color);
  priv = tooltip->priv;
  
  priv->bg = desktop_agnostic_color_new_from_string (bg_color, &err);
  if (err)
  {
    g_critical ("awn_tooltip_set_background_color: %s", err->message);
    g_error_free (err);
  }
}

void    
awn_tooltip_set_delay (AwnTooltip  *tooltip,
                       gint         msecs)
{
  g_return_if_fail (AWN_IS_TOOLTIP (tooltip));

  tooltip->priv->delay = msecs;
}

gint   
awn_tooltip_get_delay (AwnTooltip  *tooltip)
{
  g_return_val_if_fail (AWN_IS_TOOLTIP (tooltip), 0);

  return tooltip->priv->delay;
}

void
awn_tooltip_set_position_hint(AwnTooltip *tooltip,
                              AwnOrientation orient,
                              gint size)
{
  g_return_if_fail (AWN_IS_TOOLTIP (tooltip));

  AwnTooltipPrivate *priv = tooltip->priv;

  priv->orient = orient;
  priv->size = size;

  if (GTK_WIDGET_MAPPED (tooltip) && GTK_IS_WIDGET (priv->focus))
  {
    awn_tooltip_update_position (tooltip);
  }
}

