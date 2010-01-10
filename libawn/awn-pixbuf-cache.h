/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
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
 * Authored by Rodney Cryderman <rcryderman@gmail.com>
 *
 */

/* awn-pixbuf-cache.h */

#ifndef _AWN_PIXBUF_CACHE
#define _AWN_PIXBUF_CACHE

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define AWN_TYPE_PIXBUF_CACHE awn_pixbuf_cache_get_type()

#define AWN_PIXBUF_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_PIXBUF_CACHE, AwnPixbufCache))

#define AWN_PIXBUF_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_PIXBUF_CACHE, AwnPixbufCacheClass))

#define AWN_IS_PIXBUF_CACHE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_PIXBUF_CACHE))

#define AWN_IS_PIXBUF_CACHE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_PIXBUF_CACHE))

#define AWN_PIXBUF_CACHE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_PIXBUF_CACHE, AwnPixbufCacheClass))

typedef struct {
  GObject parent;
} AwnPixbufCache;

typedef struct {
  GObjectClass parent_class;
} AwnPixbufCacheClass;

void awn_pixbuf_cache_insert_pixbuf (AwnPixbufCache * pixbuf_cache,
                              GdkPixbuf * pbuf,
                              const gchar *scope, 
                              const gchar * theme_name, 
                              const gchar * icon_name);

void awn_pixbuf_cache_insert_null_result (AwnPixbufCache * pixbuf_cache,
                              const gchar *scope, 
                              const gchar * theme_name, 
                              const gchar * icon_name,
                              gint width,
                              gint height);

GdkPixbuf * awn_pixbuf_cache_lookup (AwnPixbufCache * pixbuf_cache,
                              const gchar *scope, 
                              const gchar * theme_name, 
                              const gchar * icon_name,
                       				gint width,
		                          gint height,
                              gboolean * null_result);

GdkPixbuf * awn_pixbuf_cache_lookup_simple_key (AwnPixbufCache * pixbuf_cache,
                              const gchar * simple_key,
                       				gint width,
                       				gint height);

void awn_pixbuf_cache_insert_pixbuf_simple_key (AwnPixbufCache * pixbuf_cache,
                              GdkPixbuf * pbuf,
                              const gchar * simple_key);


GType awn_pixbuf_cache_get_type (void);

void awn_pixbuf_cache_invalidate (AwnPixbufCache* pixbuf_cache);

AwnPixbufCache* awn_pixbuf_cache_new (void);

AwnPixbufCache* awn_pixbuf_cache_get_default (void);

G_END_DECLS

#endif /* _AWN_PIXBUF_CACHE */
