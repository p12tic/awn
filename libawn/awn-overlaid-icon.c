/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 *
 */

#include <glib/gstdio.h>
#include <string.h>
#include <gio/gio.h>
#include <cairo/cairo-xlib.h>
#include <math.h>
#include <awn-cairo-utils.h>

#include "awn-overlaid-icon.h"

G_DEFINE_TYPE (AwnOverlaidIcon, awn_overlaid_icon, AWN_TYPE_THEMED_ICON)

#define AWN_OVERLAID_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_OVERLAID_ICON, \
  AwnOverlaidIconPrivate))

struct _AwnOverlaidIconPrivate
{
  gpointer  * padding;
  GList     * overlays;
  gulong    sig_id;
};

/* Forwards */


/* GObject stuff */
static void
awn_overlaid_icon_dispose (GObject *object)
{
  AwnOverlaidIconPrivate *priv;

  g_return_if_fail (AWN_IS_OVERLAID_ICON (object));
  priv = AWN_OVERLAID_ICON (object)->priv;

  G_OBJECT_CLASS (awn_overlaid_icon_parent_class)->dispose (object);
}

static void
awn_overlaid_icon_class_init (AwnOverlaidIconClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
  
  obj_class->dispose = awn_overlaid_icon_dispose;
  
  g_type_class_add_private (obj_class, sizeof (AwnOverlaidIconPrivate));  
}

static gboolean
_awn_overlaid_icon_expose (GtkWidget *widget,
                           GdkEventExpose *event,
                           gpointer null)
{
  int icon_height;
  int icon_width;  
  int orientation;
  GList * iter = NULL;
  AwnOverlaidIconPrivate *priv;
  cairo_t * ctx;
  AwnEffects * effects;
  
  priv = AWN_OVERLAID_ICON_GET_PRIVATE (AWN_OVERLAID_ICON(widget));
  
  effects = awn_icon_get_effects (AWN_ICON(widget));
  g_return_val_if_fail (effects,FALSE);
  ctx = awn_effects_cairo_create(effects);
  g_return_val_if_fail (ctx,FALSE);
  g_object_get (widget, 
                "icon_height", &icon_height,
                "icon_width", &icon_width,
                NULL);     
  for(iter=g_list_first (priv->overlays);iter;iter=g_list_next (priv->overlays))
  {
    AwnOverlay* overlay = iter->data;

    awn_overlay_render_overlay (overlay,AWN_THEMED_ICON(widget),ctx,icon_width,icon_height);
  }
  
  awn_effects_cairo_destroy (effects);
  return FALSE;
}

static void
awn_overlaid_icon_init (AwnOverlaidIcon *icon)
{
  AwnOverlaidIconPrivate *priv;

  priv = icon->priv = AWN_OVERLAID_ICON_GET_PRIVATE (icon);
  priv->overlays = NULL;
}

GtkWidget *
awn_overlaid_icon_new (void)

{
  GtkWidget *overlaid_icon = NULL;
  
  overlaid_icon = g_object_new (AWN_TYPE_OVERLAID_ICON, NULL);
  return overlaid_icon;
}

void 
awn_overlaid_icon_append_overlay (AwnOverlaidIcon * icon,
                                  AwnOverlay * overlay
                                  )
{
  AwnOverlaidIconPrivate *priv;
  priv = icon->priv = AWN_OVERLAID_ICON_GET_PRIVATE (icon);
  
  /* if it's the first overlay _then_ we connect the signal*/
  if (!priv->overlays)
  {
    priv->sig_id = g_signal_connect_after (icon, "expose-event", G_CALLBACK(_awn_overlaid_icon_expose),NULL);    
  }
  priv->overlays = g_list_append (priv->overlays,overlay);   
}



