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
 * You should have received a copy of the GNU Library General Public License
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
  /* Info relating to the current icon */
  cairo_t *icon_ctx;
  
  /* Things that can be displayed on the icon */
  gchar    *message;
  gfloat    progress;
  gboolean  active;
};

/* GObject stuff */
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
  GObjectClass        *obj_class = G_OBJECT_CLASS (klass);

  obj_class->dispose = awn_icon_dispose;

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
awn_icon_set_state (AwnIcon *icon, AwnIconState  state)
{
  /*FIXME: Do something with AwnEffects */
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
awn_icon_set_icon_from_pixbuf (AwnIcon *icon, GdkPixbuf *pixbuf)
{
  AwnIconPrivate  *priv;
  GtkWidget       *widget;
  GdkVisual       *visual;
  cairo_surface_t *surface;
  gint             width, height;

  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (GDK_IS_PIXBUF (pixbuf));
  priv = icon->priv;

  free_existing_icon (icon);

  widget = GTK_WIDGET (icon);
  visual = gtk_widget_get_visual (widget);
  
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  /* Create the main icon context */
  surface = cairo_xlib_surface_create (GDK_DISPLAY (),
                                       GDK_WINDOW_XID (widget->window),
                                       GDK_VISUAL_XVISUAL (visual),
                                       width, height);
  priv->icon_ctx = cairo_create (surface);
  gdk_cairo_set_source_pixbuf (priv->icon_ctx, pixbuf, 0, 0);
  cairo_paint (priv->icon_ctx);

  /* Queue a redraw */
  gtk_widget_queue_draw (widget);

  /* This should be valid according to the docs */
  cairo_surface_destroy (surface);
}

void 
awn_icon_set_icon_from_context (AwnIcon *icon, cairo_t *ctx)
{
  AwnIconPrivate       *priv;
  cairo_surface_t      *current_surface;
  cairo_surface_type_t  type;
  
  g_return_if_fail (AWN_IS_ICON (icon));
  g_return_if_fail (ctx);
  priv = icon->priv;

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
    GdkVisual       *visual;
    cairo_surface_t *surface;
    gint             width, height;

    widget = GTK_WIDGET (icon);
    visual = gtk_widget_get_visual (widget);

    width = cairo_image_surface_get_width (current_surface);
    height = cairo_image_surface_get_height (current_surface);
    
    surface = cairo_xlib_surface_create (GDK_DISPLAY (),
                                         GDK_WINDOW_XID (widget->window),
                                         GDK_VISUAL_XVISUAL (visual),
                                         width, height);
    priv->icon_ctx = cairo_create (surface);
    cairo_set_source_surface (priv->icon_ctx, current_surface, 0, 0);
    cairo_paint (priv->icon_ctx);

    cairo_surface_destroy (surface);
  }
  else
  {
    g_warning ("Invalid surface type: Surfaces must be either xlib or image");
    return;
  }

  gtk_widget_queue_draw (GTK_WIDGET (icon));
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
