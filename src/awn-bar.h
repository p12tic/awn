/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/


#ifndef	_AWN_BAR_H
#define	_AWN_BAR_H

#include <glib.h>
#include <gtk/gtk.h>

#include <libawn/awn-settings.h>
G_BEGIN_DECLS

#define AWN_TYPE_BAR          (awn_bar_get_type())

#define AWN_BAR(obj)			    (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                              AWN_TYPE_BAR, AwnBar))

#define AWN_BAR_CLASS(obj)		(G_TYPE_CHECK_CLASS_CAST ((obj), \
                              AWN_BAR, AwnBarClass))

#define AWN_IS_BAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_BAR))

#define AWN_IS_BAR_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), AWN_TYPE_BAR))

#define AWN_BAR_GET_CLASS		(G_TYPE_INSTANCE_GET_CLASS ((obj), \
                            AWN_TYPE_BAR, AwnBarClass))

typedef struct _AwnBar AwnBar;
typedef struct _AwnBarClass AwnBarClass;

struct _AwnBar {
        GtkWindow parent;
      
};

struct _AwnBarClass {
        GtkWindowClass parent_class;
};

GType awn_bar_get_type(void);

GtkWidget *awn_bar_new(AwnSettings *settings);

void awn_bar_resize(GtkWidget *window, gint width);
gboolean _on_expose (GtkWidget *widget, GdkEventExpose *expose);
void awn_bar_set_separator_position (GtkWidget *bar, int x);
void awn_bar_set_draw_separator (GtkWidget *bar, int x);
gboolean awn_bar_separator_expose_event  (GtkWidget      *widget,
                                 GdkEventExpose *event,
                                 GtkWidget 	*bar);
float apply_perspective_x (double width, double height, double x);
float apply_perspective_y (double height);
void
awn_bar_add_separator (AwnBar *bar, GtkWidget *widget);

G_END_DECLS


#endif /* _AWN_BAR_H */

