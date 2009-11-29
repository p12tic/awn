/*
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
 * Copyright (C) 2001 Havoc Pennington
 *
 * Parts of action menu derived from the wnck action menu code in 
 * the window-action-menu.c file in libwnck sources.
 *
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

#include "config.h"

static void menu_parse_start_element (GMarkupParseContext *context,
                                      const gchar         *element_name,
                                      const gchar        **attribute_names,
                                      const gchar        **attribute_values,
                                      gpointer            user_data,
                                      GError             **error);

static char *
get_workspace_name_with_accel (WnckWindow *window, int idx)
{
  const char *name;
  int number;
 
  name = wnck_workspace_get_name (wnck_screen_get_workspace (wnck_window_get_screen (window),idx));

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
    g_object_get (icon,
                  "applet",&applet,
                  NULL);
    task_manager_remove_task_icon (TASK_MANAGER(applet), GTK_WIDGET(icon));               
    task_manager_append_launcher (TASK_MANAGER(applet),
                                  task_launcher_get_desktop_path(launcher));
  }
}


static void
_move_window_left_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;
  workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen (win),
                                                  wnck_window_get_workspace (win), 
                                                  WNCK_MOTION_LEFT);
  wnck_window_move_to_workspace (win, workspace);
}

static void
_move_window_right_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;
  workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen ( win),
                                                  wnck_window_get_workspace (win),
                                                  WNCK_MOTION_RIGHT);
  wnck_window_move_to_workspace (win, workspace);
}

static void
_move_window_up_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;
  workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen (win),
                                                  wnck_window_get_workspace (win),
                                                  WNCK_MOTION_UP);
  wnck_window_move_to_workspace (win, workspace);
}

static void
_move_window_down_cb (GtkMenuItem *menuitem, WnckWindow * win)
{
  WnckWorkspace *workspace;
  workspace = wnck_screen_get_workspace_neighbor (wnck_window_get_screen (win),
                                                  wnck_window_get_workspace (win),
                                                  WNCK_MOTION_DOWN);
  wnck_window_move_to_workspace (win, workspace);
}

static void
_move_window_to_index (GtkMenuItem *menuitem, WnckWindow * win)
{
  int workspace_index;
  workspace_index =  GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT(menuitem), g_quark_from_static_string("WORKSPACE")));  
//  workspace_index = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "workspace"));

  wnck_window_move_to_workspace (win,
                                 wnck_screen_get_workspace (wnck_window_get_screen (win),
                                 workspace_index));
}

static void
_close_window_cb (GtkMenuItem *menuitem, TaskIcon * icon)
{
  /*
   This might actually be a key event?  but it doesn't matter... the time
   field is in the same place*/
  GdkEventButton * event = (GdkEventButton*)gtk_get_current_event ();

  g_return_if_fail (event);
  if (TASK_IS_WINDOW (task_icon_get_main_item (icon)))
  {
    wnck_window_close (task_window_get_window (TASK_WINDOW(task_icon_get_main_item (icon))),event->time);
  }
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
  if (wnck_window_is_maximized (win))
    wnck_window_unmaximize (win);
  else
    wnck_window_maximize (win);
}
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
  if (wnck_window_is_pinned (win))
    wnck_window_unpin (win);
  else
    wnck_window_pin (win);
}

#if 0
static void
_spawn_menu_cmd_cb (GtkMenuItem *menuitem, GStrv cmd_and_envs)
{
  g_debug ("%s:  cmd_env_env = %p",__func__,cmd_and_envs);
  for (int i=0; i<5;i++)
  {
    g_debug ("%s: [%d] = %s",__func__,i,cmd_and_envs[i]);
  }
  g_setenv ("AWN_TASK_MENU_CMD",cmd_and_envs[0],TRUE);
  g_setenv ("AWN_TASK_PID",cmd_and_envs[1],TRUE);
  g_setenv ("AWN_TASK_XID",cmd_and_envs[2],TRUE);
  g_setenv ("AWN_TASK_EXEC",cmd_and_envs[3],TRUE);
  g_setenv ("AWN_TASK_DESKTOP",cmd_and_envs[4],TRUE);

  //Want access to shell variables...
  if (system (cmd_and_envs[0]) ==-1)
  {
    g_message ("%s: error spawning '%s'",__func__,cmd_and_envs[0]);
  }
}
#endif


