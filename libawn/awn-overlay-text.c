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


/* awn-overlay-text.c */

#include "awn-overlay-text.h"
#include <awn-cairo-utils.h>
#include <gtk/gtk.h>


G_DEFINE_TYPE (AwnOverlayText, awn_overlay_text, AWN_TYPE_OVERLAY)

#define AWN_OVERLAY_TEXT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_TEXT, AwnOverlayTextPrivate))

typedef struct _AwnOverlayTextPrivate AwnOverlayTextPrivate;

struct _AwnOverlayTextPrivate {
    gchar * text;
    gdouble font_sizing;
};

enum
{
  PROP_0,
  PROP_FONT_SIZING,
  PROP_TEXT 
};

static void 
_awn_overlay_text_render ( AwnOverlay* overlay,
                               AwnThemedIcon * icon,                          
                               cairo_t * cr,                                 
                               gint width,
                               gint height);


static void
awn_overlay_text_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayTextPrivate * priv;
  priv = AWN_OVERLAY_TEXT_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_FONT_SIZING:
      g_value_set_double (value,priv->font_sizing);
      break;
    case PROP_TEXT:
      g_value_set_string (value,priv->text);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_text_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayTextPrivate * priv;
  priv = AWN_OVERLAY_TEXT_GET_PRIVATE (object);

  switch (property_id) 
  {
    case PROP_FONT_SIZING:
      priv->font_sizing = g_value_get_double (value);
      break;
    case PROP_TEXT:
      g_free(priv->text);
      priv->text = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_text_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_text_parent_class)->dispose (object);
}

static void
awn_overlay_text_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_text_parent_class)->finalize (object);
}

static void
awn_overlay_text_class_init (AwnOverlayTextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;    

  object_class->get_property = awn_overlay_text_get_property;
  object_class->set_property = awn_overlay_text_set_property;
  object_class->dispose = awn_overlay_text_dispose;
  object_class->finalize = awn_overlay_text_finalize;
  
  AWN_OVERLAY_CLASS(klass)->render_overlay = _awn_overlay_text_render;
  
  pspec = g_param_spec_double ("font_sizing",
                               "Font Sizing",
                               "Font Sizing",
                               1.0,
                               100.0,
                               AWN_FONT_SIZE_MEDIUM,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_FONT_SIZING, pspec);   
  
  pspec = g_param_spec_string ("text",
                               "Text",
                               "Text Data",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TEXT, pspec);   
    
  g_type_class_add_private (klass, sizeof (AwnOverlayTextPrivate));  
}

static void
awn_overlay_text_init (AwnOverlayText *self)
{
  AwnOverlayTextPrivate *priv;

  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (self); 
  priv->text = NULL;  
}

AwnOverlayText*
awn_overlay_text_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_TEXT, NULL);
}


static void
awn_overlay_text_move_to (cairo_t * cr,
                           AwnOverlay* overlay,
                           gint   icon_width,
                           gint   icon_height,
                           gint   overlay_width,
                           gint   overlay_height
                           )
{
  gint yoffset = 0;
  gdouble xoffset;
  gint align;
  GdkGravity gravity;
  gdouble x_adj;
  gdouble y_adj;

  g_object_get (overlay,
               "align", &align,
               "gravity", &gravity,
                "x_adj", &x_adj,
                "y_adj", &y_adj,
               NULL);
  
  yoffset = overlay_height / 2.0 + 2; /*Magic is bad FIXME*/

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
    case GDK_GRAVITY_CENTER:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0 + xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);  
      break;
    case GDK_GRAVITY_NORTH:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0 + xoffset, 1 + icon_height / 20 + yoffset);  
      break;      
    case GDK_GRAVITY_NORTH_EAST:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, 1 + icon_height / 20 + yoffset);  
      break;
    case GDK_GRAVITY_EAST:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);
      break;      
    case GDK_GRAVITY_SOUTH_EAST:
      cairo_move_to (cr, 1 + icon_width /20+ xoffset, icon_height - overlay_height -1+ yoffset);      
      break;
    case GDK_GRAVITY_SOUTH:
      cairo_move_to (cr, icon_width/2.0 - overlay_width / 2.0+ xoffset, icon_height - overlay_height -1+ yoffset);
      break;
    case GDK_GRAVITY_SOUTH_WEST:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, icon_height - overlay_height -1+ yoffset);
      break;
    case GDK_GRAVITY_WEST:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, icon_height / 2.0 - overlay_height/2.0 + yoffset);
      break;
    case GDK_GRAVITY_NORTH_WEST:
      cairo_move_to (cr, icon_width - 1 - overlay_width+ xoffset, 1 + icon_height / 20 + yoffset);  
      break;
    default:
      g_assert_not_reached();
      
  }

}


static void 
_awn_overlay_text_render ( AwnOverlay* _overlay,
                               AwnThemedIcon * icon,
                               cairo_t * cr,                                 
                               gint width,
                               gint height)
{
  AwnOverlayText *overlay = AWN_OVERLAY_TEXT(_overlay);
  DesktopAgnosticColor * text_colour; /*FIXME*/
  AwnOverlayTextPrivate *priv;

  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (overlay); 
  
  text_colour = desktop_agnostic_color_new(&GTK_WIDGET(icon)->style->fg[GTK_STATE_ACTIVE], G_MAXUSHORT);
  awn_cairo_set_source_color (cr,text_colour);
  cairo_text_extents_t extents;
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, priv->font_sizing * width / 48.0);
  cairo_text_extents(cr, priv->text, &extents);  
  awn_overlay_text_move_to (cr, _overlay, width, height,extents.width,extents.height);
  cairo_show_text(cr, priv->text);
  g_object_unref (text_colour);  
  
}