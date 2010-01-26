/*
 * Copyright (C) 2008 Rodney Cryderman <rcryderman@gmail.com>
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifndef _AWN_THEMED_ICON_H_
#define _AWN_THEMED_ICON_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "awn-icon.h"

G_BEGIN_DECLS

#define AWN_TYPE_THEMED_ICON (awn_themed_icon_get_type ())

#define AWN_THEMED_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	AWN_TYPE_THEMED_ICON, AwnThemedIcon))

#define AWN_THEMED_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	AWN_TYPE_THEMED_ICON, AwnThemedIconClass))

#define AWN_IS_THEMED_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	AWN_TYPE_THEMED_ICON))

#define AWN_IS_THEMED_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	AWN_TYPE_THEMED_ICON))

#define AWN_THEMED_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	AWN_TYPE_THEMED_ICON, AwnThemedIconClass))

typedef struct _AwnThemedIcon        AwnThemedIcon;
typedef struct _AwnThemedIconClass   AwnThemedIconClass;
typedef struct _AwnThemedIconPrivate AwnThemedIconPrivate;
 
struct _AwnThemedIcon
{
  AwnIcon  parent;	

  AwnThemedIconPrivate *priv;
};

struct _AwnThemedIconClass
{
  AwnIconClass parent_class;

  /*< padding >*/
  void (*_themed_icon_padding0) (AwnThemedIcon *icon);
  void (*_themed_icon_padding1) (AwnThemedIcon *icon);
  void (*_themed_icon_padding2) (AwnThemedIcon *icon);
  void (*_themed_icon_padding3) (AwnThemedIcon *icon);
};

GType         awn_themed_icon_get_type           (void) G_GNUC_CONST;

GtkWidget *   awn_themed_icon_new                (void);

void          awn_themed_icon_set_state          (AwnThemedIcon *icon,
                                                  const gchar   *state);
const gchar * awn_themed_icon_get_state          (AwnThemedIcon *icon);

void          awn_themed_icon_set_size           (AwnThemedIcon *icon,
                                                  gint           size);
gint          awn_themed_icon_get_size           (AwnThemedIcon *icon);

const gchar * awn_themed_icon_get_default_theme_name (AwnThemedIcon *icon);

void          awn_themed_icon_set_info           (AwnThemedIcon  *icon,
                                                  const gchar    *applet_name,
                                                  const gchar    *uid,
                                                  GStrv         states,
                                                  GStrv	        icon_names);

void          awn_themed_icon_set_info_simple    (AwnThemedIcon  *icon,
                                                  const gchar    *applet_name,
                                                  const gchar    *uid,
                                                  const gchar    *icon_name);

void          awn_themed_icon_set_info_append    (AwnThemedIcon  *icon,
                                                  const gchar    *state,
                                                  const gchar    *icon_name);

void          awn_themed_icon_set_applet_info    (AwnThemedIcon  *icon,
                                                  const gchar    *applet_name,
                                                  const gchar    *uid);

void          awn_themed_icon_override_gtk_theme (AwnThemedIcon *icon,
                                                  const gchar   *theme_name);

GdkPixbuf *   awn_themed_icon_get_icon_at_size   (AwnThemedIcon *icon,
                                                  gint          size,
                                                  const gchar   *state);

void          awn_themed_icon_clear_icons        (AwnThemedIcon *icon,
                                                  gint           scope);

void          awn_themed_icon_clear_info         (AwnThemedIcon *icon);

void          awn_themed_icon_preload_icon       (AwnThemedIcon * icon, 
                                                  gchar * state, 
                                                  gint size);
GtkIconTheme *awn_themed_icon_get_awn_theme     (AwnThemedIcon * icon);

GtkWidget *   awn_themed_icon_create_custom_icon_item (AwnThemedIcon * icon,
                                                 const gchar * icon_name);
GtkWidget *   awn_themed_icon_create_remove_custom_icon_item (AwnThemedIcon * icon,const gchar *icon_name);

void awn_themed_icon_drag_data_received (GtkWidget        *widget, 
                                    GdkDragContext   *context,
                                    gint              x, 
                                    gint              y, 
                                    GtkSelectionData *selection_data,
                                    guint             info,
                                    guint             evt_time);


G_END_DECLS

#endif /* _AWN_THEMED_ICON_H_ */