#if 0
#define menu_item_type tokens[0]
#define menu_item_name tokens[1]
#define menu_item_cmd tokens[1]
#define menu_item_level tokens[2]
#define menu_item_text tokens[3]
#define submenu_level_name tokens[4]
#define menu_item_icon_name tokens[5]
#define reserved2 tokens[6]

static void
task_icon_fill_context_menu (TaskIcon *icon, GtkWidget *menu,const gchar * menu_level)
{
  TaskIconPrivate *priv;
  GtkWidget   *item;
  TaskItem * launcher = NULL;
  GdkPixbuf * launcher_pbuf=NULL;
  gint width,height;
  GStrv tokens;
  GValueArray * menu_desc;
  
  g_return_if_fail (TASK_IS_ICON (icon));

  priv = icon->priv;

  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU,&width,&height);
  launcher_pbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
                                              "system-run",
                                              height,
                                              GTK_ICON_LOOKUP_FORCE_SIZE,
                                              NULL);  
  launcher = task_icon_get_launcher (icon);

  for (guint idx = 0; idx < menu_desc->n_values; idx++)
  {
    gchar * item_desc;
    item_desc = g_value_dup_string (g_value_array_get_nth (menu_desc, idx));
    tokens = g_strsplit (item_desc,"::",-1);
    if (g_strv_length (tokens) != 7)
    {
      g_debug ("%s: menu item description invalid '%s'",__func__,item_desc);
      g_strfreev (tokens);
      g_free (item_desc);
      continue;
    }
    g_debug ("%s: %s",__func__,item_desc);
    g_debug ("menu_item_type %s",menu_item_type);
    if (g_strcmp0 (menu_item_type,"Internal")==0  )
    {
      g_debug ("menu item_name %s",menu_item_name);
      if (g_strcmp0(menu_item_level,menu_level)!=0)
      {
        continue;
      }
/*      
      if (g_strcmp0 (menu_item_name,"ABOUT")==0)
      {
        item = awn_applet_create_about_item (priv->applet,                
           "Copyright 2008,2009 Neil Jagdish Patel <njpatel@gmail.com>\n"
           "          2009 Hannes Verschore <hv1989@gmail.com>\n"
           "          2009 Rodney Cryderman <rcryderman@gmail.com>",
           AWN_APPLET_LICENSE_GPLV2,
           NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }
       */
/*      else  if (g_strcmp0 (menu_item_name,"ADD_LAUNCHER")==0)
      {
          item = gtk_menu_item_new_with_label (_("Add to Launcher List"));
          gtk_widget_show (item);
          g_signal_connect (item,"activate",
                            G_CALLBACK(add_to_launcher_list_cb),
                            icon);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }*/
/*      else  if (g_strcmp0 (menu_item_name,"CLOSE")==0)
      {
        item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE,NULL);
        gtk_widget_show (item);
        g_signal_connect (item,"activate",
                      G_CALLBACK(_close_window_cb),
                      icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }*/
      else  if (g_strcmp0 (menu_item_name,"CLOSEALL")==0)
      {
        g_debug ("CLOSEALL not implemented");
      }
/*      else  if (g_strcmp0 (menu_item_name,"CUSTOMIZE_ICON")==0)
      {
        item = awn_themed_icon_create_custom_icon_item (AWN_THEMED_ICON(icon),
                                                        priv->custom_name);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }*/
/*      else  if (g_strcmp0 (menu_item_name,"DOCK_PREFS")==0)
      {
        item = awn_applet_create_pref_item();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }
       */
      /*else  if (g_strcmp0 (menu_item_name,"LAUNCH")==0)
      {
        item = gtk_image_menu_item_new_with_label (_("Launch"));
        if (launcher_pbuf)
        {
          gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                      gtk_image_new_from_pixbuf (launcher_pbuf));
        }
        gtk_widget_show (item);
        g_signal_connect_swapped (item,"activate",
                      G_CALLBACK(task_item_middle_click),
                      launcher);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }*/
      else  if (g_strcmp0 (menu_item_name,"MAXIMIZE")==0)
      {
        item = gtk_menu_item_new_with_label (_("Maximize"));
        gtk_widget_show (item);
        g_signal_connect (item,"activate",
                      G_CALLBACK(_maximize_window_cb),
                      icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }      
      else  if (g_strcmp0 (menu_item_name,"MINIMIZE")==0)
      {
        item = gtk_menu_item_new_with_label (_("Minimize"));
        gtk_widget_show (item);
        g_signal_connect (item,"activate",
                      G_CALLBACK(_minimize_window_cb),
                      icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }
      else  if (g_strcmp0 (menu_item_name,"PIN")==0)
      {
        item = gtk_menu_item_new_with_label (_("Maximize"));
        gtk_widget_show (item);
        g_signal_connect (item,"activate",
                      G_CALLBACK(_pin_window_cb),
                      icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }      
/*      else  if (g_strcmp0 (menu_item_name,"REMOVE_CUSTOMIZED_ICON")==0)
      {
        item = awn_themed_icon_create_remove_custom_icon_item (AWN_THEMED_ICON(icon),
                                                        priv->custom_name);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }*/
/*      else  if (g_strcmp0 (menu_item_name,"SEPARATOR")==0)
      {
        g_debug ("add separator");
        item = gtk_separator_menu_item_new();
        gtk_widget_show(item);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }*/
      else  if (g_strcmp0 (menu_item_name,"SHADE")==0)
      {
        item = gtk_menu_item_new_with_label (_("Shade"));
        gtk_widget_show (item);
        g_signal_connect (item,"activate",
                      G_CALLBACK(_shade_window_cb),
                      icon);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }      
      else  if (g_strcmp0 (menu_item_name,"WINDOW_LIST")==0)
      {
        GtkWidget *sub_menu = gtk_menu_new ();
        item = gtk_menu_item_new_with_label ("Window List");
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(item),sub_menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        gtk_widget_show (item);
        gtk_widget_show (sub_menu);
        task_icon_fill_context_menu (icon,sub_menu,submenu_level_name);
      }
      else  if (g_strcmp0 (menu_item_name,"WNCK_ACTION_MENU_ACTIVE")==0)
      {
        if (priv->main_item && TASK_IS_WINDOW(priv->main_item))
        {
          GtkWidget * action_menu = wnck_action_menu_new (task_window_get_window (TASK_WINDOW(priv->main_item)));
          GList * children = gtk_container_get_children (GTK_CONTAINER(action_menu));
          GList * iter;
          for (iter = children; iter; )
          {
            GList * next = g_list_next (iter);
            g_object_ref (iter->data);
            gtk_container_remove (GTK_CONTAINER(action_menu),iter->data);
            gtk_menu_shell_append (GTK_MENU_SHELL(menu),iter->data);
            g_object_unref (iter->data);
            iter = next;
          }
        }
      }
      else  if (g_strcmp0 (menu_item_name,"WNCK_ACTION_SUBMENU_ACTIVE")==0)
      {
        if (priv->main_item && TASK_IS_WINDOW(priv->main_item))
        {
          GtkWidget   *sub_menu;          
          item = gtk_menu_item_new_with_label (task_window_get_name (TASK_WINDOW(priv->main_item)));
          sub_menu = wnck_action_menu_new (task_window_get_window (TASK_WINDOW(priv->main_item)));          
          gtk_menu_item_set_submenu (GTK_MENU_ITEM(item),sub_menu);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
          gtk_widget_show (item);
          gtk_widget_show (sub_menu);
        }
      }      
/*      else  if (g_strcmp0 (menu_item_name,"WNCK_ACTION_MENU_INACTIVES")==0)
      {
        GSList * iter;
        for (iter = priv->items; iter; iter=iter->next)
        {
          GtkWidget   *sub_menu;
          if ( TASK_IS_LAUNCHER (iter->data) )
          {
            continue;
          }
          if ( !task_item_is_visible (iter->data))
          {
            continue;
          }
          if ( iter->data == priv->main_item)
            continue;
          item = gtk_menu_item_new_with_label (task_window_get_name (TASK_WINDOW(iter->data)));
          sub_menu = wnck_action_menu_new (task_window_get_window (TASK_WINDOW(iter->data)));          
          gtk_menu_item_set_submenu (GTK_MENU_ITEM(item),sub_menu);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
          gtk_widget_show (item);
          gtk_widget_show (sub_menu);
        }

      }*/
      else
      {
        g_debug ("%s:  Unrecognized menu item name %s",__func__,menu_item_type);
      }
    }
    if (g_strcmp0 (menu_item_type,"External")==0  )
    {
      /*assumes the command somewhat valid... don't try to determine what
       the bin is and search the path*/
      if (menu_item_cmd && strlen (menu_item_cmd))
      {
        GtkWidget * image;
        gchar ** cmd_and_envs = g_malloc ( sizeof(gchar *) * 6);
        gchar * cmd_copy = g_strdup (menu_item_cmd);
        item = gtk_image_menu_item_new_with_label (menu_item_text);
        image = gtk_image_new_from_icon_name (menu_item_icon_name,height);
        if (image)
        {
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item),image);
        }
        gtk_widget_show (item);
        g_debug ("%s:  cmd_env_env = %p",__func__,cmd_and_envs);        
        g_signal_connect (item,"activate",
                      G_CALLBACK(_spawn_menu_cmd_cb),
                      cmd_and_envs);
        g_debug ("%s: cmd = %s",__func__,menu_item_cmd);
        cmd_and_envs[0] = cmd_copy;
        cmd_and_envs[1] = g_strdup_printf("%u",task_window_get_pid (TASK_WINDOW(priv->main_item)));
        cmd_and_envs[2] = g_strdup_printf("%lx",task_window_get_xid (TASK_WINDOW(priv->main_item)));
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
        cmd_and_envs[5] = NULL;
        g_object_weak_ref (G_OBJECT(item),(GWeakNotify)g_strfreev,cmd_and_envs);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      }
    }
    g_free (item_desc);
  }

}
#endif

