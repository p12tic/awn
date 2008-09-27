/*
 * Copyright (C) 2008 Neil Jagdish Patel
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */
#include <gdk/gdkx.h>
#include <cairo/cairo-xlib.h>

#include "awn-icon.h"

G_DEFINE_TYPE (AwnIcon, awn_icon, GTK_TYPE_DRAWING_AREA);

#define AWN_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_ICON, \
  AwnIconPrivate))

struct _AwnIconPrivate
{
  AwnEffects   *effects;
  GtkWidget    *tooltip;
  
  AwnOrientation orient;
  gint           size;

  /* Info relating to the current icon */
  cairo_t *icon_ctx;
  
  /* This is for when the icon is set before we've mapped */
  cairo_t   *queue_ctx;
  GdkPixbuf *queue_pixbuf;
  
  /* Things that can be displayed on the icon */
  gchar    *message;
  gfloat    progress;
  gboolean  active;
};

/* GObject stuff */
static gboolean
awn_icon_enter_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;

  awn_effects_start (priv->effects, AWN_EFFECT_HOVER);

  return FALSE;
}

static gboolean
awn_icon_leave_notify_event (GtkWidget *widget, GdkEventCrossing *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;

  awn_effects_stop (priv->effects, AWN_EFFECT_HOVER);

  return FALSE;
}

static gboolean
awn_icon_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
  AwnIconPrivate *priv = AWN_ICON (widget)->priv;
  cairo_t        *cr;
  gint            width, height;

  width = widget->allocation.width;
  height = widget->allocation.height;

  /* Init() effects for the draw */
  awn_effects_draw_set_window_size (priv->effects, width, height);

  /* Grab the context and clip for the region we're redrawing */
  cr = gdk_cairo_create (widget->window);
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);

  /* Clear the region */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
 
  /* Let effects do its job */
  awn_effects_draw_background (priv->effects, cr);
  
  if (priv->icon_ctx)
    awn_effects_draw_icons_cairo (priv->effects, cr, priv->icon_ctx, NULL);
  
  awn_effects_draw_foreground (priv->effects, cr);

  cairo_destroy (cr);

  return FALSE;
}

static gboolean
awn_icon_mapped (GtkWidget *widget, GdkEvent *event)
{
  AwnIcon *icon = AWN_ICON (widget);
  AwnIconPrivate *priv;
  
  g_return_val_if_fail (AWN_IS_ICON (icon), FALSE);
  priv = AWN_ICON (icon)->priv;

  if (priv->queue_ctx)
  {
    awn_icon_set_from_context (icon, priv->queue_ctx);
    cairo_destroy (priv->queue_ctx);
  }
  if (priv->queue_pixbuf)
  {
    awn_icon_set_from_pixbuf (icon, priv->queue_pixbuf);
    g_object_unref (priv->queue_pixbuf);
  }
  priv->queue_pixbuf = NULL;
  priv->queue_ctx = NULL;

  return FALSE;
}

static void
awn_icon_dispose (GObject *object)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (object));
  priv = AWN_ICON (object)->priv;

  if (priv->icon_ctx)
    cairo_destroy (priv->icon_ctx);
  priv->icon_ctx = NULL;

  if (priv->message)
    g_free (priv->message);
  priv->message = NULL;

  G_OBJECT_CLASS (awn_icon_parent_class)->dispose (object);
}

static void
awn_icon_class_init (AwnIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->dispose = awn_icon_dispose;

  wid_class->expose_event       = awn_icon_expose_event;
  wid_class->enter_notify_event = awn_icon_enter_notify_event;
  wid_class->leave_notify_event = awn_icon_leave_notify_event;

  g_type_class_add_private (obj_class, sizeof (AwnIconPrivate));
}

