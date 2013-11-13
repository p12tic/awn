/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
 * Copyright (C) 2001 Havoc Pennington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * Authored by Rodney Cryderman <rcryderman@gmail.com>
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <libwnck/libwnck.h>

#include <libdesktop-agnostic/vfs.h>
#include "libawn/gseal-transition.h"
#include <libawn/awn-utils.h>

#include "taskmanager-marshal.h"
#include "task-icon.h"

#include "task-launcher.h"
#include "task-settings.h"
#include "task-manager.h"
#include "task-icon.h"
#include "task-icon-build-context-menus.h"
#include "task-icon-private.h"

#include "config.h"

#if !GTK_CHECK_VERSION(2,14,0)
#define GTK_ICON_LOOKUP_FORCE_SIZE 0
#endif

/*
 Use these for now, might replace with awn specific icons at some point
 */
#define STOCK_DELETE "wnck-stock-delete"
#define STOCK_MAXIMIZE "wnck-stock-maximize"
#define STOCK_MINIMIZE "wnck-stock-minimize"
#define MAX_MENU_ITEM_CHARS 55

static void menu_parse_start_element (GMarkupParseContext *context,
                                      const gchar         *element_name,
                                      const gchar        **attribute_names,
                                      const gchar        **attribute_values,
                                      gpointer            user_data,
                                      GError             **error);

static GtkWidget *task_icon_get_submenu_action_menu (TaskIcon * icon, WnckWindow * win);

static char *
get_workspace_name_with_accel (WnckWindow *window, int idx)
{
  const char *name;
  int number;
  WnckWorkspace * workspace;
  const char * fallback_name = "Unnamed";

  workspace = wnck_screen_get_workspace (wnck_window_get_screen (window),idx);
  if (!workspace || !WNCK_IS_WORKSPACE(workspace))
  {
    return g_strdup (fallback_name);
  }
  name = wnck_workspace_get_name (workspace);

  g_assert (name != NULL);

  /*
   * If the name is of the form "Workspace x" where x is an unsigned
   * integer, insert a '_' before the number if it is less than 10 and
   * return it
   */
  number = 0;
  if (sscanf (name, _("Workspace %d"), &number) == 1) 
  {
      char *new_name;

      /*
       * Above name is a pointer into the Workspace struct. Here we make
       * a copy copy so we can have our wicked way with it.
       */
      if (number == 10)
        new_name = g_strdup_printf (_("Workspace 1_0"));
      else
        new_name = g_strdup_printf (_("Workspace %s%d"),
                                    number < 10 ? "_" : "",
                                    number);
      return new_name;
  }
  else {
      /*
       * Otherwise this is just a normal name. Escape any _ characters so that
       * the user's workspace names do not get mangled.  If the number is less
       * than 10 we provide an accelerator.
       */
      char *new_name;
      const char *source;
      char *dest;

      /*
       * Assume the worst case, that every character is a _.  We also
       * provide memory for " (_#)"
       */
      new_name = g_malloc0 (strlen (name) * 2 + 6 + 1);

      /*
       * Now iterate down the strings, adding '_' to escape as we go
       */
      dest = new_name;
      source = name;
      while (*source != '\0') {
          if (*source == '_')
            *dest++ = '_';
          *dest++ = *source++;
      }

      /* People don't start at workstation 0, but workstation 1 */
      if (idx< 9) {
          g_snprintf (dest, 6, " (_%d)", idx + 1);
      }
      else if (idx == 9) {
          g_snprintf (dest, 6, " (_0)");
      }

      return new_name;
  }
}

static void
add_to_launcher_list_cb (GtkMenuItem * menu_item, TaskIcon * icon)
{
  TaskLauncher    *launcher = NULL;
  
  g_return_if_fail (TASK_IS_ICON (icon));  
  
  launcher = TASK_LAUNCHER(task_icon_get_launcher (icon));
  if (launcher)
  {
    TaskManager * applet;
    gboolean grouping;
    
    g_object_get (icon,
                  "applet",&applet,
                  NULL);
    g_object_get (applet,
                  "grouping",&grouping,
                  NULL);

    task_manager_append_launcher (TASK_MANAGER(applet),
                                  task_launcher_get_desktop_path(launcher));

    if ( task_icon_is_ephemeral(icon))
    {
      g_object_set (G_OBJECT(task_icon_get_launcher (icon)),
                    "proxy", task_icon_get_proxy (icon),
                  NULL);
    }
    g_object_set (applet,
                  "grouping",grouping,
                  NULL);
  }
}

static void
remove_from_launcher_list_cb (GtkMenuItem * menu_item, TaskIcon * icon)
{
  TaskLauncher    *launcher = NULL;
  
  g_return_if_fail (TASK_IS_ICON (icon));  
  
  launcher = TASK_LAUNCHER(task_icon_get_launcher (icon));
  if (launcher)
  {
    TaskManager * applet;
    gboolean grouping;
    g_object_get (icon,
                  "applet",&applet,
                  NULL);
    g_object_get (applet,
                  "grouping",&grouping,
                  NULL);
    task_manager_remove_launcher (TASK_MANAGER(applet),
                                  task_launcher_get_desktop_path(launcher));
    g_object_set (applet,
                  "grouping",grouping,
                  NULL);
  }
}

/*
 When a window is moved to another "workspace" in Compiz it will remain active
 if it was already active...  we really prefer that this not happen
 */

static void
activate_appropriate_window (WnckWindow * win_on_the_move)
{
  WnckWorkspace * workspace;
  workspace = wnck_screen_get_active_workspace (wnck_screen_get_default());
  if (workspace)
  {
    GList* windows = wnck_screen_get_windows_stacked (wnck_screen_get_default());
    GList * iter;
    for (iter = g_list_last (windows); iter; iter=g_list_previous (iter))
    {
      if (iter->data != win_on_the_move)
      {
        if (wnck_window_is_in_viewport (iter->data,workspace) )
        {
          wnck_window_activate (iter->data,gtk_get_current_event_time ());
          break;
        }
      }
    }
  }
}

static void
_move_window_left_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;
  workspace = wnck_window_get_workspace (win);
  if ( workspace && WNCK_IS_WORKSPACE (workspace) && wnck_workspace_is_virtual (workspace) )
  {
    int x,y,w,h;
    wnck_window_get_geometry (win,&x,&y,&w,&h);
    wnck_window_set_geometry (win,
                              WNCK_WINDOW_GRAVITY_CURRENT,
                              WNCK_WINDOW_CHANGE_X,
                              x - wnck_screen_get_width (wnck_screen_get_default ()),
                              y,
                              w,
                              h);
    activate_appropriate_window (win);
  }
  else
  {
    workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen (win),
                                                  wnck_window_get_workspace (win), 
                                                  WNCK_MOTION_LEFT);

    wnck_window_move_to_workspace (win, workspace);
  }
}

static void
_move_window_right_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;
  workspace = wnck_window_get_workspace (win);
  if ( workspace && WNCK_IS_WORKSPACE (workspace) && wnck_workspace_is_virtual (workspace) )
  {
    int x,y,w,h;
    wnck_window_get_geometry (win,&x,&y,&w,&h);
    wnck_window_set_geometry (win,
                              WNCK_WINDOW_GRAVITY_CURRENT,
                              WNCK_WINDOW_CHANGE_X,
                              x + wnck_screen_get_width (wnck_screen_get_default ()),
                              y,
                              w,
                              h);
    activate_appropriate_window (win);
  }
  else
  {
    workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen ( win),
                                                    wnck_window_get_workspace (win),
                                                    WNCK_MOTION_RIGHT);
    wnck_window_move_to_workspace (win, workspace);
  }
}

