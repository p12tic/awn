/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *             Hannes Verschore <hv1989@gmail.com>
 *             Rodney Cryderman <rcryderman@gmail.com>
 *
 */

#include "dock-manager-api.h"

#define TASK_ICON_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ICON, \
  TaskIconPrivate))

struct _TaskIconPrivate
{
  //List containing the TaskItems
  GSList *items;

  //The number of TaskItems that get shown
  guint shown_items;

  //The number of TaskWindows (subclass of TaskItem) that needs attention
  guint needs_attention;

  //The number of TaskWindows (subclass of TaskItem) that have the active state.
  guint is_active;

  //The main item being used for the icon (and if alone for the text)
  TaskItem *main_item;

  //Whetever this icon is visible or not
  gboolean visible;

  //An overlay for showing number of items
  AwnOverlayText *overlay_text;

  // Config client
  DesktopAgnosticConfigClient *client;
  
  GdkPixbuf *icon;
  AwnApplet *applet;
  GtkWidget *dialog;

  /*context menu*/
  GtkWidget * menu;
  guint       autohide_cookie;
  gchar *   menu_filename;

  gboolean draggable;
  gboolean gets_dragged;

  guint    drag_tag;
  gboolean drag_motion;
  guint    drag_time;
  gint     drag_and_drop_hover_delay;

  gint old_width;
  gint old_height;
  gint old_x;
  gint old_y;
  
  /*Bound to config keys*/
  guint  max_indicators;
  guint  txt_indicator_threshold;

  gint update_geometry_id;

  /*Keep track if TaskLauncher was added through desktop file lookup
   FIXME _should_ be able to dump this by setting the task launcher visibility to false for ephemeral launchers.
   target for 0.6. */
  GObject   *proxy_obj;

  gboolean  inhibit_focus_loss;
  
  /*prop*/
  gboolean  enable_long_press;
  gint      icon_change_behavior;
  gint      desktop_copy;
  
  gboolean  long_press;     /*set to TRUE when there has been a long press so the clicked event can be ignored*/
  gchar * custom_name;

  AwnOverlayPixbuf  * overlay_app_icon;
  gboolean  overlay_application_icons;
  gdouble   overlay_application_icons_alpha;
  gdouble   overlay_application_icons_scale;
  gboolean  overlay_application_icons_swap;

  /*Plugin menu items*/
  GList * plugin_menu_items;

  TaskIconDispatcher *dbus_proxy;
};
