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

#include "awn-image.h"
#include "awn-effects.h"
#include "awn-overlayable.h"
#include "awn-overlay.h"
#include "awn-overlay-text.h"

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

  AwnOverlayText *overlay;
  gboolean overlay_added;
};

enum
{
  PROP_0,

  PROP_LABEL
};

static void
awn_image_get_property (GObject *object, guint property_id,
                        GValue *value, GParamSpec *pspec)
{
  AwnImagePrivate *priv = AWN_IMAGE_GET_PRIVATE (object);

  switch (property_id)
  {
    case PROP_LABEL:
      g_object_get_property (G_OBJECT (priv->overlay), "text", value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_image_set_property (GObject *object, guint property_id,
                        const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
  {
    case PROP_LABEL:
      awn_image_set_label (AWN_IMAGE (object), g_value_get_string(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

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
  AwnImagePrivate *priv = AWN_IMAGE_GET_PRIVATE (object);

  g_object_unref (priv->overlay);
  priv->overlay = NULL;

  G_OBJECT_CLASS (awn_image_parent_class)->finalize (object);
}

static void
awn_image_size_request (GtkWidget *widget, GtkRequisition *req)
{
  GTK_WIDGET_CLASS (awn_image_parent_class)->size_request (widget, req);

  AwnImagePrivate *priv = AWN_IMAGE_GET_PRIVATE (widget);

  if (gtk_image_get_storage_type (GTK_IMAGE (widget)) == GTK_IMAGE_EMPTY
      && priv->overlay_added)
  {
    gint w, h;
    awn_overlay_text_get_size (priv->overlay, widget, NULL, 0, &w, &h);
    req->width += w;
    req->height += h;
  }

  awn_effects_set_icon_size (priv->effects, req->width, req->height, FALSE);
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

  object_class->get_property = awn_image_get_property;
  object_class->set_property = awn_image_set_property;
  object_class->dispose      = awn_image_dispose;
  object_class->finalize     = awn_image_finalize;

  widget_class->size_request = awn_image_size_request;
  widget_class->expose_event = awn_image_expose;

  g_object_class_install_property (object_class,
    PROP_LABEL,
    g_param_spec_string ("label",
                         "Label",
                         "Label to display",
                         NULL,
                         G_PARAM_READWRITE));

  g_type_class_add_private (klass, sizeof (AwnImagePrivate));
}

static void
awn_image_init (AwnImage *self)
{
  AwnImagePrivate *priv;
  priv = self->priv = AWN_IMAGE_GET_PRIVATE (self);

  priv->effects = awn_effects_new_for_widget (GTK_WIDGET (self));

  priv->overlay = g_object_ref_sink (awn_overlay_text_new ());
  g_signal_connect_swapped (priv->overlay, "notify",
                            G_CALLBACK (gtk_widget_queue_resize), self);
}

static void awn_image_overlayable_init (AwnOverlayableIface *iface)
{
  iface->get_effects = awn_image_get_effects;
}

AwnImage*
awn_image_new (void)
{
  return g_object_new (AWN_TYPE_IMAGE, NULL);
}

AwnImage*
awn_image_new_with_label (const gchar* label)
{
  return g_object_new (AWN_TYPE_IMAGE, "label", label, NULL);
}

static AwnEffects* awn_image_get_effects (AwnOverlayable *image)
{
  AwnImage *img = AWN_IMAGE (image);

  return img->priv->effects;
}

void
awn_image_set_label (AwnImage *image, const gchar *label)
{
  AwnImagePrivate *priv;
  g_return_if_fail (AWN_IS_IMAGE (image));

  priv = image->priv;

  g_object_set (priv->overlay, "text", label, NULL);

  if (!priv->overlay_added)
  {
    awn_overlayable_add_overlay (AWN_OVERLAYABLE (image),
                                 AWN_OVERLAY (priv->overlay));
    priv->overlay_added = TRUE;
  }
}

AwnOverlayText*
awn_image_get_overlay_text (AwnImage *image)
{
  AwnImagePrivate *priv;
  g_return_val_if_fail (AWN_IS_IMAGE (image), NULL);

  priv = image->priv;

  if (!priv->overlay_added)
  {
    awn_overlayable_add_overlay (AWN_OVERLAYABLE (image),
                                 AWN_OVERLAY (priv->overlay));
    priv->overlay_added = TRUE;
  }
  return priv->overlay;
}