static void
awn_icon_init (AwnIcon *icon)
{
  AwnIconPrivate *priv;

  priv = icon->priv = AWN_ICON_GET_PRIVATE (icon);

  priv->icon_ctx = NULL;
  priv->message = NULL;
  priv->progress = 0;
  priv->active = FALSE;
  priv->queue_pixbuf = NULL;
  priv->queue_ctx = NULL;
  priv->orient = AWN_ORIENTATION_BOTTOM;
  priv->size = 50;
  priv->tooltip = awn_tooltip_new_for_widget (GTK_WIDGET (icon));

  priv->effects = awn_effects_new_for_widget (GTK_WIDGET (icon));

  gtk_widget_add_events (GTK_WIDGET (icon), GDK_ALL_EVENTS_MASK);

  g_signal_connect_after (icon, "map-event", 
                          G_CALLBACK (awn_icon_mapped), NULL);
}

GtkWidget *
awn_icon_new (void)

{
  GtkWidget *icon = NULL;

  icon = g_object_new (AWN_TYPE_ICON, 
                       NULL);

  return icon;
}

void 
awn_icon_set_orientation (AwnIcon        *icon,
                          AwnOrientation  orient)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (icon));
  priv = icon->priv;
  
  priv->orient = orient;

  switch (orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_RIGHT:
      gtk_widget_set_size_request (GTK_WIDGET (icon), priv->size * 1.2, -1);
      break;
      
    default:
      gtk_widget_set_size_request (GTK_WIDGET (icon), -1, priv->size *1.2);
      break;
  }
}

static void
update_widget_size (AwnIcon *icon)
{
  AwnIconPrivate  *priv = icon->priv;
  cairo_surface_t *surface;
  gint             width;
  gint             height;

  surface = cairo_get_target (priv->icon_ctx);
  width = cairo_xlib_surface_get_width (surface);
  height = cairo_xlib_surface_get_height (surface);

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      priv->size = width;
      gtk_widget_set_size_request (GTK_WIDGET (icon), width * 1.2, -1);
      break;

    default:
      priv->size = height;
      gtk_widget_set_size_request (GTK_WIDGET (icon), -1, height * 1.2);
  }
}

void  
awn_icon_set_effect (AwnIcon *icon, AwnEffect effect)
{ 
  g_return_if_fail (AWN_IS_ICON (icon));

  awn_effects_start (icon->priv->effects, effect);
}

AwnEffects * 
awn_icon_get_effects (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return icon->priv->effects;
}

/*
 * ICON SETTING FUNCTIONS 
 */
static void
free_existing_icon (AwnIcon *icon)
{
  AwnIconPrivate *priv = icon->priv;

  cairo_destroy (priv->icon_ctx);
  priv->icon_ctx = NULL;
}

void 
awn_icon_set_from_pixbuf (AwnIcon *icon, GdkPixbuf *pixbuf)
{
  AwnIconPrivate  *priv;
  GtkWidget       *widget;
  cairo_t         *temp_cr;
  cairo_surface_t *surface;
  gint             width, height;

  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  priv = icon->priv;

  /* If the widget isn't mapped, let's queue the pixbuf for when we are */
  if (!GTK_WIDGET_MAPPED (GTK_WIDGET (icon)))
  {
    if (GDK_IS_PIXBUF (priv->queue_pixbuf))
      g_object_unref (priv->queue_pixbuf);
    if (priv->queue_ctx)
      cairo_destroy (priv->queue_ctx);

    priv->queue_pixbuf = gdk_pixbuf_copy (pixbuf);
    return;
  }

  free_existing_icon (icon);

  widget = GTK_WIDGET (icon);
    
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  temp_cr = gdk_cairo_create (widget->window);

  /* Create the main icon context */
  surface = cairo_surface_create_similar (cairo_get_target (temp_cr),
                                          CAIRO_CONTENT_COLOR_ALPHA,
                                          width, height);
  priv->icon_ctx = cairo_create (surface);
  gdk_cairo_set_source_pixbuf (priv->icon_ctx, pixbuf, 0, 0);
  cairo_paint (priv->icon_ctx);

  /* Queue a redraw */
  update_widget_size (icon);
  gtk_widget_queue_draw (widget);

  /* This should be valid according to the docs */
  cairo_destroy (temp_cr);
  cairo_surface_destroy (surface);
}

