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

#include "awn-title.h"

#include "awn-cairo-utils.h"
#include "awn-config-client.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

G_DEFINE_TYPE(AwnTitle, awn_title, GTK_TYPE_WINDOW);

#define AWN_TITLE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                    AWN_TYPE_TITLE, \
                                    AwnTitlePrivate))

#define TITLE         "title"
#define TEXT_COLOR "text_color" /*color*/
#define BACKGROUND "background" /*color*/
#define FONT_FACE "font_face" /*bool*/

/* STRUCTS & ENUMS */

struct _AwnTitlePrivate
{
  GtkWidget *focus;

  GtkWidget *image;
  GtkWidget *label;

  gchar *font;
  AwnColor bg;
  gchar *text_col;
  gint offset;

  guint tag;
};

/* Private */
static void
awn_title_position(AwnTitle *title)
{
  AwnTitlePrivate *priv;
  gint x = 0, y = 0, w, h;
  gint fx, fy, fw, fh;

  g_return_if_fail(AWN_IS_TITLE(title));
  priv = title->priv;

  if (!GTK_IS_WIDGET(priv->focus))
  {
    gtk_widget_hide(GTK_WIDGET(title));
    return;
  }

  /* Get our dimensions */
  GtkRequisition req, req2;

  gtk_widget_size_request(GTK_WIDGET(priv->label), &req);

  gtk_widget_size_request(GTK_WIDGET(title), &req2);

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
  gtk_window_move(GTK_WINDOW(title), x, y);
}

/* Public */

static gboolean
on_prox_out(GtkWidget *focus, GdkEventCrossing *event, AwnTitle *title)
{
  awn_title_hide(title, focus);
  g_signal_handlers_disconnect_by_func(focus,on_prox_out,title);
  return FALSE;
}

static gboolean
show(gchar *text)
{
  AwnTitle *title = AWN_TITLE(awn_title_get_default());
  AwnTitlePrivate *priv;
  gchar *normal;
  gchar *markup;

  priv = title->priv;

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

  awn_title_position(title);

  gtk_widget_show_all(GTK_WIDGET(title));

  gtk_widget_queue_draw(GTK_WIDGET(title));

  g_free(normal);
  g_free(markup);
  g_free(text);

  return FALSE;
}

void
awn_title_show(AwnTitle *title, GtkWidget *focus, const gchar *text)
{
  AwnTitlePrivate *priv;

  g_return_if_fail(AWN_IS_TITLE(title));
  g_return_if_fail(GTK_IS_WIDGET(focus));
  g_return_if_fail(text);
  
  priv = title->priv;
  priv->focus = focus;

  //TMP FIX.: if bug gets solved, this can go back to normal.
  show(g_strdup(text));
  //g_timeout_add(1, (GSourceFunc)show, g_strdup(text));
}

void
awn_title_hide(AwnTitle *title, GtkWidget *focus)
{
  AwnTitlePrivate *priv;

  g_return_if_fail(AWN_IS_TITLE(title));
  g_return_if_fail(GTK_IS_WIDGET(focus));
  priv = title->priv;

  priv->focus = NULL;
  gtk_widget_hide(GTK_WIDGET(title));
}

/* Overrides */
static gboolean
awn_title_expose_event(GtkWidget *widget, GdkEventExpose *expose)
{
  AwnTitlePrivate *priv;
  cairo_t *cr = NULL;
  gint width, height;

  priv = AWN_TITLE(widget)->priv;

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
             AwnTitle *title)
{
  AwnTitlePrivate *priv;

  g_return_if_fail(AWN_IS_TITLE(title));
  priv = title->priv;

  if (priv->font)
  {
    g_free(priv->font);
  }
  priv->font = g_strdup(entry->value.str_val);
}

static void
_notify_bg(AwnConfigClientNotifyEntry *entry,
           AwnTitle *title)
{
  AwnTitlePrivate *priv;

  g_return_if_fail(AWN_IS_TITLE(title));
  priv = title->priv;

  awn_cairo_string_to_color( entry->value.str_val,&priv->bg);
}

static void
_notify_text(AwnConfigClientNotifyEntry *entry,
             AwnTitle *title)
{
  AwnTitlePrivate *priv;

  g_return_if_fail(AWN_IS_TITLE(title));
  priv = title->priv;

  if (priv->text_col)
  {
    g_free(priv->text_col);
  }
  
  priv->text_col = g_strdup(entry->value.str_val);

  priv->text_col[6] = '\0';
}

/*  GOBJECT STUFF */

static void
awn_title_finalize(GObject *obj)
{
  AwnTitle *title;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(AWN_IS_TITLE(obj));

  title = AWN_TITLE(obj);

  G_OBJECT_CLASS(awn_title_parent_class)->finalize(obj);
}

static void
awn_title_class_init(AwnTitleClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = awn_title_finalize;

  widget_class = GTK_WIDGET_CLASS(klass);
  widget_class->expose_event = awn_title_expose_event;

  g_type_class_add_private(gobject_class, sizeof(AwnTitlePrivate));

}

static void
awn_title_init(AwnTitle *title)
{
  AwnTitlePrivate *priv;
  GtkWidget *hbox;
  AwnConfigClient *client;

  priv = title->priv = AWN_TITLE_GET_PRIVATE(title);

  on_alpha_screen_changed(GTK_WIDGET(title), NULL, NULL);
  gtk_widget_set_app_paintable(GTK_WIDGET(title), TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(title), 4);

  priv->focus = NULL;

  hbox = gtk_hbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(title), hbox);

  priv->image = NULL;

  priv->label = gtk_label_new("");
  gtk_label_set_line_wrap(GTK_LABEL(priv->label), FALSE);
  gtk_label_set_ellipsize(GTK_LABEL(priv->label), PANGO_ELLIPSIZE_NONE);
  gtk_box_pack_end(GTK_BOX(hbox), priv->label, TRUE, TRUE, 4);

  gtk_window_set_policy(GTK_WINDOW(title), FALSE, FALSE, TRUE);

  g_signal_connect(title, "leave-notify-event",
                   G_CALLBACK(on_prox_out), (gpointer)title);

  /* AwnConfigClient stuff */
  client = awn_config_client_new();

  priv->font = g_strdup(
                 awn_config_client_get_string(client, TITLE, FONT_FACE, NULL));
  
  awn_config_client_notify_add(client, TITLE, FONT_FACE,
                               (AwnConfigClientNotifyFunc)_notify_font,
                               title);

  awn_cairo_string_to_color(
    awn_config_client_get_string(client, TITLE, BACKGROUND, NULL),
    &priv->bg);
  
  awn_config_client_notify_add(client, TITLE, BACKGROUND,
                               (AwnConfigClientNotifyFunc)_notify_bg,
                               title);

  priv->text_col = g_strdup(
         	   awn_config_client_get_string(client, TITLE, TEXT_COLOR, NULL)
			);
  
  priv->text_col[6] = '\0';
  awn_config_client_notify_add(client, TITLE, TEXT_COLOR,
                               (AwnConfigClientNotifyFunc)_notify_text,
                               title);
  priv->offset = awn_config_client_get_int(client, "bar", "icon_offset", NULL);
}

GtkWidget *
awn_title_get_default(void)
{
  static GtkWidget *title = NULL;

  if (!title)
  {
    title = g_object_new(AWN_TYPE_TITLE,
                         "type", GTK_WINDOW_POPUP,
                         "decorated", FALSE,
                         "skip-pager-hint", TRUE,
                         "skip-taskbar-hint", TRUE,
                         //"allow-shrink", TRUE,
                         //"allow-grow", TRUE,
                         NULL);
  }
  return title;
}

