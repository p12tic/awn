/*
 * Copyright (C) 2008 Neil Jagdish Patel
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

#ifndef _AWN_ICON_H_
#define _AWN_ICON_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "awn-effects.h"
#include "awn-tooltip.h"

G_BEGIN_DECLS

#define AWN_TYPE_ICON (awn_icon_get_type ())

#define AWN_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	AWN_TYPE_ICON, AwnIcon))

#define AWN_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	AWN_TYPE_ICON, AwnIconClass))

#define AWN_IS_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	AWN_TYPE_ICON))

#define AWN_IS_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	AWN_TYPE_ICON))

#define AWN_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	AWN_TYPE_ICON, AwnIconClass))

typedef struct _AwnIcon        AwnIcon;
typedef struct _AwnIconClass   AwnIconClass;
typedef struct _AwnIconPrivate AwnIconPrivate;
/*typedef enum   _AwnIconState   AwnIconState;*/
 
struct _AwnIcon
{
  GtkDrawingArea  parent;	

  AwnIconPrivate *priv;
};

struct _AwnIconClass
{
  GtkDrawingAreaClass parent_class;

  void (*size_changed)       (AwnIcon *icon);
  void (*clicked)            (AwnIcon *icon);
  void (*middle_clicked)     (AwnIcon *icon);
  void (*long_press)         (AwnIcon *icon);
  void (*context_menu_popup) (AwnIcon *icon, GdkEventButton *event);

  void (*icon_padding0) (AwnIcon *icon);
  void (*icon_padding1) (AwnIcon *icon);
  void (*icon_padding2) (AwnIcon *icon);
  void (*icon_padding3) (AwnIcon *icon);
};

GType           awn_icon_get_type            (void) G_GNUC_CONST;

GtkWidget *     awn_icon_new                 (void);

void            awn_icon_set_pos_type        (AwnIcon        *icon,
                                              GtkPositionType position);
GtkPositionType awn_icon_get_pos_type        (AwnIcon        *icon);

void            awn_icon_set_offset          (AwnIcon        *icon,
                                              gint            offset);
gint            awn_icon_get_offset          (AwnIcon        *icon);

void            awn_icon_set_effect          (AwnIcon     *icon, 
                                              AwnEffect    effect);

void            awn_icon_set_from_pixbuf     (AwnIcon     *icon,
                                              GdkPixbuf   *pixbuf);

void            awn_icon_set_from_context    (AwnIcon     *icon,
                                              cairo_t     *ctx);

void            awn_icon_set_from_surface    (AwnIcon         *icon,
                                              cairo_surface_t *surface);

void            awn_icon_set_custom_paint    (AwnIcon *icon,
                                              gint width, gint height);

AwnTooltip*     awn_icon_get_tooltip         (AwnIcon *icon);

void            awn_icon_set_tooltip_text    (AwnIcon     *icon,
                                              const gchar *text);
gchar *         awn_icon_get_tooltip_text    (AwnIcon     *icon);

void            awn_icon_set_is_active       (AwnIcon     *icon,
                                              gboolean     is_active);
gboolean        awn_icon_get_is_active       (AwnIcon     *icon);

void            awn_icon_set_indicator_count (AwnIcon *icon, gint count);
gint            awn_icon_get_indicator_count (AwnIcon *icon);

gboolean        awn_icon_get_hover_effects   (AwnIcon *icon);
void            awn_icon_set_hover_effects   (AwnIcon *icon, gboolean enable);

GdkRegion*      awn_icon_get_input_mask      (AwnIcon *icon);

void            awn_icon_clicked             (AwnIcon *icon);
void            awn_icon_middle_clicked      (AwnIcon *icon);

G_END_DECLS

#endif /* _AWN_ICON_H_ */

