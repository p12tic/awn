/* 
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
 */
 
#ifndef __TASK_MANAGER_UTIL_H__ 
#define __TASK_MANAGER_UTIL_H__

#include <libawn/libawn.h>
#include <libdesktop-agnostic/fdo.h>
#include "task-defines.h"


gchar * get_special_id_from_desktop (DesktopAgnosticFDODesktopEntry *entry);

gchar * get_special_id_from_window_data (gchar * cmd, gchar *res_name, 
                                      gchar * class_name,const gchar *title);

GSList * get_special_desktop_from_window_data (gchar * cmd, gchar *res_name, 
                                              gchar * class_name,
                                              const gchar *title);

gchar * get_full_cmd_from_pid (gint pid);

gboolean check_no_display_override (const gchar * fname);

gboolean check_if_blacklisted (gchar * name);

gboolean get_special_wait_from_window_data (gchar *res_name, 
                                            gchar * class_name,
                                            const gchar *title);

WinIconUse get_win_icon_use           (gchar * cmd,
                                       gchar *res_name, 
                                       gchar * class_name,
                                       const gchar *title);

gboolean utils_gdk_pixbuf_similar_to (GdkPixbuf *i1, GdkPixbuf *i2);

gboolean usable_desktop_entry (  DesktopAgnosticFDODesktopEntry * entry);

gboolean usable_desktop_file_from_path ( const gchar * path);

char* _desktop_entry_get_localized_name (DesktopAgnosticFDODesktopEntry *entry);

#endif
