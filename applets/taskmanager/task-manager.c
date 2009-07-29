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
 *             Hannes Verschore <hv1989@gmail.com>
 *             Rodney Cryderman <rcryderman@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <sys/types.h>
#include <unistd.h>

#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>

#include "task-manager.h"
#include "task-manager-glue.h"

#include "task-drag-indicator.h"
#include "task-icon.h"
//#include "task-launcher.h"
//#include "task-window.h"
#include "task-settings.h"
#include "xutils.h"

G_DEFINE_TYPE (TaskManager, task_manager, AWN_TYPE_APPLET)

#define TASK_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_MANAGER, \
  TaskManagerPrivate))

//#define DEBUG 1

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
  GtkWidget  *box;
  GSList     *icons;
  GSList     *windows;
  GHashTable *win_table;

  /* Properties */
  GSList   *launcher_paths;
  gboolean  show_all_windows;
  gboolean  only_show_launchers;
  gboolean  drag_and_drop;
  gboolean  grouping;
  gint      match_strength;
};

enum
{
  PROP_0,

  PROP_SHOW_ALL_WORKSPACES,
  PROP_ONLY_SHOW_LAUNCHERS,
  PROP_LAUNCHER_PATHS,
  PROP_DRAG_AND_DROP,
  PROP_GROUPING,
  PROP_MATCH_STRENGTH
};

/* Forwards */
static void update_icon_visible         (TaskManager   *manager, 
                                         TaskIcon      *icon);
static void on_icon_visible_changed     (TaskManager   *manager, 
                                         TaskIcon      *icon);
static void on_icon_effects_ends        (TaskIcon      *icon,
                                         AwnEffect      effect,
                                         AwnEffects    *instance);
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
static void task_manager_set_drag_and_drop (TaskManager *manager, 
                                            gboolean     drag_and_drop);

static void task_manager_set_grouping (TaskManager *manager, 
                                            gboolean     grouping);

static void task_manager_set_match_strength (TaskManager *manager, 
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

    case PROP_GROUPING:
      g_value_set_boolean (value, manager->priv->grouping);
      break;

    case PROP_MATCH_STRENGTH:
      g_value_set_int (value, manager->priv->match_strength);
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

    case PROP_MATCH_STRENGTH:
      task_manager_set_match_strength (manager, 
                                       g_value_get_int (value));
      break;

    case PROP_GROUPING:
      task_manager_set_grouping (manager,
                                 g_value_get_boolean (value));
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
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "grouping",
                          object, "grouping");  
  awn_config_bridge_bind (bridge, priv->client, 
                          AWN_CONFIG_CLIENT_DEFAULT_GROUP, "match_strength",
                          object, "match_strength");
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

  pspec = g_param_spec_boolean ("grouping",
                                "grouping",
                                "Group windows",
                                TRUE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_GROUPING, pspec);

  pspec = g_param_spec_int ("match_strength",
                            "match_strength",
                            "How radical matching is applied for grouping items",
                            0,
                            99,
                            0,
                            G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_MATCH_STRENGTH, pspec);
  
  g_type_class_add_private (obj_class, sizeof (TaskManagerPrivate));

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_task_manager_object_info);
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
}