static GtkWidget *
task_icon_get_menu_item_add_to_launcher_list (TaskIcon * icon)
{
  GtkWidget * item;

  g_return_val_if_fail (TASK_IS_ICON (icon),NULL);  
  TaskItem * launcher = task_icon_get_launcher (icon);
  if (!launcher || !task_icon_count_ephemeral_items (icon) )
  {
    return NULL;
  }
  item = gtk_menu_item_new_with_label (_("Add to Launcher List"));
  gtk_widget_show (item);
  g_signal_connect (item,"activate",
                    G_CALLBACK(add_to_launcher_list_cb),
                    icon);
  return item;
}

static GtkWidget *
task_icon_get_menu_item_close_active (TaskIcon * icon)
{
  GtkWidget * item;
  const TaskItem * main_item = task_icon_get_main_item (icon);
  if (!main_item || !TASK_IS_WINDOW (main_item) )
  {
    return NULL;
  }  
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE,NULL);
  gtk_widget_show (item);
  g_signal_connect (item,"activate",
                G_CALLBACK(_close_window_cb),
                icon);
  return item;
}

static GtkWidget *
task_icon_get_menu_item_launch (TaskIcon * icon)
{
  GtkWidget * item;
  GdkPixbuf * launcher_pbuf = NULL;
  gint width;
  gint height;
  item = gtk_image_menu_item_new_with_label (_("Launch"));
  TaskItem * launcher = task_icon_get_launcher (icon);

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
                launcher);
  g_object_unref (launcher_pbuf);
  return item;
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
    GtkWidget   *sub_menu;
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
    menuitem = gtk_menu_item_new_with_label (task_window_get_name (TASK_WINDOW(iter->data)));
    sub_menu = wnck_action_menu_new (task_window_get_window (TASK_WINDOW(iter->data)));          
    gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),sub_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    gtk_widget_show (menuitem);
    gtk_widget_show (sub_menu);
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
  GtkWidget *separator;
  GtkWidget * menuitem =NULL;
  WnckWorkspaceLayout layout;

  num_workspaces = wnck_screen_get_workspace_count (wnck_window_get_screen (win));
  if (num_workspaces == 1)
  {
    return;
  }

  if (workspace)
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

  separator = gtk_separator_menu_item_new ();
  gtk_widget_show (separator);          
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),separator);
  
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
	
      name = get_workspace_name_with_accel (win, i);
      label = g_strdup_printf ("%s", name);

      item = gtk_menu_item_new_with_label (label);