static void
_move_window_up_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;

  workspace = wnck_window_get_workspace (win);
  if ( workspace && WNCK_IS_WORKSPACE (workspace) &&wnck_workspace_is_virtual (workspace) )
  {
    int x,y,w,h;
    wnck_window_get_geometry (win,&x,&y,&w,&h);
    wnck_window_set_geometry (win,
                              WNCK_WINDOW_GRAVITY_CURRENT,
                              WNCK_WINDOW_CHANGE_Y,
                              x,
                              y - wnck_screen_get_height (wnck_screen_get_default ()),
                              w,
                              h);
    activate_appropriate_window (win);
  }
  else
  {
    workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen (win),
                                                    wnck_window_get_workspace (win),
                                                    WNCK_MOTION_UP);
    wnck_window_move_to_workspace (win, workspace);
  }
}

static void
_move_window_down_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;
  workspace = wnck_window_get_workspace (win);
  if ( workspace && WNCK_IS_WORKSPACE (workspace) &&wnck_workspace_is_virtual (workspace) )
  {
    int x,y,w,h;
    wnck_window_get_geometry (win,&x,&y,&w,&h);
    wnck_window_set_geometry (win,
                              WNCK_WINDOW_GRAVITY_CURRENT,
                              WNCK_WINDOW_CHANGE_Y,
                              x,
                              y + wnck_screen_get_height (wnck_screen_get_default ()),
                              w,
                              h);
    activate_appropriate_window (win);
  }
  else
  {

    workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen (win),
                                                    wnck_window_get_workspace (win),
                                                    WNCK_MOTION_DOWN);
    wnck_window_move_to_workspace (win, workspace);
  }
}

static void
_move_window_to_index (GtkMenuItem *menuitem, WnckWindow * win)
{
  int workspace_index;
  WnckWorkspace * workspace = NULL;
  workspace_index =  GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT(menuitem), g_quark_from_static_string("WORKSPACE")));  
//  workspace_index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "workspace"));

  workspace = wnck_window_get_workspace (win);
  if ( workspace && WNCK_IS_WORKSPACE (workspace) && wnck_workspace_is_virtual (workspace) )
  {
    int x,y,w,h;
    int viewport_x, viewport_y;
    int cols;
    int dest_col, dest_row;
    int delta_x, delta_y;
    int screen_width;
    int screen_height;
    
    screen_width = wnck_screen_get_width (wnck_screen_get_default());
    screen_height = wnck_screen_get_height (wnck_screen_get_default());
    
    viewport_x = wnck_workspace_get_viewport_x (workspace);
    viewport_y = wnck_workspace_get_viewport_y (workspace);
    cols = wnck_workspace_get_width (workspace)/screen_width;
    dest_col = workspace_index % cols;
    dest_row = workspace_index / cols;
    wnck_window_get_geometry (win,&x,&y,&w,&h);
    delta_x = (dest_col * screen_width) - viewport_x + (x % screen_width);
    delta_y = (dest_row * screen_height) - viewport_y + (y % screen_height);

    wnck_window_set_geometry (win,
                              WNCK_WINDOW_GRAVITY_STATIC,
                              WNCK_WINDOW_CHANGE_Y | WNCK_WINDOW_CHANGE_X,
                              delta_x,
                              delta_y,
                              w,
                              h);
    activate_appropriate_window (win);
  }
  else
  {
    wnck_window_move_to_workspace (win,
                                 wnck_screen_get_workspace (wnck_window_get_screen (win),
                                 workspace_index));
  }
}

static void
_close_all_cb (GtkMenuItem *menuitem, TaskIcon * icon)
{
  /*
   This might actually be a key event?  but it doesn't matter... the time
   field is in the same place*/
  GSList * list;
  GSList * i;
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();

  g_return_if_fail (event);
  list = task_icon_get_items (icon);
  for (i = list; i ; i=i->next)
  {
    if ( !TASK_IS_WINDOW (i->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (i->data))
    {
      continue;
    }
    
    wnck_window_close (task_window_get_window (TASK_WINDOW(i->data)),event->time);
  }
  gdk_event_free ((GdkEvent*)event);

}

static void
_close_window_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  /*
   This might actually be a key event?  but it doesn't matter... the time
   field is in the same place*/
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();

  g_return_if_fail (event);
  wnck_window_close (win,event->time);
  gdk_event_free ((GdkEvent*)event);
}

static void
_minimize_window_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  /*
   This might actually be a key event?  but it doesn't matter... the time
   field is in the same place*/
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();
  g_return_if_fail (event);
  if (wnck_window_is_minimized (win))
    wnck_window_unminimize (win,event->time);
  else
    wnck_window_minimize (win);
}

static void
_maximize_window_cb (GtkMenuItem *menuitem, WnckWindow *win)
{
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();
  
  if (wnck_window_is_maximized (win))
    wnck_window_unmaximize (win);
  else
  {
    wnck_window_unminimize (win,event->time);
    wnck_window_maximize (win);
  }
}

static void
_keep_above_cb (GtkMenuItem *menuitem, WnckWindow *win)
{
  if (WNCK_WINDOW_STATE_ABOVE & wnck_window_get_state (win))
  {
    wnck_window_unmake_above (win);
  }
  else
  {
    wnck_window_make_above (win);
  }
}

/*
static void
_keep_below_cb (GtkMenuItem *menuitem, WnckWindow *win)
{
  if (WNCK_WINDOW_STATE_ABOVE & wnck_window_get_state (win))
    wnck_window_make_above (win);
  else
    wnck_window_unmake_above (win);
}
*/
/*static void
_shade_window_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  if (wnck_window_is_shaded (win))
    wnck_window_unshade (win);
  else
    wnck_window_shade (win);
}
*/

static void
_pin_window_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *space = wnck_window_get_workspace (win);

  /*all the extra crap in here is courtesy of compiz*/
  if (!space)
  {
    space = wnck_screen_get_active_workspace (wnck_screen_get_default ());
  }
  if (wnck_window_is_pinned (win))
  {
    wnck_window_unpin (win);
    if (space && WNCK_IS_WORKSPACE(space) && wnck_workspace_is_virtual (space) )
    {
      GdkWindow *foreign_win = gdk_window_foreign_new(wnck_window_get_xid (win));
      gdk_window_unstick (foreign_win);
      g_object_unref (foreign_win);
    }
  }
  else
  {
    wnck_window_pin (win);
    if (space && WNCK_IS_WORKSPACE(space) && wnck_workspace_is_virtual (space) )
    {
      GdkWindow *foreign_win = gdk_window_foreign_new(wnck_window_get_xid (win));
      gdk_window_stick (foreign_win);
      g_object_unref (foreign_win);
    }
  }
}

static void
_minimize_all_cb (GtkMenuItem *menuitem, TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;

  g_return_if_fail (TASK_IS_ICON(icon));
  
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  
  priv = icon->priv;

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( wnck_window_is_minimized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    wnck_window_minimize (task_window_get_window (TASK_WINDOW(iter->data)));
  }
}

static void
_unminimize_all_cb (GtkMenuItem *menuitem, TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;

  g_return_if_fail (TASK_IS_ICON(icon));
  
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();
  g_return_if_fail (event);
  
  priv = icon->priv;

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( !wnck_window_is_minimized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    wnck_window_unminimize (task_window_get_window (TASK_WINDOW(iter->data)),event->time);
  }
}

static void
_maximize_all_cb (GtkMenuItem *menuitem, TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();

  g_return_if_fail (TASK_IS_ICON(icon));
  
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  
  priv = icon->priv;

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( wnck_window_is_maximized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    wnck_window_unminimize (task_window_get_window (TASK_WINDOW(iter->data)),event->time);
    wnck_window_maximize (task_window_get_window (TASK_WINDOW(iter->data)));
  }
}

