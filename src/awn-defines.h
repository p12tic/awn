/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifndef _HAVE_AWN_DEFINES_H
#define _HAVE_AWN_DEFINES_H

#define AWN_DBUS_NAMESPACE  "org.awnproject.Awn"
#define AWN_DBUS_PATH       "/org/awnproject/Awn"

#define AWN_DBUS_APP_PATH   AWN_DBUS_PATH
#define AWN_DBUS_PANEL_PATH AWN_DBUS_PATH"/Panel"
#define AWN_DBUS_MANAGER_PATH AWN_DBUS_PATH"/UA"

/* FIXME: Move some of these out into libawn when we can */
#define AWN_GROUP_EFFECTS          "effects"
#define AWN_EFFECTS_DOT_COLOR      "dot_color"
#define AWN_EFFECTS_RECT_COLOR     "active_rect_color"
#define AWN_EFFECTS_RECT_OUTLINE   "active_rect_outline"

#define AWN_GROUP_PANELS           "panels"
#define AWN_PANELS_HIDE_DELAY      "hide_delay"
#define AWN_PANELS_POLL_DELAY      "mouse_poll_delay"
#define AWN_PANELS_IDS             "panel_list"

#define AWN_GROUP_PANEL            "panel"
#define AWN_PANEL_PANEL_MODE       "panel_mode"
#define AWN_PANEL_EXPAND           "expand"
#define AWN_PANEL_POSITION         "orient"
#define AWN_PANEL_OFFSET           "offset"
#define AWN_PANEL_SIZE             "size"
#define AWN_PANEL_AUTOHIDE         "autohide"
#define AWN_PANEL_APPLET_LIST      "applet_list"
#define AWN_PANEL_UA_LIST          "ua_list"
#define AWN_PANEL_UA_ACTIVE_LIST   "ua_active_list"
#define AWN_PANEL_MONITOR_HEIGHT   "monitor_height"
#define AWN_PANEL_MONITOR_WIDTH    "monitor_width"
#define AWN_PANEL_MONITOR_FORCE    "monitor_force"
#define AWN_PANEL_MONITOR_NUM      "monitor_num"
#define AWN_PANEL_MONITOR_X_OFFSET "monitor_x_offset"
#define AWN_PANEL_MONITOR_Y_OFFSET "monitor_y_offset"
#define AWN_PANEL_MONITOR_ALIGN    "monitor_align"
#define AWN_PANEL_STYLE            "style"
#define AWN_PANEL_CLICKTHROUGH     "clickthrough"

#define AWN_GROUP_THEME            "theme"
#define AWN_THEME_GSTEP1           "gstep1"
#define AWN_THEME_GSTEP2           "gstep2"
#define AWN_THEME_GHISTEP1         "ghistep1"
#define AWN_THEME_GHISTEP2         "ghistep2"
#define AWN_THEME_BORDER           "border"
#define AWN_THEME_HILIGHT          "hilight"
#define AWN_THEME_TEXT_COLOR       "icon_text_color"
#define AWN_THEME_OUTLINE_COLOR    "icon_text_outline_color"

#define AWN_THEME_DLG_GTK_MODE     "dialog_gtk_mode"
#define AWN_THEME_DLG_BG           "dialog_bg"
#define AWN_THEME_DLG_TITLE_BG     "dialog_title_bg"

#define AWN_THEME_SHOW_SEP         "show_sep"
#define AWN_THEME_SEP_COLOR        "sep_color"

#define AWN_THEME_DRAW_PATTERN     "draw_pattern"
#define AWN_THEME_PATTERN_ALPHA    "pattern_alpha"
#define AWN_THEME_PATTERN_FILENAME "pattern_filename"

#define AWN_THEME_GTK_THEME_MODE   "gtk_theme_mode"
#define AWN_THEME_CORNER_RADIUS    "corner_radius"
#define AWN_THEME_PANEL_ANGLE      "panel_angle"
#define AWN_THEME_CURVINESS        "curviness"
#define AWN_THEME_CURVES_SYMMETRY  "curves_symmetry"
#define AWN_THEME_FLOATY_OFFSET    "floaty_offset"
#define AWN_THEME_THICKNESS        "thickness"

#endif /*_HAVE_AWN_DEFINES_H */

