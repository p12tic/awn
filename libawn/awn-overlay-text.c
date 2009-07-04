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
 * @short_description: Text overlay for use with #AwnIcon.
 * @see_also: #AwnEffects, #AwnOverlay, #AwnOverlayThemedIcon, #AwnOverlayThrobber,
 * #AwnOverlayPixbuf, #AwnOverlayPixbufFile, #AwnIcon
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Text overlay used with #AwnIcon.
 */

/* awn-overlay-text.c */

#include <gtk/gtk.h>
#include <pango/pangocairo.h>

#include "awn-config.h"
#include "awn-overlay-text.h"
#include "awn-cairo-utils.h"



G_DEFINE_TYPE (AwnOverlayText, awn_overlay_text, AWN_TYPE_OVERLAY)

#define AWN_OVERLAY_TEXT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_TEXT, AwnOverlayTextPrivate))

typedef struct _AwnOverlayTextPrivate AwnOverlayTextPrivate;

struct _AwnOverlayTextPrivate 
{
  gchar * text;
  gdouble font_sizing;
  PangoFontDescription *font_description;
  DesktopAgnosticColor * text_color;
  gchar                * text_color_astr;
  
  DesktopAgnosticConfigClient * client;
};

enum
{
  PROP_0,
  PROP_FONT_SIZING,
  PROP_TEXT,
  PROP_TEXT_COLOR,
  PROP_TEXT_COLOR_ASTR  
};

static void 
_awn_overlay_text_render (AwnOverlay* overlay,
                          GtkWidget *widget,
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
    case PROP_TEXT_COLOR_ASTR:
      g_value_set_string (value,priv->text_color_astr);
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
    case PROP_TEXT_COLOR_ASTR:
      g_free(priv->text_color_astr);
      priv->text_color_astr = g_value_dup_string (value);
      break;            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

// TODO unbind property & config key
static void
awn_overlay_text_dispose (GObject *object)
{
  AwnOverlayTextPrivate *priv;
  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (object);  

  if (priv->text_color)
  {
    g_object_unref (priv->text_color);
    priv->text_color = NULL;
  }
  
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
  
  AWN_OVERLAY_CLASS(klass)->render = _awn_overlay_text_render;
 

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

  pspec = g_param_spec_string ("text_color_astr",
                               "text_color_astr",
                               "Text color as string",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_TEXT_COLOR_ASTR, pspec);   
    
  g_type_class_add_private (klass, sizeof (AwnOverlayTextPrivate));  

}

static void
awn_overlay_text_init (AwnOverlayText *self)
{
  GError *error = NULL;
  AwnOverlayTextPrivate *priv;
  
  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (self);   
  priv->client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, &error);

  if (error)
  {
    g_critical ("An error occurred while trying to retrieve the configuration client: %s",
                error->message);
    g_error_free (error);
    return;
  }

  desktop_agnostic_config_client_bind (priv->client, "theme", "icon_text_color",
                                       G_OBJECT(self), "text_color_astr", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  priv->text = NULL;
  // default for text is to not apply effects to it
  awn_overlay_set_apply_effects (AWN_OVERLAY (self), FALSE);
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
_awn_overlay_text_render (AwnOverlay* _overlay,
                          GtkWidget *widget,
                          cairo_t * cr,
                          gint width,
                          gint height)
{
  AwnOverlayText *overlay = AWN_OVERLAY_TEXT(_overlay);
  DesktopAgnosticColor * text_colour = NULL;
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
  else if (priv->text_color_astr && strlen(priv->text_color_astr) )
  {
    text_colour = desktop_agnostic_color_new_from_string (priv->text_color_astr,NULL);
  }
  if (!text_colour)
  {
    text_colour = desktop_agnostic_color_new(&widget->style->fg[GTK_STATE_NORMAL], G_MAXUSHORT);
  }
  
  layout = pango_cairo_create_layout (cr);
  pango_font_description_set_absolute_size (priv->font_description, 
                                            priv->font_sizing * PANGO_SCALE * height / 48.0);
  pango_layout_set_font_description (layout, priv->font_description);
  pango_layout_set_text (layout, priv->text, -1);  
  pango_layout_get_pixel_size (layout,&layout_width,&layout_height);
  awn_overlay_move_to (_overlay,cr,  width, height,layout_width,layout_height,NULL);

  awn_cairo_set_source_color (cr,text_colour);
  g_object_unref (text_colour);       
  pango_cairo_show_layout (cr, layout);
/*

#elseif 0
   awn_cairo_set_source_color (cr,text_colour);
  g_object_unref (text_colour);         
  cairo_save (cr);
  pango_cairo_layout_path(cr, layout);  
  cairo_fill (cr);
  cairo_restore (cr);
  text_colour = desktop_agnostic_color_new(&widget->style->bg[GTK_STATE_NORMAL], G_MAXUSHORT);  
  awn_cairo_set_source_color (cr,text_colour);
  g_object_unref (text_colour);         
  cairo_set_line_width(cr, 0.3 * height / 48.0);
  awn_overlay_move_to (_overlay,cr,  width, height,layout_width,layout_height,NULL);  
  pango_cairo_layout_path(cr, layout);    
  cairo_stroke_preserve(cr);
#else
  
  if (layout_width > highest_width)
  {
    highest_width = layout_width;
  }
  cairo_rectangle (cr,coord.x - (highest_width - layout_width),coord.y,highest_width,layout_height);
  bg_color = desktop_agnostic_color_new(&widget->style->bg[GTK_STATE_NORMAL], G_MAXUSHORT*0.3);  
  awn_cairo_set_source_color (cr,bg_color);
  g_object_unref (bg_color);         
  cairo_fill (cr);
  awn_cairo_set_source_color (cr,text_colour);
  g_object_unref (text_colour);       
  
  awn_overlay_move_to (_overlay,cr,  width, height,layout_width,layout_height,&coord);  
  pango_cairo_show_layout (cr, layout);  
#endif
   */
  
  g_object_unref (layout);
}
