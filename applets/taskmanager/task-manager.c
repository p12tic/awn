/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>

#include "task-manager.h"

#include "task-drag-indicator.h"
#include "task-icon.h"
#include "task-launcher.h"
#include "task-settings.h"
#include "task-window.h"
#include "xutils.h"

G_DEFINE_TYPE (TaskManager, task_manager, AWN_TYPE_APPLET)

#define TASK_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_MANAGER, \
  TaskManagerPrivate))

static GQuark win_quark = 0;

struct _TaskManagerPrivate
{
  AwnConfigClient *client;
  TaskSettings    *settings;
  WnckScreen      *screen;


  /* Dragging properties */
  TaskIcon          *dragged_icon;
  TaskDragIndicator *drag_indicator;
  gint               drag_timeout;

  /* This is what the icons are packed into */
  GtkWidget *box;
  GSList     *icons;
  GSList     *windows;
  GSList     *launchers;
  GHashTable *win_table;

  /* Properties */
  GSList   *launcher_paths;
  gboolean  show_all_windows;
  gboolean  only_show_launchers;
  gboolean  drag_and_drop;
  gint      grouping_mode;
};

enum
{
  PROP_0,

  PROP_SHOW_ALL_WORKSPACES,
  PROP_ONLY_SHOW_LAUNCHERS,
  PROP_LAUNCHER_PATHS,
  PROP_DRAG_AND_DROP,
  PROP_GROUPING_MODE
};

enum
{
  GROUPING_NONE,
  GROUPING_UTIL,
  GROUPING_PID,
  GROUPING_WNCK_APP,
  GROUPING_WMCLASS,
  GROUPING_END
};

/* Forwards */
static void ensure_layout               (TaskManager   *manager);
static void on_window_opened            (WnckScreen    *screen, 
                                         WnckWindow    *window,
                                         TaskManager   *manager);
static void on_active_window_changed    (WnckScreen    *screen, 
                                         WnckWindow    *old_window,
                                         TaskManager   *manager);
static void on_wnck_window_closed (WnckScreen *screen, 
                                   WnckWindow *window, 
                                   TaskManager *manager);
static void task_manager_set_show_all_windows    (TaskManager *manager,
                                                  gboolean     show_all);
static void task_manager_set_show_only_launchers (TaskManager *manager, 
                                                  gboolean     show_only);
static void task_manager_refresh_launcher_paths  (TaskManager *manager,
                                                  GSList      *list);
static void task_manager_set_drag_and_drop (TaskManager *manager, 
                                            gboolean     drag_and_drop);

static void task_manager_set_grouping_mode (TaskManager *manager, 
                                            gint     drag_and_drop);

static void task_manager_orient_changed (AwnApplet *applet, 
                                         AwnOrientation orient);
static void task_manager_size_changed   (AwnApplet *applet,
                                         gint       size);

/* D&D Forwards */
static void _drag_dest_motion (TaskManager *manager,
                               gint x,
                               gint y,
                               GtkWidget *icon);
static void _drag_dest_leave   (TaskManager *manager,
                               GtkWidget *icon);
static void _drag_source_fail  (TaskManager *manager,
                                GtkWidget *icon);
static void _drag_source_begin (TaskManager *manager, 
                                GtkWidget *icon);
static void _drag_source_end   (TaskManager *manager, 
                                GtkWidget *icon);
//static gboolean drag_leaves_task_manager (TaskManager *manager);
static void _drag_add_signals (TaskManager *manager, 
                               GtkWidget *icon);
static void _drag_remove_signals (TaskManager *manager, 
                                  GtkWidget *icon);

