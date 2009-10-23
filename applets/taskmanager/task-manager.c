/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
#include <glib/gstdio.h>

#include <libwnck/libwnck.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>

#include "libawn/gseal-transition.h"

#include "task-manager.h"
#include "task-manager-glue.h"

#include "task-drag-indicator.h"
#include "task-icon.h"
#include "task-settings.h"
#include "xutils.h"
#include "util.h"

#include <X11/extensions/shape.h>

G_DEFINE_TYPE (TaskManager, task_manager, AWN_TYPE_APPLET)

#define TASK_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_MANAGER, \
  TaskManagerPrivate))

//#define DEBUG 1

#define DESKTOP_CACHE_FILENAME ".desktop_cache"

static GQuark win_quark = 0;

struct _TaskManagerPrivate
{
  DesktopAgnosticConfigClient *client;
  TaskSettings    *settings;
  WnckScreen      *screen;

  /*Used by Intellihide*/
  guint           autohide_cookie;  
  GdkWindow       *awn_gdk_window;
  GdkRegion       * awn_gdk_region;
  
  guint           attention_cookie;
  guint           attention_source;
  
  /* Dragging properties */
  TaskIcon          *dragged_icon;
  TaskDragIndicator *drag_indicator;
  gint               drag_timeout;

  /* This is what the icons are packed into */
  GtkWidget  *box;
  GSList     *icons;
  GSList     *windows;
  GHashTable *win_table;
  GHashTable *desktops_table;

  /* Properties */
  GValueArray *launcher_paths;
  gboolean     show_all_windows;
  gboolean     only_show_launchers;
  gboolean     drag_and_drop;
  gboolean     grouping;
  gboolean     intellihide;
  gint         intellihide_mode;
  gint         match_strength;
  gint         attention_autohide_timer;
};

typedef struct
{
  WnckWindow * window;
  TaskManager * manager;
} WindowOpenTimeoutData;

enum
{
  INTELLIHIDE_WORKSPACE,
  INTELLIHIDE_APP,
  INTELLIHIDE_GROUP  
};

enum
{
  PROP_0,

  PROP_SHOW_ALL_WORKSPACES,
  PROP_ONLY_SHOW_LAUNCHERS,
  PROP_LAUNCHER_PATHS,
  PROP_DRAG_AND_DROP,
  PROP_GROUPING,
  PROP_MATCH_STRENGTH,
  PROP_INTELLIHIDE,
  PROP_INTELLIHIDE_MODE,
  PROP_ATTENTION_AUTOHIDE_TIMER
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
                                                  GValueArray *list);
static void task_manager_set_drag_and_drop (TaskManager *manager, 
                                            gboolean     drag_and_drop);

static void task_manager_set_grouping (TaskManager *manager, 
                                            gboolean     grouping);

static void task_manager_set_match_strength (TaskManager *manager, 
                                             gint     drag_and_drop);

static void task_manager_position_changed (AwnApplet *applet, 
                                           GtkPositionType position);
static void task_manager_size_changed   (AwnApplet *applet,
                                         gint       size);
static void task_manager_origin_changed (AwnApplet *applet,
                                         GdkRectangle  *rect,
                                         gpointer       data);

static void task_manager_dispose (GObject *object);

static void task_manager_active_window_changed_cb (WnckScreen *screen,
                                                   WnckWindow *previous_window,
                                                   TaskManager * manager);
static void task_manager_active_workspace_changed_cb (WnckScreen    *screen,
                                                      WnckWorkspace *previous_space,
                                                      TaskManager * manager);
static void task_manager_win_geom_changed_cb (WnckWindow *window, 
                                              TaskManager * manager);
static void task_manager_win_state_changed_cb (WnckWindow * window,
                                               WnckWindowState changed_mask,
                                               WnckWindowState new_state,
                                               TaskManager * manager);


typedef enum 
{
  TASK_MANAGER_ERROR_UNSUPPORTED_WINDOW_TYPE,
  TASK_MANAGER_ERROR_NO_WINDOW_MATCH
} TaskManagerError;

static GQuark task_manager_error_quark   (void);

static GType task_manager_error_get_type (void);

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
      g_value_set_boxed (value, manager->priv->launcher_paths);
      break;
    case PROP_DRAG_AND_DROP:
      g_value_set_boolean (value, manager->priv->drag_and_drop);
      break;

    case PROP_GROUPING:
      g_value_set_boolean (value, manager->priv->grouping);
      break;

    case PROP_INTELLIHIDE:
      g_value_set_boolean (value, manager->priv->intellihide);
      break;

    case PROP_INTELLIHIDE_MODE:
      g_value_set_int (value, manager->priv->intellihide_mode);
      break;

    case PROP_MATCH_STRENGTH:
      g_value_set_int (value, manager->priv->match_strength);
      break;

    case PROP_ATTENTION_AUTOHIDE_TIMER:
      g_value_set_int (value, manager->priv->attention_autohide_timer);
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
      if (manager->priv->launcher_paths)
      {
        g_value_array_free (manager->priv->launcher_paths);
        manager->priv->launcher_paths = NULL;
      }
      manager->priv->launcher_paths = (GValueArray*)g_value_dup_boxed (value);
      task_manager_refresh_launcher_paths (manager,
                                           manager->priv->launcher_paths);
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
      
    case PROP_INTELLIHIDE:
      /* TODO move into a function */
      manager->priv->intellihide = g_value_get_boolean (value);
      if (!manager->priv->intellihide && manager->priv->autohide_cookie)
      {     
        awn_applet_uninhibit_autohide (AWN_APPLET(manager), manager->priv->autohide_cookie);
        manager->priv->autohide_cookie = 0;
      }
      if (manager->priv->intellihide && !manager->priv->autohide_cookie)
      {     
        manager->priv->autohide_cookie = awn_applet_inhibit_autohide (AWN_APPLET(manager),"Intellihide" );
      }      
      break;

    case PROP_INTELLIHIDE_MODE:
      manager->priv->intellihide_mode = g_value_get_int (value);
      break;
      
    case PROP_ATTENTION_AUTOHIDE_TIMER:
      manager->priv->attention_autohide_timer = g_value_get_int (value);
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_manager_constructed (GObject *object)
{
  TaskManagerPrivate *priv;
  GtkWidget          *widget;
  
  G_OBJECT_CLASS (task_manager_parent_class)->constructed (object);
  
  priv = TASK_MANAGER_GET_PRIVATE (object);
  widget = GTK_WIDGET (object);

  priv->desktops_table = g_hash_table_new_full (g_str_hash,g_str_equal,g_free,g_free);

  priv->client = awn_config_get_default_for_applet (AWN_APPLET (object), NULL);

  /* Connect up the important bits */
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "show_all_windows",
                                       object, "show_all_windows", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "only_show_launchers",
                                       object, "only_show_launchers", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "launcher_paths",
                                       object, "launcher_paths", FALSE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "drag_and_drop",
                                       object, "drag_and_drop", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "grouping",
                                       object, "grouping", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "intellihide",
                                       object, "intellihide", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);  
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "intellihide_mode",
                                       object, "intellihide_mode", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);    
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "match_strength",
                                       object, "match_strength", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "attention_autohide_timer",
                                       object, "attention_autohide_timer", FALSE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  
  g_signal_connect (priv->screen,"active-window-changed",
                    G_CALLBACK(task_manager_active_window_changed_cb),object);
  g_signal_connect (priv->screen,"active-workspace-changed",
                    G_CALLBACK(task_manager_active_workspace_changed_cb),object);
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
  obj_class->dispose = task_manager_dispose;

  app_class->position_changed = task_manager_position_changed;
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

  pspec = g_param_spec_boxed ("launcher-paths",
                              "launcher paths",
                              "List of paths to launcher desktop files",
                              G_TYPE_VALUE_ARRAY,
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

  pspec = g_param_spec_boolean ("intellihide",
                                "intellihide",
                                "Intellihide",
                                TRUE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_INTELLIHIDE, pspec);

  pspec = g_param_spec_int ("intellihide_mode",
                                "intellihide mode",
                                "Intellihide mode",
                                0,
                                2,
                                1,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_INTELLIHIDE_MODE, pspec);

  pspec = g_param_spec_int ("match_strength",
                            "match_strength",
                            "How radical matching is applied for grouping items",
                            0,
                            99,
                            0,
                            G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_MATCH_STRENGTH, pspec);
  
  pspec = g_param_spec_int ("attention_autohide_timer",
                            "Attention Autohide Timer",
                            "Number of seconds to inhibit autohide when a window requests attention",
                            0,
                            9999,
                            10,
                            G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ATTENTION_AUTOHIDE_TIMER, pspec);

  g_type_class_add_private (obj_class, sizeof (TaskManagerPrivate));

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                   &dbus_glib_task_manager_object_info);

  dbus_g_error_domain_register (task_manager_error_quark (), NULL, 
                                task_manager_error_get_type ());
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

  priv->settings = task_settings_get_default (AWN_APPLET(manager));
  
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

  /* connect to our origin-changed signal for updating icon geometry */
  g_signal_connect (manager, "origin-changed",
                    G_CALLBACK (task_manager_origin_changed), NULL);
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
                            "display-name", "Task Manager",
                            "uid", uid,
                            "panel-id", panel_id,
                            NULL);
  return manager;
}