AwnApplet *
task_manager_new (const gchar *name,
                  const gchar *uid,
                  gint         panel_id)
{
  static AwnApplet *manager = NULL;

  if (!manager)
    manager = g_object_new (TASK_TYPE_MANAGER,
                            "canonical-name", name,
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
 * WNCK_SCREEN CALLBACKS
 */

/**
 * This signal is only connected for windows which were of type normal/utility
 * and were initially "skip-tasklist". If they are not skip-tasklist anymore
 * we treat them as newly opened windows.
 * STATE: done
 */
static void
on_window_state_changed (WnckWindow      *window,
                         WnckWindowState  changed_mask,
                         WnckWindowState  new_state,
                         TaskManager     *manager)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  // test if they don't skip-tasklist anymore
  if (!wnck_window_is_skip_tasklist (window))
  {
    g_signal_handlers_disconnect_by_func (window, 
                                          on_window_state_changed, manager);
    on_window_opened (NULL, window, manager);
    return;
  }
}

/**
 * The active WnckWindow has changed.
 * Retrieve the TaskWindows and update there active state 
 */
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

/**
 * When the property 'show_all_windows' is False,
 * workspace switches are monitored. Whenever one happens
 * all TaskWindows are notified.
 */
static void
on_workspace_changed (TaskManager *manager) //... has more arguments
{
  TaskManagerPrivate *priv;
  GSList             *w;
  WnckWorkspace      *space;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = manager->priv;
  space = wnck_screen_get_active_workspace (priv->screen);
  
  for (w = priv->windows; w; w = w->next)
  {
    TaskWindow *window = w->data;

    if (!TASK_IS_WINDOW (window)) continue;

    task_window_set_active_workspace (window, space);
  }
}

/*
 * TASK_ICON CALLBACKS
 */

static void
update_icon_visible (TaskManager *manager, TaskIcon *icon)
{
  TaskManagerPrivate *priv;
  gboolean visible = FALSE;

  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = manager->priv;
  
  if (task_icon_is_visible (icon) && 
      (!priv->only_show_launchers || task_icon_contains_launcher (icon)))
  {
    visible = TRUE;
  }

  if (visible && !GTK_WIDGET_VISIBLE (icon))
  {
    gtk_widget_show (GTK_WIDGET (icon));
    awn_effects_start_ex (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                          AWN_EFFECT_OPENING, 1, FALSE, FALSE);
  }

  if (!visible && GTK_WIDGET_VISIBLE (icon))
  {
    awn_effects_start_ex (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                          AWN_EFFECT_CLOSING, 1, FALSE, TRUE);
    //hidding of TaskIcon happens when effect is done.
  }
}

static void
on_icon_visible_changed (TaskManager *manager, TaskIcon *icon)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  update_icon_visible (manager, icon);
}

static void
on_icon_effects_ends (TaskIcon   *icon,
                      AwnEffect   effect,
                      AwnEffects *instance)
{
  g_return_if_fail (TASK_IS_ICON (icon));

  if (effect == AWN_EFFECT_CLOSING)
  {
    gtk_widget_hide (GTK_WIDGET (icon));

    //if (task_icon_count_items (icon) == 0)
    //  gtk_widget_destroy (GTK_WIDGET (icon));
  }
}

/**
 * This function gets called whenever a task-window gets finalized.
 * It removes the task-window from the list.
 * State: done
 */
static void
window_closed (TaskManager *manager, GObject *old_item)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = manager->priv;
  priv->windows = g_slist_remove (priv->windows, old_item);
}

/**
 * This function gets called whenever a task-icon gets finalized.
 * It removes the task-icon from the gslist and update layout
 * (so it gets removed from the bar)
 * State: done
 */
static void
icon_closed (TaskManager *manager, GObject *old_icon)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = manager->priv;
  priv->icons = g_slist_remove (priv->icons, old_icon);
}


static gboolean
find_desktop (TaskIcon *icon, gchar * class_name)
{
  gchar * lower;
  gchar * desktop;
  const gchar* const * system_dirs = g_get_system_data_dirs ();
  GStrv   iter;
  GStrv   tokens;
  TaskItem     *launcher = NULL;
  
  g_return_val_if_fail (class_name,FALSE);
  lower = g_utf8_strdown (class_name, -1);
  
#ifdef DEBUG
  g_debug ("%s: wm class = %s",__func__,class_name);
  g_debug ("%s: lower = %s",__func__,lower);
#endif      
  
  for (iter = (GStrv)system_dirs; *iter; iter++)
  {
    desktop = g_strdup_printf ("%sapplications/%s.desktop",*iter,lower);
#ifdef DEBUG
    g_debug ("%s: desktop = %s",__func__,desktop);
#endif      
    launcher = task_launcher_new_for_desktop_file (desktop);
    if (launcher)
    {
#ifdef DEBUG
      g_debug ("launcher %p",launcher);
#endif
      task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);
      g_free (lower);
      g_free (desktop);
      return TRUE;
    }
    g_free (desktop);
  }
  if (!launcher)
  {
    desktop = g_strdup_printf ("%sapplications/%s.desktop",g_get_user_data_dir (),lower);
    launcher = task_launcher_new_for_desktop_file (desktop);
    if (launcher)
    {
#ifdef DEBUG
      g_debug ("launcher %p",launcher);
#endif
      task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);
      g_free (desktop);
      g_free  (lower);
      return TRUE;
    }        
    g_free (desktop);        
  }
  
  g_strdelimit (lower,"-.:,=+_~!@#$%^()[]{}'",' ');
  tokens = g_strsplit (lower," ",-1);
  if (tokens)
  {
    gchar * stripped = g_strjoinv(NULL,tokens);
    g_strfreev (tokens);    
    if (g_strcmp0 (stripped, class_name) !=0)
    {
      if (find_desktop (icon,stripped) )
      {
        g_free (lower);
        g_free (stripped);
        return TRUE;
      }
    }  
    g_free (stripped);    
  }
  g_free (lower);  
  return FALSE;
}