/* GObject stuff */
static void
task_manager_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  TaskManager *manager = TASK_MANAGER (object);

  switch (prop_id)
  {
    case PROP_SHOW_ALL_WORKSPACES:
      g_value_set_boolean (value, manager->priv->show_all_windows); 
      break;

    case PROP_ONLY_SHOW_LAUNCHERS:
      g_value_set_boolean (value, manager->priv->only_show_launchers); 
      break;

    case PROP_LAUNCHER_PATHS:
      g_value_set_pointer (value, manager->priv->launcher_paths);
      break;

    case PROP_DRAG_AND_DROP:
      g_value_set_boolean (value, manager->priv->drag_and_drop);
      break;
      
    case PROP_GROUPING_MODE:
      g_value_set_int (value, manager->priv->grouping_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_manager_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  TaskManager *manager = TASK_MANAGER (object);

  switch (prop_id)
  {
    case PROP_SHOW_ALL_WORKSPACES:
      task_manager_set_show_all_windows (manager, g_value_get_boolean (value));
      break;

    case PROP_ONLY_SHOW_LAUNCHERS:
      task_manager_set_show_only_launchers (manager, 
                                            g_value_get_boolean (value));
      break;

    case PROP_LAUNCHER_PATHS:
      task_manager_refresh_launcher_paths (manager, 
                                           g_value_get_pointer (value));
      break;

    case PROP_DRAG_AND_DROP:
      task_manager_set_drag_and_drop (manager, 
                                      g_value_get_boolean (value));
      break;
    
    case PROP_GROUPING_MODE:
      task_manager_set_grouping_mode (manager, 
                                      g_value_get_int (value));
      break;

      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_manager_constructed (GObject *object)
{
  TaskManagerPrivate *priv;
  AwnConfigBridge    *bridge;
  GtkWidget          *widget;
  gchar              *uid = NULL;

  G_OBJECT_CLASS (task_manager_parent_class)->constructed (object);
  
  priv = TASK_MANAGER_GET_PRIVATE (object);
  widget = GTK_WIDGET (object);

  priv->settings = task_settings_get_default ();

  /* Load the uid */
  /* FIXME: AwnConfigClient needs to associate the default schema when uid is
   * used
   */
  g_object_get (object, "uid", &uid, NULL);
  priv->client = awn_config_client_new_for_applet ("taskmanager", NULL);
  g_free (uid);

  /* Connect up the important bits */
  bridge = awn_config_bridge_get_default ();
  awn_config_bridge_bind (bridge, priv->client, 
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "show_all_windows", 
                          object, "show_all_windows");
  awn_config_bridge_bind (bridge, priv->client, 
                        AWN_CONFIG_CLIENT_DEFAULT_GROUP, "only_show_launchers", 
                          object, "only_show_launchers");
  awn_config_bridge_bind_list (bridge, priv->client, 
                             AWN_CONFIG_CLIENT_DEFAULT_GROUP, "launcher_paths",
                             AWN_CONFIG_CLIENT_LIST_TYPE_STRING,
                             object, "launcher_paths");
  awn_config_bridge_bind (bridge, priv->client, 
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "drag_and_drop",
                          object, "drag_and_drop");
  awn_config_bridge_bind (bridge, priv->client, 
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "grouping_mode",
                          object, "grouping_mode");
}

static void
task_manager_class_init (TaskManagerClass *klass)
{
  GParamSpec     *pspec;
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  AwnAppletClass *app_class = AWN_APPLET_CLASS (klass);

  obj_class->constructed = task_manager_constructed;
  obj_class->set_property = task_manager_set_property;
  obj_class->get_property = task_manager_get_property;

  app_class->orient_changed = task_manager_orient_changed;
  app_class->size_changed   = task_manager_size_changed;

  /* Install properties first */
  pspec = g_param_spec_boolean ("show_all_windows",
                                "show-all-workspaces",
                                "Show windows from all workspaces",
                                TRUE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_SHOW_ALL_WORKSPACES, pspec);

  pspec = g_param_spec_boolean ("only_show_launchers",
                                "only-show-launchers",
                                "Only show launchers",
                                FALSE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ONLY_SHOW_LAUNCHERS, pspec);

  pspec = g_param_spec_pointer ("launcher_paths",
                                "launcher-paths",
                                "List of paths to launcher desktop files",
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_LAUNCHER_PATHS, pspec);

  pspec = g_param_spec_boolean ("drag_and_drop",
                                "drag-and-drop",
                                "Show windows from all workspaces",
                                TRUE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_DRAG_AND_DROP, pspec);

  pspec = g_param_spec_int ("grouping_mode",
                                "grouping_mode",
                                "Window Grouping Mode",
                                0,
                                GROUPING_END-1,
                                0,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_GROUPING_MODE, pspec);
  
  g_type_class_add_private (obj_class, sizeof (TaskManagerPrivate));
}

static void
task_manager_init (TaskManager *manager)
{
  TaskManagerPrivate *priv;
  	
  priv = manager->priv = TASK_MANAGER_GET_PRIVATE (manager);

  priv->screen = wnck_screen_get_default ();
  priv->launcher_paths = NULL;

  wnck_set_client_type (WNCK_CLIENT_TYPE_PAGER);

  win_quark = g_quark_from_string ("task-window-quark");

  /* Create the icon box */
  priv->box = awn_icon_box_new_for_applet (AWN_APPLET (manager));
  gtk_container_add (GTK_CONTAINER (manager), priv->box);
  gtk_widget_show (priv->box);

  /* Create drag indicator */
  priv->drag_indicator = TASK_DRAG_INDICATOR(task_drag_indicator_new());
  gtk_container_add (GTK_CONTAINER (priv->box), GTK_WIDGET(priv->drag_indicator));
  gtk_widget_hide (GTK_WIDGET(priv->drag_indicator));
  if(priv->drag_and_drop)
    _drag_add_signals(manager, GTK_WIDGET(priv->drag_indicator));
  /* TODO: free !!! */
  priv->dragged_icon = NULL;
  priv->drag_timeout = 0;

  /* connect to the relevent WnckScreen signals */
  g_signal_connect (priv->screen, "window-opened", 
                    G_CALLBACK (on_window_opened), manager);
  g_signal_connect (priv->screen, "active-window-changed",  
                    G_CALLBACK (on_active_window_changed), manager);
  g_signal_connect_swapped (priv->screen, "active-workspace-changed",
                            G_CALLBACK (ensure_layout), manager);
  g_signal_connect_swapped (priv->screen, "viewports-changed",
                            G_CALLBACK (ensure_layout), manager);
  g_signal_connect (priv->screen, "window-closed",
                    G_CALLBACK(on_wnck_window_closed),manager);        
}

AwnApplet *
task_manager_new (const gchar *uid,
                  gint         panel_id)
{
  static AwnApplet *manager = NULL;

  if (!manager)
    manager = g_object_new (TASK_TYPE_MANAGER,
                            "uid", uid,
                            "panel-id", panel_id,
                            NULL);
  return manager;
}

/*
 * AwnApplet stuff
 */
static void 
task_manager_orient_changed (AwnApplet *applet, 
                             AwnOrientation orient)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (applet));
  priv = TASK_MANAGER (applet)->priv;

  if (priv->settings)
    priv->settings->orient = orient;

  task_drag_indicator_refresh (priv->drag_indicator);
}

static void 
task_manager_size_changed   (AwnApplet *applet,
                             gint       size)
{
  TaskManagerPrivate *priv;
  GSList *i;

  g_return_if_fail (TASK_IS_MANAGER (applet));
  priv = TASK_MANAGER (applet)->priv;

  if (priv->settings)
    priv->settings->panel_size = size;

  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *icon = i->data;

    if (TASK_IS_ICON (icon))
      task_icon_refresh_icon (icon);
  }

  task_drag_indicator_refresh (priv->drag_indicator);
}