/*
 * AwnApplet stuff
 */
static void 
task_manager_position_changed (AwnApplet *applet, 
                               GtkPositionType position)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (applet));
  priv = TASK_MANAGER (applet)->priv;

  if (priv->settings)
    priv->settings->position = position;

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
      task_icon_refresh_icon (icon,size);
  }

  task_drag_indicator_refresh (priv->drag_indicator);
}

static void task_manager_origin_changed (AwnApplet *applet,
                                         GdkRectangle  *rect,
                                         gpointer       data)
{
  TaskManagerPrivate *priv;
  GSList *i;

  g_return_if_fail (TASK_IS_MANAGER (applet));
  priv = TASK_MANAGER (applet)->priv;

  // our origin changed, we need to update icon geometries
#ifdef DEBUG
  g_debug ("Got origin-changed, updating icon geometries...");
#endif

  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *icon = i->data;

    if (TASK_IS_ICON (icon))
      task_icon_schedule_geometry_refresh (icon);
  }
}

static void
task_manager_dispose (GObject *object)
{
  TaskManagerPrivate *priv;

  priv = TASK_MANAGER_GET_PRIVATE (object);

  desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                        object,
                                                        NULL);
  if (priv->autohide_cookie)
  {     
    awn_applet_uninhibit_autohide (AWN_APPLET(object), priv->autohide_cookie);
    priv->autohide_cookie = 0;
  }
  if (priv->awn_gdk_window)
  {
    g_object_unref (priv->awn_gdk_window);
    priv->awn_gdk_window = NULL;
  }

  G_OBJECT_CLASS (task_manager_parent_class)->dispose (object);
}

/*
 * WNCK_SCREEN CALLBACKS
 */

/*
 * This signal is only connected for windows which were of type normal/utility
 * and were initially "skip-tasklist". If they are not skip-tasklist anymore
 * we treat them as newly opened windows.
 * STATE: done
 *
 */
static void
on_window_state_changed (WnckWindow      *window,
                         WnckWindowState  changed_mask,
                         WnckWindowState  new_state,
                         TaskManager     *manager)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  /* test if they don't skip-tasklist anymore*/
  if (!wnck_window_is_skip_tasklist (window))
  {
    g_signal_handlers_disconnect_by_func (window, 
                                          on_window_state_changed, manager);
    on_window_opened (NULL, window, manager);
    return;
  }
}


static gboolean
uninhibit_timer (gpointer manager)
{
  TaskManagerPrivate *priv;
  
  g_return_val_if_fail (TASK_IS_MANAGER (manager),FALSE);
  priv = TASK_MANAGER(manager)->priv;

  awn_applet_uninhibit_autohide (AWN_APPLET(manager),priv->attention_cookie);
  priv->attention_cookie = 0;
  priv->attention_source = 0;
  return FALSE;
}

static void
check_attention_requested (WnckWindow      *window,
                         WnckWindowState  changed_mask,
                         WnckWindowState  new_state,
                         TaskManager     *manager)
{
  TaskManagerPrivate *priv;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  WnckWindowState win_state = wnck_window_get_state (window);
  
  /* inhibit autohide for a length of time.  If it's already inhibited due 
    to attention then reset timer
   */
  if (priv->attention_autohide_timer)
  {
    if ( (win_state  & WNCK_WINDOW_STATE_DEMANDS_ATTENTION) || 
        (win_state & WNCK_WINDOW_STATE_URGENT) )
    {
      if (priv->attention_cookie)
      {
        g_source_remove (priv->attention_source);
      }
      else
      {
        priv->attention_cookie = awn_applet_inhibit_autohide (AWN_APPLET(manager),
                                                              "Attention" );      
      }
      priv->attention_source = g_timeout_add_seconds (priv->attention_autohide_timer,
                                                      uninhibit_timer,manager);
    }
  }
}


/*
 * The active WnckWindow has changed.
 * Retrieve the TaskWindows and update their active state 
 *
 * TODO Store the active TaskWin into a TaskManager field so it can be used
 * for group value in intellihide_mode and minimize code bloat in other places.
 * It'll probably end up being useful to store this info for other purposes also.
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

/*
 * When the property 'show_all_windows' is False,
 * workspace switches are monitored. Whenever one happens
 * all TaskWindows are notified.
 */
static void
on_workspace_changed (TaskManager *manager) /*... has more arguments*/
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
  /*if show_all_windows config key is false then we show/hide different
   icons on workspace switches.  We don't want to play closing 
   animations on them, opening animations are played as it seems to provide
   a better visual cue of what has changed*/
  gboolean do_hide_animation = FALSE;

  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = manager->priv;
  
  if ( task_icon_is_visible (icon) && ( !priv->only_show_launchers || 
        (task_icon_contains_launcher (icon) && !task_icon_count_ephemeral_items(icon))))
  {
    visible = TRUE;
  }

  if ( !task_icon_contains_launcher (icon) )
  {
    if (task_icon_count_items(icon)==0)
    {
      do_hide_animation = TRUE;
    }
  }
  else
  {
    if (task_icon_count_items (icon) <= task_icon_count_ephemeral_items (icon))
    {
      do_hide_animation = TRUE;
    }
  }

  
  if (visible && !gtk_widget_get_visible (GTK_WIDGET(icon)))
  {
    gtk_widget_show (GTK_WIDGET (icon));
    awn_effects_start_ex (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                        AWN_EFFECT_OPENING, 1, FALSE, FALSE);
  }

  if (!visible && gtk_widget_get_visible (GTK_WIDGET(icon)))
  {
    if (!do_hide_animation)
    {
      gtk_widget_hide (GTK_WIDGET(icon));
    }
    else
    {
      awn_effects_start_ex (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                          AWN_EFFECT_CLOSING, 1, FALSE, TRUE);
    } 
  }
}

