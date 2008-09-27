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
#include "awn-tooltip.h"

#include "awn-cairo-utils.h"
#include "awn-config-client.h"

G_DEFINE_TYPE (AwnTooltip, awn_tooltip, GTK_TYPE_WINDOW);

#define AWN_TOOLTIP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                      AWN_TYPE_TOOLTIP, AwnTooltipPrivate))

#define TOOLTIP      "tooltip"
#define TEXT_COLOR "text_color" /*color*/
#define BACKGROUND "background" /*color*/
#define FONT_FACE  "font_face" /*bool*/

/* STRUCTS & ENUMS */

struct _AwnTooltipPrivate
{
  GtkWidget *focus;

  GtkWidget *image;
  GtkWidget *label;

  gchar    *font;
  AwnColor  bg;
  gchar    *text_col;
  gint      offset;

  guint     tag;
};

/* Private */
static void
awn_tooltip_position(AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;
  gint x = 0, y = 0, w, h;
  gint fx, fy, fw, fh;

  g_return_if_fail(AWN_IS_TOOLTIP(tooltip));
  priv = tooltip->priv;

  if (!GTK_IS_WIDGET(priv->focus))
  {
    gtk_widget_hide(GTK_WIDGET(tooltip));
    return;
  }

  /* Get our dimensions */
  GtkRequisition req, req2;

  gtk_widget_size_request(GTK_WIDGET(priv->label), &req);

  gtk_widget_size_request(GTK_WIDGET(tooltip), &req2);

  if (req2.width > req.width)
  {
    w = req2.width;
    h = req2.height;
  }
  else
  {
    // TODO: find better way than adding 8
    w = req.width + 8;
    h = req.height + 8;
  }

  /* Get the dimesions of the widget we are focusing on */
  gdk_window_get_origin(priv->focus->window, &fx, &fy);
  gtk_widget_get_size_request(priv->focus, &fw, &fh);

  /* Find and set our position */
  x = fx + (fw / 2) - (w / 2);
  y = fy + (fh / 8) - h / 2;

  if (x < 0)
  {
    x = 0;
  }
  gtk_window_move(GTK_WINDOW(tooltip), x, y);
}

/* Public */

static gboolean
on_prox_out(GtkWidget *focus, GdkEventCrossing *event, AwnTooltip *tooltip)
{
  awn_tooltip_hide(tooltip, focus);
  g_signal_handlers_disconnect_by_func(focus,on_prox_out,tooltip);
  return FALSE;
}

static gboolean
show(gchar *text)
{
  AwnTooltip *tooltip = AWN_TOOLTIP(awn_tooltip_get_default());
  AwnTooltipPrivate *priv;
  gchar *normal;
  gchar *markup;

  priv = tooltip->priv;

  if (priv->focus == NULL)
  {
    return FALSE;
  }
  
  normal = g_markup_escape_text(text, -1);
  markup = g_strdup_printf("<span foreground='#%s' font_desc='%s'>%s</span>",
                           priv->text_col,
                           priv->font,
                           normal
                          );

  gtk_label_set_max_width_chars(GTK_LABEL(priv->label), 120);
  gtk_label_set_ellipsize(GTK_LABEL(priv->label), PANGO_ELLIPSIZE_END);
  gtk_label_set_markup(GTK_LABEL(priv->label), markup);

  awn_tooltip_position(tooltip);

  gtk_widget_show_all(GTK_WIDGET(tooltip));

  gtk_widget_queue_draw(GTK_WIDGET(tooltip));

  g_free(normal);
  g_free(markup);
  g_free(text);

  return FALSE;
}

void
awn_tooltip_show(AwnTooltip *tooltip, GtkWidget *focus, const gchar *text)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail(AWN_IS_TOOLTIP(tooltip));
  g_return_if_fail(GTK_IS_WIDGET(focus));
  g_return_if_fail(text);
  
  priv = tooltip->priv;
  priv->focus = focus;

  g_timeout_add(1, (GSourceFunc)show, g_strdup(text));
}

void
awn_tooltip_hide(AwnTooltip *tooltip, GtkWidget *focus)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail(AWN_IS_TOOLTIP(tooltip));
  g_return_if_fail(GTK_IS_WIDGET(focus));
  priv = tooltip->priv;

  priv->focus = NULL;
  gtk_widget_hide(GTK_WIDGET(tooltip));
}

/* Overrides */
static gboolean
awn_tooltip_expose_event(GtkWidget *widget, GdkEventExpose *expose)
{
  AwnTooltipPrivate *priv;
  cairo_t *cr = NULL;
  gint width, height;

  priv = AWN_TOOLTIP(widget)->priv;

  if (!GDK_IS_DRAWABLE(widget->window))
  {
    return FALSE;
  }
  width = widget->allocation.width;

  height = widget->allocation.height;

  cr = gdk_cairo_create(widget->window);

  if (!cr)
  {
    return FALSE;
  }

  /* Clear the background to transparent */
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);

  cairo_paint(cr);

  /* Draw */
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_set_source_rgba(cr, priv->bg.red,
                        priv->bg.green,
                        priv->bg.blue,
                        priv->bg.alpha);

  awn_cairo_rounded_rect(cr,0, 0,width, height,15.0, ROUND_ALL);

  cairo_fill(cr);

  /* Clean up */
  cairo_destroy(cr);

  /* Propagate the signal */
  gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                 gtk_bin_get_child(GTK_BIN(widget)),
                                 expose);

  return TRUE;
}


