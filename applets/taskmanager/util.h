/* 
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com> 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
 
#ifndef __TASK_MANAGER_UTIL_H__ 
#define __TASK_MANAGER_UTIL_H__

#include <libawn/libawn.h>
#include <libdesktop-agnostic/fdo.h>


typedef enum{
  USE_DEFAULT=0,
  USE_ALWAYS=1,
  USE_NEVER=2
}WinIconUse;

gchar * get_special_id_from_desktop (DesktopAgnosticFDODesktopEntry *entry);

gchar * get_special_id_from_window_data (gchar * cmd, gchar *res_name, 
                                      gchar * class_name,const gchar *title);

GSList * get_special_desktop_from_window_data (gchar * cmd, gchar *res_name, 
                                              gchar * class_name,
                                              const gchar *title);

gchar * get_full_cmd_from_pid (gint pid);

gboolean check_if_blacklisted (gchar * name);

gboolean get_special_wait_from_window_data (gchar *res_name, 
                                            gchar * class_name,
                                            const gchar *title);

WinIconUse get_win_icon_use           (gchar * cmd,
                                       gchar *res_name, 
                                       gchar * class_name,
                                       const gchar *title);
#endif