static void
on_icon_visible_changed (TaskManager *manager, TaskIcon *icon)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  update_icon_visible (manager, icon);
}

static gboolean
_destroy_icon_cb (GtkWidget * icon)
{
  gtk_widget_destroy (icon);
  return FALSE;
}

static void
on_icon_effects_ends (TaskIcon   *icon,
                      AwnEffect   effect,
                      AwnEffects *instance)
{  
  g_return_if_fail (TASK_IS_ICON (icon));

  if (effect == AWN_EFFECT_CLOSING)
  {
    gboolean destroy = FALSE;
    /*conditional operator*/
    if ( !task_icon_contains_launcher (icon) )
    {
      if (task_icon_count_items(icon)==0)
      {
        destroy = TRUE;
      }
    }
    else
    {
      if (task_icon_count_items (icon) <= task_icon_count_ephemeral_items (icon))
      {
        destroy = TRUE;
      }
    }
    if (destroy)
    {
      /*we're done... disconnect this handler or it's going to get called again...
       for this object*/
      g_signal_handlers_disconnect_by_func (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)),
                            G_CALLBACK (on_icon_effects_ends), icon);
      /*
       In the case of simple effects we receive this signal _before_ 
       the TaskItems are finalized.  By using the g_idle_add we defer the 
       destruction of the icon until after the TaskItems are gone (it's just 
       far less nasty than the alternative
      */
      gtk_widget_hide (GTK_WIDGET(icon));
      g_idle_add ((GSourceFunc)_destroy_icon_cb,icon);
    }
    else
    {
      gtk_widget_hide (GTK_WIDGET(icon));
    }
  }
}

/*
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

/*
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


static TaskItem *
get_launcher(gchar * desktop)
{
  TaskItem     *launcher = NULL;  
  DesktopAgnosticFDODesktopEntry *entry=NULL;
  DesktopAgnosticVFSFile *file;

  file = desktop_agnostic_vfs_file_new_for_path (desktop, NULL);
  if (file)
  {
    if (desktop_agnostic_vfs_file_exists (file) )
    {
      entry = desktop_agnostic_fdo_desktop_entry_new_for_file (file, NULL);
    }
    g_object_unref (file);
  }
  if (entry)
  {
    gboolean key_exists = desktop_agnostic_fdo_desktop_entry_key_exists (entry,"NoDisplay");
    if (!key_exists || !desktop_agnostic_fdo_desktop_entry_get_boolean (entry,"NoDisplay"))
    {
      launcher = task_launcher_new_for_desktop_file (desktop);
    }
    g_object_unref (entry);                                                    
  }
  return launcher;
}
/*
See note in util.c
 
 The desktop matching code will be refactored and put into its own object
 early in the 0.6 dev cycle.
 
 */

/*
 Create a path <systemdir><name>.desktop
 Does the desktop file exist (case sensitive)?
 If so then append as an ephemeral item to the item list.  An ephemeral item
 disappears when their are only ephemeral items left in the list.
 
 If the desktop file is not discovered then it will recursively call itself
 for any subdirectories within system_dir while doing a case insensitive match
 on all the filenames.
 
 FIXME: cleanup.
 */
static gchar *
task_icon_check_system_dir_for_desktop (TaskIcon *icon,
                                        gchar * system_dir, 
                                        gchar * name)
{
  gchar * desktop;
  TaskItem     *launcher = NULL;
  GDir      * dir;
  gchar     * result;
 
  if (check_if_blacklisted(name) )
  {
    return NULL;
  }
  desktop = g_strdup_printf ("%s%s.desktop",system_dir,name);
//#define DEBUG
#ifdef DEBUG
  g_debug ("%s: desktop = %s",__func__,desktop);
#endif      
  launcher = get_launcher(desktop);  
  if (launcher)
  {
#ifdef DEBUG
    g_debug ("launcher %p",launcher);
#endif
    task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);
    return desktop;
  }
  g_free (desktop);
  
  dir = g_dir_open (system_dir,0,NULL);
  if (dir)
  {
    const char * fname;	
    while ( (fname = g_dir_read_name (dir)))
    {
//      g_debug ("fname = %s",fname);
      gchar * new_path = g_strdup_printf ("%s%s/",system_dir,fname);      
      if ( g_file_test (new_path,G_FILE_TEST_IS_DIR) )
      {
        result = task_icon_check_system_dir_for_desktop (icon,new_path,name);
        if (result)
        {
          g_free (new_path);
          g_dir_close (dir);
          return result;
        }
      }
      else
      {
        gchar * filename_lower;
        g_free (new_path);
        gchar * name_desktop = g_strdup_printf ("/%s.desktop",name);
        new_path = g_strdup_printf ("%s%s",system_dir,fname);
        desktop = new_path;
        filename_lower = g_utf8_strdown (desktop, -1);
        if ( g_strrstr (filename_lower,name_desktop) )
        {
          launcher = get_launcher(desktop);
          if (launcher)
          {
#ifdef DEBUG
            g_debug ("%s: found %s",__func__,desktop);
#endif
            g_free (filename_lower);
            g_free (new_path);            
            task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);
            return name_desktop;
          }
        }
        g_free (name_desktop);        
        g_free (filename_lower);        
      }
      g_free (new_path);
    }
    g_dir_close (dir);    
  }
  return FALSE;
}

/*
 TODO: refactor into a few smaller functions or possibly just clean up the logic
 with judicious use of goto.
 */
