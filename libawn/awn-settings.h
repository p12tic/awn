/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  Notes : This is the actual icon on the app, the "Application Icon"
*/

#ifndef _AWN_SETTINGS_H
#define _AWN_SETTINGS_H

#include <glib.h>
#include <gtk/gtk.h>

#include <libawn/awn-cairo-utils.h>

#define AWN_MIN_BAR_HEIGHT 10

typedef struct
{

  /* Misc globals */
  GtkIconTheme *icon_theme;
  GtkWidget *bar;
  GtkWidget *window;
  GtkWidget *title;
  GtkWidget *appman;
  GtkWidget *hot;
  gint task_width;
  gboolean show_dialog;

  /* monitor settings */
  GdkRectangle monitor;
  gboolean force_monitor;
  int monitor_height;
  int monitor_width;
  gboolean panel_mode;

  gboolean auto_hide;
  gboolean hidden;
  gboolean hiding;
  gint auto_hide_delay;
  gboolean keep_below;

  int bar_height;
  int bar_angle;
  gfloat bar_pos;
  gboolean no_bar_resize_ani;

  /* Bar appearance settings */
  gboolean rounded_corners;
  gfloat corner_radius;
  gboolean render_pattern;
  gboolean expand_bar;

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
  int arrow_offset;
  gboolean tasks_have_arrows;

  gboolean name_change_notify;
  gboolean alpha_effect;
  gint icon_effect;
  float icon_alpha;
  float reflection_alpha_mult;
  gint frame_rate;
  gboolean icon_depth_on;

  int icon_offset;
  int reflection_offset;
  gboolean show_shadows;

  /* Title settings */
  AwnColor text_color;
  AwnColor shadow_color;
  AwnColor background;
  gchar *font_face;

  gboolean btest;
  float ftest;
  char* stest;
  AwnColor ctest;
  GSList *ltest;

  /* Curves settings */
  int bar_width;
  gfloat curviness;
  gfloat curves_symmetry;

} AwnSettings;


AwnSettings* awn_settings_new(void);

/* returns singleton */
AwnSettings* awn_get_settings(void);

#endif /* _AWN_SETTINGS_H */