/*
 * The guts of the show or hide logic
 */
static void
ensure_layout (TaskManager *manager)
{
  TaskManagerPrivate *priv = manager->priv;
  WnckWorkspace *space;
  GSList         *i;

  space = wnck_screen_get_active_workspace (priv->screen);

  if (!WNCK_IS_WORKSPACE (space))
  {
    return;
  }

  /* Go through all the TaskIcons to make sure that they should be shown */
  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *icon = i->data;
    
    if (!TASK_IS_ICON (icon))
      continue;
    
    /* If the icon gets dragged, it shouldn't be shown */
    if( icon == priv->dragged_icon )
    {
      gtk_widget_hide (GTK_WIDGET (icon));
      continue;
    }

    /* Show launchers regardless of workspace */
    if (task_icon_is_launcher (icon))
    {
      gtk_widget_show (GTK_WIDGET (icon));
      continue;
    }

    /* FIXME: Add support for start-up notification icons too */

    /* 
     * Only show normal window icons if a) the use wants to see them and b) if
     * they are on the correct workspace
     */
    if (priv->only_show_launchers)
    {
      gtk_widget_hide (GTK_WIDGET (icon));
    }
    else if (task_icon_is_skip_taskbar (icon))
    {
      gtk_widget_hide (GTK_WIDGET (icon));
    } 
    else if (task_icon_is_in_viewport (icon, space))
    {
      gtk_widget_show (GTK_WIDGET (icon));
    }
    else if (priv->show_all_windows)
    {
      gtk_widget_show (GTK_WIDGET (icon));  
    }
    else
    {
      gtk_widget_hide (GTK_WIDGET (icon));
    }
  }
  
  /* Hide drag_indicator if there is no AwnIcon currently dragged */
  if(priv->dragged_icon == NULL)
    gtk_widget_hide (GTK_WIDGET(priv->drag_indicator));
}

/*
 * WNCK_SCREEN CALLBACKS
 */
static void
on_window_state_changed (WnckWindow      *window,
                         WnckWindowState  changed_mask,
                         WnckWindowState  new_state,
                         TaskManager     *manager)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  /* This signal is only connected for windows which were of type normal/utility
   * and were initially "skip-tasklist". If they are not skip-tasklist anymore
   * we treat them as newly opened windows
   */
  if (!wnck_window_is_skip_tasklist (window))
  {
    g_signal_handlers_disconnect_by_func (window, 
                                          on_window_state_changed, manager);
    on_window_opened (NULL, window, manager);
    return;
  }
}

static void 
on_wnck_window_closed (WnckScreen *screen, WnckWindow *window, TaskManager *manager)
{
  GSList *i;  
  TaskManagerPrivate *priv = manager->priv;  
  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *taskicon = i->data;
    if (!TASK_IS_ICON (taskicon))
      continue;
    task_icon_remove_window (taskicon, window);            
  }
}

