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

#include "task-icon.h"
#include "task-settings.h"
#include "task-window.h"

G_DEFINE_TYPE (TaskManager, task_manager, AWN_TYPE_APPLET);

#define TASK_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_MANAGER, \
  TaskManagerPrivate))

struct _TaskManagerPrivate
{
  AwnConfigClient *client;
  TaskSettings    *settings;
  WnckScreen      *screen;

  /* This is what the icons are packed into */
  GtkWidget *box;
  GSList     *icons;
  GSList     *windows;

  /* Properties */
  GSList   *launcher_paths;
  gboolean  show_all_windows;
  gboolean  only_show_launchers;
};

enum
{
  PROP_0,

  PROP_SHOW_ALL_WORKSPACES,
  PROP_ONLY_SHOW_LAUNCHERS,
  PROP_LAUNCHER_PATHS
};

/* Forwards */
static void ensure_layout               (TaskManager   *manager);
static void on_window_opened            (WnckScreen    *screen, 
                                         WnckWindow    *window,
                                         TaskManager   *manager);
static void on_active_window_changed    (WnckScreen    *screen, 
                                         WnckWindow    *old_window,
                                         TaskManager   *manager);
static void task_manager_set_show_all_windows    (TaskManager *manager,
                                                  gboolean     show_all);
static void task_manager_set_show_only_launchers (TaskManager *manager, 
                                                  gboolean     show_only);
static void task_manager_refresh_launcher_paths  (TaskManager *manager,
                                                  GSList      *list);

static void task_manager_orient_changed (AwnApplet *applet, 
                                         AwnOrientation orient);
static void task_manager_size_changed   (AwnApplet *applet,
                                         gint       size);

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

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_manager_constructed (GObject *object)
{
  TaskManager        *manager = TASK_MANAGER (object);
  TaskManagerPrivate *priv;
  AwnConfigBridge    *bridge;
  GtkWidget          *widget;
  gchar              *uid = NULL;
  
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

  /* Create the icon box */
  priv->box = awn_icon_box_new_for_applet (AWN_APPLET (object));
  gtk_container_add (GTK_CONTAINER (manager), priv->box);
  gtk_widget_show (priv->box);

  /* connect to the relevent WnckScreen signals */
  g_signal_connect (priv->screen, "window-opened", 
                    G_CALLBACK (on_window_opened), manager);
  g_signal_connect (priv->screen, "active-window-changed",  
                    G_CALLBACK (on_active_window_changed), manager);
  g_signal_connect_swapped (priv->screen, "active-workspace-changed",
                            G_CALLBACK (ensure_layout), manager);
  g_signal_connect_swapped (priv->screen, "viewports-changed",
                            G_CALLBACK (ensure_layout), manager);
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
}

AwnApplet *
task_manager_new (const gchar *uid,
                  gint         orient,
                  gint         size)
{
  static AwnApplet *manager = NULL;

  if (!manager)
    manager = g_object_new (TASK_TYPE_MANAGER,
                            "uid", uid,
                            "orient", orient,
                            "size", size,
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
  TaskManager *manager = TASK_MANAGER (applet);

  g_return_if_fail (TASK_IS_MANAGER (manager));

  //awn_icon_box_set_orientation (AWN_ICON_BOX (manager->priv->box), orient);
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
}


/*
 * The guts of the show or hide logic
 */
static gboolean
update_icon_geometry (TaskManager *manager)
{
  TaskManagerPrivate *priv = manager->priv;
  GSList *i;
  
  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *icon = i->data;

    if (TASK_IS_ICON (icon))
      task_icon_refresh_geometry (icon);
  }
  return FALSE;
}

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
  
  /* We can update the window icon geometry in an idle */
  g_idle_add ((GSourceFunc)update_icon_geometry, manager);
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

static gboolean
try_to_place_window (TaskManager *manager, WnckWindow *window)
{
  /* FIXME: */
  return FALSE;
}

static gboolean
try_to_match_window_to_launcher (TaskManager *manager, TaskWindow *window)
{
  /* FIXME: */
  return FALSE;
}

static gboolean
try_to_match_window_to_sn_context (TaskManager *mananger, TaskWindow *window)
{
  /* FIXME: */
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
   * If it's a utility window, see if we can find it a home with another, 
   * existing TaskWindow, so we don't have a ton of icons for no reason
   */
  if (type == WNCK_WINDOW_UTILITY && try_to_place_window (manager, window))
  {
    g_debug ("WINDOW PLACED: %s\n", wnck_window_get_name (window));
    return;
  }

  /* 
   * We couldn't append the window to a pre-existing TaskWindow, so we'll need
   * to make a new one
   */
  taskwin = task_window_new (window);
  priv->windows = g_slist_append (priv->windows, taskwin);
  g_object_weak_ref (G_OBJECT (taskwin), (GWeakNotify)window_closed, manager);
     
  /* Okay, time to check the launchers if we can get a match */
  if (try_to_match_window_to_launcher (manager, taskwin))
  {
    g_debug ("WINDOW MATCHED: %s\n", wnck_window_get_name (window));
    return;
  }

  /* Try the startup-notification windows */
  if (try_to_match_window_to_sn_context (manager, taskwin))
  {
    g_debug ("WINDOW STARTUP: %s\n", wnck_window_get_name (window));
    return;
  }

  /* If we've come this far, the window deserves a spot on the task-manager!
   * Time to create a TaskIcon for it
   */
  icon = task_icon_new_for_window (taskwin);
  gtk_container_add (GTK_CONTAINER (priv->box), icon);
  gtk_widget_show (icon);

  priv->icons = g_slist_append (priv->icons, icon);
  g_signal_connect_swapped (icon, "ensure_layout", 
                            G_CALLBACK (ensure_layout), manager);
  g_object_weak_ref (G_OBJECT (icon), (GWeakNotify)icon_closed, manager);  
  
  /* Finally, make sure all is well on the taskbar */
  ensure_layout (manager);

  //g_debug ("WINDOW OPENED: %s\n", wnck_window_get_name (window));
}

static void 
on_active_window_changed (WnckScreen    *screen, 
                          WnckWindow    *old_window,
                          TaskManager   *manager)
{
  //g_debug ("ACTIVE_WINDOW_CHANGED\n");
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

  g_debug ("%s\n", show_all ? "showing all windows":"not showing all windows");
}

static void
task_manager_set_show_only_launchers (TaskManager *manager, 
                                      gboolean     show_only)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));
  manager->priv->only_show_launchers = show_only;

  ensure_layout (manager);

  g_debug ("%s\n", show_only ? "only show launchers":"show everything");
}

static void 
task_manager_refresh_launcher_paths (TaskManager *manager,
                                     GSList      *list)
{
  TaskManagerPrivate *priv;
  GSList *l;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  for (l = list; l; l = l->next)
    g_print ("LAUNCHER: %s\n", (gchar*)l->data);
}
