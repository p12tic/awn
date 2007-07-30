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
 *
 *  Notes : This is the actual icon on the app, the "Application Icon" 
*/

#ifndef	_AWN_GCONF_H
#define	_AWN_GCONF_H

#include <glib.h>
#include <gtk/gtk.h>

#include <gconf/gconf-client.h>



typedef struct {
	float red;
	float green;
	float blue;
	float alpha;

} AwnColor; /* spelt incorrectly, in the interest of brevity ;) */

typedef struct {
	
	/* Misc globals */
	GtkIconTheme *icon_theme;
	GtkWidget *bar;
	GtkWidget *window;
	GtkWidget *title;
	GtkWidget *appman;
	gint task_width;
	
	/* monitor settings */
	GdkRectangle monitor;
	gboolean force_monitor;
	int monitor_height;
	int monitor_width;
	gboolean panel_mode;
	
	gboolean auto_hide;
	gboolean hidden;
	
	int bar_height;
	int bar_angle;

	/* Bar appearance settings */
	gboolean rounded_corners;
	gfloat corner_radius;
	gboolean render_pattern;
	
	gchar *pattern_uri;
	gfloat pattern_alpha;
	
	AwnColor g_step_1;
	AwnColor g_step_2;
	AwnColor g_histep_1;
	AwnColor g_histep_2;
	AwnColor border_color;
	AwnColor hilight_color;
	
	gboolean show_separator;
	AwnColor sep_color;
	
	/* Window Manager Settings */
	gboolean show_all_windows;
	GSList *launchers;
	
	/* Task settings */
	gboolean use_png;
	gchar *active_png;
	
	AwnColor arrow_color;
	gboolean tasks_have_arrows;
	gboolean fade_effect;
	
	gboolean name_change_notify;
	gboolean alpha_effect;
	gboolean hover_bounce_effect;
	
	int icon_offset;
	
	/* Title settings */
	AwnColor text_color;
	AwnColor shadow_color;
	AwnColor background;
	gboolean italic;
	gboolean bold;
	gfloat font_size;
	
	
	gboolean btest;
	float ftest;
	char* stest;
	AwnColor ctest;
	GSList *ltest;
		
} AwnSettings;


AwnSettings* awn_gconf_new(void);

void awn_gconf_set_window_to_update(GtkWidget *window);

#endif /* _AWN_GCONF_H */