void 
awn_icon_set_from_context (AwnIcon *icon, cairo_t *ctx)
{
  AwnIconPrivate       *priv;
  cairo_surface_t      *current_surface;
  cairo_surface_type_t  type;
  
  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (ctx);
  priv = icon->priv;

  /* If the widget isn't mapped, let's queue the ctx for when we are */
  if (!GTK_WIDGET_MAPPED (GTK_WIDGET (icon)))
  {
    if (GDK_IS_PIXBUF (priv->queue_pixbuf))
      g_object_unref (priv->queue_pixbuf);
    if (priv->queue_ctx)
      cairo_destroy (priv->queue_ctx);

    priv->queue_ctx = ctx;
    cairo_reference (ctx);
    return;
  }
  free_existing_icon (icon);

  current_surface = cairo_get_target (ctx);
  type = cairo_surface_get_type (current_surface);

  if (type == CAIRO_SURFACE_TYPE_XLIB)
  {
    /* 'tis all good */
    priv->icon_ctx = ctx;
    cairo_reference (priv->icon_ctx);
  }
  else if (type == CAIRO_SURFACE_TYPE_IMAGE)
  {
    /* Let's convert it to an xlib surface */
    GtkWidget       *widget;
    cairo_t         *temp_cr;
    cairo_surface_t *surface;
    gint             width, height;

    widget = GTK_WIDGET (icon);
    
    temp_cr = gdk_cairo_create (widget->window);

    width = cairo_image_surface_get_width (current_surface);
    height = cairo_image_surface_get_height (current_surface);
    
    surface = cairo_surface_create_similar (cairo_get_target (temp_cr), 
                                            CAIRO_CONTENT_COLOR_ALPHA,
                                            width, height);
    priv->icon_ctx = cairo_create (surface);
    cairo_set_source_surface (priv->icon_ctx, current_surface, 0, 0);
    cairo_paint (priv->icon_ctx);

    cairo_destroy (temp_cr);
    cairo_surface_destroy (surface);
  }
  else
  {
    g_warning ("Invalid surface type: Surfaces must be either xlib or image");
    return;
  }

  update_widget_size (icon);
  gtk_widget_queue_draw (GTK_WIDGET (icon));
}

/*
 * The tooltip 
 */
void   
awn_icon_set_tooltip_text (AwnIcon     *icon,
                           const gchar *text)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  awn_tooltip_set_text (AWN_TOOLTIP (icon->priv->tooltip), text);
}

const gchar * 
awn_icon_get_tooltip_text (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return awn_tooltip_get_text (AWN_TOOLTIP (icon->priv->tooltip));
}

/*
 * ICON EMBLEMS
 */

void 
awn_icon_set_message (AwnIcon *icon, const gchar  *message)
{
  AwnIconPrivate *priv;

  g_return_if_fail (AWN_IS_ICON (icon));
  priv = icon->priv;

  if (priv->message)
    g_free (priv->message);

  if (message)
    priv->message = g_strdup (message);
  else
    priv->message = NULL;

  gtk_widget_queue_draw (GTK_WIDGET (icon));
}

const gchar * 
awn_icon_get_message (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), NULL);

  return icon->priv->message;
}  

void
awn_icon_set_progress (AwnIcon *icon, gfloat progress)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  icon->priv->progress = progress;

  gtk_widget_queue_draw (GTK_WIDGET (icon));
}

gfloat        
awn_icon_get_progress (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), 0.0);

  return icon->priv->progress;
}

void
awn_icon_set_is_active (AwnIcon *icon, gboolean is_active)
{
  g_return_if_fail (AWN_IS_ICON (icon));

  icon->priv->active = is_active;

  gtk_widget_queue_draw (GTK_WIDGET (icon));
}

gboolean      
awn_icon_get_is_active (AwnIcon *icon)
{
  g_return_val_if_fail (AWN_IS_ICON (icon), FALSE);

  return icon->priv->active;
}
