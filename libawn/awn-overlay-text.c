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

/**
 * SECTION:AwnOverlayText
 * @short_description: Text overlay for use with #AwnOverlaidIcon.
 * @see_also: #AwnOverlaidIcon, #AwnOverlay, #AwnOverlayIcon, #AwnOverlayThrobber
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Text overlay used with #AwnOverlaidIcon.
 */

/* awn-overlay-text.c */

#include <gtk/gtk.h>
#include <pango/pangocairo.h>

#include "awn-overlay-text.h"
#include "awn-cairo-utils.h"



G_DEFINE_TYPE (AwnOverlayText, awn_overlay_text, AWN_TYPE_OVERLAY)

#define AWN_OVERLAY_TEXT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_TEXT, AwnOverlayTextPrivate))

typedef struct _AwnOverlayTextPrivate AwnOverlayTextPrivate;

struct _AwnOverlayTextPrivate {
    gchar * text;
    gdouble font_sizing;
    PangoFontDescription *font_description;
    DesktopAgnosticColor * text_color;
};

enum
{
  PROP_0,
  PROP_FONT_SIZING,
  PROP_TEXT,
  PROP_TEXT_COLOR
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
    case PROP_TEXT_COLOR:
      g_value_set_object (value,priv->text_color);
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
    case PROP_TEXT_COLOR:
      if (priv->text_color)
      {
        g_object_unref (priv->text_color);
      }
      priv->text_color = g_value_get_object (value);
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

  AwnOverlayTextPrivate *priv;
  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (object);  
  if (priv->text)
  {
    g_free (priv->text);
  }
  if (priv->text_color)
  {
    g_object_unref (priv->text_color);
  }
  if (priv->font_description)
  {
    pango_font_description_free (priv->font_description);
  }
  G_OBJECT_CLASS (awn_overlay_text_parent_class)->finalize (object); 
}

static void
awn_overlay_text_constructed (GObject *object)
{
  AwnOverlayTextPrivate *priv;

  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (object); 
  
  if (G_OBJECT_CLASS (awn_overlay_text_parent_class)->constructed )
  {
    G_OBJECT_CLASS (awn_overlay_text_parent_class)->constructed (object);    
  }
  
  /*FIXME...  make into properties and hook up a signal for prop changes*/
  
  priv->font_description = pango_font_description_new ();
  pango_font_description_set_family (priv->font_description, "sans");
  pango_font_description_set_weight (priv->font_description, PANGO_WEIGHT_SEMIBOLD);
  pango_font_description_set_stretch (priv->font_description, PANGO_STRETCH_CONDENSED);  
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
  object_class->constructed = awn_overlay_text_constructed;
  
  AWN_OVERLAY_CLASS(klass)->render_overlay = _awn_overlay_text_render;
 

/**
 * AwnOverlayText:font-sizing:
 *
 * The Absolute font size as defined for use with #PangoFontDescription at 
 * %PANGO_SCALE at an #AwnIcon height of 48 pixels.  The following standard 
 * awn font sizes are predefined:  %AWN_FONT_SIZE_EXTRA_SMALL, 
 * %AWN_FONT_SIZE_SMALL, %AWN_FONT_SIZE_MEDIUM, %AWN_FONT_SIZE_LARGE, 
 * %AWN_FONT_SIZE_EXTRA_LARGE.  The default size is %AWN_FONT_SIZE_MEDIUM.  
 * Standard doubles can be used but all attempts should be made to use one of 
 * the standard awn font sizes.
 */        
  
  pspec = g_param_spec_double ("font-sizing",
                               "Font Sizing",
                               "Font Sizing",
                               1.0,
                               100.0,
                               AWN_FONT_SIZE_MEDIUM,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_FONT_SIZING, pspec);   
  

/**
 * AwnOverlayText:text:
 *
 * The text to display as a string.
 */        
    
  pspec = g_param_spec_string ("text",
                               "Text",
                               "Text Data",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TEXT, pspec);   


/**
 * AwnOverlayText:text-color:
 *
 * Text color to use as a #DesktopAgnosticColor.  The default value is %NULL.
 * If NULL then the current GTK+ Themes foreground #GTK_ACTIVE_STATE color 
 * will be used.
 */        
    
  pspec = g_param_spec_object ("text-color",
                               "Text Colour",
                               "Text Colour",
                               DESKTOP_AGNOSTIC_TYPE_COLOR,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TEXT_COLOR, pspec);   
  
  g_type_class_add_private (klass, sizeof (AwnOverlayTextPrivate));  

}

static void
awn_overlay_text_init (AwnOverlayText *self)
{
  AwnOverlayTextPrivate *priv;

  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (self); 
  priv->text = NULL;  
  
}

/**
 * awn_overlay_text_new:
 *
 * Creates a new instance of #AwnOverlayText.
 * Returns: an instance of #AwnOverlayText.
 */
AwnOverlayText*
awn_overlay_text_new (void)
{
  return g_object_new (AWN_TYPE_OVERLAY_TEXT, 
                       "gravity", GDK_GRAVITY_CENTER,
                       NULL);
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
  gint layout_width;
  gint layout_height;
  PangoLayout *layout;

  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (overlay); 

  if (priv->text_color)
  {
    text_colour = priv->text_color;
    g_object_ref (text_colour);
  }
  else
  {
    text_colour = desktop_agnostic_color_new(&GTK_WIDGET(icon)->style->fg[GTK_STATE_ACTIVE], G_MAXUSHORT);
  }
  awn_cairo_set_source_color (cr,text_colour);
  g_object_unref (text_colour);       
  
  layout = pango_cairo_create_layout (cr);
  pango_font_description_set_absolute_size (priv->font_description, 
                                            priv->font_sizing * PANGO_SCALE * height / 48.0);
  pango_layout_set_font_description (layout, priv->font_description);
  pango_layout_set_text (layout, priv->text, -1);  
  pango_layout_get_pixel_size (layout,&layout_width,&layout_height);
  awn_overlay_move_to (_overlay,cr,  width, height,layout_width,layout_height,NULL);
  pango_cairo_show_layout (cr, layout);

  g_object_unref (layout);
}