static gchar *
find_desktop (TaskIcon *icon, gchar * name)
{
  gchar * lower;
  gchar * desktop;
  const gchar* const * system_dirs = g_get_system_data_dirs ();
  GStrv   iter;
  GStrv   tokens;
  TaskItem     *launcher = NULL;
  gchar * name_stripped = NULL;
  const gchar * extensions[] = { ".py",".pl",".exe",NULL};
  gchar * result;
  
  g_return_val_if_fail (name,FALSE);
  
#ifdef DEBUG
  g_debug ("%s:  name = %s",__func__,name);
#endif
  
  /*if the name has an of extensions then get rid of them*/
  for (iter = (GStrv)extensions; *iter; iter++)
  {
    gchar * filename_lower = g_utf8_strdown (*iter, -1);
    if ( g_strrstr (name,filename_lower) )
    {
      g_free (filename_lower);
      name_stripped = g_strdup (name);
      *g_strrstr (name_stripped,*iter) = '\0';
      break;
    }
    g_free (filename_lower);
  }  

  lower = g_utf8_strdown (name_stripped?name_stripped:name, -1);
  g_free (name_stripped);
//#define DEBUG
#ifdef DEBUG
  g_debug ("%s: name = %s",__func__,name);
  g_debug ("%s: lower = %s",__func__,lower);
#endif      

  /*
   Check in the application dirs of all System dirs
   If we find the desktop file it will get added as an ephemeral item and 
   TRUE will be returned.
   */
  for (iter = (GStrv)system_dirs; *iter; iter++)
  {
    gchar * applications_dir = g_strdup_printf ("%s/applications/",*iter);
    result = task_icon_check_system_dir_for_desktop (icon,applications_dir,lower);
    if (result)
    {
      g_free (lower);
      g_free (applications_dir);
      return result;
    }
    g_free (applications_dir);
  }

  /*
   Didn't find a desktop file in the system dirs...
   So check the user's data dir directory for an applications subdir containing
   our desktop file
   */
  
  desktop = g_strdup_printf ("%s/applications/%s.desktop",g_get_user_data_dir (),lower);
  
#ifdef DEBUG  
  g_debug ("%s: local desktop search = %s",__func__,desktop);
#endif
  launcher = get_launcher (desktop);
  if (launcher)
  {
#ifdef DEBUG
    g_debug ("launcher %p",launcher);
#endif
    task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);
    g_free  (lower);
    return desktop;
  }
  else
  {
    gchar * local_dir = g_strdup_printf ("%s/applications/",g_get_user_data_dir ());
    result = task_icon_check_system_dir_for_desktop (icon,local_dir,lower);
    if (result)
    {
      g_free (local_dir);
      g_free (lower);
      return result;
    }
    g_free (local_dir);
  }
  g_free (desktop);
  
  /*
   Didn't find anything... so attempt to strip out odd characters and retry 
   the process if any were successfully removed.
   */
  g_strdelimit (lower,"-.:,=+_~!@#$%^()[]{}'",' ');
  tokens = g_strsplit (lower," ",-1);
  if (tokens)
  {
    gchar * stripped = g_strjoinv(NULL,tokens);
    g_strfreev (tokens);    
    if (g_strcmp0 (stripped, name) !=0)
    {
      result = find_desktop (icon,stripped);
      if (result)
      {
        g_free (lower);
        g_free (stripped);
        return result;
      }
    }  
    g_free (stripped);    
  }
  g_free (lower);  
  return FALSE;
}

/*
 Similar to find_desktop but attempting to match using regexes instead of 
 direct string comparisions
 This is fuzzier so if a desktop file is matched it will do a comparision
 of Exec to the cmd... if they don't match then the desktop file match will
 be discarded.
 */
static gchar *
find_desktop_fuzzy (TaskIcon *icon, gchar * class_name, gchar *cmd)
{
  gchar * lower;
  gchar * lower_regex_escaped;
  gchar * desktop_regex_str;
  GRegex  * desktop_regex;
  const gchar* const * system_dirs = g_get_system_data_dirs ();
  GStrv   iter;
  TaskItem     *launcher = NULL;
  gchar ** tokens;
  
  g_return_val_if_fail (class_name,NULL);
  lower = g_utf8_strdown (class_name, -1);
  lower_regex_escaped = g_regex_escape_string (lower,-1);
//#define DEBUG 1
#ifdef DEBUG
  g_debug ("%s: wm class = %s",__func__,class_name);
  g_debug ("%s: lower = %s",__func__,lower);
#endif      

  g_return_val_if_fail (strlen (lower),NULL);
  /*
   TODO compile the regex
   */
  desktop_regex_str = g_strdup_printf (".*%s.*desktop",lower_regex_escaped);
  desktop_regex = g_regex_new (desktop_regex_str,G_REGEX_CASELESS,0,NULL);
#ifdef DEBUG
    g_debug ("%s: desktop regex = %s",__func__,desktop_regex_str);
#endif      
  g_free (desktop_regex_str);
  desktop_regex_str = NULL;
  g_free (lower_regex_escaped);
  lower_regex_escaped = NULL;
  g_return_val_if_fail (desktop_regex,NULL);
  
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
          // TODO handle GErrors.  Do we really want to handle them?
          DesktopAgnosticVFSFile *file = desktop_agnostic_vfs_file_new_for_path (full_path, NULL);
          DesktopAgnosticFDODesktopEntry * desktop = desktop_agnostic_fdo_desktop_entry_new_for_file (file, NULL);
          if (desktop)
          {
            const gchar * fdo_options[] = {"%f","%F","%u","%U","%d","%D","%n","%N","%i","%c","%k","%v","%m",NULL};
            const gchar ** i;
            gchar ** j;
            gchar * exec = NULL;
            gchar * exec_regex_escaped = NULL;
            gchar ** exec_tokens;
            gchar * exec_base;
            gchar * cmd_base;
            gchar ** cmd_tokens;

            if (desktop_agnostic_fdo_desktop_entry_key_exists (desktop,"Exec"))
            {
              exec = desktop_agnostic_fdo_desktop_entry_get_string (desktop, "Exec");
              if (exec)
              {
                exec_regex_escaped = g_regex_escape_string (exec,-1);            
              }
            }
            g_free (exec);
            exec = NULL;
            if (!exec_regex_escaped)
            {
              continue;
            }
            g_object_unref (desktop);
            for (i=fdo_options; *i;i++)
            {
              exec_tokens = g_strsplit (exec_regex_escaped,*i,-1);
              g_free (exec_regex_escaped);
              exec_regex_escaped = g_strjoinv ("",exec_tokens);
              g_strfreev (exec_tokens);
            }
            exec_tokens = g_strsplit(exec_regex_escaped," ",-1);
            exec_base = g_path_get_basename (exec_tokens[0]);
            g_free (exec_tokens[0]);
            exec_tokens[0] = exec_base;
            for (j=exec_tokens; *j; j++)
            {
              g_strstrip (*j);
            }
            g_free (exec_regex_escaped);
            exec_regex_escaped = g_strjoinv (".*",exec_tokens);
            g_strfreev (exec_tokens);

            cmd_tokens = g_strsplit (cmd," ",-1);
            cmd_base = g_path_get_basename (cmd_tokens[0]);
            g_free (cmd_tokens[0]);
            cmd_tokens [0] = cmd_base;
            for (j = cmd_tokens; *j; j++)
            {
              gchar * str = *j;
              g_strstrip (str);
              if ( str[0] == '-')
              {
                *str = '\0';
              }
            }
            cmd_base = g_strjoinv (" ",cmd_tokens);
            g_strfreev (cmd_tokens);
#ifdef DEBUG            
            g_debug ("%s:  exec =   '%s'",__func__,exec_regex_escaped);
            g_debug ("%s:  cmd_base =   '%s'",__func__,cmd_base);
#endif              
            if ( g_regex_match_simple (exec_regex_escaped, cmd,G_REGEX_CASELESS,0) )              
            {
              launcher = get_launcher (full_path);
              if (launcher)
              {
                task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);
                g_regex_unref (desktop_regex);
                g_free (exec_regex_escaped);
                g_free (lower);
                g_free (cmd_base);
                return full_path;
              }
            }
            g_free (cmd_base);
            cmd_base = NULL;
            g_free (exec_regex_escaped);
            exec_regex_escaped = NULL;
          }
          g_object_unref (file);
          g_free (full_path);
          full_path = NULL;
        }
      }
      g_dir_close (dir);
    }
    g_free (dir_name);
    dir_name = NULL;
  }
  g_regex_unref (desktop_regex);
  
  tokens = g_strsplit (lower,"-",-1);
  g_free (lower);  
  lower = NULL;
  if (tokens && tokens[0] && tokens[1] )
  {
    gchar * result = find_desktop_fuzzy (icon, tokens[0], cmd);
    g_strfreev (tokens);
    if (result)
    {
      return result;
    }
  }
  
  return NULL;
