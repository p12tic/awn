/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 *
 */

/**
 * SECTION: awn-overlay-text
 * @short_description: Text overlay for use with #AwnIcon.
 * @see_also: #AwnEffects, #AwnOverlay, #AwnOverlayThemedIcon, #AwnOverlayThrobber,
 * #AwnOverlayPixbuf, #AwnOverlayPixbufFile, #AwnIcon
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Text overlay used with #AwnIcon.
 */

/**
 * AwnOverlayText:
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
  DesktopAgnosticColor * text_outline_color;
  gchar                * text_outline_color_astr;
  gint                   font_mode;
  gdouble                text_outline_width;
  
  DesktopAgnosticConfigClient * client;
};

enum
{
   FONT_MODE_SOLID,
   FONT_MODE_OUTLINE,  
   FONT_MODE_OUTLINE_REVERSED
};  

enum
{
  PROP_0,
  PROP_FONT_SIZING,
  PROP_TEXT,
  PROP_TEXT_COLOR,
  PROP_TEXT_COLOR_ASTR,
  PROP_TEXT_OUTLINE_COLOR,
  PROP_TEXT_OUTLINE_COLOR_ASTR,
  PROP_FONT_MODE,
  PROP_TEXT_OUTLINE_WIDTH
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
    case PROP_TEXT_OUTLINE_COLOR:
      g_value_set_object (value,priv->text_outline_color);
      break;
    case PROP_TEXT_OUTLINE_COLOR_ASTR:
      g_value_set_string (value,priv->text_outline_color_astr);
      break;
    case PROP_FONT_MODE:
      g_value_set_int (value,priv->font_mode);
      break;      
    case PROP_TEXT_OUTLINE_WIDTH:
      g_value_set_double (value,priv->text_outline_width);
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
      priv->text_color = g_value_dup_object (value);
      break;
    case PROP_TEXT_COLOR_ASTR:
      g_free(priv->text_color_astr);
      priv->text_color_astr = g_value_dup_string (value);
      break;            
    case PROP_TEXT_OUTLINE_COLOR:
      if (priv->text_outline_color)
      {
        g_object_unref (priv->text_outline_color);
      }
      priv->text_outline_color = g_value_dup_object (value);
      break;
    case PROP_TEXT_OUTLINE_COLOR_ASTR:
      g_free(priv->text_outline_color_astr);
      priv->text_outline_color_astr = g_value_dup_string (value);
      break;            
    case PROP_FONT_MODE:
      priv->font_mode = g_value_get_int (value);
      break;      
    case PROP_TEXT_OUTLINE_WIDTH:
      priv->text_outline_width = g_value_get_double (value);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_text_dispose (GObject *object)
{
  AwnOverlayTextPrivate *priv;
  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (object);  

  desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                        object, NULL);

  if (priv->text_color)
  {
    g_object_unref (priv->text_color);
    priv->text_color = NULL;
  }
  if (priv->text_outline_color)
  {
    g_object_unref (priv->text_outline_color);
    priv->text_outline_color = NULL;
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

  if (priv->text_color_astr)
  {
    g_free (priv->text_color_astr);
  }

  if (priv->text_outline_color_astr)
  {
    g_free (priv->text_outline_color_astr);
  }
  
  G_OBJECT_CLASS (awn_overlay_text_parent_class)->finalize (object);
}

static void
awn_overlay_text_constructed (GObject *object)
{
  AwnOverlayTextPrivate *priv;
  GError *error = NULL;

  priv =  AWN_OVERLAY_TEXT_GET_PRIVATE (object); 
  
  if (G_OBJECT_CLASS (awn_overlay_text_parent_class)->constructed )
  {
    G_OBJECT_CLASS (awn_overlay_text_parent_class)->constructed (object);    
  }
  
  priv->client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, &error);

  if (error)
  {
    g_critical ("An error occurred while trying to retrieve the configuration client: %s",
                error->message);
    g_error_free (error);
    return;
  }

  desktop_agnostic_config_client_bind (priv->client, "theme", "icon_text_color",
                                       object, "text-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  desktop_agnostic_config_client_bind (priv->client, "theme", "icon_text_outline_color",
                                       object, "text-outline-color", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  desktop_agnostic_config_client_bind (priv->client, "theme", "icon_font_mode",
                                       object, "font-mode", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  desktop_agnostic_config_client_bind (priv->client, "theme", "icon_text_outline_width",
                                       object, "text-outline-width", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  priv->font_description = pango_font_description_new ();
  pango_font_description_set_family (priv->font_description, "sans");
  pango_font_description_set_weight (priv->font_description, PANGO_WEIGHT_SEMIBOLD);
  pango_font_description_set_stretch (priv->font_description, PANGO_STRETCH_CONDENSED);  

}

static void
awn_overlay_text_class_init (AwnOverlayTextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

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
  
  g_object_class_install_property (object_class,
    PROP_FONT_SIZING,
    g_param_spec_double ("font-sizing",
                         "Font Sizing",
                         "Font Sizing",
                         1.0,
                         100.0,
                         AWN_FONT_SIZE_MEDIUM,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));
  

/**
 * AwnOverlayText:text:
 *
 * The text to display as a string.
 */        
    
  g_object_class_install_property (object_class,
    PROP_TEXT,
    g_param_spec_string ("text",
                         "Text",
                         "Text Data",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));