static gboolean
find_desktop_fuzzy (TaskIcon *icon, gchar * class_name, gchar *cmd)
{
  gchar * lower;
  gchar * desktop_regex_str;
  GRegex  * desktop_regex;
  const gchar* const * system_dirs = g_get_system_data_dirs ();
  GStrv   iter;
  TaskItem     *launcher = NULL;
  
  g_return_val_if_fail (class_name,FALSE);
  lower = g_utf8_strdown (class_name, -1);
  
//#define DEBUG 1
#ifdef DEBUG
  g_debug ("%s: wm class = %s",__func__,class_name);
  g_debug ("%s: lower = %s",__func__,lower);
#endif      

  /*
   TODO compile the regex
   */
  desktop_regex_str = g_strdup_printf (".*%s.*desktop",lower);  
  desktop_regex = g_regex_new (desktop_regex_str,0,0,NULL);

#ifdef DEBUG
    g_debug ("%s: desktop regex = %s",__func__,desktop_regex_str);
#endif      
  g_free (lower);    
  g_free (desktop_regex_str);
  g_return_val_if_fail (desktop_regex,FALSE);
  
  for (iter = (GStrv)system_dirs; *iter; iter++)
  {
    gchar * dir_name = g_strdup_printf ("%sapplications",*iter);
    GDir  * dir = g_dir_open (dir_name,0,NULL);
    if (dir)
    {
      const gchar * filename;
      while ( (filename = g_dir_read_name (dir)) )
      {
        if ( g_regex_match (desktop_regex, filename,0,NULL) )
        {
          gchar * full_path = g_strdup_printf ("%s/%s",dir_name,filename);
#ifdef DEBUG
          g_debug ("%s:  regex matched full path =   %s",__func__,full_path);
#endif       
          AwnDesktopItem * desktop = awn_desktop_item_new (full_path);
          if (desktop)
          {
            gchar * exec = awn_desktop_item_get_exec (desktop);
            awn_desktop_item_free (desktop);
#ifdef DEBUG
            g_debug ("%s:  exec =   %s",__func__,exec);
            g_debug ("%s:  cmd =   %s",__func__,cmd);            
#endif            
            if (exec)
            {
              /*
               May need some adjustments.
                Possible conversion to a regex
               */
              if ( g_regex_match_simple (exec,cmd,G_REGEX_CASELESS,0) 
                  || g_regex_match_simple (cmd, exec,G_REGEX_CASELESS,0))
              {
                launcher = task_launcher_new_for_desktop_file (full_path);
                if (launcher)
                {
                  task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);
                  g_regex_unref (desktop_regex);
                  g_free (exec);
                  return TRUE;
                }
              }
              g_free (exec);              
            }            
          }          
          g_free (full_path);
        }
      }
      g_dir_close (dir);
    }
    g_free (dir_name);
  }
  g_regex_unref (desktop_regex);
  return FALSE;
//#undef DEBUG
  
}


/**
 * Whenever a new window gets opened it will try to place it
 * in an awn-icon or will create a new awn-icon.
 * State: adjusted
 */
