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

/* awn-overlay-pixbuf-file.c */

#include <math.h>

#include "awn-overlay-pixbuf-file.h"

G_DEFINE_TYPE (AwnOverlayPixbufFile, awn_overlay_pixbuf_file, AWN_TYPE_OVERLAY_PIXBUF)

#define AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_OVERLAY_PIXBUF_FILE, AwnOverlayPixbufFilePrivate))

typedef struct _AwnOverlayPixbufFilePrivate AwnOverlayPixbufFilePrivate;

struct _AwnOverlayPixbufFilePrivate {
    gchar * file_name;
    GHashTable *pixbufs;  
  
    void  (*parent_render_overlay) (AwnOverlay* overlay,
                                   AwnThemedIcon * icon,
                                   cairo_t * cr,                                 
                                   gint width,
                                   gint height);  
};

enum
{
  PROP_0,
  PROP_FILE_NAME
};

static void 
_awn_overlay_pixbuf_file_render ( AwnOverlay* _overlay,
                               AwnThemedIcon * icon,
                               cairo_t * cr,                                 
                               gint icon_width,
                               gint icon_height);

extern void 
_awn_overlay_pixbuf_render ( AwnOverlay* _overlay,
                               AwnThemedIcon * icon,
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
  
  /*FIXME need to clear the hash table on theme changes also */
  g_signal_connect (object, "notify::file-name",
                  G_CALLBACK(_awn_overlaid_pixbuf_file_clear_hash),
                  object);   
}


static void
awn_overlay_pixbuf_file_class_init (AwnOverlayPixbufFileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;        
  
  object_class->get_property = awn_overlay_pixbuf_file_get_property;
  object_class->set_property = awn_overlay_pixbuf_file_set_property;
  object_class->dispose = awn_overlay_pixbuf_file_dispose;
  object_class->finalize = awn_overlay_pixbuf_file_finalize;
  object_class->constructed = awn_overlay_pixbuf_file_constructed;
  
  AWN_OVERLAY_CLASS(klass)->render_overlay = _awn_overlay_pixbuf_file_render;
  
  pspec = g_param_spec_string ("file-name",
                               "File name",
                               "File Name",
                               "",
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
  g_object_class_install_property (object_class, PROP_FILE_NAME, pspec);   
  
  g_type_class_add_private (klass, sizeof (AwnOverlayPixbufFilePrivate));  
}

static void
awn_overlay_pixbuf_file_init (AwnOverlayPixbufFile *self)
{
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (self);
  
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
_awn_overlay_pixbuf_file_render ( AwnOverlay* _overlay,
                               AwnThemedIcon * icon,
                               cairo_t * cr,                                 
                               gint icon_width,
                               gint icon_height)
{
  gdouble scale;
  GdkPixbuf * current_pixbuf;
  gint scaled_width;
  gint scaled_height;

  AwnOverlayPixbufFile * overlay = AWN_OVERLAY_PIXBUF_FILE (_overlay);
  AwnOverlayPixbufFilePrivate * priv = AWN_OVERLAY_PIXBUF_FILE_GET_PRIVATE (overlay);  
  
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
    current_pixbuf = gdk_pixbuf_new_from_file_at_scale (priv->file_name, 
                                                scaled_width, 
                                                scaled_height, 
                                                TRUE, NULL);
    if (current_pixbuf)
    {
      g_object_set (overlay, 
                    "pixbuf", current_pixbuf, 
                    NULL);
    }
    else
    {
      g_warning ("AwnOverlayPixbufFile: Failed to load pixbuf (%s)\n",priv->file_name);
      return;
    }
  }
  g_object_unref (current_pixbuf);
  
  if ( (gdk_pixbuf_get_width (current_pixbuf) != scaled_width) || 
      (gdk_pixbuf_get_height (current_pixbuf) != scaled_height))
  {
    current_pixbuf = gdk_pixbuf_new_from_file_at_scale (priv->file_name, 
                                                scaled_width, 
                                                scaled_height, 
                                                TRUE, NULL);                                                        
    if (current_pixbuf)
    {
      g_object_set (overlay, 
                    "pixbuf", current_pixbuf, 
                    NULL);
    }
    else
    {
      g_warning ("AwnOverlayPixbufFile: Failed to load pixbuf (%s)\n",priv->file_name);
      return;
    }
    g_object_unref (current_pixbuf);    
  }

  _awn_overlay_pixbuf_render ( _overlay,icon,cr,icon_width,icon_height);
}