static gboolean
check_wmclass(TaskWindow *taskwin,gchar * win_res_name, gchar *win_class_name)
{
  gboolean result;
  gchar   *temp;
  gchar   *res_name = NULL;
  gchar   *class_name = NULL;

  _wnck_get_wmclass (task_window_get_xid (taskwin), &res_name, &class_name);
  if (res_name)
  {
    temp = res_name;
    res_name = g_utf8_strdown (temp, -1);
    g_free (temp);
  }
  
  if (class_name)
  {
    temp = class_name;
    class_name = g_utf8_strdown (temp, -1);
    g_free (temp);
  }
  result =  ( 
              (g_strcmp0 (res_name,win_res_name)==0) 
              ||
              (g_strcmp0 (class_name,win_class_name)==0)
             );
  g_free (res_name);
  g_free (class_name);

  return result;
}

static gboolean
try_to_place_window_by_wmclass (TaskManager *manager, WnckWindow *window)
{
  GSList *i;
  TaskManagerPrivate *priv = manager->priv;
  gchar   *temp;
  gchar   *res_name = NULL;
  gchar   *class_name = NULL;

  /* Grab the appropriete info */
  _wnck_get_wmclass (wnck_window_get_xid (window), &res_name, &class_name);

  if (res_name)
  { 
    temp = res_name;
    res_name = g_utf8_strdown (temp, -1);
    g_free (temp);
  }
  
  if (class_name)
  {
    temp = class_name;
    class_name = g_utf8_strdown (temp, -1);
    g_free (temp);
  }
  
  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *taskicon = i->data;
    TaskWindow *taskwin = NULL;
    
    if (!TASK_IS_ICON (taskicon))
      continue;
    
    taskwin = task_icon_get_window(taskicon);

    if ( check_wmclass (taskwin,res_name,class_name) )
    {
      task_icon_append_window (taskicon, task_window_new (window));
      g_object_set_qdata (G_OBJECT (window), win_quark, taskwin);
      g_free (res_name);
      g_free (class_name);
      return TRUE;
    }
  }

  g_free (res_name);
  g_free (class_name);
  return FALSE;  
}

static gboolean
try_to_place_window_by_wnck_app (TaskManager *manager, WnckWindow *window)
{
  TaskManagerPrivate *priv = manager->priv;
  GSList *i;
  WnckApplication *taskwin_app;

  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *taskicon = i->data;
    TaskWindow *taskwin = NULL;
    
    if (!TASK_IS_ICON (taskicon))
      continue;
    
    taskwin = task_icon_get_window(taskicon);
		taskwin_app = task_window_get_application (taskwin);
    if ( taskwin_app && (taskwin_app == wnck_window_get_application (window)))
    {
      task_icon_append_window (taskicon, task_window_new (window));
      g_object_set_qdata (G_OBJECT (window), win_quark, taskwin);
      return TRUE;
    }
  }
  
  return FALSE;  
}

static gboolean
try_to_place_window_by_pid (TaskManager *manager, WnckWindow *window)
{
  TaskManagerPrivate *priv = manager->priv;
  GSList *i;
	gint taskwin_pid = -1;
  gint matches = 0;

  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *taskicon = i->data;
    TaskWindow *taskwin = NULL;
    
    if (!TASK_IS_ICON (taskicon))
      continue;
    
    taskwin = task_icon_get_window(taskicon);
		taskwin_pid = task_window_get_pid (taskwin);
    if ( taskwin_pid && (taskwin_pid == wnck_window_get_pid (window)))
    {
      if (matches)
      {
        task_icon_append_window (taskicon, task_window_new (window));
        g_object_set_qdata (G_OBJECT (window), win_quark, taskwin);
        return TRUE;
      }
      matches++;
    }
  }
  return FALSE;
}

static gboolean
try_to_place_util_window (TaskManager *manager, WnckWindow *window)
{
  WnckWindowType type = wnck_window_get_window_type (window);
  TaskManagerPrivate *priv = manager->priv;
  GSList *w;
  gint taskwin_pid = -1;
  gint matches = 0;
  
  if ( (type !=  WNCK_WINDOW_UTILITY) && (type != WNCK_WINDOW_DIALOG) )
  {
    return FALSE;
  }
  
  for (w = priv->icons; w; w = w->next)
  {
    TaskIcon *taskicon = w->data;
    TaskWindow *taskwin = NULL;
    
    if (!TASK_IS_ICON (taskicon))
      continue;
    
    taskwin = task_icon_get_window(taskicon);
    taskwin_pid = task_window_get_pid (taskwin);
    if ( taskwin_pid && (taskwin_pid == wnck_window_get_pid (window)))
    {
      if (matches)
      {
        task_icon_append_window (taskicon, task_window_new (window));
        g_object_set_qdata (G_OBJECT (window), win_quark, taskwin);
        return TRUE;
      }
      matches++;
    }
  } 
  return FALSE;
} 

