/*
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
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
 */

/* awn-icons.h */

#ifndef _AWN_ICONS
#define _AWN_ICONS

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libawn/awn-applet.h>

G_BEGIN_DECLS

#define AWN_TYPE_ICONS awn_icons_get_type()

#define AWN_ICONS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_ICONS, AwnIcons))

#define AWN_ICONS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_ICONS, AwnIconsClass))

#define AWN_IS_ICONS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_ICONS))

#define AWN_IS_ICONS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_ICONS))

#define AWN_ICONS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_ICONS, AwnIconsClass))

typedef struct { 
  GObject parent;
} AwnIcons;

typedef struct {
  GObjectClass parent_class;
} AwnIconsClass;

typedef void(* AwnIconsChange )(AwnIcons * fx,gpointer user_data);

GType awn_icons_get_type (void);

AwnIcons* 
awn_icons_new (const gchar * applet_name);

GdkPixbuf * 
awn_icons_get_icon_at_height(AwnIcons * icons, const gchar * state, gint height);

GdkPixbuf * 
awn_icons_get_icon_simple_at_height(AwnIcons * icons, gint height);

GdkPixbuf * 
awn_icons_get_icon(AwnIcons * icons, const gchar * state);

GdkPixbuf * 
awn_icons_get_icon_simple(AwnIcons * icons);

void 
awn_icons_set_height(AwnIcons * icons, gint height);

void 
awn_icons_set_icons_info(AwnIcons * icons,
                              GtkWidget * applet,
                              const gchar * uid,
                              gint height,
                              const GStrv states,
                              const GStrv icon_names);

void 
awn_icons_set_icon_info(AwnIcons * icons,
                             GtkWidget * applet,
                             const gchar * uid, 
                             gint height,
                             const gchar *icon_name);

void
awn_icons_override_gtk_theme(AwnIcons * icons,gchar * theme_name);
                        
/*
* These callbacks should be implemented as a signals.
*/                             
void 
awn_icons_set_changed_cb(AwnIcons * icons,AwnIconsChange fn,gpointer user_data);                             

G_END_DECLS

#endif /* _AWN_ICONS */
