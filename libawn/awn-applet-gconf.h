/*
 * awn-applet-gconf.h: awn applet preferences handling.
 *
 * Copyright (C) 2001-2003 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 *     Mark McLoughlin <mark@skynet.ie>
 *     Neil Jagdish Patel <njpatel@gmail.com> (Adapted for Awn)
 */

#ifndef __AWN_APPLET_GCONF_H__
#define __AWN_APPLET_GCONF_H__

#include <glib/gmacros.h>
#include <glib/gerror.h>
#include <gconf/gconf-value.h> 

#include <awn-applet.h>

G_BEGIN_DECLS

gchar       *awn_applet_gconf_get_full_key (AwnApplet     *applet,
					      const gchar     *key);

void         awn_applet_gconf_set_bool     (AwnApplet     *applet,
					      const gchar     *key,
					      gboolean         the_bool,
					      GError         **opt_error);
void         awn_applet_gconf_set_int      (AwnApplet     *applet,
					      const gchar     *key,
					      gint             the_int,
					      GError         **opt_error);
void         awn_applet_gconf_set_string   (AwnApplet     *applet,
					      const gchar     *key,
					      const gchar     *the_string,
					      GError         **opt_error);
void         awn_applet_gconf_set_float    (AwnApplet     *applet,
					      const gchar     *key,
					      gdouble          the_float,
					      GError         **opt_error);
void         awn_applet_gconf_set_list     (AwnApplet     *applet,
					      const gchar     *key,
					      GConfValueType   list_type,
					      GSList          *list,
					      GError         **opt_error);
void         awn_applet_gconf_set_value    (AwnApplet     *applet,
					      const gchar     *key,
					      GConfValue      *value,
					      GError         **opt_error);

gboolean     awn_applet_gconf_get_bool     (AwnApplet     *applet,
					      const gchar     *key,
					      GError         **opt_error);
gint         awn_applet_gconf_get_int      (AwnApplet     *applet,
					      const gchar     *key,
					      GError         **opt_error);
gchar       *awn_applet_gconf_get_string   (AwnApplet     *applet,
					      const gchar     *key,
					      GError         **opt_error);
gdouble      awn_applet_gconf_get_float    (AwnApplet     *applet,
					      const gchar     *key,
					      GError         **opt_error);
GSList      *awn_applet_gconf_get_list     (AwnApplet     *applet,
					      const gchar     *key,
					      GConfValueType   list_type,
					      GError         **opt_error);
GConfValue  *awn_applet_gconf_get_value    (AwnApplet     *applet,
					      const gchar     *key,
					      GError         **opt_error);

G_END_DECLS

#endif /* __AWN_APPLET_GCONF_H__ */