//#undef DEBUG
  
}

/*
 If other attempts to find a desktop file fail then this function will
 typically get called which use tables of data to match problem apps to 
 desktop file names (refer to util.c)
 */
static gchar *
find_desktop_special_case (TaskIcon *icon, gchar * cmd, gchar *res_name, 
                                 gchar * class_name,const gchar *title)
{
  gchar * result = NULL;
  GSList * desktops = get_special_desktop_from_window_data (cmd,
                                                                  res_name,
                                                                  class_name,
                                                                  title);
  if (desktops)
  {
    GSList * iter;
    for (iter = desktops; iter; iter = iter->next)
    {
      result = find_desktop (icon,iter->data);
      if ( !result && (strlen (cmd) > 8) ) 
      {
        result = find_desktop_fuzzy (icon,iter->data,cmd);
      }
      if (result)
      {
        break;
      }
    }
    g_slist_foreach (desktops,(GFunc)g_free,NULL);
    g_slist_free (desktops);
  }

  return result;
}

static gchar *
search_desktop_cache (TaskManager * manager, TaskIcon * icon,gchar * class_name, 
                      gchar * res_name, gchar * cmd, gchar *id)
{
  TaskManagerPrivate *priv;
  gchar * key;
  gchar * desktop_path;
  TaskItem *launcher = NULL;

  priv = manager->priv;
  key = g_strdup_printf("%s::%s::%s::%s",class_name,res_name,cmd,id);
  desktop_path = g_hash_table_lookup (priv->desktops_table,key);

  if (desktop_path)
  {
    launcher = get_launcher (desktop_path);
    if (launcher)
    {
      task_icon_append_ephemeral_item (TASK_ICON (icon), launcher);      
      return g_strdup (desktop_path);
    }
  }
  return NULL;
}