static void
_unmaximize_all_cb (GtkMenuItem *menuitem, TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();
  
  g_return_if_fail (TASK_IS_ICON(icon));
  
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  
  priv = icon->priv;

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( !wnck_window_is_maximized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    wnck_window_unminimize (task_window_get_window (TASK_WINDOW(iter->data)),event->time);
    wnck_window_unmaximize (task_window_get_window (TASK_WINDOW(iter->data)));
  }
}

static void
_spawn_menu_cmd_cb (GtkMenuItem *menuitem, GStrv cmd_and_envs)
{
  gchar * shell_value = g_object_get_qdata (G_OBJECT (menuitem),g_quark_from_static_string("shell_value"));
  GError * err = NULL;
  
  g_setenv ("AWN_TASK_MENU_CMD",cmd_and_envs[0],TRUE);
  g_setenv ("AWN_TASK_PID",cmd_and_envs[1],TRUE);
  g_setenv ("AWN_TASK_XID",cmd_and_envs[2],TRUE);
  g_setenv ("AWN_TASK_EXEC",cmd_and_envs[3],TRUE);
  g_setenv ("AWN_TASK_DESKTOP",cmd_and_envs[4],TRUE);
  g_setenv ("AWN_TASK_DEBUG_TASKMAN_PID",cmd_and_envs[5],TRUE);
  g_setenv ("AWN_TASK_LEADER_XID",cmd_and_envs[6],TRUE);
  //Want access to shell variables...
  if (g_strcmp0 (shell_value, "yes")==0 || g_strcmp0 (shell_value, "true")==0)
  {
    if (system (cmd_and_envs[0]) ==-1)
    {     
      g_message ("%s: system() error '%s'",__func__,cmd_and_envs[0]);
    }
  }
  else
  {
    if (!g_spawn_command_line_async (cmd_and_envs[0],&err) )
    {
      g_message ("%s: spawn() error '%s'",__func__,err->message);
      g_error_free (err);
      err = NULL;
    }
  }
      
}

static GtkWidget *
task_icon_get_menu_item_add_to_launcher_list (TaskIcon * icon)
{
  g_return_val_if_fail (TASK_IS_ICON (icon),NULL);  
  
  GtkWidget * item;
  TaskIconPrivate * priv = TASK_ICON_GET_PRIVATE(icon);
  GValueArray *launcher_paths;
  GValue val = {0,};
  const TaskItem * launcher = task_icon_get_launcher (icon);
  gboolean found = FALSE;
  if (launcher)
  {
    const gchar * launcher_path = task_launcher_get_desktop_path (TASK_LAUNCHER(launcher));
    g_object_get (G_OBJECT (priv->applet), "launcher_paths", &launcher_paths, NULL);
    g_value_init (&val, G_TYPE_STRING);
    for (guint idx = 0; idx < launcher_paths->n_values; idx++)
    {
      const gchar *path = g_value_get_string (g_value_array_get_nth (launcher_paths, idx));
      if (g_strcmp0 (path,launcher_path)==0)
      {
        found = TRUE;
      }
    }
    g_value_unset (&val);
    g_value_array_free (launcher_paths);
  }
  if (found || !launcher || !task_icon_is_ephemeral (icon)  )
  {
    return NULL;
  }
  item = gtk_menu_item_new_with_label (_("Add as Launcher"));
  gtk_widget_show (item);
  g_signal_connect (item,"activate",
                    G_CALLBACK(add_to_launcher_list_cb),
                    icon);
  return item;
}

static GtkWidget *
task_icon_get_menu_item_remove_from_launcher_list (TaskIcon * icon)
{
  g_return_val_if_fail (TASK_IS_ICON (icon),NULL);  
  
  GtkWidget * item;
  TaskIconPrivate * priv = TASK_ICON_GET_PRIVATE(icon);
  GValueArray *launcher_paths;
  GValue val = {0,};
  const TaskItem * launcher = task_icon_get_launcher (icon);
  gboolean found = FALSE;
  if (launcher)
  {
    const gchar * launcher_path = task_launcher_get_desktop_path (TASK_LAUNCHER(launcher));
    g_object_get (G_OBJECT (priv->applet), "launcher_paths", &launcher_paths, NULL);
    g_value_init (&val, G_TYPE_STRING);
    for (guint idx = 0; idx < launcher_paths->n_values; idx++)
    {
      const gchar *path = g_value_get_string (g_value_array_get_nth (launcher_paths, idx));
      if (g_strcmp0 (path,launcher_path)==0)
      {
        found = TRUE;
      }
    }
    g_value_unset (&val);
    g_value_array_free (launcher_paths);
  }
  if ( !found || !launcher )
  {
    return NULL;
  }
  item = gtk_menu_item_new_with_label (_("Remove Launcher"));
  gtk_widget_show (item);
  g_signal_connect (item,"activate",
                    G_CALLBACK(remove_from_launcher_list_cb),
                    icon);
  return item;
}

static GtkWidget *
task_icon_get_menu_item_close_active (TaskIcon * icon, WnckWindow * win)
{
  GtkWidget * item;
  const TaskItem * main_item = task_icon_get_main_item (icon);
  GtkWidget * image;
  if (!main_item || !TASK_IS_WINDOW (main_item) )
  {
    return NULL;
  }  
  item = gtk_image_menu_item_new_with_mnemonic (_("_Close"));
  image = gtk_image_new_from_stock (STOCK_DELETE,GTK_ICON_SIZE_MENU);

  if (image)
  {
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item),image);
  }
  gtk_widget_show (item);
  g_signal_connect (item,"activate",
                G_CALLBACK(_close_window_cb),
                win);
  return item;
}

static GtkWidget *
task_icon_get_menu_item_close_all (TaskIcon * icon)
{
  GtkWidget * item;
  GtkWidget * image;
  const TaskItem * main_item = task_icon_get_main_item (icon);
  if (task_icon_count_tasklist_windows (icon) <=1)
  {
    return NULL;
  }
  if (!main_item || !TASK_IS_WINDOW (main_item) )
  {
    return NULL;
  }  
  item = gtk_image_menu_item_new_with_mnemonic (_("_Close All"));
  image = gtk_image_new_from_stock (STOCK_DELETE,GTK_ICON_SIZE_MENU);
  if (image)
  {
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item),image);
  }

  if (image)
  {
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),image);
  }
  gtk_widget_show (item);
  g_signal_connect (item,"activate",
                G_CALLBACK(_close_all_cb),
                icon);
  return item;
}

static GtkWidget *
task_icon_get_minimize_all (TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  GtkWidget * menuitem = NULL;
  
  priv = icon->priv;

  if (task_icon_count_tasklist_windows (icon) <=1)
  {
    return NULL;
  }

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( wnck_window_is_minimized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    menuitem = gtk_image_menu_item_new_with_label (_("Minimize all"));
    gtk_widget_show (menuitem);
    g_signal_connect (menuitem,"activate",G_CALLBACK(_minimize_all_cb),icon);
  }
  return menuitem;
}

static GtkWidget *
task_icon_get_unminimize_all (TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  GtkWidget * menuitem = NULL;
  
  priv = icon->priv;

  if (task_icon_count_tasklist_windows (icon) <=1)
  {
    return NULL;
  }

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( !wnck_window_is_minimized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    menuitem = gtk_image_menu_item_new_with_label (_("Unminimize all"));
    gtk_widget_show (menuitem);
    g_signal_connect (menuitem,"activate",G_CALLBACK(_unminimize_all_cb),icon);
  }
  return menuitem;
}