static gboolean
try_to_place_window (TaskManager *manager, WnckWindow *window)
{
  TaskManagerPrivate *priv = manager->priv;
  gboolean result = FALSE;
  switch(priv->grouping_mode)
  {
    case GROUPING_WMCLASS:  /*Fall through*/
      result =result?result:try_to_place_window_by_wmclass(manager,window);
    case GROUPING_WNCK_APP:/*Fall through*/
      result =result?result:try_to_place_window_by_wnck_app(manager,window);      
    case GROUPING_PID: /*Don't Fall through*/
      result =result?result:try_to_place_window_by_pid(manager,window);
      break;
    case GROUPING_UTIL:/*Don't Fall through*/
      result =result?result:try_to_place_util_window(manager,window);
      break;
    case GROUPING_NONE:
      break;
    default:
      g_assert_not_reached();
  }
  return result;
}

static gboolean
try_to_match_window_to_launcher (TaskManager *manager, WnckWindow *window)
{
  TaskManagerPrivate *priv = manager->priv;
  GSList  *l;
  gchar   *temp;
  gchar   *res_name = NULL;
  gchar   *class_name = NULL;
  gint     pid;
  gboolean res = FALSE;

  /* Grab the appropriete info */
  pid = wnck_window_get_pid (window);
  _wnck_get_wmclass (wnck_window_get_xid (window), &res_name, &class_name);

  if (res_name)
  {
    temp = res_name;
    res_name = g_utf8_strdown (temp, -1);
    g_free (temp);
  }
  
  if (class_name)
  {
    temp = class_name;
    class_name = g_utf8_strdown (temp, -1);
    g_free (temp);
  }

  /* Try and match */
  for (l = priv->launchers; l; l = l->next)
  {
    TaskLauncher *launcher = l->data;

    if (!TASK_IS_LAUNCHER (launcher))
      continue;

    if (task_launcher_has_window (launcher))
      continue;
    
    if (!task_launcher_try_match (launcher, pid, res_name, class_name))
      continue;

    /* As it matched this launcher, we can set the window to the launcher and
     * get on with it
     */
    task_launcher_set_window (launcher, window);
    g_object_set_qdata (G_OBJECT (window), win_quark, launcher);
    res = TRUE;
  }
  g_free (res_name);
  g_free (class_name);

  return res;
}

static gboolean
try_to_match_window_to_sn_context (TaskManager *manager, WnckWindow *window)
{
  return FALSE;
}

static void
window_closed (TaskManager *manager, GObject *old_window)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  priv->windows = g_slist_remove (priv->windows, old_window);

  ensure_layout (manager);
}

static void
icon_closed (TaskManager *manager, GObject *old_icon)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  priv->icons = g_slist_remove (priv->icons, old_icon);

  ensure_layout (manager);
}

static void 
on_window_opened (WnckScreen    *screen, 
                  WnckWindow    *window,
                  TaskManager   *manager)
{
  TaskManagerPrivate *priv;
  GtkWidget          *icon;
  TaskWindow         *taskwin;
  WnckWindowType      type;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  g_return_if_fail (WNCK_IS_WINDOW (window));
  priv = manager->priv;

  type = wnck_window_get_window_type (window);

  switch (type)
  {
    case WNCK_WINDOW_DESKTOP:
    case WNCK_WINDOW_DOCK:
    case WNCK_WINDOW_TOOLBAR:
    case WNCK_WINDOW_MENU:
    case WNCK_WINDOW_SPLASHSCREEN:
      return; /* No need to worry about these */

    default:
      break;
  }
  
  /* 
   * If it's skip tasklist, connect to the state-changed signal and see if
   * it ever becomes a normal window
   */
  if (wnck_window_is_skip_tasklist (window))
  {
    g_signal_connect (window, "state-changed", 
                      G_CALLBACK (on_window_state_changed), manager);    
    return;
  }

  /*
   */
    
  if ( priv->grouping_mode && try_to_place_window (manager,window))
  {
    g_debug ("WINDOW PLACED: %s", wnck_window_get_name (window));
    return;
  }
 
  /* Okay, time to check the launchers if we can get a match */
  if (try_to_match_window_to_launcher (manager, window))
  {
    g_debug ("WINDOW MATCHED: %s", wnck_window_get_name (window));
    return;
  }

  /* Try the startup-notification windows */
  if (try_to_match_window_to_sn_context (manager, window))
  {
    g_debug ("WINDOW STARTUP: %s", wnck_window_get_name (window));
    return;
  }

  /* 
   * We couldn't append the window to a pre-existing TaskWindow, so we'll need
   * to make a new one
   */
  taskwin = task_window_new (window);
  priv->windows = g_slist_append (priv->windows, taskwin);
  g_object_weak_ref (G_OBJECT (taskwin), (GWeakNotify)window_closed, manager);
  g_object_set_qdata (G_OBJECT (window), win_quark, taskwin); 
  
  /* If we've come this far, the window deserves a spot on the task-manager!
   * Time to create a TaskIcon for it
   */
  icon = task_icon_new_for_window (taskwin);
  gtk_container_add (GTK_CONTAINER (priv->box), icon);
  gtk_widget_show (icon);

  priv->icons = g_slist_append (priv->icons, icon);
  g_signal_connect_swapped (icon, "ensure_layout", 
                            G_CALLBACK (ensure_layout), manager);

  /* reordening through D&D */
  if(priv->drag_and_drop)
    _drag_add_signals(manager, icon);

  g_object_weak_ref (G_OBJECT (icon), (GWeakNotify)icon_closed, manager);  
  
  /* Finally, make sure all is well on the taskbar */
  ensure_layout (manager);
}

