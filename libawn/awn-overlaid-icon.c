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

#define MOVE_FLAGS_NONE 0x00
#define MOVE_FLAGS_TEXT 0x01


struct _AwnOverlaidIconPrivate
{
  gpointer  * padding;
  GList     * overlays;
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



static void
awn_overlaid_icon_move_to (cairo_t * cr,
                           AwnOverlay* overlay,
                           gint   icon_width,
                           gint   icon_height,
                           gint   overlay_width,
                           gint   overlay_height,
                           guint flags)
{
  gint yoffset = 0;
  gdouble xoffset;
  gint align;
  AwnGravity gravity;
  gdouble x_adj;
  gdouble y_adj;

  g_object_get (overlay,
               "align", &align,
               "gravity", &gravity,
                "x_adj", &x_adj,
                "y_adj", &y_adj,
               NULL);
  if (flags & MOVE_FLAGS_TEXT) /*text rendered top left hand corner of the text is not where one would expect it to be */
  {
    yoffset = overlay_height / 2.0 + 2; /*Magic is bad FIXME*/
  }
  switch (align)
  {
    case AWN_OVERLAY_ALIGN_CENTRE:
      xoffset = 0;
      break;
    case AWN_OVERLAY_ALIGN_LEFT:
      xoffset = overlay_width / 2.0;
      break;
    case AWN_OVERLAY_ALIGN_RIGHT:
      xoffset = -1 * overlay_width / 2.0;      
      break;
    default:
      g_assert_not_reached();
  }
  xoffset = xoffset + (x_adj * icon_width);
  yoffset = yoffset + (y_adj * icon_height);
  switch (gravity)
  {
    case AWN_GRAVITY_CENTRE:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0 + xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);  
      break;
    case AWN_GRAVITY_N:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0 + xoffset, 1 + icon_height / 20 + yoffset);  
      break;      
    case AWN_GRAVITY_NE:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, 1 + icon_height / 20 + yoffset);  
      break;
    case AWN_GRAVITY_E:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);
      break;      
    case AWN_GRAVITY_SE:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, icon_height - overlay_height -1+ yoffset);      
      break;
    case AWN_GRAVITY_S:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0+ xoffset, icon_height - overlay_height -1+ yoffset);
      break;
    case AWN_GRAVITY_SW:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, icon_height - overlay_height -1+ yoffset);
      break;
    case AWN_GRAVITY_W:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);
      break;
    case AWN_GRAVITY_NW:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, 1 + icon_height / 20 + yoffset);  
      break;
    default:
      g_assert_not_reached();
      
  }

}
                           

static void
awn_overlaid_icon_render_text (cairo_t * cr, 
                               AwnOverlaidIcon * icon, 
                               gint width,
                               gint height,
                               AwnOverlay* overlay)
{
  AwnOverlaidIconPrivate *priv;
  DesktopAgnosticColor * text_colour; /*FIXME*/

  text_colour = desktop_agnostic_color_new(&GTK_WIDGET(icon)->style->fg[GTK_STATE_ACTIVE], G_MAXUSHORT);
  awn_cairo_set_source_color (cr,text_colour);
  cairo_text_extents_t extents;
  priv = AWN_OVERLAID_ICON_GET_PRIVATE (icon);
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, overlay->type_specific.text_data.sizing * width / 48.0);
  cairo_text_extents(cr, overlay->data.text, &extents);  
  awn_overlaid_icon_move_to (cr, overlay, width, height,extents.width,extents.height,MOVE_FLAGS_TEXT);
  cairo_show_text(cr, overlay->data.text);
  g_object_unref (text_colour);  
}

static void
awn_overlaid_icon_render_surface (cairo_t * cr, 
                               AwnOverlaidIcon * icon, 
                               gint width,
                               gint height,
                               AwnOverlay* overlay)
{
  
}

static void
awn_overlaid_icon_render_pixbuf (cairo_t * cr, 
                               AwnOverlaidIcon * icon, 
                               gint width,
                               gint height,
                               AwnOverlay* overlay)
{
  
}