static GtkWidget *
task_icon_get_maximize_all (TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  GtkWidget * menuitem = NULL;
  
  priv = icon->priv;

  if (task_icon_count_tasklist_windows (icon) <=1)
  {
    return NULL;
  }

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( wnck_window_is_maximized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    menuitem = gtk_image_menu_item_new_with_label (_("Maximize all"));
    gtk_widget_show (menuitem);
    g_signal_connect (menuitem,"activate",G_CALLBACK(_maximize_all_cb),icon);
  }
  return menuitem;
}

static GtkWidget *
task_icon_get_unmaximize_all (TaskIcon * icon)
{
  TaskIconPrivate * priv = NULL;
  GSList * items = task_icon_get_items (icon);
  GSList * iter;
  GtkWidget * menuitem = NULL;
  
  priv = icon->priv;

  if (task_icon_count_tasklist_windows (icon) <=1)
  {
    return NULL;
  }

  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( !wnck_window_is_maximized ( task_window_get_window (TASK_WINDOW(iter->data))))
    {
      continue;
    }
    menuitem = gtk_image_menu_item_new_with_label (_("Unmaximize all"));
    gtk_widget_show (menuitem);
    g_signal_connect (menuitem,"activate",G_CALLBACK(_unmaximize_all_cb),icon);
  }
  return menuitem;
}

static GtkWidget *
task_icon_get_menu_item_maximize (TaskIcon * icon,WnckWindow *win)
{
  GtkWidget * menuitem = NULL;
  GtkWidget * image = NULL;
  if (! wnck_window_is_maximized(win))
  {
    menuitem = gtk_image_menu_item_new_with_mnemonic (_("Ma_ximize"));
    image = gtk_image_new_from_stock (STOCK_MAXIMIZE,GTK_ICON_SIZE_MENU);
  }
  else if (! wnck_window_is_minimized(win))
  {
    menuitem = gtk_image_menu_item_new_with_mnemonic (_("Unma_ximize"));
  }
  if (image)
  {
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menuitem),image);
  }

  if (menuitem)
  {
    gtk_widget_show (menuitem);
    g_signal_connect (menuitem,"activate",
                G_CALLBACK(_maximize_window_cb),
                win);
  }
  return menuitem;
}

static GtkWidget *
task_icon_get_menu_keep_above (TaskIcon * icon,WnckWindow *win)
{
  GtkWidget * menuitem = NULL;

  if (WNCK_WINDOW_STATE_BELOW & wnck_window_get_state (win) )
  {
    return NULL;
  }
  if (! wnck_window_is_minimized(win))
  {
    menuitem = gtk_check_menu_item_new_with_mnemonic (_("Always on _Top"));    
    gtk_widget_show (menuitem);
    if (WNCK_WINDOW_STATE_ABOVE & wnck_window_get_state (win) )
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menuitem),TRUE);
    }
    else
    {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menuitem),FALSE);
    }
    g_signal_connect (menuitem,"activate",
                G_CALLBACK(_keep_above_cb),
                win);
  } 
  return menuitem;
}

static GtkWidget *
task_icon_get_menu_item_minimize (TaskIcon * icon,WnckWindow *win)
{
  GtkWidget * menuitem = NULL;
  GtkWidget * image = NULL;
  if (! wnck_window_is_minimized(win))
  {
    menuitem = gtk_image_menu_item_new_with_mnemonic (_("Mi_nimize"));
    image = gtk_image_new_from_stock (STOCK_MINIMIZE,GTK_ICON_SIZE_MENU);    
  }
  else
  {
    menuitem = gtk_menu_item_new_with_mnemonic (_("Unmi_nimize"));
  }
  if (image)
  {
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem),image);
  }
  gtk_widget_show (menuitem);
  g_signal_connect (menuitem,"activate",
                G_CALLBACK(_minimize_window_cb),
                win);
  
  return menuitem;
}

static GtkWidget *
task_icon_get_menu_item_launch (TaskIcon * icon)
{
  GtkWidget * item;
  GdkPixbuf * launcher_pbuf = NULL;
  gint width;
  gint height;
  item = gtk_image_menu_item_new_with_label (_("Launch"));
  const TaskItem * launcher = task_icon_get_launcher (icon);

  if (!launcher)
  {
    return NULL;
  }
  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU,&width,&height);
  launcher_pbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
                                              "system-run",
                                              height,
                                              GTK_ICON_LOOKUP_FORCE_SIZE,
                                              NULL);  

  if (launcher_pbuf)
  {
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                gtk_image_new_from_pixbuf (launcher_pbuf));
  }
  gtk_widget_show (item);
  g_signal_connect_swapped (item,"activate",
                G_CALLBACK(task_item_middle_click),
                (gpointer)launcher);
  g_object_unref (launcher_pbuf);
  return item;
}

static GtkWidget *
task_icon_get_menu_item_pinned (TaskIcon * icon,WnckWindow * win)
{
  GtkWidget * menuitem = NULL;
  if (! wnck_window_is_pinned(win))
  {  
      menuitem = gtk_menu_item_new_with_label (_("Always on Visible Workspace"));
  }
  if (wnck_window_is_pinned(win))
  {  
      menuitem = gtk_menu_item_new_with_label (_("Only on This Workspace"));
  }
  gtk_widget_show (menuitem);
  g_signal_connect (menuitem,"activate",
                G_CALLBACK(_pin_window_cb),
                win);
  return menuitem;
}