static void 
on_window_opened (WnckScreen    *screen, 
                  WnckWindow    *window,
                  TaskManager   *manager)
{
  /*TODO
   This functions is becoming a beast.  look into chopping into bits
   */
  TaskManagerPrivate *priv;
  GtkWidget          *icon;
  TaskItem           *item;
  WnckWindowType      type;
  GSList             *w;
  TaskIcon *match      = NULL;
  gint match_score     = 0;
  gint max_match_score = 0;
  gboolean            found_desktop = FALSE;

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

#ifdef DEBUG  
  g_debug ("%s: Window opened: %s",__func__,wnck_window_get_name (window));  
  g_debug ("xid = %lu, pid = %d",wnck_window_get_xid (window),wnck_window_get_pid (window));
#endif  
  /* 
    for some reason the skip tasklist property for the taskmanager toggles briefly
   off and on in certain circumstances.  Nip this in the bud.
   */
  if ( wnck_window_get_pid (window) == getpid() || 
      g_strcmp0 (wnck_window_get_name (window),"awn-applet")==0 )
  {
    return;
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

  // create a new TaskWindow containing the WnckWindow
  item = task_window_new (window);
  g_object_set_qdata (G_OBJECT (window), win_quark, TASK_WINDOW (item));

  priv->windows = g_slist_append (priv->windows, item);
  g_object_weak_ref (G_OBJECT (item), (GWeakNotify)window_closed, manager);

  // see if there is a icon that matches
  for (w = priv->icons; w; w = w->next)
  {
    TaskIcon *taskicon = w->data;

#ifdef DEBUG
    if (!TASK_IS_ICON (taskicon))
      g_debug ("Not a task icon");
#endif
    if (!TASK_IS_ICON (taskicon)) continue;
#ifdef DEBUG
    g_debug ("contains launcher is %d",task_icon_contains_launcher (taskicon) );
#endif
    match_score = task_icon_match_item (taskicon, item);
    if (match_score > max_match_score)
    {
      max_match_score = match_score;
      match = taskicon;
    }
  }
#ifdef DEBUG
  g_debug("Matching score: %i, must be bigger then:%i, groups: %i", max_match_score, 99-priv->match_strength, max_match_score > 99-priv->match_strength);
#endif  
  if  (match
       &&
       (priv->grouping || (task_icon_count_items(match)==1) ) 
       &&
       ( max_match_score > 99-priv->match_strength))
  {
    task_icon_append_item (match, item);
  }
  else
  {
    /* grab the class name.
     look through the various freedesktop system/user dirs for the 
     associated desktop file.  If we find it then dump the associated 
     launcher into the the dialog.
    */
    gchar   *res_name = NULL;
    gchar   *class_name = NULL;
    gchar   *cmd;
    glibtop_proc_args buf;    
    cmd = glibtop_get_proc_args (&buf,wnck_window_get_pid (window),1024);    
    
    icon = task_icon_new (AWN_APPLET (manager));
    task_window_get_wm_class(TASK_WINDOW (item), &res_name, &class_name); 
#ifdef DEBUG
      g_debug ("%s: class name  = %s, res name = %s",__func__,class_name,res_name);
#endif      
    
    if (class_name && strlen (class_name))
    {
      found_desktop = find_desktop (TASK_ICON(icon), class_name);
    }
    
    /*This _may_ result in unacceptable false positives.  Testing.*/
    if (!found_desktop)
    {
      #ifdef DEBUG
      g_debug ("%s:  cmd = '%s'",__func__,);
      #endif
      if (cmd)
      {
        found_desktop = find_desktop (TASK_ICON(icon), cmd);
      }
    }
    if (!found_desktop)
    {
      found_desktop = find_desktop_fuzzy (TASK_ICON(icon),class_name, cmd);
    }
    g_free (cmd);     
    g_free (class_name);
    g_free (res_name);
    task_icon_append_item (TASK_ICON (icon), item);
    priv->icons = g_slist_append (priv->icons, icon);
    gtk_container_add (GTK_CONTAINER (priv->box), icon);

    /* reordening through D&D */
    if(priv->drag_and_drop)
      _drag_add_signals(manager, icon);

    g_object_weak_ref (G_OBJECT (icon), (GWeakNotify)icon_closed, manager);
    g_signal_connect_swapped (icon, 
                              "visible-changed",
                              G_CALLBACK (on_icon_visible_changed), 
                              manager);
    g_signal_connect_swapped (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                              "animation-end", 
                              G_CALLBACK (on_icon_effects_ends), 
                              icon);

    update_icon_visible (manager, TASK_ICON (icon));
  }
}

/*
 * PROPERTIES
 */
static void
task_manager_set_show_all_windows (TaskManager *manager,
                                   gboolean     show_all)
{
  TaskManagerPrivate *priv;
  GSList             *w;
  WnckWorkspace      *space = NULL;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = manager->priv;

  if (priv->show_all_windows == show_all) return;
 
  manager->priv->show_all_windows = show_all;

  if (show_all)
  {
    // Remove signals of workspace changes
    g_signal_handlers_disconnect_by_func(priv->screen, 
                                         G_CALLBACK (on_workspace_changed), 
                                         manager);
    
    // Set workspace to NULL, so TaskWindows aren't tied to workspaces anymore
    space = NULL;
  }
  else
  {
    // Add signals to WnckScreen for workspace changes
    g_signal_connect_swapped (priv->screen, "viewports-changed",
                              G_CALLBACK (on_workspace_changed), manager);
    g_signal_connect_swapped (priv->screen, "active-workspace-changed",
                              G_CALLBACK (on_workspace_changed), manager);

    // Retrieve the current active workspace
    space = wnck_screen_get_active_workspace (priv->screen);
  }

  /* Update the workspace for every TaskWindow.
   * NULL if the windows aren't tied to a workspace anymore */
  for (w = priv->windows; w; w = w->next)
  {
    TaskWindow *window = w->data;
    if (!TASK_IS_WINDOW (window)) continue;
    task_window_set_active_workspace (window, space);
  }
  
  g_debug ("%s", show_all ? "showing all windows":"not showing all windows");
}

/**
 * The property 'show_only_launchers' changed.
 * So update the property and update the visiblity of every icon.
 */
static void
task_manager_set_show_only_launchers (TaskManager *manager, 
                                      gboolean     only_show_launchers)
{
  TaskManagerPrivate *priv;
  GSList             *w;

  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = manager->priv;
  priv->only_show_launchers = only_show_launchers;

  for (w = priv->icons; w; w = w->next)
  {
    TaskIcon *icon = w->data;

    if (!TASK_IS_ICON (icon)) continue;

    update_icon_visible (manager, icon);
  }
  
  g_debug ("%s", only_show_launchers ? "only show launchers":"show everything");
}

/**
 * Checks when launchers got added/removed in the list in gconf/file.
 * It removes the launchers from the task-icons and add those
 * that aren't already on the bar.
 * State: partial - TODO: refresh of a list
 */
static void 
task_manager_refresh_launcher_paths (TaskManager *manager,
                                     GSList      *list)
{
  TaskManagerPrivate *priv;
  GSList *d;
  GSList *i;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
  
  /* FIXME: I guess we should add something to check whether the user has
   * removed a launcher. Make sure we don't remove a launcher which has a 
   * window set, wait till the window is closed
   *
   * FIXME:  This approach just is not going to work..
   *         IMO we should do something similar to 
   *         awn_applet_manager_refresh_applets() in awn-applet-manager.c
   */
  
  for (d = list; d; d = d->next)
  {  
    gboolean found;
    found = FALSE;
    for (i = priv->icons; i ;i = i->next)
    {
      TaskIcon * icon = i->data;
      GSList   * items = task_icon_get_items (icon);
      GSList   * item_iter;
      for (item_iter = items; item_iter; item_iter = item_iter->next)
      {
        TaskItem * item = item_iter->data;
        
        if ( TASK_IS_LAUNCHER (item) )
        {
          if (g_strcmp0 (task_launcher_get_desktop_path(TASK_LAUNCHER(item)),d->data)==0)
          {
            found = TRUE;
          }
        }
      }
    }
    if (!found)
    {
      TaskItem     *launcher = NULL;
      GtkWidget     *icon;
      
      launcher = task_launcher_new_for_desktop_file (d->data);
      icon = task_icon_new (AWN_APPLET (manager));
      task_icon_append_item (TASK_ICON (icon), launcher);
      gtk_container_add (GTK_CONTAINER (priv->box), icon);
      gtk_box_reorder_child (GTK_BOX (priv->box), icon, g_slist_position (list,d));
      priv->icons = g_slist_insert (priv->icons, icon, g_slist_position (list,d));

      g_object_weak_ref (G_OBJECT (icon), (GWeakNotify)icon_closed, manager);
      g_signal_connect_swapped (icon, 
                                "visible-changed",
                                G_CALLBACK (on_icon_visible_changed), 
                                manager);
      g_signal_connect_swapped (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                                "animation-end", 
                                G_CALLBACK (on_icon_effects_ends), 
                                icon);
      
      update_icon_visible (manager, TASK_ICON (icon));

      /* reordening through D&D */
      if(priv->drag_and_drop)
        _drag_add_signals(manager, icon);        
    }
  }
#if 0
  for (d = list; d; d = d->next)
  {
    GtkWidget     *icon;
    TaskItem     *launcher = NULL;

    launcher = task_launcher_new_for_desktop_file (d->data);

    if (!launcher) continue;
    /*Nasty... but can't just disable yet*/
    icon = task_icon_new (AWN_APPLET (manager));
    task_icon_append_item (TASK_ICON (icon), launcher);
    gtk_container_add (GTK_CONTAINER (priv->box), icon);

    priv->icons = g_slist_append (priv->icons, icon);

    g_object_weak_ref (G_OBJECT (icon), (GWeakNotify)icon_closed, manager);
    g_signal_connect_swapped (icon, 
                              "visible-changed",
                              G_CALLBACK (on_icon_visible_changed), 
                              manager);
    g_signal_connect_swapped (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                              "animation-end", 
                              G_CALLBACK (on_icon_effects_ends), 
                              icon);
    
    update_icon_visible (manager, TASK_ICON (icon));

    /* reordening through D&D */
    if(priv->drag_and_drop)
      _drag_add_signals(manager, icon);
  }
#endif       
  for (d = list; d; d = d->next)
    g_free (d->data);
  g_slist_free (list);
}

static void
task_manager_set_match_strength (TaskManager *manager,
                                 gint match_strength)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));
  manager->priv->match_strength = match_strength;
}