static void 
on_active_window_changed (WnckScreen    *screen, 
                          WnckWindow    *old_window,
                          TaskManager   *manager)
{
  TaskManagerPrivate *priv;
  WnckWindow         *active = NULL;
  TaskWindow         *taskwin = NULL;
  TaskWindow         *old_taskwin = NULL;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  active = wnck_screen_get_active_window (priv->screen);

  if (WNCK_IS_WINDOW (old_window))
    old_taskwin = (TaskWindow *)g_object_get_qdata (G_OBJECT (old_window),
                                                    win_quark);
  if (WNCK_IS_WINDOW (active))
    taskwin = (TaskWindow *)g_object_get_qdata (G_OBJECT (active), win_quark);

  if (TASK_IS_WINDOW (old_taskwin))
    task_window_set_is_active (old_taskwin, FALSE);
  if (TASK_IS_WINDOW (taskwin))
    task_window_set_is_active (taskwin, TRUE);
}

/*
 * PROPERTIES
 */
static void
task_manager_set_show_all_windows (TaskManager *manager,
                                   gboolean     show_all)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));
  manager->priv->show_all_windows = show_all;

  ensure_layout (manager);

  g_debug ("%s", show_all ? "showing all windows":"not showing all windows");
}

static void
task_manager_set_show_only_launchers (TaskManager *manager, 
                                      gboolean     show_only)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));
  manager->priv->only_show_launchers = show_only;

  ensure_layout (manager);

  g_debug ("%s", show_only ? "only show launchers":"show everything");
}

static void 
task_manager_refresh_launcher_paths (TaskManager *manager,
                                     GSList      *list)
{
  TaskManagerPrivate *priv;
  GSList *d;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
  
  /* FIXME: I guess we should add something to check whether the user has
   * removed a launcher. Make sure we don't remove a launcher which has a 
   * window set, wait till the window is closed
   */
  for (d = list; d; d = d->next)
  {
    GtkWidget    *icon;
    TaskLauncher *launcher = NULL;
    GSList        *l;

    for (l = priv->launchers; l; l = l->next)
    {
      TaskLauncher *launch = l->data;

      if (!TASK_IS_LAUNCHER (launch))
        continue;
      
      if (g_strcmp0 (d->data, task_launcher_get_desktop_path (launch)) == 0)
      {
        launcher = launch;
        break;
      }
    }

    if (TASK_IS_LAUNCHER (launcher))
      continue;

    launcher = task_launcher_new_for_desktop_file (d->data);

    if (!launcher)
      continue;

    priv->launchers = g_slist_append (priv->launchers, launcher);
    
    icon = task_icon_new_for_window (TASK_WINDOW (launcher));
    gtk_container_add (GTK_CONTAINER (priv->box), icon);
    gtk_widget_show (icon);

    priv->icons = g_slist_append (priv->icons, icon);
    g_signal_connect_swapped (icon, "ensure_layout", 
                               G_CALLBACK (ensure_layout), manager);
    g_object_weak_ref (G_OBJECT (icon), (GWeakNotify)icon_closed, manager);

    /* reordening through D&D */
    if(priv->drag_and_drop)
      _drag_add_signals(manager, icon);

  }
  for (d = list; d; d = d->next)
    g_free (d->data);
  g_slist_free (list);

  /* Finally, make sure all is well on the taskbar */
  ensure_layout (manager);
}

static void
task_manager_set_grouping_mode (TaskManager *manager,
                                   gint grouping_mode)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));
  manager->priv->grouping_mode = grouping_mode;

  ensure_layout (manager);
}