static void
task_icon_get_menu_item_submenu_action_menu_inactives (TaskIcon * icon,GtkMenu * menu)
{

  GtkWidget * menuitem;
  GSList * iter;
  GSList * items = task_icon_get_items (icon);
  const TaskItem * main_item = task_icon_get_main_item (icon);
  
  for (iter = items; iter; iter=iter->next)
  {
    if ( TASK_IS_LAUNCHER (iter->data) )
    {
      continue;
    }
    if ( !task_item_is_visible (iter->data))
    {
      continue;
    }
    if ( iter->data == main_item)
      continue;
    menuitem = task_icon_get_submenu_action_menu (icon, task_window_get_window(iter->data));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
}

static void
task_icon_inline_menu_move_to_workspace (TaskIcon * icon,GtkMenu * menu,WnckWindow * win)
{
  WnckWorkspace * workspace = wnck_window_get_workspace (win);
  gint num_workspaces;
  gint present_workspace;
  gint i;
  GtkWidget *submenu;
  GtkWidget * menuitem =NULL;
  WnckWorkspaceLayout layout;

  num_workspaces = wnck_screen_get_workspace_count (wnck_window_get_screen (win));
  if (num_workspaces == 1  && ( !workspace || ( WNCK_IS_WORKSPACE(workspace) && !wnck_workspace_is_virtual (workspace))))
  {
    return;
  }
  else if (num_workspaces > 1 )
  {
    if (workspace && WNCK_IS_WORKSPACE(workspace))
    {
      present_workspace = wnck_workspace_get_number (workspace);
    }
    else
    {
      present_workspace = -1;
    }

    wnck_screen_calc_workspace_layout (wnck_window_get_screen (win),
                                       num_workspaces,
                                       present_workspace,
                                       &layout);
  }
  else if ( workspace && WNCK_IS_WORKSPACE (workspace) )
  {
    int x,y,w,h;

    wnck_window_get_geometry (win,&x,&y,&w,&h);
    x = x + wnck_workspace_get_viewport_x (workspace);
    y = y + wnck_workspace_get_viewport_y (workspace);
    layout.current_col = present_workspace = x/wnck_screen_get_width (wnck_screen_get_default());
    layout.current_col = present_workspace = x/wnck_screen_get_width (wnck_screen_get_default());
    layout.current_row = present_workspace = y/wnck_screen_get_height (wnck_screen_get_default());
    layout.cols = wnck_workspace_get_width (workspace) / wnck_screen_get_width(wnck_screen_get_default());
    layout.rows = wnck_workspace_get_height (workspace) / wnck_screen_get_height(wnck_screen_get_default());
    num_workspaces = layout.cols * layout.rows;
    present_workspace = layout.current_col + 
                        layout.current_row * 
                        layout.cols;
  }
  else
  {
    return;
  }
  
  if (!wnck_window_is_pinned (win))
    {
      if (layout.current_col > 0) 
        {
          menuitem = gtk_menu_item_new_with_mnemonic (_("Move to Workspace _Left"));
          gtk_menu_shell_append (GTK_MENU_SHELL (menu),menuitem);
          g_signal_connect (G_OBJECT (menuitem), "activate",
                          G_CALLBACK (_move_window_left_cb),
                          win);
          gtk_widget_show (menuitem);          
        }

      
      if ((layout.current_col < layout.cols - 1) && (layout.current_row * layout.cols + (layout.current_col + 1) < num_workspaces ))
        {
          menuitem = gtk_menu_item_new_with_mnemonic (_("Move to Workspace _Right"));
          gtk_menu_shell_append (GTK_MENU_SHELL (menu),menuitem);
          g_signal_connect (G_OBJECT (menuitem), "activate",
                          G_CALLBACK (_move_window_right_cb),
                          win);
          gtk_widget_show (menuitem);          
        }
       
      if (layout.current_row > 0)
        {
          menuitem = gtk_menu_item_new_with_mnemonic (_("Move to Workspace _Up"));
          gtk_menu_shell_append (GTK_MENU_SHELL (menu),menuitem);
          g_signal_connect (G_OBJECT (menuitem), "activate",
                          G_CALLBACK (_move_window_up_cb),
                          win);
          gtk_widget_show (menuitem);          
        }

      if ((layout.current_row < layout.rows - 1) && ((layout.current_row + 1) * layout.cols + layout.current_col) < num_workspaces)
        {
          menuitem = gtk_menu_item_new_with_mnemonic (_("Move to Workspace _Down"));
          gtk_menu_shell_append (GTK_MENU_SHELL (menu),menuitem);
          g_signal_connect (G_OBJECT (menuitem), "activate",
                          G_CALLBACK (_move_window_down_cb),
                          win);
          gtk_widget_show (menuitem);          
        }
    }

  menuitem = gtk_menu_item_new_with_mnemonic (_("Move to Another _Workspace")); 
  gtk_widget_show (menuitem);

  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu),menuitem);

  for (i = 0; i < num_workspaces; i++)
    {
      char *name, *label;
      GtkWidget *item;

      if (workspace && WNCK_IS_WORKSPACE (workspace) && !wnck_workspace_is_virtual (workspace) )
      {
        name = get_workspace_name_with_accel (win, i);
        label = g_strdup_printf ("%s", name);
        g_free (name);
      }
      else
      {
        label = g_strdup_printf ("%d",i+1);
      }
      item = gtk_menu_item_new_with_label (label);
      g_object_set_qdata (G_OBJECT (item), g_quark_from_static_string("WORKSPACE"), GINT_TO_POINTER (i));
      if (i == present_workspace)
        gtk_widget_set_sensitive (item, FALSE);
      g_signal_connect (G_OBJECT (item), "activate",
                      G_CALLBACK (_move_window_to_index),
                      win);
      gtk_widget_show (item);
      gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
      g_free (label);	
    }
}

static void
task_icon_inline_action_simple_menu (TaskIcon * icon, GtkMenu * menu, WnckWindow * win)
{

  GtkWidget * menuitem;


  if ((menuitem = task_icon_get_menu_item_minimize (icon,win)))
  {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_show (menuitem);
  }

  if ((menuitem = task_icon_get_menu_item_maximize (icon,win)))
  {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_show (menuitem);
  }
  
  menuitem = task_icon_get_menu_item_close_active (icon,win);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  gtk_widget_show (menuitem);
}

static GtkWidget *
task_icon_get_submenu_action_simple_menu (TaskIcon * icon, WnckWindow * win)
{
  GtkWidget * submenu;
  GtkWidget * menuitem;
  GtkWidget *menu_label;

  menuitem = gtk_menu_item_new_with_label ( wnck_window_get_name(win));
  menu_label = gtk_bin_get_child(GTK_BIN(menuitem));
  gtk_label_set_max_width_chars (GTK_LABEL(menu_label),MAX_MENU_ITEM_CHARS);
  gtk_label_set_ellipsize (GTK_LABEL(menu_label),PANGO_ELLIPSIZE_MIDDLE);
  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),submenu);
  task_icon_inline_action_simple_menu (icon, GTK_MENU(submenu),win);
  gtk_widget_show_all (submenu);  
  gtk_widget_show_all (menuitem);
  return menuitem;
}


static void
task_icon_inline_action_menu (TaskIcon * icon, GtkMenu * menu, WnckWindow * win)
{

  GtkWidget * menuitem;

  if ((menuitem = task_icon_get_menu_item_minimize (icon,win)))
  {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }

  if ((menuitem = task_icon_get_menu_item_maximize (icon,win)))
  {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }

  //move
  //resize
  menuitem = gtk_separator_menu_item_new ();
  gtk_widget_show (menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  if ( ( menuitem = task_icon_get_menu_keep_above (icon,win)))
  {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);  
  }
  
  if ( ( menuitem = task_icon_get_menu_item_pinned (icon,win)))
  {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);  
  }
  
  task_icon_inline_menu_move_to_workspace (icon,menu,win);
  
  menuitem = gtk_separator_menu_item_new ();
  gtk_widget_show (menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  
  menuitem = task_icon_get_menu_item_close_active (icon,win);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

}

static GtkWidget *
task_icon_get_submenu_action_menu (TaskIcon * icon, WnckWindow * win)
{
  GtkWidget * submenu;
  GtkWidget * menuitem;
  GdkPixbuf * pbuf;
  GtkWidget * image = NULL;
  GtkWidget * menu_label;
  gint width;
  gint height;

  menuitem = gtk_image_menu_item_new_with_label ( wnck_window_get_name(win));
  menu_label = gtk_bin_get_child(GTK_BIN(menuitem));
  gtk_label_set_max_width_chars (GTK_LABEL(menu_label),MAX_MENU_ITEM_CHARS);
  gtk_label_set_ellipsize (GTK_LABEL(menu_label),PANGO_ELLIPSIZE_MIDDLE);
  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU,&width,&height);
  pbuf = wnck_window_get_icon (win);
  g_object_ref (pbuf);
  if (pbuf)
  {
    if (gdk_pixbuf_get_height (pbuf) != height)
    {
      GdkPixbuf *scaled;
      scaled = gdk_pixbuf_scale_simple (pbuf, height, height, GDK_INTERP_BILINEAR);
      g_object_unref (pbuf);
      pbuf = scaled;
    }
    image = gtk_image_new_from_pixbuf (pbuf);
    g_object_unref (pbuf);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menuitem),image);
  }
  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),submenu);

  task_icon_inline_action_menu (icon, GTK_MENU(submenu),win);
  gtk_widget_show_all (menuitem);
  return menuitem;
}

static void
task_icon_inline_action_menu_active (TaskIcon * icon,GtkMenu * menu)
{
  const TaskItem * main_item = task_icon_get_main_item (icon);
  if (main_item && TASK_IS_WINDOW(main_item))
  {  
    WnckWindow *win = task_window_get_window (TASK_WINDOW(main_item));
    task_icon_inline_action_menu (icon,menu,win);
  }
}

