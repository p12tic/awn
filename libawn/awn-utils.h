/*
 *  Copyright (C) 
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * F#include <gtk/gtk.h>ree Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
 * awn_vfs_init:
 *
 * Starts up the VFS library routines that Awn uses, if necessary.
 */
void awn_vfs_init (void);

/**
 * get_offset_modifier_by_path_type:
 * @path_type:
 * @orient:
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
gfloat get_offset_modifier_by_path_type (AwnPathType path_type,
                                         AwnOrientation orient,
                                         gfloat offset_modifier,
                                         gint pos_x, gint pos_y,
                                         gint width, gint height);

#endif
