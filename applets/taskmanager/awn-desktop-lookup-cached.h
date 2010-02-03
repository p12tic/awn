/*
 * Copyright (C) 2009,2010 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 *
 * Authored by Rodney Cryderman <rcryderman@gmail.com>
 */

/* awn-desktop-lookup-cached.h */

#ifndef _AWN_DESKTOP_LOOKUP_CACHED
#define _AWN_DESKTOP_LOOKUP_CACHED

#include "awn-desktop-lookup.h"
#include <glib-object.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define AWN_TYPE_DESKTOP_LOOKUP_CACHED awn_desktop_lookup_cached_get_type()

#define AWN_DESKTOP_LOOKUP_CACHED(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCached))

#define AWN_DESKTOP_LOOKUP_CACHED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCachedClass))

#define AWN_IS_DESKTOP_LOOKUP_CACHED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_DESKTOP_LOOKUP_CACHED))

#define AWN_IS_DESKTOP_LOOKUP_CACHED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_DESKTOP_LOOKUP_CACHED))

#define AWN_DESKTOP_LOOKUP_CACHED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCachedClass))

typedef struct {
  AwnDesktopLookup parent;
} AwnDesktopLookupCached;

typedef struct {
  AwnDesktopLookupClass parent_class;
} AwnDesktopLookupCachedClass;

GType awn_desktop_lookup_cached_get_type (void);

AwnDesktopLookupCached* awn_desktop_lookup_cached_new (void);

const gchar *awn_desktop_lookup_search_by_wnck_window (AwnDesktopLookupCached * lookup, WnckWindow * win);

G_END_DECLS

#endif /* _AWN_DESKTOP_LOOKUP_CACHED */

