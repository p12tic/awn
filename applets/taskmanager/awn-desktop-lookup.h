/*
 * Copyright (C) 2010 Rodney Cryderman <rcryderman@gmail.com>
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
/* awn-desktop-lookup.h */

#ifndef _AWN_DESKTOP_LOOKUP
#define _AWN_DESKTOP_LOOKUP

#include <glib-object.h>

G_BEGIN_DECLS

#define AWN_TYPE_DESKTOP_LOOKUP awn_desktop_lookup_get_type()

#define AWN_DESKTOP_LOOKUP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_DESKTOP_LOOKUP, AwnDesktopLookup))

#define AWN_DESKTOP_LOOKUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_DESKTOP_LOOKUP, AwnDesktopLookupClass))

#define AWN_IS_DESKTOP_LOOKUP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_DESKTOP_LOOKUP))

#define AWN_IS_DESKTOP_LOOKUP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_DESKTOP_LOOKUP))

#define AWN_DESKTOP_LOOKUP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_DESKTOP_LOOKUP, AwnDesktopLookupClass))

typedef struct {
  GObject parent;
} AwnDesktopLookup;

typedef struct {
  GObjectClass parent_class;
} AwnDesktopLookupClass;

GType awn_desktop_lookup_get_type (void);

AwnDesktopLookup* awn_desktop_lookup_new (void);
gchar *awn_desktop_lookup_search_for_desktop (gulong xid);
gchar *awn_desktop_lookup_search_cache (AwnDesktopLookup* * self,
                                 gchar * class_name,
                                 gchar * res_name,
                                 gchar * cmd, 
                                 gchar *id);
gchar *awn_desktop_lookup_special_case (gchar * cmd,
                                 gchar *res_name, 
                                 gchar * class_name,
                                 gchar * wm_icon_name,
                                 gchar * wm_name,
                                 gchar * window_role);



G_END_DECLS

#endif /* _AWN_DESKTOP_LOOKUP */