static void
task_icon_insert_plugin_menu_items (TaskIcon * icon,GtkMenu * menu)
{
  TaskIconPrivate * priv = TASK_ICON_GET_PRIVATE (icon);
  GList * i;

  for (i=priv->plugin_menu_items;i;i=i->next)
  {
    gtk_widget_show (i->data);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), i->data);
  }
}

typedef enum{
      DBUS_SIGNAL,
      EXTERNAL_COMMAND,
      INTERNAL_ABOUT,
      INTERNAL_ADD_TO_LAUNCHER_LIST,
      INTERNAL_CLOSE_ACTIVE,
      INTERNAL_CLOSE_ALL,
      INTERNAL_CUSTOMIZE_ICON,
      INTERNAL_DOCK_PREFS,
      INTERNAL_INLINE_ACTION_MENU_ACTIVE,
      INTERNAL_INLINE_PLUGINS,
      INTERNAL_INLINE_SUBMENUS_ACTION_MENU_INACTIVES,
      INTERNAL_LAUNCH,
      INTERNAL_MAXIMIZE_ALL,
      INTERNAL_MINIMIZE_ALL,
      INTERNAL_REMOVE_CUSTOMIZED_ICON,
      INTERNAL_REMOVE_FROM_LAUNCHER_LIST,
      INTERNAL_SEPARATOR,
      INTERNAL_SMART_WNCK_MENU,
      INTERNAL_SMART_WNCK_SIMPLE_MENU,
      INTERNAL_UNMAXIMIZE_ALL,
      INTERNAL_UNMINIMIZE_ALL,
      MENU,
      SUBMENU,
      UNKNOWN_ITEM_TYPE
}MenuType;

typedef struct
{
  MenuType  context_menu_item_type;
  const gchar     * value;
}ContextMenuItemType;

static GtkWidget * lastitem = NULL;

const ContextMenuItemType context_menu_item_type_list[] = {
        { DBUS_SIGNAL,"Dbus-Signal"},
        { EXTERNAL_COMMAND,"External-Command"},
        { INTERNAL_ABOUT,"Internal-About"},
        { INTERNAL_ADD_TO_LAUNCHER_LIST,"Internal-Add-To-Launcher-List"},
        { INTERNAL_CLOSE_ACTIVE,"Internal-Close-Active"},
        { INTERNAL_CLOSE_ALL,"Internal-Close-All"},
        { INTERNAL_CUSTOMIZE_ICON,"Internal-Customize-Icon"},
        { INTERNAL_DOCK_PREFS,"Internal-Dock-Prefs"},
        { INTERNAL_INLINE_ACTION_MENU_ACTIVE,"Internal-Inline-Action-Menu-Active"},
        { INTERNAL_INLINE_SUBMENUS_ACTION_MENU_INACTIVES,"Internal-Inline-Submenus-Action-Menu-Inactives"},
        { INTERNAL_INLINE_PLUGINS,"Internal-Inline-Plugins"},
        { INTERNAL_LAUNCH,"Internal-Launch"},
        { INTERNAL_MAXIMIZE_ALL,"Internal-Maximize-All"},
        { INTERNAL_MINIMIZE_ALL,"Internal-Minimize-All"},
        { INTERNAL_REMOVE_CUSTOMIZED_ICON,"Internal-Remove-Customized-Icon"},
        { INTERNAL_REMOVE_FROM_LAUNCHER_LIST,"Internal-Remove-From-Launcher-List"},
        { INTERNAL_SEPARATOR,"Internal-Separator"},
        { INTERNAL_SMART_WNCK_MENU,"Internal-Smart-Wnck-Menu"},
        { INTERNAL_SMART_WNCK_SIMPLE_MENU,"Internal-Smart-Wnck-Simple-Menu"},
        { INTERNAL_UNMAXIMIZE_ALL,"Internal-Unmaximize-All"},
        { INTERNAL_UNMINIMIZE_ALL,"Internal-Unminimize-All"},
        { UNKNOWN_ITEM_TYPE,NULL}
};


#if !GLIB_CHECK_VERSION (2,18,0)
  GtkWidget *_smenu = NULL;
#endif

  /* Called for close tags </foo> */
static void
menu_parse_end_element (GMarkupParseContext *context,
                          const gchar         *element_name,
                          gpointer             user_data,
                          GError             **error)
{
  if (g_strcmp0 (element_name,"submenu")==0)
  {
#if GLIB_CHECK_VERSION (2,18,0)
    g_markup_parse_context_pop (context);
#else
    _smenu = NULL;
#endif
  }
}

  /* Called for character data */
  /* text is not nul-terminated */
static void
menu_parse_text (GMarkupParseContext *context,
                      const gchar         *text,
                      gsize                text_len,  
                      gpointer             user_data,
                      GError             **error)
{
  if (text && text_len && lastitem)
  {
    gchar * s = g_strdup (text);
    s = g_strstrip (s);
    if ( (strlen(s)))
    {      
#if GTK_CHECK_VERSION (2,16,0)
      gtk_menu_item_set_label (GTK_MENU_ITEM (lastitem), text);
#else
    GtkWidget *menu_label = gtk_bin_get_child(GTK_BIN(lastitem));
    gtk_label_set_text(GTK_LABEL(menu_label), text); 
#endif
     

    }
  }
}

/* Called on error, including one set by other
   * methods in the vtable. The GError should not be freed.
   */
static void menu_parse_error (GMarkupParseContext *context,
                              GError              *error,
                              gpointer             user_data)
{
//  g_debug ("%s:",__func__);
}

GMarkupParser sub_markup_parser = {menu_parse_start_element,
                           menu_parse_end_element,
                           menu_parse_text,
                           NULL,
                           menu_parse_error};