static void
process_window_opened (WnckWindow    *window,
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
  gchar*             found_desktop = FALSE;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  g_return_if_fail (WNCK_IS_WINDOW (window));

  g_signal_handlers_disconnect_by_func(window, process_window_opened, manager);
  
  priv = manager->priv;
  type = wnck_window_get_window_type (window);
    
  /*
   For Intellihide.  It may be ok to connect this after we the switch (where
   some windows are filtered out)
   */
  g_signal_connect (window,"geometry-changed",
                  G_CALLBACK(task_manager_win_geom_changed_cb),manager);
  g_signal_connect (window,"state-changed",
                    G_CALLBACK(task_manager_win_state_changed_cb),manager);
  
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
   TODO:  Investigate wth this is happening...  it bothers me.
   */
  if ( wnck_window_get_pid (window) == getpid() || 
      g_strcmp0 (wnck_window_get_name (window),"awn-applet")==0 )
  {
    return;
  }
  /* 
   * If it's skip tasklist, connect to the state-changed signal and see if
   * it ever becomes a normal window
   *
   * NOTE:  Shouldn't we just be connecting everything that gets to this point
   * to state-changed.  Do we have the case of as window switching from in the 
   * tasklist to skip_tasklist?   TODO:  investigate.
   NOTE:  This _still_ bothers me.
   */
  if (wnck_window_is_skip_tasklist (window))
  {
    g_signal_connect (window, "state-changed", 
                      G_CALLBACK (on_window_state_changed), manager);    
    return;
  }

  g_signal_connect (window, "state-changed", 
                    G_CALLBACK (check_attention_requested), manager);    
  
  /* create a new TaskWindow containing the WnckWindow*/
  item = task_window_new (AWN_APPLET(manager), window);
  g_object_set_qdata (G_OBJECT (window), win_quark, TASK_WINDOW (item));

  priv->windows = g_slist_append (priv->windows, item);
  g_object_weak_ref (G_OBJECT (item), (GWeakNotify)window_closed, manager);

  /* see if there is a icon that matches*/
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
  /*
   if match is not 0
   and 
   we're doing grouping || there are 1 items in the TaskIcon and that is a Launcher
   and 
   the matching meets the match strength
   */
  if  (match
       &&
       (priv->grouping || ( (task_icon_count_items(match)==1) && (task_icon_contains_launcher (match)) ) ) 
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
    gchar   *full_cmd;
    gchar   *cmd_basename = NULL;
    gchar * id = NULL;
    glibtop_proc_args buf;
    
    cmd = glibtop_get_proc_args (&buf,wnck_window_get_pid (window),1024);    
    full_cmd = get_full_cmd_from_pid (wnck_window_get_pid (window));
    if (cmd)
    {
      cmd_basename = g_path_get_basename (cmd);
    }
    
    icon = task_icon_new (AWN_APPLET (manager));
    task_window_get_wm_class(TASK_WINDOW (item), &res_name, &class_name);

#ifdef DEBUG
      g_debug ("%s: class name  = %s, res name = %s",__func__,class_name,res_name);
#endif      
    
    id = get_special_id_from_window_data (cmd, res_name, class_name, wnck_window_get_name(window));
    found_desktop = search_desktop_cache (manager,TASK_ICON(icon),class_name,res_name,cmd_basename,id);
/*
     Possible TODO:
     Check for saved signature.
     If we save a signature,desktopfile pair when we successfully discover a
     desktop file then we don't need to search through a bunch of directories
     for an app multiple times...
     Question: How bad is the performance hit atm?
     
     TODO Note:  Desktop file lookup is important if we want to be able to offer
     a favourite apps options (automatically show the top 4 apps in the bar for 
     example).   
     
     We may as well implement both the signature/desktop caching along with 
     stats for favourite app usage in a unified manner.
*/

    /*
     Various permutations on finding a desktop file for the app.
     
     Class name may eventually be shown to give a false positive for something.
     
     */
    if (!found_desktop)
    {
      if (res_name && strlen (res_name))
      {
        found_desktop = find_desktop (TASK_ICON(icon), res_name);
      }

      if (!found_desktop && class_name && strlen (class_name))
      {
        found_desktop = find_desktop (TASK_ICON(icon), class_name);
      }    
      /*This _may_ result in unacceptable false positives.  Testing.*/
      if (!found_desktop && full_cmd && strlen (full_cmd))
      {
        #ifdef DEBUG
        g_debug ("%s:  full cmd = '%s'",__func__,full_cmd);
        #endif
        found_desktop = find_desktop (TASK_ICON(icon), full_cmd);
      }
      if (!found_desktop && cmd && strlen (cmd))
      {
        #ifdef DEBUG
        g_debug ("%s:  cmd = '%s'",__func__,cmd);
        #endif
        found_desktop = find_desktop (TASK_ICON(icon), cmd);
      }
      
      if (!found_desktop && full_cmd && strlen (full_cmd) )
      {
        found_desktop = find_desktop_fuzzy (TASK_ICON(icon),class_name, full_cmd);
      }
      if (!found_desktop && cmd && strlen (cmd))
      {
        found_desktop = find_desktop_fuzzy (TASK_ICON(icon),class_name, cmd);
      }
      if (!found_desktop && full_cmd && strlen (full_cmd) )
      {
        found_desktop = find_desktop_fuzzy (TASK_ICON(icon),res_name, full_cmd);
      }
      if (!found_desktop && cmd && strlen (cmd))
      {
        found_desktop = find_desktop_fuzzy (TASK_ICON(icon),res_name, cmd);
      }
      
      if (!found_desktop && cmd_basename && strlen (cmd_basename) )
      {
        found_desktop = find_desktop (TASK_ICON(icon), cmd_basename);
      }
      
      if (!found_desktop)
      {
        found_desktop = find_desktop_special_case (TASK_ICON(icon),full_cmd,res_name,
                                                   class_name,
                                                   wnck_window_get_name (window));
      }
      if (!found_desktop)
      {
        found_desktop = find_desktop_special_case (TASK_ICON(icon),cmd,res_name,
                                                   class_name,
                                                   wnck_window_get_name (window));
      }
      if (found_desktop)
      {
        gchar * key = g_strdup_printf("%s::%s::%s::%s",class_name,res_name,cmd_basename,id);
        
        g_hash_table_insert (priv->desktops_table, key,found_desktop);
      }
    }
/*
     Possible TODO
     if found and signature has not already been saved then save it.
*/
    g_free (id);    
    g_free (full_cmd);
    g_free (cmd);     
    g_free (class_name);
    g_free (res_name);
    g_free (cmd_basename);
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
  
static gboolean
_wait_for_name_change_timeout (WindowOpenTimeoutData * data)
{
  gchar * res_name = NULL;
  gchar * class_name = NULL;

  _wnck_get_wmclass (wnck_window_get_xid (data->window),
                     &res_name, &class_name);
  /*only go ahead and process if the title is still useless*/
  if (get_special_wait_from_window_data (res_name, 
                                         class_name,
                                         wnck_window_get_name(data->window) ) )
  {
    process_window_opened (data->window,data->manager);
  }  
  g_free (res_name);
  g_free (class_name);
  return FALSE;
}

/*
 * Whenever a new window gets opened it will try to place it
 * in an awn-icon or will create a new awn-icon.
 * State: adjusted
 *
 * TODO: document all the possible match ratings in one place.  Evaluate if they
 * are sane.
 */
static void 
on_window_opened (WnckScreen    *screen, 
                  WnckWindow    *window,
                  TaskManager   *manager)
{
  /*this function is about certain windows setting a very unexpected, unuseful,
   title in certain circumstances when it first opens then changing it almost
   immediately to something that is useful.  get_special_wait_from_window_data()
   checks for one of those cases (open office being opened with a through a 
   data file is one), and if that is the situations it connect a "name-change" 
   signal and defers processing till then (after 500ms it will just go ahead*/
  gchar * res_name = NULL;
  gchar * class_name = NULL;
  WindowOpenTimeoutData * win_timeout_data;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  g_return_if_fail (WNCK_IS_WINDOW (window));
  
  win_timeout_data = g_malloc (sizeof (WindowOpenTimeoutData));
  win_timeout_data->window = window;
  win_timeout_data->manager = manager;
  _wnck_get_wmclass (wnck_window_get_xid (window),
                     &res_name, &class_name);
  if (get_special_wait_from_window_data (res_name, 
                                         class_name,
                                         wnck_window_get_name(window) ) )
  {
    g_signal_connect (window,"name-changed",G_CALLBACK(process_window_opened),manager);
    g_timeout_add (500,(GSourceFunc)_wait_for_name_change_timeout,win_timeout_data);
  }
  else
  {
    process_window_opened (window,manager);
  }
  g_free (res_name);
  g_free (class_name);  
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
    /* Remove signals of workspace changes*/
    g_signal_handlers_disconnect_by_func(priv->screen, 
                                         G_CALLBACK (on_workspace_changed), 
                                         manager);
    
    /* Set workspace to NULL, so TaskWindows aren't tied to workspaces anymore*/
    space = NULL;
  }
  else
  {
    /* Add signals to WnckScreen for workspace changes*/
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
    if (!TASK_IS_WINDOW (window)) 
    {
      continue;
    }
    task_window_set_active_workspace (window, space);
  }
}

/*
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

    if (!TASK_IS_ICON (icon)) 
    {
      continue;
    }

    update_icon_visible (manager, icon);
  }
}

void 
task_manager_remove_task_icon (TaskManager  *manager, GtkWidget *icon)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
  
  priv->icons = g_slist_remove (priv->icons,icon);
}

void
task_manager_append_launcher(TaskManager  *manager, const gchar * launcher_path)
{
  TaskManagerPrivate *priv;
  GValueArray *launcher_paths;
  GValue val = {0,};
  
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  g_object_get (G_OBJECT (manager), "launcher_paths", &launcher_paths, NULL);
  g_value_init (&val, G_TYPE_STRING);
  g_value_set_string (&val, launcher_path);
  launcher_paths = g_value_array_append (launcher_paths, &val);
  g_object_set (G_OBJECT (manager), "launcher_paths", launcher_paths, NULL);
  g_value_unset (&val);
  task_manager_refresh_launcher_paths (manager, launcher_paths);
  g_value_array_free (launcher_paths);
}

/*
 * Checks when launchers got added/removed in the list in gconf/file.
 * It removes the launchers from the task-icons and add those
 * that aren't already on the bar.
 */
static void
task_manager_refresh_launcher_paths (TaskManager *manager,
                                     GValueArray *list)
{
  TaskManagerPrivate *priv;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
  /*
   Find launchers in the the launcher list do not yet have a TaskIcon and 
   add them
   */
  for (guint idx = 0; idx < list->n_values; idx++)
  {
    gchar *path;
    gboolean found;

    path = g_value_dup_string (g_value_array_get_nth (list, idx));
    found = FALSE;

    for (GSList *icon_iter = priv->icons;
         icon_iter != NULL;
         icon_iter = icon_iter->next)
    {
      GSList *items = task_icon_get_items (TASK_ICON (icon_iter->data));

      for (GSList *item_iter = items;
           item_iter != NULL;
           item_iter = item_iter->next)
      {
        TaskItem *item = item_iter->data;

        if (TASK_IS_LAUNCHER (item) &&
            g_strcmp0 (task_launcher_get_desktop_path (TASK_LAUNCHER (item)),
                       path) == 0)
        {
          gint cur_pos;
          found = TRUE;
          cur_pos = g_slist_position (priv->icons,icon_iter);
          if (cur_pos != (gint)idx)
          {
            TaskIcon * icon = icon_iter->data;
            priv->icons = g_slist_remove_link (priv->icons,icon_iter);
            priv->icons = g_slist_insert (priv->icons,icon,idx);
            gtk_box_reorder_child (GTK_BOX (priv->box), GTK_WIDGET(icon), idx);            
          }
          break;
        }
      }
      if (found)
      {
        break;
      }
    }
    if (!found)
    {
      TaskItem  *launcher = NULL;
      GtkWidget *icon;

      launcher = task_launcher_new_for_desktop_file (path);
      
      if (launcher)
      {
        icon = task_icon_new (AWN_APPLET (manager));
        task_icon_append_item (TASK_ICON (icon), launcher);
        gtk_container_add (GTK_CONTAINER (priv->box), icon);
        gtk_box_reorder_child (GTK_BOX (priv->box), icon, idx);
        priv->icons = g_slist_insert (priv->icons, icon, idx);

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
        if (priv->drag_and_drop)
        {
          _drag_add_signals(manager, icon);
        }
      }
      else
      {
        g_debug ("%s: Bad desktop file '%s'", __func__, path);
      }
    }
    g_free (path);
  }
  
  /*
   Find non-ephemeral launchers in the TaskIcons that are not represented in the
   launcher list and make them ephemeral  (Removes them from the TaskIcons once
   they contain no TaskWindows)
   */
  for (GSList *icon_iter = priv->icons;
       icon_iter != NULL;
       icon_iter = icon_iter->next)
  {
    TaskLauncher * launcher = TASK_LAUNCHER(task_icon_get_launcher (icon_iter->data));
    gboolean found;
    const gchar * launcher_name = NULL;
    found = FALSE;
    if (launcher)
    {
      launcher_name = task_launcher_get_desktop_path (launcher);
      for (guint idx = 0; idx < list->n_values; idx++)
      {
        gchar *path;
        path = g_value_dup_string (g_value_array_get_nth (list, idx));
        if (g_strcmp0(launcher_name,path)==0)
        {
          found = TRUE;
        }
        g_free (path);
      }
    } 
    if (launcher && !found)
    {
      if ( !task_icon_count_ephemeral_items (icon_iter->data) )
      {
        /*if there are only ephemeral items in the icon then it will 
         trigger it's removal.  Otherwise the it will be removed when it is
         no longer managing ay TaskWindows*/        
        task_icon_increment_ephemeral_count (icon_iter->data);
        icon_iter = priv->icons;
      }
    }
  }
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

  /*connect or dissconnect the dragging signals*/
  for (i = priv->icons; i; i = i->next)
  {
    TaskIcon *icon = i->data;
    
    if (!TASK_IS_ICON (icon)) 
    {
      continue;
    }

    if(drag_and_drop)
    {
      _drag_add_signals (manager, GTK_WIDGET(icon));
    }
    else
    {
      /*FIXME: Stop any ongoing move*/
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
    //"visible", // FIXME: uncomment once it's implemented
    NULL
  };

  *supported_keys = g_strdupv ((char **)known_keys);

  return TRUE;
}

/*
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

    if (!TASK_IS_WINDOW (taskwindow)) 
    {
      continue;
    }
    wnck_app = task_window_get_application (taskwindow);
    if (WNCK_IS_APPLICATION(wnck_app))
    {
      name = wnck_application_get_name(wnck_app);
      if (name && g_ascii_strcasecmp (window, name) == 0) 
      {/* FIXME: UTF-8 ?!*/
        return taskwindow;
      }
    }
    name = task_window_get_name (taskwindow);
    if (name && g_ascii_strcasecmp (window, name) == 0) /* FIXME: UTF-8 ?!*/
    {
      return taskwindow;
    }
  }
  return NULL;
}