static void
task_manager_set_drag_and_drop (TaskManager *manager, 
                                gboolean     drag_and_drop)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  TaskManagerPrivate *priv = manager->priv;
  GSList         *i;

  priv->drag_and_drop = drag_and_drop;

  //connect or dissconnect the dragging signals
  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *icon = i->data;
    
    if (!TASK_IS_ICON (icon)) continue;

    if(drag_and_drop)
    {
      _drag_add_signals (manager, GTK_WIDGET(icon));
    }
    else
    {
      //FIXME: Stop any ongoing move
      _drag_remove_signals (manager, GTK_WIDGET(icon));
    }
  }
  if(drag_and_drop)
  {
    _drag_add_signals (manager, GTK_WIDGET(priv->drag_indicator));
  }
  else
  {
    _drag_remove_signals (manager, GTK_WIDGET(priv->drag_indicator));
  }

  g_debug("%s", drag_and_drop?"D&D is on":"D&D is off");
}


static void
task_manager_set_grouping (TaskManager *manager, 
                                gboolean  grouping)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  TaskManagerPrivate *priv = manager->priv;
  
  priv->grouping = grouping;
}
/**
 * D-BUS functionality
 */

gboolean
task_manager_get_capabilities (TaskManager *manager,
                               GStrv *supported_keys,
                               GError **error)
{
  const gchar *known_keys[] =
  {
    "icon-file",
    "progress",
    "message",
    "visible",
    NULL
  };

  *supported_keys = g_strdupv ((char **)known_keys);

  return TRUE;
}