static void
menu_parse_start_element (GMarkupParseContext *context,
                                      const gchar         *element_name,
                                      const gchar        **attribute_names,
                                      const gchar        **attribute_values,
                                      gpointer            user_data,
                                      GError             **error)
{
  const gchar ** name_iter = attribute_names;
  const gchar ** value_iter = attribute_values;
  GtkWidget * menuitem = NULL;
  GtkWidget * menu = user_data;
  TaskIcon * icon = NULL;
  TaskIconPrivate * priv = NULL;
  MenuType item_type = UNKNOWN_ITEM_TYPE;
  const gchar * cmd_value = NULL;
  const gchar * icon_value = NULL;
  const gchar * args_value = NULL;
  const gchar * text_value = NULL;
  const gchar * shell_value = NULL;
  const gchar * custom_name = NULL;
  GtkWidget *submenu = NULL;
  AwnApplet * applet = NULL;
  gint height;
  gint width;

#if !GLIB_CHECK_VERSION (2,18,0)
  if (_smenu)
  {
    menu = _smenu;
  }
#endif
  g_return_if_fail (GTK_IS_MENU(menu));
  icon =  g_object_get_qdata (G_OBJECT(menu), g_quark_from_static_string("ICON"));  
  g_assert (TASK_IS_ICON(icon));

  lastitem = NULL;
  g_object_get (icon,
                "applet",&applet,
                NULL);
  priv = icon->priv;
  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU,&width,&height);
  if (g_strcmp0 (element_name,"menuitem")==0)
  {
    for (;*name_iter && *value_iter;name_iter++,value_iter++)
    {
      const gchar * name = *name_iter;
      const gchar * value = *value_iter;
      if (g_strcmp0 (name,"type")==0)
      {
        guint i;
        item_type = UNKNOWN_ITEM_TYPE;
        for (i=0; i < (sizeof(context_menu_item_type_list)/sizeof (ContextMenuItemType));i++)
        {
          const ContextMenuItemType * ctx_item = &context_menu_item_type_list[i];
          if ( g_strcmp0 (value,ctx_item->value)==0)
          {
            item_type = ctx_item->context_menu_item_type;
            break;
          }
        }
      }
      else if (g_strcmp0 (name,"args")==0)
      {
        args_value = value;
      }
      else if (g_strcmp0 (name,"icon")==0)
      {
        icon_value = value;
      }
      else if (g_strcmp0 (name,"cmd")==0)
      {
        cmd_value = value;
      }
      else if (g_strcmp0 (name,"text")==0)
      {
        text_value = value;
      }
      else if (g_strcmp0 (name,"shell")==0)
      {
        shell_value = value;
      }
    }
  }
  else if (g_strcmp0 (element_name,"submenu")==0)
  {
    item_type = SUBMENU;
    for (;*name_iter && *value_iter;name_iter++,value_iter++)
    {
      const gchar * name = *name_iter;
      const gchar * value = *value_iter;
      if (g_strcmp0 (name,"icon")==0)
      {
        icon_value = value;
      }
      else if (g_strcmp0 (name,"text")==0)
      {
        text_value = value;
      }
    }
  }
  else if (g_strcmp0 (element_name,"menu")==0)
  {
    item_type = MENU;
  }
  
  menuitem = NULL;
  switch (item_type)
  {
    case DBUS_SIGNAL:
      g_warning ("%s: stub... plugin support not present",__func__);
      break;
    case EXTERNAL_COMMAND:
      {
        const TaskItem * mainitem = task_icon_get_main_item (icon);
        const TaskItem * launcher = task_icon_get_launcher (icon);
        if (!mainitem || !TASK_IS_WINDOW (mainitem))
        {
          break;
        }     
        GtkWidget * image;
        gchar ** cmd_and_envs = g_malloc ( sizeof(gchar *) * 8);
        gchar * cmd_copy = g_strdup (cmd_value);
        gchar * sh = g_strdup (shell_value?shell_value:"true");

        menuitem = gtk_image_menu_item_new_with_label (text_value);
        g_object_set_qdata_full (G_OBJECT (menuitem),
                                 g_quark_from_static_string("shell_value"),
                                 sh,
                                 (GDestroyNotify) g_free);
        image = gtk_image_new_from_icon_name (icon_value,GTK_ICON_SIZE_MENU);
        if (image)
        {
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menuitem),image);
        }
        g_signal_connect (menuitem,"activate",
                      G_CALLBACK(_spawn_menu_cmd_cb),
                      cmd_and_envs);
        cmd_and_envs[0] = cmd_copy;
        cmd_and_envs[1] = g_strdup_printf("%u",task_window_get_pid (TASK_WINDOW(mainitem)));
        cmd_and_envs[2] = g_strdup_printf("%lx",task_window_get_xid (TASK_WINDOW(mainitem)));
        if (launcher)
        {
          cmd_and_envs[3] = g_strdup(task_launcher_get_exec(launcher));
          cmd_and_envs[4] = g_strdup(task_launcher_get_desktop_path(TASK_LAUNCHER(launcher)));
        }
        else
        {
          cmd_and_envs[3] = g_strdup("");
          cmd_and_envs[4] = g_strdup("");
        }
        cmd_and_envs[5] = g_strdup_printf("%u",getpid());
        cmd_and_envs[6] = g_strdup_printf("%lx", wnck_window_get_group_leader (task_window_get_window(TASK_WINDOW(mainitem))));
        cmd_and_envs[7] = NULL;
        g_object_weak_ref (G_OBJECT(menuitem),(GWeakNotify)g_strfreev,cmd_and_envs);
      }
      gtk_widget_show_all (menuitem);
      break;
    case INTERNAL_ABOUT:
      menuitem = awn_applet_create_about_item (applet,
         "Copyright 2008,2009 Neil Jagdish Patel <njpatel@gmail.com>\n"
         "          2009 Hannes Verschore <hv1989@gmail.com>\n"
         "          2009 Rodney Cryderman <rcryderman@gmail.com>",
         AWN_APPLET_LICENSE_GPLV2,
         VERSION,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
      break;
    case INTERNAL_ADD_TO_LAUNCHER_LIST:
      menuitem = task_icon_get_menu_item_add_to_launcher_list (icon);
      break;
    case INTERNAL_CLOSE_ACTIVE:
      if (TASK_IS_WINDOW (task_icon_get_main_item (icon)))
      {      
        menuitem = task_icon_get_menu_item_close_active (icon,task_window_get_window(TASK_WINDOW(task_icon_get_main_item (icon))));
      }
      break;
    case INTERNAL_CLOSE_ALL:
      menuitem = task_icon_get_menu_item_close_all (icon);
      break;
    case INTERNAL_CUSTOMIZE_ICON:
      custom_name = task_icon_get_custom_name(icon);
      if (custom_name)
      {
        menuitem = awn_themed_icon_create_custom_icon_item (AWN_THEMED_ICON(icon),custom_name);
      }
      break;
    case INTERNAL_DOCK_PREFS:
      menuitem = awn_applet_create_pref_item();
      break;
    case INTERNAL_INLINE_ACTION_MENU_ACTIVE:
      task_icon_inline_action_menu_active (icon,GTK_MENU(menu));
      break;
    case INTERNAL_INLINE_PLUGINS:
      task_icon_insert_plugin_menu_items (icon, GTK_MENU (menu));
      break;
    case INTERNAL_INLINE_SUBMENUS_ACTION_MENU_INACTIVES:
      task_icon_get_menu_item_submenu_action_menu_inactives (icon,GTK_MENU(menu));
      break;
    case INTERNAL_LAUNCH:
      menuitem = task_icon_get_menu_item_launch(icon);
      break;
    case INTERNAL_MAXIMIZE_ALL:
      menuitem = task_icon_get_maximize_all (icon);
      break;
    case INTERNAL_MINIMIZE_ALL:
      menuitem = task_icon_get_minimize_all (icon);
      break;
    case INTERNAL_REMOVE_CUSTOMIZED_ICON:
      menuitem = awn_themed_icon_create_remove_custom_icon_item (AWN_THEMED_ICON(icon),task_icon_get_custom_name(icon));
      break;
    case INTERNAL_REMOVE_FROM_LAUNCHER_LIST:
      menuitem = task_icon_get_menu_item_remove_from_launcher_list (icon);
      break;
    case INTERNAL_SEPARATOR:
      {
        GList * children = gtk_container_get_children (GTK_CONTAINER(menu));
        if (!GTK_IS_SEPARATOR_MENU_ITEM(g_list_last(children)->data))
        {
          menuitem = gtk_separator_menu_item_new();
          gtk_widget_show(menuitem);
        }
      }
      break;
    case INTERNAL_SMART_WNCK_MENU:
      /*TODO move into a function.*/
      if (task_icon_count_tasklist_windows (icon) == 1)
      {
        task_icon_inline_action_menu_active (icon,GTK_MENU(menu));
      }
      else if (task_icon_count_tasklist_windows (icon) > 1)
      {
        GtkWidget * main_item = GTK_WIDGET(priv->main_item);
        GSList * item_list = task_icon_get_items (icon);
        GtkWidget * item;
        GSList * i;

        item = task_icon_get_minimize_all (icon);
        if (item)
        {
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        item = task_icon_get_unminimize_all (icon);
        if (item)
        {
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        item = task_icon_get_maximize_all (icon);
        if (item)
        {
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        item = task_icon_get_unmaximize_all (icon);
        if (item)
        {
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }

        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        item = task_icon_get_submenu_action_menu (icon,task_window_get_window (TASK_WINDOW(main_item)));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        for (i=item_list;i;i=i->next)
        {
          if ( !i->data )
          {
            continue;
          }
          if ( !TASK_IS_WINDOW (i->data) )
          {
            continue;
          }
          if (i->data == main_item)
          {
            continue;
          }
          item = task_icon_get_submenu_action_menu (icon, task_window_get_window (TASK_WINDOW(i->data)));
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        item = task_icon_get_menu_item_close_all (icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }
      break;
    case INTERNAL_SMART_WNCK_SIMPLE_MENU:
      /*TODO move into a function.*/
      if (task_icon_count_tasklist_windows (icon) == 1 && TASK_IS_WINDOW(priv->main_item))
      {
        task_icon_inline_action_simple_menu (icon, GTK_MENU(menu), task_window_get_window (TASK_WINDOW(priv->main_item)));
      }
      else if (task_icon_count_tasklist_windows (icon) > 1)
      {
        GtkWidget * main_item = GTK_WIDGET(priv->main_item);
        GSList * item_list = task_icon_get_items (icon);
        GtkWidget * sub;
        GSList * i;
        main_item = task_icon_get_minimize_all (icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), main_item);
        main_item = task_icon_get_maximize_all (icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), main_item);
        main_item = gtk_separator_menu_item_new();
        gtk_widget_show(main_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), main_item);
        
        if (main_item && TASK_IS_WINDOW (main_item))
        {
          sub = task_icon_get_submenu_action_simple_menu (icon, task_window_get_window (TASK_WINDOW(main_item)));
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), sub);
        }
        for (i=item_list;i;i=i->next)
        {
          if ( !i->data )
          {
            continue;
          }
          if ( !TASK_IS_WINDOW (i->data) )
          {
            continue;
          }
          if (i->data == main_item)
          {
            continue;
          }
          sub = task_icon_get_submenu_action_simple_menu (TASK_ICON(icon), task_window_get_window (TASK_WINDOW(i->data)));
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), sub);
        }
        main_item = gtk_separator_menu_item_new();
        gtk_widget_show(main_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), main_item);
        main_item = task_icon_get_menu_item_close_all (icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), main_item);
      }        
      break;
    case INTERNAL_UNMAXIMIZE_ALL:
      menuitem = task_icon_get_unmaximize_all (icon);
      break;
    case INTERNAL_UNMINIMIZE_ALL:
      menuitem = task_icon_get_unminimize_all (icon);
      break;
    case MENU:
      break;
    case SUBMENU:
#if GLIB_CHECK_VERSION (2,18,0)
      menuitem = gtk_image_menu_item_new_with_label ( text_value?text_value:"");
      submenu = gtk_menu_new ();
      g_object_set_qdata (G_OBJECT(submenu), g_quark_from_static_string("ICON"),icon);  
      gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),submenu);
      gtk_widget_show_all (menuitem);
      g_markup_parse_context_push (context,&sub_markup_parser,submenu);
#else
      if (_smenu)
      {
        g_critical ("%s: Error parsing context menu xml. Only one submenu level is allowed with GLIB < 2.18.0",__func__);
      }
      menuitem = gtk_image_menu_item_new_with_label ( text_value?text_value:"");
      submenu = gtk_menu_new ();
      g_object_set_qdata (G_OBJECT(submenu), g_quark_from_static_string("ICON"),icon);  
      gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),submenu);
      gtk_widget_show_all (menuitem);
      _smenu = submenu;
#endif
      break;
    case UNKNOWN_ITEM_TYPE:
      {
        gint line_number;
        gint char_number;
        g_markup_parse_context_get_position (context,&line_number,&char_number);
        g_warning ("%s: Unknown item type, element_name = %s, line = %d",__func__, element_name,line_number);
      }
      break;
    default:
      g_assert_not_reached();
      break;
  }
  if (menuitem && GTK_IS_WIDGET(menuitem))
  {
    if (icon_value)
    {
      GtkWidget * image;
      image = gtk_image_new_from_icon_name (icon_value,GTK_ICON_SIZE_MENU);
      if (image)
      {
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menuitem),image);
      }
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
  lastitem = menuitem;  
}

GtkWidget *
task_icon_build_context_menu(TaskIcon * icon)
{
  GError * err=NULL;
  gchar * contents=NULL;
  static gboolean done_once = FALSE;
  GMarkupParseContext * markup_parser_context = NULL;
  GMarkupParser markup_parser = {menu_parse_start_element,
                                 menu_parse_end_element,
                               menu_parse_text,
                               NULL,
                               menu_parse_error};
  gchar * base_menu_filename = NULL;  
  gchar * menu_filename = NULL;
  GtkWidget * menu = gtk_menu_new();

  if (!done_once)
  {
    /*
     get wnck to prim the icon factory with it's stock icons
     Use for now.. maybe replace min,max.close with custom icons.
     */
    WnckWindow *win = wnck_screen_get_active_window (wnck_screen_get_default() );
    if (win)
    {
      GtkWidget * wnck_menu = wnck_action_menu_new ( win);
      gtk_widget_destroy (wnck_menu);
      done_once = TRUE;
    }
  }
  
  g_object_set_qdata (G_OBJECT(menu), g_quark_from_static_string("ICON"),icon);
  
  gtk_widget_show_all (menu);
  g_object_get (icon,
                "menu_filename",&base_menu_filename,
                NULL);
  if (g_path_is_absolute (base_menu_filename) )
  {
    menu_filename = g_strdup (base_menu_filename);
  }
  else
  {
    /* FIXME  FIXME  FIXME  FIXME  FIXME  FIXME  FIXME  FIXME  FIXME */
    menu_filename = g_strdup_printf ("%s/taskmanager/menus/%s",APPLETDATADIR,base_menu_filename);
//    menu_filename = g_strdup_printf ("/usr/local/share/avant-window-navigator/applets/taskmanager/menus/%s",base_menu_filename);
  }
  g_free (base_menu_filename);
  if ( g_file_get_contents (menu_filename,&contents,NULL,&err))
  {
    markup_parser_context = g_markup_parse_context_new (&markup_parser,0,menu,(GDestroyNotify) NULL);
  }
  if (err)
  {
    g_warning ("%s: error loading menu file %s.  %s",__func__,menu_filename,err->message);
    g_error_free (err);
    err=NULL;
    g_warning ("%s: Attempting to load standard.xml",__func__);
    menu_filename = g_strdup_printf ("%s/taskmanager/menus/standard.xml",APPLETDATADIR);
    if ( g_file_get_contents (menu_filename,&contents,NULL,&err))
    {
      markup_parser_context = g_markup_parse_context_new (&markup_parser,0,menu,(GDestroyNotify) NULL);
    }
    if (err)
    {
      g_warning ("%s: error loading menu file %s.  %s",__func__,menu_filename,err->message);
      g_error_free (err);
      err=NULL;
      return menu; //return empty menu.
    }
  }
  if (markup_parser_context)
  {
    g_markup_parse_context_parse (markup_parser_context,contents,strlen (contents),&err);
    if (err)
    {
      g_message ("%s: error parsing menu file %s.  %s",__func__,menu_filename,err->message);
      g_error_free (err);
      err=NULL;
    }
    g_markup_parse_context_free (markup_parser_context);
  }
  
  g_free (menu_filename);

  GList * children = gtk_container_get_children (GTK_CONTAINER(menu));
  if (GTK_IS_SEPARATOR_MENU_ITEM(g_list_last(children)->data))
  {
    gtk_widget_hide (GTK_WIDGET(g_list_last(children)->data));
  }
  if (GTK_IS_SEPARATOR_MENU_ITEM(g_list_first(children)->data))
  {
    gtk_widget_hide (GTK_WIDGET(g_list_first(children)->data));
  }  
  return menu;
}
