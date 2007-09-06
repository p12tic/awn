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
static gint hide_delay = 1000 / 20;

static gboolean
_move_bar (AwnSettings *settings)
{
	if (settings->hidden && !settings->hiding) {
		dest_y = 0;
		settings->hidden = FALSE;
	}
 	if (current_y > dest_y) {
		if (current_y - dest_y == 1) {
			current_y -= 1;
		} else {
			current_y -= 2;
		}
 	} else if (current_y < dest_y) {
 		if (hide_delay > 0 && current_y == 0) {
 			hide_delay--;
 			return TRUE;
 		}
		if (dest_y - current_y == 1) {
			current_y += 1;
		} else {
			current_y += 2;
		}
 	} else {
 		hide_delay = settings->auto_hide_delay / 20;
 		effect_lock = FALSE;
		settings->hiding = FALSE;
 		return FALSE;
	}

	gint x, y;
	
	gtk_window_get_position (GTK_WINDOW (settings->bar), &x, &y);
	gtk_window_move (GTK_WINDOW (settings->bar), x, settings->monitor.height - ((settings->bar_height +2)*2) - settings->icon_offset + current_y);
	
	gtk_window_get_position (GTK_WINDOW (settings->window), &x, &y);
	gtk_window_move (GTK_WINDOW (settings->window), x, settings->monitor.height - ((settings->bar_height)*2) - settings->icon_offset + current_y);
	
	return TRUE;
	
}

static gboolean
_lower_bar (AwnSettings *settings)
{
	if(!settings->hiding) {
		hide_delay = settings->auto_hide_delay / 20;
		return TRUE;
	}

	if (hide_delay > 0) {
 		hide_delay--;
 		return TRUE;
 	}

 	hide_delay = settings->auto_hide_delay / 20;
 	
 	gtk_window_set_keep_below(GTK_WINDOW (settings->bar), TRUE);
	gtk_window_set_keep_below(GTK_WINDOW (settings->window), TRUE);
	gtk_window_stick(GTK_WINDOW (settings->bar));
	gtk_window_stick(GTK_WINDOW (settings->window));
	gtk_window_set_decorated(GTK_WINDOW (settings->window), FALSE);
	gtk_window_set_decorated(GTK_WINDOW (settings->bar), FALSE);
	effect_lock = FALSE;
	settings->hiding = FALSE;
	return FALSE;
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
		if(settings->keep_below){
			g_timeout_add (20, (GSourceFunc)_lower_bar, (gpointer)settings);
		}
		else{
			g_timeout_add (20, (GSourceFunc)_move_bar, (gpointer)settings);
		}
		effect_lock = TRUE;
	}
	
	//gtk_widget_hide (settings->title);
	settings->hidden = TRUE;
	settings->hiding = TRUE;
}

void 
awn_show (AwnSettings *settings)
{
	if(settings->keep_below){
		gtk_window_set_keep_above(GTK_WINDOW (settings->bar), TRUE);
		gtk_window_set_keep_above(GTK_WINDOW (settings->window), TRUE);
		gtk_window_set_decorated(GTK_WINDOW (settings->window), FALSE);
		gtk_window_set_decorated(GTK_WINDOW (settings->bar), FALSE);
		settings->hidden = FALSE;
		settings->hiding = FALSE;
		return;
	}
	
	dest_y = 0;

	if (!effect_lock) {
		g_timeout_add (20, (GSourceFunc)_move_bar, (gpointer)settings);
		effect_lock = TRUE;
	}
	
	//gtk_widget_show (settings->title);
	settings->hidden = FALSE;
	settings->hiding = FALSE;
}


