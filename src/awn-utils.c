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

#include "awn-utils.h"  

static gboolean effect_lock = FALSE;
static gint current_y = 0;
static gint dest_y = 0;

static gboolean
_move_bar (AwnSettings *settings)
{
 	if (current_y > dest_y) {
		if (current_y - dest_y == 1) {
			current_y -= 1;
		} else {
			current_y -= 2;
		}
 	} else if (current_y < dest_y) {
		if (dest_y - current_y == 1) {
			current_y += 1;
		} else {
			current_y += 2;
		}
 	} else {
 		effect_lock = FALSE;
 		return FALSE;
	}

	gint x, y;
	
	gtk_window_get_position (GTK_WINDOW (settings->bar), &x, &y);
	gtk_window_move (GTK_WINDOW (settings->bar), x, settings->monitor.height - ((settings->bar_height +2)*2) + current_y);
	
	gtk_window_get_position (GTK_WINDOW (settings->window), &x, &y);
	gtk_window_move (GTK_WINDOW (settings->window), x, settings->monitor.height - ((settings->bar_height)*2) - settings->icon_offset + current_y);
	
	return TRUE;
	
}


void 
awn_hide (AwnSettings *settings)
{
	//gtk_widget_hide (settings->window);
	//gtk_widget_hide (settings->bar);
	//hide_window (GTK_WINDOW (settings->bar));
	//hide_window (GTK_WINDOW (settings->window));
	
	dest_y = settings->bar_height + settings->icon_offset+4;
	
	if (!effect_lock) {
		g_timeout_add (20, (GSourceFunc)_move_bar, (gpointer)settings);
		effect_lock = TRUE;
	} 
	
	gtk_widget_hide (settings->title);
	settings->hidden = TRUE;
}

void 
awn_show (AwnSettings *settings)
{
	
	dest_y = 0;

	if (!effect_lock) {
		g_timeout_add (20, (GSourceFunc)_move_bar, (gpointer)settings);
		effect_lock = TRUE;
	}
	
	gtk_widget_show (settings->title);
	settings->hidden = FALSE;
}