static void
on_alpha_screen_changed(GtkWidget* pWidget,
                        GdkScreen* pOldScreen,
                        GtkWidget* pLabel)
{
  GdkScreen* pScreen = gtk_widget_get_screen(pWidget);
  GdkColormap* pColormap = gdk_screen_get_rgba_colormap(pScreen);

  if (!pColormap)
  {
    pColormap = gdk_screen_get_rgb_colormap(pScreen);
  }

  gtk_widget_set_colormap(pWidget, pColormap);
}

static void
_notify_font(AwnConfigClientNotifyEntry *entry,
             AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail(AWN_IS_TOOLTIP(tooltip));
  priv = tooltip->priv;

  if (priv->font)
  {
    g_free(priv->font);
  }
  priv->font = g_strdup(entry->value.str_val);
}

static void
_notify_bg(AwnConfigClientNotifyEntry *entry,
           AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail(AWN_IS_TOOLTIP(tooltip));
  priv = tooltip->priv;

  awn_cairo_string_to_color( entry->value.str_val,&priv->bg);
}

static void
_notify_text(AwnConfigClientNotifyEntry *entry,
             AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;

  g_return_if_fail(AWN_IS_TOOLTIP(tooltip));
  priv = tooltip->priv;

  if (priv->text_col)
  {
    g_free(priv->text_col);
  }
  
  priv->text_col = g_strdup(entry->value.str_val);

  priv->text_col[6] = '\0';
}

/*  GOBJECT STUFF */

static void
awn_tooltip_finalize(GObject *obj)
{
  AwnTooltip *tooltip;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(AWN_IS_TOOLTIP(obj));

  tooltip = AWN_TOOLTIP(obj);

  G_OBJECT_CLASS(awn_tooltip_parent_class)->finalize(obj);
}

static void
awn_tooltip_class_init(AwnTooltipClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = awn_tooltip_finalize;

  widget_class = GTK_WIDGET_CLASS(klass);
  widget_class->expose_event = awn_tooltip_expose_event;

  g_type_class_add_private(gobject_class, sizeof(AwnTooltipPrivate));

}

static void
awn_tooltip_init(AwnTooltip *tooltip)
{
  AwnTooltipPrivate *priv;
  GtkWidget *hbox;
  AwnConfigClient *client;

  priv = tooltip->priv = AWN_TOOLTIP_GET_PRIVATE(tooltip);

  on_alpha_screen_changed(GTK_WIDGET(tooltip), NULL, NULL);
  gtk_widget_set_app_paintable(GTK_WIDGET(tooltip), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(tooltip), 4);

  priv->focus = NULL;

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(tooltip), hbox);

  priv->image = NULL;

  priv->label = gtk_label_new("");
  gtk_label_set_line_wrap(GTK_LABEL(priv->label), FALSE);
  gtk_label_set_ellipsize(GTK_LABEL(priv->label), PANGO_ELLIPSIZE_NONE);
  gtk_box_pack_end(GTK_BOX(hbox), priv->label, TRUE, TRUE, 4);

  gtk_window_set_policy(GTK_WINDOW(tooltip), FALSE, FALSE, TRUE);

  g_signal_connect(tooltip, "leave-notify-event",
                   G_CALLBACK(on_prox_out), (gpointer)tooltip);

  /* AwnConfigClient stuff */
  client = awn_config_client_new();

  priv->font = g_strdup(
                 awn_config_client_get_string(client, TOOLTIP, FONT_FACE, NULL));
  
  awn_config_client_notify_add(client, TOOLTIP, FONT_FACE,
                               (AwnConfigClientNotifyFunc)_notify_font,
                               tooltip);

  awn_cairo_string_to_color(
    awn_config_client_get_string(client, TOOLTIP, BACKGROUND, NULL),
    &priv->bg);
  
  awn_config_client_notify_add(client, TOOLTIP, BACKGROUND,
                               (AwnConfigClientNotifyFunc)_notify_bg,
                               tooltip);

  priv->text_col = g_strdup(
         	   awn_config_client_get_string(client, TOOLTIP, TEXT_COLOR, NULL)
			);
  
  priv->text_col[6] = '\0';
  awn_config_client_notify_add(client, TOOLTIP, TEXT_COLOR,
                               (AwnConfigClientNotifyFunc)_notify_text,
                               tooltip);
  priv->offset = awn_config_client_get_int(client, "bar", "icon_offset", NULL);
}

GtkWidget *
awn_tooltip_get_default(void)
{
  static GtkWidget *tooltip = NULL;

  if (!tooltip)
  {
    tooltip = g_object_new(AWN_TYPE_TOOLTIP,
                         "type", GTK_WINDOW_POPUP,
                         "decorated", FALSE,
                         "skip-pager-hint", TRUE,
                         "skip-taskbar-hint", TRUE,
                         NULL);
  }
  return tooltip;
}

