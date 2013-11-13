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
 * SECTION: awn-overlay-pixbuf-file
 * @short_description: An subclass of #AwnOverlayPixbuf to overlay a #GdkPixbuf 
 * loaded from a Filename.
 * @see_also: #AwnOverlay, #AwnOverlayText, #AwnOverlayThemedIcon, 
 * #AwnOverlayThrobber, #AwnOverlayPixbuf, #AwnEffects
 * @stability: Unstable
 * @include: libawn/libawn.h
 *
 * Overlay a #GdkPixbuf, loaded using the filename provided, on an 
 * #AwnOverlaidIcon.  You probably should _NOT_ be using this object.  You 
 * should, probably, be using #AwnOverlayThemedIcon.
 * 
 */

/**
 * AwnOverlayPixbufFile:
 *
 * Overlay a #GdkPixbuf, loaded using the filename provided, on an 
 * #AwnOverlaidIcon.  You probably should _NOT_ be using this object.  You 
 * should, probably, be using #AwnOverlayThemedIcon.
 * 
 */

/* awn-overlay-pixbuf-file.c */

#include <string.h>
#include <math.h>

#include "awn-overlay-pixbuf-file.h"

G_DEFINE_TYPE (AwnOverlayPixbufFile, awn_overlay_pixbuf_file, AWN_TYPE_OVERLAY_PIXBUF)

#define AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_PIXBUF_FILE, AwnOverlayPixbufFilePrivate))

typedef struct _AwnOverlayPixbufFilePrivate AwnOverlayPixbufFilePrivate;

struct _AwnOverlayPixbufFilePrivate {
    gchar * file_name;
    GHashTable *pixbufs;

    gint  icon_width;
    gint  icon_height;
    /*Used to keep missing file from spamming the console messages*/
    gboolean  emitted_warning;
};

enum
{
  PROP_0,
  PROP_FILE_NAME
};

static gboolean
awn_overlay_pixbuf_file_load (AwnOverlayPixbufFile *overlay, 
                              gchar * filename);


static void 
_awn_overlay_pixbuf_file_render (AwnOverlay* _overlay,
                                 GtkWidget *widget,
                                 cairo_t * cr,
                                 gint icon_width,
                                 gint icon_height);

static void
awn_overlay_pixbuf_file_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (object);
  
  switch (property_id) 
  {
    case PROP_FILE_NAME:
      g_value_set_string (value,priv->file_name);
      priv->emitted_warning = FALSE;
      break;             
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}



static void
awn_overlay_pixbuf_file_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (object);  
  switch (property_id) 
  {
    case PROP_FILE_NAME:
      g_free(priv->file_name);      
      priv->file_name = g_value_dup_string (value);
      if (priv->file_name)
      {
        awn_overlay_pixbuf_file_load (AWN_OVERLAY_PIXBUF_FILE(object),
                                  priv->file_name);
      }
      break;    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_overlay_pixbuf_file_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_overlay_pixbuf_file_parent_class)->dispose (object);
}

static void
awn_overlay_pixbuf_file_finalize (GObject *object)
{
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (object);  

  g_free (priv->file_name);
  g_hash_table_destroy (priv->pixbufs);    
  
  G_OBJECT_CLASS (awn_overlay_pixbuf_file_parent_class)->finalize (object);
}

static void
_awn_overlaid_pixbuf_file_clear_hash (GObject *pspec,
                                      GParamSpec *gobject,
                                      AwnOverlayPixbufFile* overlay)
{
  AwnOverlayPixbufFilePrivate * priv;
  priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (overlay);

  g_object_set (overlay, "pixbuf", NULL, NULL);

  /*clear the hash table... the icon has changed in some fundamental way*/
  g_hash_table_remove_all (priv->pixbufs);
  
}

static void
awn_overlay_pixbuf_file_constructed (GObject *object)
{
  if ( G_OBJECT_CLASS( awn_overlay_pixbuf_file_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_overlay_pixbuf_file_parent_class)->constructed (object);
  }
  
  g_signal_connect (object, "notify::file-name",
                  G_CALLBACK(_awn_overlaid_pixbuf_file_clear_hash),
                  object);   
}