/**
 * AwnOverlayText:text-color:
 *
 * Text color to use as a #DesktopAgnosticColor.  The default value is %NULL.
 * If NULL then the current GTK+ Themes foreground #GTK_ACTIVE_STATE color 
 * will be used.
 */        
    
  g_object_class_install_property (object_class,
    PROP_TEXT_COLOR,
    g_param_spec_object ("text-color",
                         "Text Colour",
                         "Text Colour",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_TEXT_COLOR_ASTR,
    g_param_spec_string ("text-color-astr",
                         "Text color Astr",
                         "Text color as string",
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_TEXT_OUTLINE_COLOR,
    g_param_spec_object ("text-outline-color",
                         "Text Outline Colour",
                         "Text Outline Colour",
                         DESKTOP_AGNOSTIC_TYPE_COLOR,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_TEXT_OUTLINE_COLOR_ASTR,
    g_param_spec_string ("text-outline-color-astr",
                         "Text Outline Color Astr",
                         "Text outline color as string",
                         "",
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_FONT_MODE,
    g_param_spec_int ("font-mode",
                      "Font Mode",
                      "Font Mode",
                      FONT_MODE_SOLID,
                      FONT_MODE_OUTLINE_REVERSED,
                      FONT_MODE_SOLID,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
    PROP_TEXT_OUTLINE_WIDTH,
    g_param_spec_double ("text-outline-width",
                         "Text Outline Width",
                         "Text Outline Width",
                         0.0, 10.0, 2.5,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  
  g_type_class_add_private (klass, sizeof (AwnOverlayTextPrivate));  

}

static void
awn_overlay_text_init (AwnOverlayText *self)
{
  AwnOverlayTextPrivate *priv;
  
  priv = AWN_OVERLAY_TEXT_GET_PRIVATE (self);

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

void awn_overlay_text_get_size (AwnOverlayText *overlay,
                                GtkWidget *widget,
                                gchar *text,
                                gint size,
                                gint *width, gint *height)
{
  PangoLayout *layout;
  if (size < 1)
  {
    size = 48;
  }
  g_return_if_fail (AWN_IS_OVERLAY_TEXT (overlay));
  AwnOverlayTextPrivate *priv = AWN_OVERLAY_TEXT_GET_PRIVATE (overlay);

  layout = gtk_widget_create_pango_layout (widget, NULL);
  pango_font_description_set_absolute_size (priv->font_description,
                                            priv->font_sizing * PANGO_SCALE * size / 48.0);
  pango_layout_set_font_description (layout, priv->font_description);
  pango_layout_set_text (layout, text ? text : priv->text, -1);
  pango_layout_get_pixel_size (layout, width, height);
  g_object_unref (layout);
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
  DesktopAgnosticColor * text_outline_colour = NULL;
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
  if (priv->text_outline_color)
  {
    text_outline_colour = priv->text_outline_color;
    g_object_ref (text_outline_colour);
  }
  else if (priv->text_outline_color_astr && strlen(priv->text_outline_color_astr) )
  {
    text_outline_colour = desktop_agnostic_color_new_from_string (priv->text_outline_color_astr,NULL);
  }
  if (!text_outline_colour)
  {
    text_outline_colour = desktop_agnostic_color_new(&widget->style->bg[GTK_STATE_NORMAL], G_MAXUSHORT);
  }
  
  layout = pango_cairo_create_layout (cr);
  pango_font_description_set_absolute_size (priv->font_description, 
                                            priv->font_sizing * PANGO_SCALE * height / 48.0);
  pango_layout_set_font_description (layout, priv->font_description);
  pango_layout_set_text (layout, priv->text, -1);  
  pango_layout_get_pixel_size (layout,&layout_width,&layout_height);
  awn_overlay_move_to (_overlay,cr,  width, height,layout_width,layout_height,NULL);


  switch (priv->font_mode)
  {
    default:
    case FONT_MODE_SOLID:
      awn_cairo_set_source_color (cr,text_colour);
      pango_cairo_show_layout (cr, layout);      
      break;
    case FONT_MODE_OUTLINE:
    case FONT_MODE_OUTLINE_REVERSED:
      cairo_save (cr);

      cairo_set_line_width(cr, priv->text_outline_width * height / 48.0);
      // first paint the outline
      /*conditional operator*/      
      awn_cairo_set_source_color (cr,priv->font_mode==FONT_MODE_OUTLINE?
                                  text_outline_colour:text_colour);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
      pango_cairo_layout_path (cr, layout);
      cairo_stroke_preserve (cr);

      // now the text itself
      awn_overlay_move_to (_overlay, cr, width, height,
                           layout_width, layout_height, NULL);
      /*conditional operator*/
      awn_cairo_set_source_color (cr,priv->font_mode==FONT_MODE_OUTLINE?
                                  text_colour:text_outline_colour);
      cairo_fill (cr);

      cairo_restore (cr);
      break;
  }
  g_object_unref (text_colour);
  g_object_unref (text_outline_colour);  
  g_object_unref (layout);
}
