/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
 *  Copyright (C) 2009 Mark Lee <avant-wn@lazymalevolence.com>
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
*/

#ifndef __AWN_UTILITY_H
#define __AWN_UTILITY_H

#include <gtk/gtk.h>
#include "awn-defines.h"

/**
 * awn_utils_ensure_transparent_bg:
 * @widget: Widget which should have transparent background.
 *
 * Ensures that the widget has transparent background all the time
 * by connecting to the GtkWidget::realize and GtkWidget::style-set signals.
 */
void awn_utils_ensure_transparent_bg (GtkWidget *widget);

/**
 * awn_utils_make_transparent_bg:
 * @widget: Widget which background will be modified.
 *
 * Modifies the background pixmap on the widget to be transparent if composited
 * environment is used.
 */
void awn_utils_make_transparent_bg  (GtkWidget *widget);

/**
 * awn_utils_get_offset_modifier_by_path_type:
 * @path_type:
 * @position:
 * @offset:
 * @offset_modifier:
 * @pos_x:
 * @pos_y:
 * @width:
 * @height:
 *
 * Computes modifier for offset value based on current path_type and position
 * of a widget on the panel.
 *
 * Returns: Offset modifier, offset value should be multiplied by this
 * modifier.
 */
gfloat awn_utils_get_offset_modifier_by_path_type (AwnPathType path_type,
                                                   GtkPositionType position,
                                                   gint offset,
                                                   gfloat offset_modifier,
                                                   gint pos_x, gint pos_y,
                                                   gint width, gint height);

/**
 * awn_utils_gslist_to_gvaluearray:
 * @list: The #GSList of #gchar pointers to convert.
 *
 * Converts a #GSList of strings to a #GValueArray, suitable for use with a
 * configuration client.
 *
 * Returns: A newly allocated #GValueArray (the #GValue elements and their
 *          contents are also newly allocated).
 */
GValueArray* awn_utils_gslist_to_gvaluearray (GSList *list);

/**
 * awn_utils_show_menu_images:
 * @menu: A GtkMenu.
 *
 * Set all instances #GtkImageMenuItem in the #GtkMenu are set to visible. A 
 * null op for GTK+ < 2.16.0
 *
 */
void awn_utils_show_menu_images (GtkMenu * menu);

const gchar *awn_utils_get_gtk_icon_theme_name (GtkIconTheme * theme);

/**
 * awn_utils_menu_set_position_widget_relative:
 * @menu: a GtkMenu
 * @px: address of the gint representing the horizontal position 
        where the menu shall be drawn. This is an output parameter.
 * @py: address of the gint representing the vertical position 
 *       where the menu shall be drawn. This is an output parameter.
 * @push_in: This parameter controls how menus placed outside the monitor are 
 *           handled. This doesn't work well.
 * @data: the data supplied by the user in the gtk_menu_popup() data parameter.
 *
 * This function (*GtkMenuPositionFunc) is used when calling gtk_menu_popup() 
 * which controls the positioning of the menu when it is displayed.
 * We use this custom positioning to popup menu relative to an icon/applet.
 * See for example "awn_icon_popup_gtk_menu" into awn-icon.c
 */
void awn_utils_menu_set_position_widget_relative (GtkMenu *menu,
                                                  gint *px,
                                                  gint *py,
                                                  gboolean *push_in,
                                                  gpointer data);
#endif