static void
awn_overlay_pixbuf_file_class_init (AwnOverlayPixbufFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->get_property = awn_overlay_pixbuf_file_get_property;
  object_class->set_property = awn_overlay_pixbuf_file_set_property;
  object_class->dispose = awn_overlay_pixbuf_file_dispose;
  object_class->finalize = awn_overlay_pixbuf_file_finalize;
  object_class->constructed = awn_overlay_pixbuf_file_constructed;
  
  AWN_OVERLAY_CLASS(klass)->render = _awn_overlay_pixbuf_file_render;
  
  g_object_class_install_property (object_class,
    PROP_FILE_NAME,
    g_param_spec_string ("file-name",
                         "File name",
                         "File Name",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));
  
  g_type_class_add_private (klass, sizeof (AwnOverlayPixbufFilePrivate));  
}

static void
awn_overlay_pixbuf_file_init (AwnOverlayPixbufFile *self)
{
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (self);

  priv->icon_height = 48;  /*FIXME replaces with a named constant */
  priv->icon_width = 48;  
  priv->file_name = NULL;
  priv->pixbufs = g_hash_table_new_full (g_str_hash,g_str_equal, g_free, g_object_unref);  
}

AwnOverlayPixbufFile*
awn_overlay_pixbuf_file_new (gchar * file_name)
{
  return g_object_new (AWN_TYPE_OVERLAY_PIXBUF_FILE,
                       "file-name",file_name,
                       NULL);
}


/* 
 FIXME
 
 This function needs a little bit of work.  It can be cleaner.
 
 Also need to decide if it merits pixbuf caching... probably a good idea
 */

static void 
_awn_overlay_pixbuf_file_render (AwnOverlay* _overlay,
                                 GtkWidget *widget,
                                 cairo_t * cr,
                                 gint icon_width,
                                 gint icon_height)
{
  gdouble scale;
  GdkPixbuf * current_pixbuf;
  gint scaled_width;
  gint scaled_height;
  gboolean good = TRUE;

  AwnOverlayPixbufFile * overlay = AWN_OVERLAY_PIXBUF_FILE (_overlay);
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (overlay);  

  g_return_if_fail (priv->file_name);
  
  priv->icon_height = icon_width;  /*stored so we know what size to ask for when file name changed*/
  priv->icon_width = icon_height;  
  
  
  g_object_get (_overlay,
                "scale",&scale,
                "pixbuf",&current_pixbuf,
                NULL);
  
  scaled_width = lround (icon_width * scale);  
  scaled_height = lround (icon_height * 
                          scaled_width / 
                          icon_width);
  
  if (!current_pixbuf)
  {
      good = awn_overlay_pixbuf_file_load (overlay, priv->file_name); 
  }    
  else if ( (gdk_pixbuf_get_width (current_pixbuf) != scaled_width) && 
      (gdk_pixbuf_get_height (current_pixbuf) != scaled_height))
  {
      g_object_unref (current_pixbuf);    
      awn_overlay_pixbuf_file_load (overlay, priv->file_name);    
  }
  else  
  {
    g_object_unref (current_pixbuf);
  }

  if (good)
  {
    AWN_OVERLAY_CLASS (awn_overlay_pixbuf_file_parent_class)->
      render (_overlay, widget, cr, icon_width, icon_height);
  }
}

static gboolean
awn_overlay_pixbuf_file_load (AwnOverlayPixbufFile *overlay, 
                              gchar * file_name)
{
  gdouble scale;
  GdkPixbuf * pixbuf;
  gint scaled_width;
  gint scaled_height;
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (overlay);  
  
  g_object_get (overlay,
                "scale",&scale,
                NULL);
  /*This will happen during object instantiation.  file_name will get set
   before scale.  In which case we defer the file load... it'll get picked up
   during the file render*/
  if ( ! ( scale>0.01) )
  {
    return FALSE;
  }
  g_return_val_if_fail (file_name,FALSE);
  g_return_val_if_fail (strlen(file_name),FALSE);  
  
  scaled_width = lround (priv->icon_width * scale);  
  scaled_height = lround (priv->icon_height * 
                          scaled_width / 
                          priv->icon_width);

  pixbuf = gdk_pixbuf_new_from_file_at_scale (file_name, 
                                              scaled_width, 
                                              scaled_height, 
                                              TRUE, NULL);
  if (pixbuf)
  {
    g_object_set (overlay, 
                  "pixbuf", pixbuf, 
                  NULL);
    g_object_unref (pixbuf);    
  }
  else
  {
    if (!priv->emitted_warning)
    {
      g_warning ("%s: Failed to load pixbuf (%s)",__func__,file_name);
      priv->emitted_warning = TRUE;
    }
    return FALSE;
  }
  return TRUE;
}