static void
task_manager_set_drag_and_drop (TaskManager *manager, 
                                gboolean     drag_and_drop)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  TaskManagerPrivate *priv = manager->priv;
  GSList         *i;

  priv->drag_and_drop = drag_and_drop;

  if(drag_and_drop)
  {
    //connect to the dragging signals
    for (i = priv->icons; i; i = i->next)
    {
      TaskIcon *icon = i->data;
      
      if (!TASK_IS_ICON (icon)) continue;

      _drag_add_signals (manager, GTK_WIDGET(icon));
    }
    _drag_add_signals (manager, GTK_WIDGET(priv->drag_indicator));
  }
  else
  {
    //disconnect the dragging signals
    for (i = priv->icons; i; i = i->next)
    {
      TaskIcon *icon = i->data;
      
      if (!TASK_IS_ICON (icon)) continue;

      _drag_remove_signals (manager, GTK_WIDGET(icon));
    }
    _drag_remove_signals (manager, GTK_WIDGET(priv->drag_indicator));
    //FIXME: Stop any ongoing move
  }

  g_debug("%s", drag_and_drop?"D&D is on":"D&D is off");
}

/*
 * Position Icons through dragging
 */

static void
_drag_dest_motion(TaskManager *manager, gint x, gint y, GtkWidget *icon)
{
  gint move_to, moved;
  GList* childs;
  TaskManagerPrivate *priv;
  AwnOrientation orient;
  guint size;
  double action;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = TASK_MANAGER_GET_PRIVATE (manager);

  g_return_if_fail(priv->dragged_icon != NULL);

  /* There was a timeout to check if the cursor left the bar */
  if(priv->drag_timeout)
  {
    g_source_remove (priv->drag_timeout);
    priv->drag_timeout = 0;
  }

  orient = awn_applet_get_orientation (AWN_APPLET(manager));
  size = awn_applet_get_size (AWN_APPLET(manager));
  childs = gtk_container_get_children (GTK_CONTAINER(priv->box));
  move_to = g_list_index (childs, GTK_WIDGET(icon));
  moved = g_list_index (childs, GTK_WIDGET(priv->drag_indicator));

  g_return_if_fail (move_to != -1);
  g_return_if_fail (moved != -1);

  if(orient == AWN_ORIENTATION_TOP || orient == AWN_ORIENTATION_BOTTOM)
    action = (double)x/size;
  else
    action = (double)y/size;

  if(action < 0.5)
  {
    if(moved > move_to)
    {
      gtk_box_reorder_child (GTK_BOX(priv->box), GTK_WIDGET(priv->drag_indicator), move_to);
    }
    gtk_widget_show(GTK_WIDGET(priv->drag_indicator));
  }
  else
  {
    if(moved < move_to)
    {
      gtk_box_reorder_child (GTK_BOX(priv->box), GTK_WIDGET(priv->drag_indicator), move_to);
    }
    gtk_widget_show(GTK_WIDGET(priv->drag_indicator));
  }
}

static void 
_drag_source_begin(TaskManager *manager, GtkWidget *icon)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  TaskManagerPrivate *priv = TASK_MANAGER_GET_PRIVATE (manager);

  /* dragging starts, so setup everything */
  if(!priv->dragged_icon)
  {
    g_return_if_fail (TASK_IS_ICON (icon));
    priv->dragged_icon = TASK_ICON(icon);
    gtk_widget_hide (GTK_WIDGET(icon));

    //it will move the drag_indicator to the right spot
    _drag_dest_motion(manager, 0, 0, icon);
  }
  else 
  {
    g_print("ERROR: previous drag wasn't done yet ?!");
    g_return_if_reached();
  }
}

static void 
_drag_source_fail(TaskManager *manager, GtkWidget *icon)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  TaskManagerPrivate *priv = TASK_MANAGER_GET_PRIVATE (manager);

  if(!priv->dragged_icon) return;

  //gtk_widget_hide (GTK_WIDGET (priv->drag_indicator));
  //gtk_widget_show (GTK_WIDGET (priv->dragged_icon));
  //priv->dragged_icon = NULL;

  // Handle a fail like a drop for now
  _drag_source_end(manager, NULL);
}

static void 
_drag_dest_leave (TaskManager *manager, GtkWidget *icon)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  /*
  TaskManagerPrivate *priv = TASK_MANAGER_GET_PRIVATE (manager);

  //FIXME: REMOVE OLD TIMER AND SET NEW ONE
  if(!priv->drag_timeout)
    priv->drag_timeout = g_timeout_add (200, (GSourceFunc)drag_leaves_task_manager, manager);
  */
}

//static gboolean
//drag_leaves_task_manager (TaskManager *manager)
//{
//  g_return_val_if_fail (TASK_IS_MANAGER (manager), FALSE);
//
//  TaskManagerPrivate *priv = TASK_MANAGER_GET_PRIVATE (manager);
//
//  gtk_widget_hide(GTK_WIDGET(priv->drag_indicator));
//
//  return FALSE;
//}