/*
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

static GdkRegion*
xutils_get_input_shape (GdkWindow *window)
{
  GdkRegion *region = gdk_region_new ();

  GdkRectangle rect;
  XRectangle *rects;
  int count = 0;
  int ordering = 0;

  gdk_error_trap_push ();
  rects = XShapeGetRectangles (GDK_WINDOW_XDISPLAY (window),
                               GDK_WINDOW_XID (window),
                               ShapeInput, &count, &ordering);
  gdk_error_trap_pop ();

  for (int i=0; i<count; ++i)
  {
    rect.x = rects[i].x;
    rect.y = rects[i].y;
    rect.width = rects[i].width;
    rect.height = rects[i].height;

    gdk_region_union_with_rect (region, &rect);
  }
  if (rects) free(rects);

  return region;
}

/* 
 Governs the panel autohide when Intellihide is enabled.
 If a window in the relevant window list intersects with the awn panel then
 autohide will be uninhibited otherwise it will be inhibited.
 */

static void
task_manager_check_for_intersection (TaskManager * manager,
                                     WnckWorkspace * space,
                                     WnckApplication * app)
{
  TaskManagerPrivate  *priv;
  GList * windows;
  GList * iter;
  gboolean  intersect = FALSE;
  GdkRectangle awn_rect;
  gint depth;
  gint64 xid; 
  GdkRegion * updated_region;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  /*
   Generate a GdkRegion for the awn panel_size
   */
  if (!priv->awn_gdk_window)
  {
    g_object_get (manager, "panel-xid", &xid, NULL);    
    priv->awn_gdk_window = gdk_window_foreign_new ( xid);
  }
  g_return_if_fail (priv->awn_gdk_window);
  gdk_window_get_geometry (priv->awn_gdk_window,&awn_rect.x,
                           &awn_rect.y,&awn_rect.width,
                           &awn_rect.height,&depth);  
  /*
   gdk_window_get_geometry gives us an x,y or 0,0
   Fix that using get root origin.
   */
  gdk_window_get_root_origin (priv->awn_gdk_window,&awn_rect.x,&awn_rect.y);
  /*
   We cache the region for reuse for situations where the input mask is an empty
   region when the panel is hidden.  In that case we reuse the last good 
   region.
   */
  updated_region = xutils_get_input_shape (priv->awn_gdk_window);
  g_return_if_fail (updated_region);
  if (gdk_region_empty(updated_region))
  {
    gdk_region_destroy (updated_region);
  }
  else
  {
    if (priv->awn_gdk_region)
    {
      gdk_region_destroy (priv->awn_gdk_region);
    }
    priv->awn_gdk_region = updated_region; 
    gdk_region_offset (priv->awn_gdk_region,awn_rect.x,awn_rect.y);    
  }
  
  /*
   Get the list of windows to check for intersection
   */
  switch (priv->intellihide_mode)
  {
    case INTELLIHIDE_WORKSPACE:
      windows = wnck_screen_get_windows (priv->screen);
      break;
    case INTELLIHIDE_GROUP:  /*TODO... Implement this for now same as app*/
    case INTELLIHIDE_APP:
    default:
      windows = wnck_application_get_windows (app);
      break;
  }
  
  /*
   Check window list for intersection... ignoring skip tasklist and those on
   non-active workspaces
   */
  for (iter = windows; iter; iter = iter->next)
  {
    GdkRectangle win_rect;

    if (!wnck_window_is_visible_on_workspace (iter->data,space))
    {
      continue;
    }
    if (wnck_window_is_minimized(iter->data) )
    {
      continue;
    }
    if ( wnck_window_get_window_type (iter->data) == WNCK_WINDOW_DESKTOP )
    {
      continue;
    }    
    /*
     It may be a good idea to go the same route as we go with the 
     panel to get the GdkRectangle.  But in practice it's _probably_
     not necessary
     */
    wnck_window_get_geometry (iter->data,&win_rect.x,
                              &win_rect.y,&win_rect.width,
                              &win_rect.height);

    if (gdk_region_rect_in (priv->awn_gdk_region, &win_rect) != 
          GDK_OVERLAP_RECTANGLE_OUT)
    {
#ifdef DEBUG
      g_debug ("Intersect with %s, %d",wnck_window_get_name (iter->data),
               wnck_window_get_pid(iter->data));
#endif
      intersect = TRUE;
      break;
    }
  }
  
  /*
   Allow panel to hide (if necessary)
   */
  if (intersect && priv->autohide_cookie)
  {     
    awn_applet_uninhibit_autohide (AWN_APPLET(manager), priv->autohide_cookie);
#ifdef DEBUG
    g_debug ("me eat cookie: %u",priv->autohide_cookie);
#endif
    priv->autohide_cookie = 0;
  }
  
  /*
   Inhibit Hide if not already doing so
   */
  if (!intersect && !priv->autohide_cookie)
  {
    priv->autohide_cookie = awn_applet_inhibit_autohide (AWN_APPLET(manager), "Intellihide");
#ifdef DEBUG    
    g_debug ("cookie is %u",priv->autohide_cookie);
#endif
  }

}