static void
awn_overlaid_icon_render_icon (cairo_t * cr, 
                               AwnOverlaidIcon * icon, 
                               gint width,
                               gint height,
                               AwnOverlay* overlay)
{
  
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
    switch ( overlay->overlay_type)
    {
      case  AWN_OVERLAY_TEXT:
        awn_overlaid_icon_render_text (ctx, AWN_OVERLAID_ICON(widget), 
                                       icon_width,
                                       icon_height,
                                       overlay);
        break;
      case AWN_OVERLAY_SURFACE:
        awn_overlaid_icon_render_surface (ctx, AWN_OVERLAID_ICON(widget), 
                                       icon_width,
                                       icon_height,
                                       overlay);
        break;        
      case AWN_OVERLAY_PIXBUF:  
        awn_overlaid_icon_render_pixbuf (ctx, AWN_OVERLAID_ICON(widget), 
                                       icon_width,
                                       icon_height,
                                       overlay);
        break;        
      case AWN_OVERLAY_ICON:
        awn_overlaid_icon_render_icon (ctx, AWN_OVERLAID_ICON(widget), 
                                       icon_width,
                                       icon_height,
                                       overlay);
        break;
      default:
        g_assert_not_reached();
    }
  }
  
  awn_effects_cairo_destroy (effects);
  return FALSE;
}

static void
awn_overlaid_icon_init (AwnOverlaidIcon *icon)
{
  AwnOverlaidIconPrivate *priv;

  priv = icon->priv = AWN_OVERLAID_ICON_GET_PRIVATE (icon);
  
  g_signal_connect_after (icon, "expose-event", G_CALLBACK(_awn_overlaid_icon_expose),NULL);

  priv->overlays = NULL;
}

GtkWidget *
awn_overlaid_icon_new (void)

{
  GtkWidget *overlaid_icon = NULL;
  
  overlaid_icon = g_object_new (AWN_TYPE_OVERLAID_ICON, 
                              NULL);

  return overlaid_icon;
}


AwnOverlay *
awn_overlaid_icon_append_overlay (AwnOverlaidIcon * icon,AwnOverlayType  type,
                                                      AwnGravity      grav,
                                                      gpointer        data)
{
  AwnOverlaidIconPrivate *priv;
  AwnOverlay* overlay;
  guint align;
  AwnGravity gravity;
  gint x_adj;
  gint y_adj;

  priv = icon->priv = AWN_OVERLAID_ICON_GET_PRIVATE (icon);
  overlay = awn_overlay_new();

  g_object_set (overlay,
               "align", AWN_OVERLAY_ALIGN_RIGHT,
               "gravity", grav,
                "x_adj", 0.3,
                "y_adj", 0.0,
               NULL);
  
  switch (type)
  {
    case AWN_OVERLAY_TEXT:
      overlay->type_specific.text_data.sizing = AWN_FONT_SIZE_MEDIUM;
      overlay->data.text = g_strdup (data);
      break;
    case AWN_OVERLAY_SURFACE:
      overlay->data.srfc = data;
      cairo_surface_reference (overlay->data.srfc);
      break;
    case AWN_OVERLAY_PIXBUF:  
      overlay->data.pixbuf = data;
      g_object_ref (overlay->data.pixbuf);
      break;
    case AWN_OVERLAY_ICON:
      overlay->data.icon_name = g_strdup (data);
      break;
    default:
      g_assert_not_reached();
  }
  priv->overlays = g_list_append (priv->overlays,overlay);   
  return overlay;
}

AwnOverlay *  awn_overlaid_icon_change_overlay_data (AwnOverlaidIcon * icon,
                                                    AwnOverlay  *overlay,
                                                    gpointer new_data)
{
  switch (overlay->overlay_type)
  {
    case AWN_OVERLAY_TEXT:
      g_free (overlay->data.text);
      overlay->data.text = g_strdup ((gchar*)new_data);
      break;
    case AWN_OVERLAY_SURFACE:
      cairo_surface_destroy (overlay->data.srfc);
      overlay->data.srfc = new_data;
      cairo_surface_reference (overlay->data.srfc);
      break;      
    case AWN_OVERLAY_PIXBUF:  
      g_object_unref (overlay->data.pixbuf);
      overlay->data.pixbuf = new_data;
      g_object_ref (overlay->data.pixbuf);
      break;      
    case AWN_OVERLAY_ICON:
      overlay->data.icon_name = g_strdup (new_data);
      break;      
    default:
      g_assert_not_reached();
  }
  return overlay;
}