/**
 * Find the window that corresponds to the given window name.
 * First try to match the application name, then the normal name.
 */
static TaskWindow*
_match_name (TaskManager *manager, const gchar* window)
{
  TaskManagerPrivate *priv;
  WnckApplication *wnck_app = NULL;
  const gchar *name = NULL;
  GSList *w;

  g_return_val_if_fail (TASK_IS_MANAGER (manager), FALSE);
  priv = manager->priv;  
  
  for (w = priv->windows; w; w = w->next)
  {
    TaskWindow *taskwindow = w->data;

    if (!TASK_IS_WINDOW (taskwindow)) continue;

    wnck_app = task_window_get_application (taskwindow);
    if (WNCK_IS_APPLICATION(wnck_app))
    {
      name = wnck_application_get_name(wnck_app);
      if (name && g_ascii_strcasecmp (window, name) == 0) // FIXME: UTF-8 ?!
        return taskwindow;
    }

    name = task_window_get_name (taskwindow);
    if (name && g_ascii_strcasecmp (window, name) == 0) // FIXME: UTF-8 ?!
      return taskwindow;
  }
  
  return NULL;
}

/**
 * Find the window that corresponds to the given xid
 */
static TaskWindow*
_match_xid (TaskManager *manager, gint64 window)
{
  TaskManagerPrivate *priv;
  gint64 xid;
  GSList *w;

  g_return_val_if_fail (TASK_IS_MANAGER (manager), FALSE);
  priv = manager->priv;
  
  for (w = priv->windows; w; w = w->next)
  {
    TaskWindow *taskwindow = w->data;
    
    if (!TASK_IS_WINDOW (taskwindow)) continue;

    xid = task_window_get_xid (taskwindow);
    if (xid && window == xid)
      return taskwindow;
  }
  
  return NULL;
}

