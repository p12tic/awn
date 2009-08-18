/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*-
 *
 * GSEAL transition definitions.
 *
 * Copyright (C) 2009 Mark Lee <marklee@src.gnome.org>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef __GSEAL_TRANSITION_H__
#define __GSEAL_TRANSITION_H__

#ifndef GSEAL
#define gtk_selection_data_get_data(x) (x)->data
#define gtk_selection_data_get_length(x) (x)->length
#define gtk_socket_get_plug_window(x) (x)->plug_window
#define gtk_widget_get_window(x) (x)->window
#endif

#if !GTK_CHECK_VERSION(2,17,5)
#define gtk_widget_get_has_window(x) (!GTK_WIDGET_NO_WINDOW (x))
#endif

#if !GTK_CHECK_VERSION(2,17,7)
#define gtk_widget_get_allocation(w, alloc) { *(alloc) = (w)->allocation; }
#define gtk_widget_get_visible(x) GTK_WIDGET_VISIBLE(x)
#endif

#endif
/* vim: set ts=2 sts=2 sw=2 ai cindent : */