//      g_object_set_data (G_OBJECT (item), "workspace", GINT_TO_POINTER (i));
      g_object_set_qdata (G_OBJECT (item), g_quark_from_static_string("WORKSPACE"), GINT_TO_POINTER (i));
      if (i == present_workspace)
        gtk_widget_set_sensitive (item, FALSE);
      g_signal_connect (G_OBJECT (item), "activate",
                      G_CALLBACK (_move_window_to_index),
                      win);
      gtk_widget_show (item);
      gtk_menu_shell_append (GTK_MENU_SHELL (submenu), item);
      g_free (name);
      g_free (label);	
    }
}

static void
task_icon_inline_action_menu (TaskIcon * icon, GtkMenu * menu, WnckWindow * win)
{

  GtkWidget * menuitem;
  
  if (! wnck_window_is_maximized(win))
  {
    menuitem = gtk_menu_item_new_with_label (_("Maximize"));
    gtk_widget_show (menuitem);
    g_signal_connect (menuitem,"activate",
                  G_CALLBACK(_maximize_window_cb),
                  win);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
  if (! wnck_window_is_minimized(win))
  {  
    menuitem = gtk_menu_item_new_with_label (_("Minimize"));
    gtk_widget_show (menuitem);
    g_signal_connect (menuitem,"activate",
                  G_CALLBACK(_minimize_window_cb),
                  win);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }

  menuitem = gtk_separator_menu_item_new ();
  gtk_widget_show (menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  if (! wnck_window_is_pinned(win))
  {  
      menuitem = gtk_menu_item_new_with_label (_("Always on Visible Workspace"));
      gtk_widget_show (menuitem);
      g_signal_connect (menuitem,"activate",
                    G_CALLBACK(_pin_window_cb),
                    win);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
  if (wnck_window_is_pinned(win))
  {  
      menuitem = gtk_menu_item_new_with_label (_("Only on This Workspace"));
      gtk_widget_show (menuitem);
      g_signal_connect (menuitem,"activate",
                    G_CALLBACK(_pin_window_cb),
                    win);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }

  task_icon_inline_menu_move_to_workspace (icon,menu,win);
  
  menuitem = gtk_separator_menu_item_new ();
  gtk_widget_show (menuitem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  
  menuitem = task_icon_get_menu_item_close_active (icon);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

}

static void
task_icon_inline_action_menu_active (TaskIcon * icon,GtkMenu * menu)
{
/* TODO As I feared reparenting the action menu items is unstable.
   minimize,unmaximize,move,resize,always on top,always on visible workspace,
   only on this workspace,move to ws right/left, move to ws up/down,move to another workspace,
   close
*/
  const TaskItem * main_item = task_icon_get_main_item (icon);
  if (main_item && TASK_IS_WINDOW(main_item))
  {  
    WnckWindow *win = task_window_get_window (TASK_WINDOW(main_item));
    task_icon_inline_action_menu (icon,menu,win);
  }
}

typedef enum{
      INTERNAL_ABOUT,
      INTERNAL_ADD_TO_LAUNCHER_LIST,
      INTERNAL_CLOSE_ACTIVE,
      INTERNAL_CUSTOMIZE_ICON,
      INTERNAL_DOCK_PREFS,
      INTERNAL_LAUNCH,
      INTERNAL_REMOVE_CUSTOMIZED_ICON,
      INTERNAL_SEPARATOR,
      INTERNAL_SUBMENU,
      INTERNAL_INLINE_ACTION_MENU_ACTIVE,
      INTERNAL_INLINE_SUBMENUS_ACTION_MENU_INACTIVES,
      UNKNOWN
}MenuType;


  /* Called for close tags </foo> */
static void
menu_parse_end_element (GMarkupParseContext *context,
                          const gchar         *element_name,
                          gpointer             user_data,
                          GError             **error)
{
//  g_debug ("%s: %s",__func__,element_name);
  if (g_strcmp0 (element_name,"submenu")==0)
  {
    g_markup_parse_context_pop (context);
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
//  g_debug ("%s: text = %s",__func__,text);
}

/* Called on error, including one set by other
   * methods in the vtable. The GError should not be freed.
   */
static void menu_parse_error (GMarkupParseContext *context,
                              GError              *error,
                              gpointer             user_data)
{
  g_debug ("%s:",__func__);
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
//  g_debug ("%s: element name = %s",__func__,element_name);
  const gchar ** name_iter = attribute_names;
  const gchar ** value_iter = attribute_values;
  GtkWidget * menuitem = NULL;
  GtkWidget * menu = user_data;
  TaskIcon * icon = NULL;
  TaskIconPrivate * priv = NULL;
  MenuType item_type = UNKNOWN;
  const gchar * type_value = NULL;
  const gchar * display_value = NULL;
  GtkWidget *submenu = NULL;
  AwnApplet * applet = NULL;
  
  g_return_if_fail (GTK_IS_MENU(menu));
  icon =  g_object_get_qdata (G_OBJECT(menu), g_quark_from_static_string("ICON"));  
  g_assert (TASK_IS_ICON(icon));
  g_object_get (icon,
                "applet",&applet,
                NULL);
  priv = icon->priv;
  if (g_strcmp0 (element_name,"menuitem")==0)
  {
    for (;*name_iter && *value_iter;name_iter++,value_iter++)
    {
      const gchar * name = *name_iter;
      const gchar * value = *value_iter;
  //    g_debug ("%s: %s -> %s ",__func__,*name_iter,*value_iter);
      if (g_strcmp0 (name,"type")==0)
      {
        if (g_strcmp0 (value,"Internal-About")==0)
        {
          item_type = INTERNAL_ABOUT;
        }
        else  if (g_strcmp0 (value,"Internal-Add-To-Launcher-List")==0)
        {
          item_type = INTERNAL_ADD_TO_LAUNCHER_LIST;
        }
        else  if (g_strcmp0 (value,"Internal-Close-Active")==0)
        {
          item_type = INTERNAL_CLOSE_ACTIVE;
        }
        else  if (g_strcmp0 (value,"Internal-Customize-Icon")==0)
        {
          item_type = INTERNAL_CUSTOMIZE_ICON;
        }
        else if (g_strcmp0 (value, "Internal-Dock-Prefs")==0)
        {
          item_type = INTERNAL_DOCK_PREFS;
        }
        else if (g_strcmp0 (value, "Internal-Launch")==0)
        {
          item_type = INTERNAL_LAUNCH;
        }
        else  if (g_strcmp0 (value,"Internal-Remove-Customized-Icon")==0)
        {
          item_type = INTERNAL_REMOVE_CUSTOMIZED_ICON;
        }
        else  if (g_strcmp0 (value,"Internal-Separator")==0)
        {
          item_type = INTERNAL_SEPARATOR;
        }
        else  if (g_strcmp0 (value,"Internal-Inline-Submenus-Action-Menu-Inactives")==0)
        {
          item_type = INTERNAL_INLINE_SUBMENUS_ACTION_MENU_INACTIVES;
          break;
        }
        else  if (g_strcmp0 (value,"Internal-Inline-Action-Menu-Active")==0)
        {
          item_type = INTERNAL_INLINE_ACTION_MENU_ACTIVE;
          break;
        }
        else
        {
          type_value = value;
          item_type = UNKNOWN;
        }
      }
    }
  }
  else if (g_strcmp0 (element_name,"submenu")==0)
  {
    item_type = INTERNAL_SUBMENU;
    for (;*name_iter && *value_iter;name_iter++,value_iter++)
    {
      const gchar * name = *name_iter;
      const gchar * value = *value_iter;
      if (g_strcmp0 (name,"display")==0)
      {
        display_value = value;
      }
    }
  }

  menuitem = NULL;
  switch (item_type)
  {
    case INTERNAL_ABOUT:
      menuitem = awn_applet_create_about_item (applet,
         "Copyright 2008,2009 Neil Jagdish Patel <njpatel@gmail.com>\n"
         "          2009 Hannes Verschore <hv1989@gmail.com>\n"
         "          2009 Rodney Cryderman <rcryderman@gmail.com>",
         AWN_APPLET_LICENSE_GPLV2,
         NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
      break;
    case INTERNAL_ADD_TO_LAUNCHER_LIST:
      menuitem = task_icon_get_menu_item_add_to_launcher_list (icon);
      break;
    case INTERNAL_CLOSE_ACTIVE:
      menuitem = task_icon_get_menu_item_close_active (icon);
      break;
    case INTERNAL_CUSTOMIZE_ICON:
      menuitem = awn_themed_icon_create_custom_icon_item (AWN_THEMED_ICON(icon),task_icon_get_custom_name(icon));
      break;
    case INTERNAL_DOCK_PREFS:
      menuitem = awn_applet_create_pref_item();
      break;
    case INTERNAL_LAUNCH:
      menuitem = task_icon_get_menu_item_launch(icon);
      break;
    case INTERNAL_REMOVE_CUSTOMIZED_ICON:
      menuitem = awn_themed_icon_create_remove_custom_icon_item (AWN_THEMED_ICON(icon),task_icon_get_custom_name(icon));
      break;
    case INTERNAL_SEPARATOR:
      menuitem = gtk_separator_menu_item_new();
      gtk_widget_show(menuitem);
      break;
    case INTERNAL_SUBMENU:
      menuitem = gtk_image_menu_item_new_with_label ( display_value?display_value:"");
      submenu = gtk_menu_new ();
      g_object_set_qdata (G_OBJECT(submenu), g_quark_from_static_string("ICON"),icon);  
      gtk_menu_item_set_submenu (GTK_MENU_ITEM(menuitem),submenu);
      gtk_widget_show_all (menuitem);
      g_markup_parse_context_push (context,&sub_markup_parser,submenu);
      break;
    case INTERNAL_INLINE_ACTION_MENU_ACTIVE:
      task_icon_inline_action_menu_active (icon,GTK_MENU(menu));
      break;
    case INTERNAL_INLINE_SUBMENUS_ACTION_MENU_INACTIVES:
      task_icon_get_menu_item_submenu_action_menu_inactives (icon,GTK_MENU(menu));
      break;
    case UNKNOWN:
      g_debug ("%s: Unknown type value of %s",__func__,type_value);
      break;
  }
  if (menuitem && GTK_IS_WIDGET(menuitem))
  {
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  }
}

GtkWidget *
task_icon_build_context_menu(TaskIcon * icon)
{
  GError * err=NULL;
  gchar * contents=NULL;  
  GMarkupParseContext * markup_parser_context = NULL;
  GMarkupParser markup_parser = {menu_parse_start_element,
                                 menu_parse_end_element,
                               menu_parse_text,
                               NULL,
                               menu_parse_error};
  gchar * base_menu_filename = NULL;  
  gchar * menu_filename = NULL;
  GtkWidget * menu = gtk_menu_new();
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
//    menu_filename = g_strdup_printf ("%s/taskmanager/menus/%s",APPLETDATADIR,priv->menu_filename);
    menu_filename = g_strdup_printf ("/usr/local/share/avant-window-navigator/applets/taskmanager/menus/%s",base_menu_filename);
  }
  g_free (base_menu_filename);
  if ( g_file_get_contents (menu_filename,&contents,NULL,&err))
  {
    markup_parser_context = g_markup_parse_context_new (&markup_parser,0,menu,(GDestroyNotify) NULL);
  }
  if (err)
  {
    g_message ("%s: error loading menu file %s.  %s",__func__,menu_filename,err->message);
    g_error_free (err);
    err=NULL;
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
  return menu;
}
