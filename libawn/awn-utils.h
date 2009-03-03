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

/*
 * awn_utils_ensure_tranparent_bg:
 * @widget: Widget which should have transparent background.
 *
 * Ensures that the widget has transparent background all the time
 * by connecting to the GtkWidget::realize and GtkWidget::style-set signals.
 */
void awn_utils_ensure_tranparent_bg (GtkWidget *widget);

/*
 * awn_utils_make_transparent_bg:
 * @widget: Widget which background will be modified.
 *
 * Modifies the background pixmap on the widget to be transparent if composited
 * environment is used.
 */
void awn_utils_make_transparent_bg  (GtkWidget *widget);

#endif