static void 
_drag_source_end(TaskManager *manager, GtkWidget *icon)
{
  TaskManagerPrivate *priv;
  gint move_to;
  GList* childs;
  GSList* d;
  GSList* launchers = NULL;
  TaskLauncher* launcher;
  gchar* launcher_path;
  GError *err = NULL;

  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = TASK_MANAGER_GET_PRIVATE (manager);
  if(priv->dragged_icon == NULL)
    return;

  if(priv->drag_timeout)
  {
    g_source_remove (priv->drag_timeout);
    priv->drag_timeout = 0;
  }

  // Move the AwnIcon to the dropped place 
  childs = gtk_container_get_children (GTK_CONTAINER(priv->box));
  move_to = g_list_index (childs, GTK_WIDGET (priv->drag_indicator));
  gtk_box_reorder_child (GTK_BOX (priv->box), GTK_WIDGET (priv->dragged_icon), move_to);

  // Hide the DragIndicator and show AwnIcon again 
  gtk_widget_hide (GTK_WIDGET (priv->drag_indicator));
  gtk_widget_show (GTK_WIDGET (priv->dragged_icon));

  // If workspace isn't the same as the active one,
  // it means you switched to another workspace when dragging.
  // This means you want the window on that screen.
  // FIXME: Make a way to get the WnckWindow out of AwnIcon
 
  //screen = wnck_window_get_screen(priv->window);
  //active_workspace = wnck_screen_get_active_workspace (screen);
  //if (active_workspace && !wnck_window_is_on_workspace(priv->window, active_workspace))
  //  wnck_window_move_to_workspace(priv->window, active_workspace);

  // Update the position in the config (Gconf) if the AwnIcon is a launcher.
  // FIXME: support multiple launchers in one AwnIcon?

  if (task_icon_is_launcher (priv->dragged_icon))
  {
    // get the updated list
    childs = gtk_container_get_children (GTK_CONTAINER(priv->box));
    while(childs)
    {
      if( TASK_IS_ICON(childs->data) && task_icon_is_launcher (TASK_ICON (childs->data)))
      {
        launcher = task_icon_get_launcher (TASK_ICON (childs->data));
        launcher_path = g_strdup (task_launcher_get_desktop_path (launcher));
        launchers = g_slist_prepend (launchers, launcher_path);
      }
      childs = childs->next;
    }
    launchers = g_slist_reverse(launchers);

    awn_config_client_set_list (priv->client, 
                                AWN_CONFIG_CLIENT_DEFAULT_GROUP, 
                                "launcher_paths", 
                                AWN_CONFIG_CLIENT_LIST_TYPE_STRING, 
                                launchers, 
                                &err);
    for (d = launchers; d; d = d->next)
      g_free (d->data);
    g_slist_free (launchers);

    if (err) {
      g_warning ("Error: %s", err->message);
      return;
    }
  }

  priv->dragged_icon = NULL;
}

static void 
_drag_add_signals (TaskManager *manager, GtkWidget *icon)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));
  g_return_if_fail (TASK_IS_ICON (icon)||TASK_IS_DRAG_INDICATOR (icon));

  // listen to signals of 'source' start getting dragged, only if it is an icon
  if(TASK_IS_ICON (icon))
  {
    g_object_set(icon, "draggable", TRUE, NULL);
    g_signal_connect_swapped (icon, "source_drag_begin", G_CALLBACK (_drag_source_begin), manager);
    g_signal_connect_swapped (icon, "source_drag_end", G_CALLBACK (_drag_source_end), manager);
    g_signal_connect_swapped (icon, "source_drag_fail", G_CALLBACK (_drag_source_fail), manager);
  }

  g_signal_connect_swapped (icon, "dest_drag_motion", G_CALLBACK (_drag_dest_motion), manager);
  g_signal_connect_swapped (icon, "dest_drag_leave", G_CALLBACK (_drag_dest_leave), manager);
}

static void 
_drag_remove_signals (TaskManager *manager, GtkWidget *icon)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));
  g_return_if_fail (TASK_IS_ICON (icon)||TASK_IS_DRAG_INDICATOR (icon));

  if(TASK_IS_ICON (icon))
  {
    g_object_set(icon, "draggable", FALSE, NULL);
    g_signal_handlers_disconnect_by_func(icon, G_CALLBACK (_drag_source_begin), manager);
    g_signal_handlers_disconnect_by_func(icon, G_CALLBACK (_drag_source_end), manager);
    g_signal_handlers_disconnect_by_func(icon, G_CALLBACK (_drag_source_fail), manager);
  }

  g_signal_handlers_disconnect_by_func(icon, G_CALLBACK (_drag_dest_motion), manager);
  g_signal_handlers_disconnect_by_func(icon, G_CALLBACK (_drag_dest_leave), manager);
}
