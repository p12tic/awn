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
 * AwnImage:
 *
 * Widget derived from #GtkImage, implementing #AwnOverlayable, so it can be
 * used with overlays, animations and other eye-candy (using #AwnEffects).
 */

#include "awn-image.h"
#include "awn-effects.h"
#include "awn-overlayable.h"

static void awn_image_overlayable_init (AwnOverlayableIface *iface);

static AwnEffects* awn_image_get_effects (AwnOverlayable *image);

G_DEFINE_TYPE_WITH_CODE (AwnImage, awn_image, GTK_TYPE_IMAGE,
                         G_IMPLEMENT_INTERFACE (AWN_TYPE_OVERLAYABLE,
                                                awn_image_overlayable_init))

#define AWN_IMAGE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_IMAGE, AwnImagePrivate))

struct _AwnImagePrivate
{
  AwnEffects *effects;
};

static void
awn_image_dispose (GObject *object)
{
  AwnImagePrivate *priv = AWN_IMAGE_GET_PRIVATE (object);

  if (priv->effects)
  {
    g_object_unref (priv->effects);
    priv->effects = NULL;
  }

  G_OBJECT_CLASS (awn_image_parent_class)->dispose (object);
}

static void
awn_image_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_image_parent_class)->finalize (object);
}

static void
awn_image_size_request (GtkWidget *widget, GtkRequisition *req)
{
  gint xpad, ypad, offset;
  GtkPositionType pos;
  GTK_WIDGET_CLASS (awn_image_parent_class)->size_request (widget, req);

  AwnImagePrivate *priv = AWN_IMAGE_GET_PRIVATE (widget);

  gtk_misc_get_padding (GTK_MISC (widget), &xpad, &ypad);
  awn_effects_set_icon_size (priv->effects,
                             req->width - xpad * 2,
                             req->height - ypad * 2,
                             FALSE);

  g_object_get (priv->effects, "position", &pos, NULL);
  if (pos == GTK_POS_TOP || pos == GTK_POS_BOTTOM)
  {
    offset = ypad;
  }
  else
  {
    offset = xpad;
  }
  g_object_set (priv->effects, "icon-offset", offset, NULL);
}

static gboolean
awn_image_expose (GtkWidget *widget, GdkEventExpose *event)
{
  AwnImagePrivate *priv = AWN_IMAGE (widget)->priv;
  GtkImage *image = GTK_IMAGE (widget);
  cairo_t *cr;
  GdkPixbuf *pixbuf = NULL;
  GdkPixmap *pixmap = NULL;

  switch (gtk_image_get_storage_type (GTK_IMAGE (widget)))
  {
    case GTK_IMAGE_EMPTY:
      break;
    case GTK_IMAGE_PIXBUF:
      pixbuf = gtk_image_get_pixbuf (image);
      break;
    case GTK_IMAGE_PIXMAP:
      gtk_image_get_pixmap (image, &pixmap, NULL);
      break;
    default:
      g_warning ("AwnImage doesn't support this storage type");
      return FALSE;
  }

  cr = awn_effects_cairo_create_clipped (priv->effects, event);
  g_return_val_if_fail (cr, FALSE);

  if (pixbuf || pixmap)
  {
    if (pixbuf)
    {
      gdk_cairo_set_source_pixbuf (cr, pixbuf, 0.0, 0.0);
    }
    else if (pixmap)
    {
      gdk_cairo_set_source_pixmap (cr, pixmap, 0.0, 0.0);
    }

    cairo_paint (cr);
  }

  awn_effects_cairo_destroy (priv->effects);

  return TRUE;
}

static void
awn_image_class_init (AwnImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose      = awn_image_dispose;
  object_class->finalize     = awn_image_finalize;

  widget_class->size_request = awn_image_size_request;
  widget_class->expose_event = awn_image_expose;

  g_type_class_add_private (klass, sizeof (AwnImagePrivate));
}

static void
awn_image_init (AwnImage *self)
{
  AwnImagePrivate *priv;
  priv = self->priv = AWN_IMAGE_GET_PRIVATE (self);

  priv->effects = awn_effects_new_for_widget (GTK_WIDGET (self));
}

static void awn_image_overlayable_init (AwnOverlayableIface *iface)
{
  iface->get_effects = awn_image_get_effects;
}

/**
 * awn_image_new:
 *
 * Creates new instance of #AwnImage.
 *
 * Returns: An instance of #AwnImage.
 */
AwnImage*
awn_image_new (void)
{
  return g_object_new (AWN_TYPE_IMAGE, NULL);
}

static AwnEffects* awn_image_get_effects (AwnOverlayable *image)
{
  AwnImage *img = AWN_IMAGE (image);

  return img->priv->effects;
}