/*
  Active window has changed.  If intellhide is active we need to check for 
 window instersections
 */
static void 
task_manager_active_window_changed_cb (WnckScreen *screen,
                                                   WnckWindow *previous_window,
                                                   TaskManager * manager)
{
  TaskManagerPrivate  *priv;
  WnckWindow          *win;
  WnckApplication     *app;
  WnckWorkspace       *space;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  if (!priv->intellihide)
  {
/*    g_warning ("%s: Intellihide callback invoked with Intellihide off",__func__);*/
    return;
  }
  
  win = wnck_screen_get_active_window (screen);
  if (!win)
  {
    /*
     This tends to happen when the last window on workspace is moved to a
     different workspace or minimized.  In which case we have a problem if 
     we had intersection and the panel was hidden, it will continue hide.
     So inhibit the autohide if there is no active window.
     */
    if (!priv->autohide_cookie)
    {
      priv->autohide_cookie = awn_applet_inhibit_autohide (AWN_APPLET(manager), "Intellihide");
#ifdef DEBUG    
      g_debug ("%s: cookie is %u",__func__,priv->autohide_cookie);
#endif
    }
    return;
  }
  app = wnck_window_get_application (win);
  space = wnck_screen_get_active_workspace (screen);
  task_manager_check_for_intersection (manager,space,app);
}
/*
 Workspace changed... check window intersections for new workspace if Intellidide
 is active 
 */
static void 
task_manager_active_workspace_changed_cb (WnckScreen    *screen,
                                                      WnckWorkspace *previous_space,
                                                      TaskManager * manager)
{
  TaskManagerPrivate *priv;
  WnckApplication     *app;
  WnckWorkspace       *space;
  WnckWindow          *win;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
  if (!priv->intellihide)
  {
/*    g_warning ("%s: Intellihide callback invoked with Intellihide off",__func__);    */
    return;
  }
  win = wnck_screen_get_active_window (screen);
  if (!win)
  {
    if (!priv->autohide_cookie)
    {
      priv->autohide_cookie = awn_applet_inhibit_autohide (AWN_APPLET(manager), "Intellihide");
#ifdef DEBUG    
      g_debug ("%s: cookie is %u",__func__,priv->autohide_cookie);
#endif
    }    
    return;
  }
  
  app = wnck_window_get_application (win);
  space = wnck_screen_get_active_workspace (screen);

  task_manager_check_for_intersection (manager,space,app);
}
/*
 A window's geometry has channged.  If Intellihide is active then check for
 intersections
 */
static void
task_manager_win_geom_changed_cb (WnckWindow *window, TaskManager * manager)
{
/*
   TODO... only check for intersect if window is in the active application.
   practically speaking it should be in most cases */
  TaskManagerPrivate  *priv;
  WnckWindow          *win;
  WnckApplication     *app;
  WnckWorkspace       *space;
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
 
  if (!priv->intellihide)
  {
/*    g_warning ("%s: Intellihide callback invoked with Intellihide off",__func__);*/
    return;
  }
  win = wnck_screen_get_active_window (priv->screen);
  if (!win)
  {
    return;
  }
  app = wnck_window_get_application (win);
  space = wnck_screen_get_active_workspace (priv->screen);
  task_manager_check_for_intersection (manager,space,app);  
}

static void task_manager_win_state_changed_cb (WnckWindow * window,
                                               WnckWindowState changed_mask,
                                               WnckWindowState new_state,
                                               TaskManager * manager)
{
  TaskManagerPrivate  *priv;
  WnckWindow          *win;
  WnckApplication     *app;
  WnckWorkspace       *space;
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
   
  if (!priv->intellihide)
  {
    return;
  }
  win = wnck_screen_get_active_window (priv->screen);
  if (!win)
  {
    return;
  }
  app = wnck_window_get_application (win);
  space = wnck_screen_get_active_workspace (priv->screen);
  task_manager_check_for_intersection (manager,space,app);  
  
}

static GQuark
task_manager_error_quark (void)
{
  static GQuark quark = 0;
  if (!quark)
    quark = g_quark_from_static_string ("task_manager_error");
  return quark;
}

static GType
task_manager_error_get_type (void)
{
  static GType etype = 0;

  if (!etype)
  {
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }
    static const GEnumValue values[] =
    {
      // argh! DBus enums can't have spaces in the nick!
      ENUM_ENTRY(TASK_MANAGER_ERROR_UNSUPPORTED_WINDOW_TYPE,
                 "UnsupportedWindowType"),
      ENUM_ENTRY(TASK_MANAGER_ERROR_NO_WINDOW_MATCH,
                 "NoWindowMatch"),
      { 0, 0, 0 }
    };

    etype = g_enum_register_static ("TaskManagerError", values);
  }

  return etype;
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
    g_set_error (error, 
                 task_manager_error_quark (),
                 TASK_MANAGER_ERROR_UNSUPPORTED_WINDOW_TYPE,
                 "Window can be specified only by its name or XID");

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
        if (filename && filename[0] != '\0')
        {
          g_object_set_property (G_OBJECT (item->icon_overlay),
                                 "file-name", value);
        }

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
        if (g_value_get_int (value) != -1)
        {
          g_object_set_property (G_OBJECT (item->progress_overlay),
                                 "percent-complete", value);
        }

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

        const gchar* text = g_value_get_string (value);
        g_object_set (G_OBJECT (item->text_overlay),
                      "active", text && text[0] != '\0', NULL);
        if (text && text[0] != '\0')
        {
          g_object_set_property (G_OBJECT (item->text_overlay), "text", value);
        }

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
    g_set_error (error,
                 task_manager_error_quark (),
                 TASK_MANAGER_ERROR_NO_WINDOW_MATCH,
                 "No matching window found to update.");

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
  GtkPositionType position;
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

  position = awn_applet_get_pos_type (AWN_APPLET(manager));
  size = awn_applet_get_size (AWN_APPLET(manager));
  childs = gtk_container_get_children (GTK_CONTAINER(priv->box));
  move_to = g_list_index (childs, GTK_WIDGET(icon));
  moved = g_list_index (childs, GTK_WIDGET(priv->drag_indicator));

  g_return_if_fail (move_to != -1);
  g_return_if_fail (moved != -1);

  if(position == GTK_POS_TOP || position == GTK_POS_BOTTOM)
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
#ifdef DEBUG  
  else 
  {   
    g_print("ERROR: previous drag wasn't done yet ?!");
    g_return_if_reached();
  }
#endif  
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

    GValue *val;

    val = g_value_init (val, G_TYPE_BOXED);
    g_value_set_boxed (val, launchers);

    desktop_agnostic_config_client_set_value (priv->client,
                                              DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                              "launcher_paths",
                                              val, &err);

    g_value_unset (val);

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