gboolean
task_manager_update (TaskManager *manager,
                     GValue *window,
                     GHashTable *hints, /* mappings from string to GValue */
                     GError **error)
{
  TaskManagerPrivate *priv;
  TaskWindow *matched_window = NULL;

  g_return_val_if_fail (TASK_IS_MANAGER (manager), FALSE);
  
  priv = manager->priv;
  
  if (G_VALUE_HOLDS_STRING (window))
  {
    matched_window = _match_name (manager, g_value_get_string (window));
  }
  else if (G_VALUE_HOLDS_INT64 (window))
  {
    matched_window = _match_xid (manager, g_value_get_int64 (window));
  }
  else
  {
    //G_ERROR stuff
    return FALSE;
  }

  if (matched_window)
  {
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, hints);
    while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
      gchar *key_name = (gchar *)key;
      if (strcmp ("icon-file", key_name) == 0)
      {
        //g_debug ("Request to change icon-file...");
        TaskItem *item = TASK_ITEM (matched_window);
        if (item->icon_overlay == NULL)
        {
          item->icon_overlay = awn_overlay_pixbuf_file_new (NULL);
          g_object_set (G_OBJECT (item->icon_overlay),
                        "use-source-op", TRUE,
                        "scale", 1.0, NULL);
          GtkWidget *image = task_item_get_image_widget (item);
          AwnOverlayable *over = AWN_OVERLAYABLE (image);
          awn_overlayable_add_overlay (over,
                                       AWN_OVERLAY (item->icon_overlay));
        }

        const gchar* filename = g_value_get_string (value);
        g_object_set (G_OBJECT (item->icon_overlay),
                      "active", filename && filename[0] != '\0', NULL);
        g_object_set_property (G_OBJECT (item->icon_overlay),
                               "file-name", value);

        // this refreshes the overlays on TaskIcon
        task_item_set_task_icon (item, task_item_get_task_icon (item));
      }
      else if (strcmp ("progress", key_name) == 0)
      {
        //g_debug ("Request to change progress...");
        TaskItem *item = TASK_ITEM (matched_window);
        if (item->progress_overlay == NULL)
        {
          item->progress_overlay = awn_overlay_progress_circle_new ();
          GtkWidget *image = task_item_get_image_widget (item);
          AwnOverlayable *over = AWN_OVERLAYABLE (image);
          awn_overlayable_add_overlay (over,
                                       AWN_OVERLAY (item->progress_overlay));
        }

        g_object_set (G_OBJECT (item->progress_overlay),
                      "active", g_value_get_int (value) != -1, NULL);
        g_object_set_property (G_OBJECT (item->progress_overlay),
                               "percent-complete", value);

        // this refreshes the overlays on TaskIcon
        task_item_set_task_icon (item, task_item_get_task_icon (item));
      }
      else if (strcmp ("message", key_name) == 0)
      {
        //g_debug ("Request to change message...");
        TaskItem *item = TASK_ITEM (matched_window);
        if (item->text_overlay == NULL)
        {
          item->text_overlay = awn_overlay_text_new ();
          g_object_set (G_OBJECT (item->text_overlay),
                        "font-sizing", AWN_FONT_SIZE_LARGE, NULL);
          GtkWidget *image = task_item_get_image_widget (item);
          AwnOverlayable *over = AWN_OVERLAYABLE (image);
          awn_overlayable_add_overlay (over, AWN_OVERLAY (item->text_overlay));
        }

        g_object_set_property (G_OBJECT (item->text_overlay), "text", value);

        // this refreshes the overlays on TaskIcon
        task_item_set_task_icon (item, task_item_get_task_icon (item));
      }
      else if (strcmp ("visible", key_name) == 0)
      {
        g_debug ("Request to change visibility...");
      }
      else
      {
        g_debug ("Taskmanager doesn't understand the key: %s", key_name);
      }
    }
    
    return TRUE;
  }
  else
  {
    g_debug ("No matching window found to update.");

    return FALSE;
  }
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
  //GSList* d;
  //GSList* launchers = NULL;
  //TaskLauncher* launcher;
  //gchar* launcher_path;
  //GError *err = NULL;

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

/*  if (task_icon_is_launcher (priv->dragged_icon))
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
*/

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
