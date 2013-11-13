/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 * Copyright (C) 2009, 2010 Rodney Cryderman <rcryderman@gmail.com>
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

#include "libawn/awn-pixbuf-cache.h"
#include "awn-desktop-lookup-cached.h"
#include "task-manager.h"
#include "task-manager-panel-connector.h"

#include "dock-manager-api.h"

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

static const GtkTargetEntry drop_types[] = 
{
  { (gchar*)"text/uri-list", 0, 0 },
  { (gchar*)"STRING", 0, 0 },
  { (gchar*)"text/plain", 0,  },
  { (gchar*)"awn/task-icon", 0, 0 }
};
static const gint n_drop_types = G_N_ELEMENTS (drop_types);


typedef struct
{
  DesktopAgnosticConfigClient *panel_instance_client;
  GdkWindow * foreign_window;
  GdkRegion * foreign_region;
  TaskManagerPanelConnector * connector;
  gint        intellihide_mode;
  guint       autohide_cookie;
} TaskManagerAwnPanelInfo;

struct _TaskManagerPrivate
{
  DesktopAgnosticConfigClient *client;

  DBusGConnection *connection;
  DBusGProxy      *proxy;
  
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
  /*list of window res_names that are to be hidden*/
  GList      *hidden_list;
  
  GHashTable *win_table;
  GHashTable *desktops_table;
  GHashTable *intellihide_panel_instances;

  /*
   Used during grouping configuration changes for optimization purposes
   */
  GList *grouping_list;
  /* Properties */
  GValueArray *launcher_paths;

  guint        attention_cookie;
  guint        attention_source;

  gboolean     show_all_windows;
  gboolean     only_show_launchers;
  gboolean     drag_and_drop;
  gboolean     grouping;
  gboolean     icon_grouping;
  gint         match_strength;
  gint         attention_autohide_timer;
  gint         desktop_copy;
  
  guint        attention_required_reminder_id;
  gint         attention_required_reminder;

  AwnDesktopLookupCached * desktop_lookup;
  TaskManagerDispatcher *dbus_proxy;

  GtkWidget * add_icon;
  guint       add_icon_source;
  
};

typedef struct
{
  WnckWindow * window;
  TaskManager * manager;
} WindowOpenTimeoutData;

enum
{
  INTELLIHIDE_NONE,
  INTELLIHIDE_WORKSPACE,
  INTELLIHIDE_APP,
  INTELLIHIDE_GROUP  
};

enum{
  DESKTOP_COPY_ALL=0,
  DESKTOP_COPY_OWNER,
  DESKTOP_COPY_NONE
}DesktopCopy;

enum
{
  PROP_0,

  PROP_SHOW_ALL_WORKSPACES,
  PROP_ONLY_SHOW_LAUNCHERS,
  PROP_LAUNCHER_PATHS,
  PROP_DRAG_AND_DROP,
  PROP_GROUPING,
  PROP_ICON_GROUPING,
  PROP_MATCH_STRENGTH,
  PROP_ATTENTION_AUTOHIDE_TIMER,
  PROP_DESKTOP_COPY,
  PROP_ATTENTION_REQUIRED_REMINDER
};


enum
{
  GROUPING_CHANGED,
  
  LAST_SIGNAL
};
static guint32 _taskman_signals[LAST_SIGNAL] = { 0 };

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
static void task_manager_check_for_intersection (TaskManager * manager,
                                                  WnckWorkspace * space,
                                                  WnckApplication * app);

static void task_manager_win_geom_changed_cb (WnckWindow *window, 
                                              TaskManager * manager);
static void task_manager_win_closed_cb (WnckScreen *screen,
                                        WnckWindow *window, 
                                        TaskManager * manager);
static void task_manager_win_state_changed_cb (WnckWindow * window,
                                               WnckWindowState changed_mask,
                                               WnckWindowState new_state,
                                               TaskManager * manager);
static TaskIcon * task_manager_find_icon_containing_desktop (TaskManager * manager,
                                                             const gchar * desktop);

static gboolean _attention_required_reminder_cb (TaskManager * manager);

static void task_manager_intellihide_change_cb (const char* group, 
                                    const char* key, 
                                    GValue* value, 
                                    int * user_data);

static void _icon_dest_drag_data_received (GtkWidget      *widget,
                                   GdkDragContext *context,
                                   gint            x,
                                   gint            y,
                                   GtkSelectionData *sdata,
                                   guint           info,
                                   guint           time_,
                                   TaskManager     *manager);

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

    case PROP_ICON_GROUPING:
      g_value_set_boolean (value, manager->priv->icon_grouping);
      break;

    case PROP_MATCH_STRENGTH:
      g_value_set_int (value, manager->priv->match_strength);
      break;

    case PROP_ATTENTION_AUTOHIDE_TIMER:
      g_value_set_int (value, manager->priv->attention_autohide_timer);
      break;

    case PROP_ATTENTION_REQUIRED_REMINDER:
      g_value_set_int (value, manager->priv->attention_required_reminder);
      break;

    case PROP_DESKTOP_COPY:
      g_value_set_int (value, manager->priv->desktop_copy);
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

  g_return_if_fail (TASK_IS_MANAGER (object));

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
      task_manager_set_drag_and_drop (manager, g_value_get_boolean (value));
      break;

    case PROP_MATCH_STRENGTH:
      task_manager_set_match_strength (manager,g_value_get_int (value));
      break;

    case PROP_GROUPING:
      task_manager_set_grouping (manager,g_value_get_boolean (value));
      break;
      
    case PROP_ICON_GROUPING:
      manager->priv->icon_grouping = g_value_get_boolean (value);
      break;
            
    case PROP_ATTENTION_AUTOHIDE_TIMER:
      manager->priv->attention_autohide_timer = g_value_get_int (value);
      break;

    case PROP_ATTENTION_REQUIRED_REMINDER:
      if (manager->priv->attention_required_reminder_id)
      {
        g_source_remove (manager->priv->attention_required_reminder_id);
        manager->priv->attention_required_reminder_id = 0;
      }
      manager->priv->attention_required_reminder = g_value_get_int (value);
      if (manager->priv->attention_required_reminder>0)
      {
        manager->priv->attention_required_reminder_id = g_timeout_add_seconds ( manager->priv->attention_required_reminder,
                                                                             (GSourceFunc)_attention_required_reminder_cb,
                                                                             object);
      }
      break;
    case PROP_DESKTOP_COPY:
      manager->priv->desktop_copy = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
_delete_panel_info_cb (TaskManagerAwnPanelInfo * panel_info)
{
  g_object_unref (panel_info->connector);
  g_object_unref (panel_info->foreign_window);
  gdk_region_destroy (panel_info->foreign_region);
  g_free (panel_info);
}

static void
_on_panel_added (DBusGProxy *proxy,guint panel_id,TaskManager *applet)
{
  TaskManagerPrivate *priv = TASK_MANAGER_GET_PRIVATE (applet);
  GError * error = NULL;
  TaskManagerAwnPanelInfo * panel_info = NULL;
  gchar * uid = g_strdup_printf("-999%d",panel_id);
  
  g_assert (!g_hash_table_lookup (priv->intellihide_panel_instances,GINT_TO_POINTER (panel_id)));
  panel_info = g_malloc0 (sizeof (TaskManagerAwnPanelInfo) );
  panel_info->connector = task_manager_panel_connector_new (panel_id);
  g_free (uid);
  panel_info->panel_instance_client = awn_config_get_default (panel_id, NULL);
  panel_info->intellihide_mode = desktop_agnostic_config_client_get_int (
                                             panel_info->panel_instance_client, 
                                             "panel", 
                                             "intellihide_mode", 
                                              &error);
  if (error)
  {
    g_debug ("%s: error accessing intellihide_mode. \"%s\"",__func__,error->message);
    g_error_free (error);
    error = NULL;
  }
  desktop_agnostic_config_client_notify_add (panel_info->panel_instance_client, 
                                             "panel", 
                                             "intellihide_mode", 
                                             (DesktopAgnosticConfigNotifyFunc)task_manager_intellihide_change_cb, 
                                             &panel_info->intellihide_mode, 
                                             &error);
  if (error)
  {
    g_debug ("%s: error binding intellihide_mode. \"%s\"",__func__,error->message);
    g_error_free (error);
    error = NULL;
  }

  if (!panel_info->intellihide_mode && panel_info->autohide_cookie)
  {     
    task_manager_panel_connector_uninhibit_autohide (panel_info->connector, panel_info->autohide_cookie);
    panel_info->autohide_cookie = 0;
  }
  if (panel_info->intellihide_mode && !panel_info->autohide_cookie)
  {     
    panel_info->autohide_cookie = task_manager_panel_connector_inhibit_autohide (panel_info->connector,"Intellihide" );
  }   

  g_hash_table_insert (priv->intellihide_panel_instances,GINT_TO_POINTER(panel_id),panel_info);                                                                           
}

static void
_on_panel_removed (DBusGProxy *proxy,guint panel_id,TaskManager * applet)
{
  TaskManagerPrivate *priv = TASK_MANAGER_GET_PRIVATE (applet);
  TaskManagerAwnPanelInfo * panel_info = g_hash_table_lookup (priv->intellihide_panel_instances,GINT_TO_POINTER (panel_id));

  desktop_agnostic_config_client_remove_instance (panel_info->panel_instance_client);
  g_assert (g_hash_table_remove (priv->intellihide_panel_instances,GINT_TO_POINTER (panel_id)));
}

static void
task_manager_constructed (GObject *object)
{
  TaskManagerPrivate *priv;
  GtkWidget          *widget;
  GError             *error=NULL;
  GStrv              panel_paths;
  
  G_OBJECT_CLASS (task_manager_parent_class)->constructed (object);
  
  priv = TASK_MANAGER_GET_PRIVATE (object);
  widget = GTK_WIDGET (object);

  priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                           "org.awnproject.Awn",
                                            "/org/awnproject/Awn",
                                           "org.awnproject.Awn.App");
  if (!priv->proxy)
  {
    g_warning("%s: Could not connect to mothership!\n",__func__);
  }
  else
  {
    dbus_g_proxy_add_signal (priv->proxy, "PanelAdded",
                             G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (priv->proxy, "PanelRemoved",
                             G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_connect_signal (priv->proxy, "PanelAdded",
                                 G_CALLBACK (_on_panel_added), object, 
                                 NULL);
    dbus_g_proxy_connect_signal (priv->proxy, "PanelRemoved",
                                 G_CALLBACK (_on_panel_removed), object, 
                                 NULL);

  }    
  
  /*
   Set the cache size of our AwnPixbufCache to something a bit bigger.

   FIXME: ? possible config option.
   */
  g_object_set(awn_pixbuf_cache_get_default (),
               "max-cache-size",32,
               NULL);

  priv->desktops_table = g_hash_table_new_full (g_str_hash,g_str_equal,g_free,g_free);
  priv->intellihide_panel_instances = g_hash_table_new_full (g_direct_hash,g_direct_equal,
                                                             NULL,
                                                             (GDestroyNotify)_delete_panel_info_cb);                                    

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
                                       "icon_grouping",
                                       object, "icon_grouping", TRUE,
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
                                       object, "attention_autohide_timer", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "attention_required_reminder",
                                       object, "attention_required_reminder", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "desktop_copy",
                                       object, "desktop copy", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);

  g_signal_connect (priv->screen,"active-window-changed",
                    G_CALLBACK(task_manager_active_window_changed_cb),object);
  g_signal_connect (priv->screen,"active-workspace-changed",
                    G_CALLBACK(task_manager_active_workspace_changed_cb),object);

  priv->desktop_lookup = awn_desktop_lookup_cached_new ();

  /* DBus interface */
  priv->dbus_proxy = task_manager_dispatcher_new (TASK_MANAGER (object));

  if (priv->proxy)
  {
    dbus_g_proxy_call (priv->proxy, "GetPanels",
                     &error,
                     G_TYPE_INVALID, 
                     G_TYPE_STRV, &panel_paths,
                     G_TYPE_INVALID);
    if (error)
    {
      g_debug ("%s: %s",__func__,error->message);
      g_error_free (error);
      error = NULL;
    }
    else
    {
      for (gint i=0; panel_paths[i];i++)
      {
        //strlen is like this as a reminder.
        _on_panel_added (priv->proxy,
                         atoi(panel_paths[i] + strlen("/org/awnproject/Awn/Panel")),
                         TASK_MANAGER(object));
        
      }
    }    
  }
  priv->add_icon = awn_themed_icon_new ();
  awn_themed_icon_set_size (AWN_THEMED_ICON(priv->add_icon),awn_applet_get_size (AWN_APPLET(object)));
  awn_themed_icon_set_info_simple (AWN_THEMED_ICON (priv->add_icon),
                                   "::no_drop::taskmanager",
                                   "dummy",
                                   "add");
  gtk_container_add (GTK_CONTAINER (priv->box),priv->add_icon);
  gtk_widget_hide (priv->add_icon);
  gtk_widget_add_events (GTK_WIDGET (priv->add_icon), GDK_ALL_EVENTS_MASK);
  gtk_drag_dest_set (GTK_WIDGET (priv->add_icon), 
                     GTK_DEST_DEFAULT_ALL & (~GTK_DEST_DEFAULT_HIGHLIGHT),
                     drop_types, n_drop_types,
                     GDK_ACTION_COPY | GDK_ACTION_MOVE);
  g_signal_connect (priv->add_icon,
                    "drag-data-received",
                     G_CALLBACK(_icon_dest_drag_data_received),
                    object);
                    
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

  pspec = g_param_spec_boolean ("icon_grouping",
                                "icon_grouping",
                                "Icon Grouping",
                                TRUE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ICON_GROUPING, pspec);

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
                            4,
                            G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ATTENTION_AUTOHIDE_TIMER, pspec);

  pspec = g_param_spec_int ("attention_required_reminder",
                            "Attention Required Reminder Timer",
                            "Attention Required Reminder Timer",
                            -1,
                            9999,
                            60,
                            G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ATTENTION_REQUIRED_REMINDER, pspec);

  pspec = g_param_spec_int ("desktop_copy",
                            "When/if to copy desktop files",
                            "When/if to copy desktop files",
                            DESKTOP_COPY_ALL,
                            DESKTOP_COPY_NONE,
                            DESKTOP_COPY_OWNER,
                            G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_DESKTOP_COPY, pspec);
  
  /* Install signals */
  _taskman_signals[GROUPING_CHANGED] =
		g_signal_new ("grouping_changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskManagerClass, grouping_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN, 
			      G_TYPE_NONE, 1,
            G_TYPE_BOOLEAN);

  g_type_class_add_private (obj_class, sizeof (TaskManagerPrivate));

  dbus_g_error_domain_register (task_manager_error_quark (), NULL, 
                                task_manager_error_get_type ());
}

static void
task_manager_init (TaskManager *manager)
{
  TaskManagerPrivate *priv;
  GError         *error = NULL;
  priv = manager->priv = TASK_MANAGER_GET_PRIVATE (manager);
  
  priv->screen = wnck_screen_get_default ();
  priv->launcher_paths = NULL;
  priv->hidden_list = NULL; 
  priv->add_icon_source = 0;
  priv->add_icon = NULL;
  
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

  priv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
  priv->proxy = NULL;  
  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free(error);
  }

  /* connect to the relevent WnckScreen signals */
  g_signal_connect (priv->screen, "window-opened", 
                    G_CALLBACK (on_window_opened), manager);
  g_signal_connect (priv->screen, "active-window-changed",  
                    G_CALLBACK (on_active_window_changed), manager);

  /* Used for Intellihide */
  g_signal_connect (priv->screen,"window-closed",
                    G_CALLBACK(task_manager_win_closed_cb),manager);

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
  if (priv->connection)
  {
    if (priv->proxy) g_object_unref (priv->proxy);
    dbus_g_connection_unref (priv->connection);
    priv->connection = NULL;
    priv->proxy = NULL;
  }
  
  /*
  if (priv->autohide_cookie)
  {     
    awn_applet_uninhibit_autohide (AWN_APPLET(object), priv->autohide_cookie);
    priv->autohide_cookie = 0;
  }*/
/*  if (priv->awn_gdk_window)
  {
    g_object_unref (priv->awn_gdk_window);
    priv->awn_gdk_window = NULL;
  }*/

  G_OBJECT_CLASS (task_manager_parent_class)->dispose (object);
}

/*
 * WNCK_SCREEN CALLBACKS
 */

/* 
 check for state change to and from skip tasklist.
 It appears we don't need to monitor loss of skip tasklist, but going to leave
 as is for the moment.  FIXME ?
 */
static void
on_window_state_changed (WnckWindow      *window,
                         WnckWindowState  changed_mask,
                         WnckWindowState  new_state,
                         TaskManager     *manager)
{
  TaskManagerPrivate *priv;
  TaskWindow        * task_win=NULL;
  TaskIcon          * icon=NULL;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));

  priv = TASK_MANAGER_GET_PRIVATE (manager);

  /* test if they don't skip-tasklist anymore*/
  if (changed_mask & WNCK_WINDOW_STATE_SKIP_TASKLIST)
  {
    /*find the TaskWindow and destroy it*/
    for (GSList *icon_iter = priv->icons;icon_iter != NULL && !task_win;icon_iter = icon_iter->next)
    {
      icon = icon_iter->data;
      GSList *items = task_icon_get_items (icon);
      for (GSList *item_iter = items;item_iter != NULL;item_iter = item_iter->next)
      {
        TaskItem *item = item_iter->data;
        
        if (TASK_IS_WINDOW (item) && window==task_window_get_window(TASK_WINDOW(item)) )
        {
          task_win = TASK_WINDOW(item);
          break;
        }
      }
    }
    if (new_state & WNCK_WINDOW_STATE_SKIP_TASKLIST)
    {
        /*looks like we're ok in this case*/
    }  
    else
    {
      if (!task_win)
      {
        /*do this if we don't have TaskWindow for window yet*/
        g_signal_handlers_disconnect_by_func (window,G_CALLBACK(on_window_state_changed),manager);
        on_window_opened (NULL, window, manager);
      }
    }    
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
  /* This will reset the nag timer*/
  g_object_set (manager,
                "attention_required_reminder",priv->attention_required_reminder,
                NULL);
  return FALSE;
}


static gboolean
_attention_required_reminder_cb (TaskManager * manager)
{
  TaskManagerPrivate *priv;
  GSList * iter;
  
  g_return_val_if_fail (TASK_IS_MANAGER (manager),FALSE);
  priv = manager->priv;  
  for (iter=priv->windows; iter; iter = iter->next)
  {
    WnckWindowState win_state = wnck_window_get_state (task_window_get_window(iter->data));
    if ( (win_state  & WNCK_WINDOW_STATE_DEMANDS_ATTENTION) || 
        (win_state & WNCK_WINDOW_STATE_URGENT) )
    {
      if (priv->attention_autohide_timer && priv->attention_required_reminder)
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
  return TRUE;
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

/*
 TODO:  needs some cleanup.
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
  if ( task_icon_is_visible (icon) && 
      ( !priv->only_show_launchers || !task_icon_is_ephemeral (icon))
      )
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
    if ( !task_icon_count_tasklist_windows (icon))
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

  if (!visible )
  {
    if (do_hide_animation)
    {
      awn_effects_start_ex (awn_overlayable_get_effects (AWN_OVERLAYABLE (icon)), 
                          AWN_EFFECT_CLOSING, 1, FALSE, TRUE);
    }
    else
    {
      gtk_widget_hide (GTK_WIDGET(icon));
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
      if (task_icon_count_items (icon)==1)
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
    else if (!task_icon_is_visible (icon))
    {
      gtk_widget_hide (GTK_WIDGET(icon));
    }
    else
    {
      gtk_widget_show (GTK_WIDGET(icon));
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
get_launcher(TaskManager * manager, const gchar * desktop)
{
  TaskItem     *launcher = NULL;  
  DesktopAgnosticFDODesktopEntry *entry=NULL;
  DesktopAgnosticVFSFile *file;

  g_assert (TASK_IS_MANAGER (manager));
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
    launcher = task_launcher_new_for_desktop_file (AWN_APPLET(manager),desktop);
    g_object_unref (entry);                                                    
  }
  return launcher;
}

void
task_manager_add_icon(TaskManager *manager, TaskIcon * icon)
{
  TaskManagerPrivate *priv;
  
  priv = manager->priv;  
  priv->icons = g_slist_append (priv->icons, icon);

  gtk_container_add (GTK_CONTAINER (priv->box), GTK_WIDGET(icon));
  if (priv->icon_grouping)
  {
    const TaskItem * launcher = task_icon_get_launcher (icon);
    if (launcher)
    {
      const gchar * desktop = task_launcher_get_desktop_path (TASK_LAUNCHER (launcher));
      TaskIcon * first_match = task_manager_find_icon_containing_desktop (manager,desktop);
      if (first_match)
      {
        GList * l = gtk_container_get_children (GTK_CONTAINER (priv->box));
        gint pos = g_list_index (l,first_match);
        if (pos != -1)
        {
          pos++;
          gtk_box_reorder_child ( GTK_BOX (priv->box),GTK_WIDGET (icon),pos++);
        }
        g_list_free (l);
      }
    }
  }
  
  /* reordening through D&D */
  if(priv->drag_and_drop)
  {
    _drag_add_signals(manager, GTK_WIDGET(icon));
  }

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
  task_icon_refresh_icon (TASK_ICON(icon),awn_applet_get_size(AWN_APPLET(manager)));
}  

static gboolean
task_manager_check_for_hidden (TaskManager * manager, TaskWindow * item)
{
  TaskManagerPrivate *priv;
  priv = manager->priv;
  gboolean hidden = FALSE;

  /*see if the new window is on the hidden_list*/
  gchar * res_name=NULL;
  gchar * class_name=NULL;
  task_window_get_wm_class(TASK_WINDOW (item), &res_name, &class_name);    
  /*see if the new window is on the hidden_list*/
  for (GList *iter=priv->hidden_list;iter;iter=g_list_next(iter) )
  {
    if (  (g_strcmp0(iter->data,res_name)==0)||
        (g_strcmp0(iter->data,class_name)==0) )
    {
      task_window_set_hidden (TASK_WINDOW(item),TRUE);
      hidden = TRUE;
      break;
    }
  }
  g_free (res_name);
  g_free (class_name);
  return hidden;
}

/*
 Finds last icon in the first matching sequence of desktops. 
 */
static TaskIcon *
task_manager_find_icon_containing_desktop (TaskManager * manager,const gchar * desktop)
{
  TaskManagerPrivate *priv;
  priv = manager->priv;
  TaskIcon * res = NULL;
  GList * icons = gtk_container_get_children (GTK_CONTAINER(priv->box));

  for (GList *icon_iter = icons; icon_iter ;icon_iter = icon_iter->next)
  {
    if (!TASK_IS_ICON(icon_iter->data))
    {
      continue;
    }
    const TaskItem * launcher = task_icon_get_launcher (icon_iter->data);
    if (launcher)
    {
      if (g_strcmp0 (desktop, task_launcher_get_desktop_path(TASK_LAUNCHER(launcher)))==0)
      {
        res = icon_iter->data;
      }
      else if (res)
      {
        return res;
      }
    }
  }
  return res;
}

static TaskIcon *
task_manager_find_window (TaskManager * manager, WnckWindow * window)
{
  TaskManagerPrivate *priv;
  priv = manager->priv;
    
  for (GSList *icon_iter = priv->icons; icon_iter ;icon_iter = icon_iter->next)
  {
    TaskIcon *icon = icon_iter->data;
    GSList *items = task_icon_get_items (icon);
    for (GSList *item_iter = items;item_iter != NULL;item_iter = item_iter->next)
    {
      TaskItem *item = item_iter->data;
      
      if (TASK_IS_WINDOW (item) && window==task_window_get_window(TASK_WINDOW(item)) )
      {
        return icon;
      }
    }
  }
  return NULL;
}


static const gchar *
search_for_desktop (TaskIcon * icon,TaskItem *item,gboolean thorough)
{
  /* grab the class name.
   look through the various freedesktop system/user dirs for the 
   associated desktop file.  If we find it then dump the associated 
   launcher into the the dialog.
  */
  gchar * id = NULL;
  const gchar * found_desktop = NULL;
  TaskManager * manager;
  TaskManagerPrivate *priv;
  WnckWindow * win;

  g_return_val_if_fail (TASK_IS_WINDOW(item),NULL);
  g_return_val_if_fail (TASK_IS_ICON(icon),NULL);

  win = task_window_get_window(TASK_WINDOW(item));
  g_object_get (icon,"applet",&manager,NULL);
  priv = manager->priv;

  found_desktop = awn_desktop_lookup_search_by_wnck_window (priv->desktop_lookup,win);  
  if (TASK_IS_WINDOW(item))
  {
    gchar   *res_name = NULL;
    gchar   *class_name = NULL;
    gchar   *cmd;
    gchar   *full_cmd;
    glibtop_proc_args buf;

    _wnck_get_wmclass (wnck_window_get_xid (win),&res_name, &class_name);
    cmd = glibtop_get_proc_args (&buf,wnck_window_get_pid (win),1024);    
    full_cmd = get_full_cmd_from_pid ( wnck_window_get_pid (win));
    task_window_set_use_win_icon (TASK_WINDOW(item),get_win_icon_use (full_cmd,
                                                                    res_name,
                                                                    class_name,
                                                                    task_window_get_name (TASK_WINDOW(item))));
    g_free (full_cmd);
    g_free (cmd);     
    g_free (class_name);
    g_free (res_name);
  }
  g_free (id);    
  return found_desktop;
}

static void
window_name_changed_cb  (TaskWindow *item,const gchar *name, TaskIcon * icon)
{
  const gchar *found_desktop = NULL;

  g_return_if_fail (TASK_IS_WINDOW(item));
  g_return_if_fail (TASK_IS_ICON(icon));

  found_desktop = search_for_desktop (TASK_ICON(icon),TASK_ITEM(item),FALSE);
  if (found_desktop)
  {
    g_signal_handlers_disconnect_by_func(item, window_name_changed_cb, icon);
    if (found_desktop)
    {
      TaskManager * manager = TASK_MANAGER(task_icon_get_applet (icon));
      TaskItem * launcher = get_launcher (manager,found_desktop);

      if (launcher)
      {
        task_icon_append_item (TASK_ICON (icon), launcher);
      }      
    }          
  }
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
  const gchar         *found_desktop = NULL;
  TaskIcon            *containing_icon = NULL;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  g_return_if_fail (WNCK_IS_WINDOW (window));

  g_signal_handlers_disconnect_by_func(window, process_window_opened, manager);
  
  priv = manager->priv;
  type = wnck_window_get_window_type (window);

  /*Due to the insanity of oo it might already be in the icon list...
   I'm sure another app will come along some day so it won't just be 
   OpenOffice but till then the blame lies there.
   */
  containing_icon = task_manager_find_window (manager,window);
  if ( containing_icon )
  {
    return;
  }
    
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
  
  if ( g_strcmp0 (wnck_window_get_name (window),"awn-applet")==0 )
  {
    return;
  }
  /* 
   * Monitors state change to and from skip tasklist.
   */
  g_signal_connect (window, "state-changed", G_CALLBACK (on_window_state_changed), manager);

  if (wnck_window_is_skip_pager (window))
  {
    return;
  }
  g_signal_connect (window, "state-changed", 
                    G_CALLBACK (check_attention_requested), manager);    
  
  /* create a new TaskWindow containing the WnckWindow*/
  item = task_window_new (AWN_APPLET(manager), NULL, window);
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
    /*
     is the task icon in the midst of disappearing?
     */
    if (!task_icon_get_proxy (taskicon) )
    {
      continue;
    }
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
  task_manager_check_for_hidden (manager,TASK_WINDOW(item));

  if  (match
       &&
       (priv->grouping || ( (task_icon_count_items(match)==1) && (task_icon_contains_launcher (match)) ) ) 
       &&
       ( max_match_score > 99-priv->match_strength))
  {
    if (TASK_IS_ICON (match))
    {
      GObject * p = task_icon_get_proxy (match);
      if (p && G_IS_OBJECT(p))
      {
        g_object_set (item,
                      "proxy",task_icon_get_proxy(match),
                      NULL);
        task_icon_append_item (match, item);
      }
      else
      {
        g_warning ("TaskIcon: Window closed before match");
        return;
      }
    }
    else
    {
      g_warning ("TaskIcon: not an icon... bailing");
      return;
    }
  }
  else
  {

    icon = task_icon_new (AWN_APPLET (manager));

    found_desktop = search_for_desktop (TASK_ICON(icon),item,TRUE);
    g_object_set (item,
              "proxy",task_icon_get_proxy(TASK_ICON(icon)),
              NULL);
    if (found_desktop)
    {
      TaskItem * launcher = get_launcher (manager,found_desktop);
      if (launcher)
      {
        if (task_icon_count_items (TASK_ICON (icon))==0)
        {
          g_object_unref (task_icon_get_proxy(TASK_ICON(icon)));
        }                                 
        task_icon_append_item (TASK_ICON (icon), launcher);
        task_icon_append_item (TASK_ICON (icon), item);
      }
      else
      {
        task_icon_append_item (TASK_ICON (icon), item);
      }
    }
    else
    {
      task_icon_append_item (TASK_ICON (icon), item);
    }
    
    if ( !found_desktop)
    {
      g_signal_connect (item,"name-changed",G_CALLBACK(window_name_changed_cb), icon);
    }
    task_manager_add_icon (manager,TASK_ICON (icon));
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

  if (wnck_window_is_skip_tasklist(window))
  {
    return;
  }

  _wnck_get_wmclass (wnck_window_get_xid (window),
                     &res_name, &class_name);
  if ( g_strcmp0(res_name,"awn-applet") != 0)
  {
    if (get_special_wait_from_window_data (res_name, 
                                           class_name,
                                           wnck_window_get_name(window) ) )
    {
      win_timeout_data = g_malloc (sizeof (WindowOpenTimeoutData));
      win_timeout_data->window = window;
      win_timeout_data->manager = manager;    
      g_signal_connect (window,"name-changed",G_CALLBACK(process_window_opened),manager);
      g_timeout_add (2000,(GSourceFunc)_wait_for_name_change_timeout,win_timeout_data);
    }
    else
    {
      process_window_opened (window,manager);
    }
  }
  g_free (res_name);
  g_free (class_name);  
}

/*
 * PROPERTIES
 */

gboolean
task_manager_get_show_all_windows (TaskManager * manager)
{
  TaskManagerPrivate *priv;
  g_return_val_if_fail (TASK_IS_MANAGER (manager),TRUE);

  priv = manager->priv;

  return priv->show_all_windows;
}
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

void
task_manager_remove_launcher(TaskManager  *manager, const gchar * launcher_path)
{
  TaskManagerPrivate *priv;
  GValueArray *launcher_paths;
  GValue val = {0,};

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  g_object_get (G_OBJECT (manager), "launcher_paths", &launcher_paths, NULL);
  g_value_init (&val, G_TYPE_STRING);
  g_value_set_string (&val, launcher_path);
  for (guint idx = 0; idx < launcher_paths->n_values; idx++)
  {
    gchar *path;
    path = g_value_dup_string (g_value_array_get_nth (launcher_paths, idx));
    if ( g_strcmp0 (path,launcher_path)==0)
    {
      g_value_array_remove (launcher_paths,idx);
      break;
    }
  }
  g_object_set (G_OBJECT (manager), "launcher_paths", launcher_paths, NULL);
  g_value_unset (&val);
  task_manager_refresh_launcher_paths (manager, launcher_paths);
  g_value_array_free (launcher_paths);
}

static void
task_manager_intellihide_change_cb (const char* group, 
                                    const char* key, 
                                    GValue* value, 
                                    int * user_data)
{
  *user_data = g_value_get_int (value);
}

/*
 * Checks when launchers got added/removed in the list in gconf/file.
 * It removes the launchers from the task-icons and add those
 * that aren't already on the bar.
 */
static void
task_manager_refresh_launcher_paths (TaskManager *manager, GValueArray *list)
{
  TaskManagerPrivate *priv;
  gboolean ephemeral = TRUE;

  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  task_manager_add_icon_hide (manager);
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
      TaskItem  *launcher = NULL;
      
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
          launcher = item;
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
      if (found && launcher)
      {
        ephemeral = task_icon_is_ephemeral (icon_iter->data);
        if (ephemeral) /*then it shouldn't be*/
        {
          g_object_set (G_OBJECT(task_icon_get_launcher (icon_iter->data)),
                          "proxy",task_icon_get_proxy (icon_iter->data),
                          NULL);
          icon_iter = priv->icons;
        }
        break;
      }
    }
    if (!found)
    {      
      GtkWidget *icon;

      if (usable_desktop_file_from_path (path) )
      {
        TaskItem  *launcher = NULL;
        launcher = task_launcher_new_for_desktop_file (AWN_APPLET(manager),path);
        if (launcher)
        {
          icon = task_icon_new (AWN_APPLET (manager));
          g_object_set (G_OBJECT(launcher),
                        "proxy",task_icon_get_proxy (TASK_ICON(icon)),
                        NULL);
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
      ephemeral = task_icon_is_ephemeral (icon_iter->data);
      if (!ephemeral) /*then it should be*/
      {
        g_object_set (G_OBJECT(task_icon_get_launcher (icon_iter->data)),
                        "proxy",NULL,
                        NULL);
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
task_manager_regroup_launcher_icon (TaskManager * manager,TaskIcon * grouping_icon)
{
  g_assert (TASK_IS_ICON(grouping_icon));
  g_assert (TASK_IS_MANAGER(manager));
  
  GSList * i;
  GSList * j;
  TaskManagerPrivate * priv = manager->priv;
  GtkWidget * grouping_launcher = GTK_WIDGET(task_icon_get_launcher (grouping_icon));

  g_assert (grouping_launcher);
  for (i=task_icon_get_items(grouping_icon);i; i=i->next)
  {
    TaskItem * item = i->data;
    if (TASK_IS_LAUNCHER(item))
    {
      continue;
    }
    if (g_list_find (priv->grouping_list,item))
    {
      continue;
    }
    priv->grouping_list = g_list_prepend (priv->grouping_list,item);
  }
  for (i=priv->icons; i; i=i->next)
  {
    GtkWidget * launcher;
    TaskIcon  * icon;
    icon = i->data;
    if (icon == grouping_icon)
    {
      continue;
    }
    launcher = GTK_WIDGET(task_icon_get_launcher (icon));
    if (!launcher ) /* no launcher...  need to do matches on these eventually TODO */
    {
      continue;
    }
    /* Is i an existing permanent launcher... if so then ignore.*/
    else if ( !task_icon_is_ephemeral (icon))
    {
      continue;
    }
    /*ok... non-permanent launcher check if the desktop file paths match.*/
    if ( g_strcmp0 (task_launcher_get_desktop_path(TASK_LAUNCHER(grouping_launcher)),
                    task_launcher_get_desktop_path(TASK_LAUNCHER(launcher)) )==0)
    {
      for (j=task_icon_get_items(icon);j; j?j=j->next:j)
      {

        TaskItem * item = j->data;
        if (TASK_IS_LAUNCHER(item))
        {
          continue;
        }
        if (g_list_find (priv->grouping_list,item))
        {
          continue;
        }
        task_icon_moving_item (grouping_icon,icon,item);
        priv->grouping_list = g_list_prepend (priv->grouping_list,item);
        j = task_icon_get_items(icon);
      }
    }
  }
}

static void
task_manager_regroup_nonlauncher_icon (TaskManager * manager,TaskIcon * grouping_icon)
{
  g_assert (TASK_IS_ICON(grouping_icon));
  g_assert (TASK_IS_MANAGER(manager));
  
  GSList * i;
  GSList * j;
  GSList * w; 
  GSList * k;
  TaskManagerPrivate * priv = manager->priv;
  for (i=task_icon_get_items(grouping_icon);i; i=i->next)
  {
    TaskItem * item = i->data;
    if (g_list_find (priv->grouping_list,item))
    {
      continue;
    }
    priv->grouping_list = g_list_prepend (priv->grouping_list,item);
  }

  for (i=priv->icons; i; i=i->next)
  {
    TaskIcon  * icon;
    icon = i->data;    
    if (icon == grouping_icon)
    {
      continue;
    }
    if (task_icon_contains_launcher (icon) )
    {
      continue;
    }
    for (j=task_icon_get_items(icon);j;j?j=j->next:j)
    {
      gint match_score = 0;
      gint max_match_score = 0;
      TaskIcon * match = NULL;
      TaskItem * item = j->data;
      if (g_list_find (priv->grouping_list,item))
      {
        continue;
      }      
      for (w = priv->icons; w; w = w->next)
      {
        TaskIcon *taskicon = w->data;
        /*optimization....  if the icon we're checking against has a launcher
         then the item would have been grouped with it on open if it could have 
         been*/
        if ( task_icon_contains_launcher (taskicon))
        {
          continue;
        }
        match_score = task_icon_match_item (taskicon, item);
        if (match_score > max_match_score)
        {
          max_match_score = match_score;
          match = taskicon;
        }
      }
      if  (
           match && (match==grouping_icon)
           &&
           (priv->grouping || ( (task_icon_count_items(match)==1) && (task_icon_contains_launcher (match)) ) ) 
           &&
           ( max_match_score > 99-priv->match_strength))
      {
        /*we have one match in this icon.  dump all the other items in also and
         get the hell out of the inner loop.  3 nested loops nasty... short 
         circuiting as much as possible*/
        task_icon_moving_item (grouping_icon,icon,item);
        priv->grouping_list = g_list_prepend (priv->grouping_list,item);
        for (k=task_icon_get_items (icon);k;k=k->next)
        {
          if (g_list_find (priv->grouping_list,k->data))
          {
            continue;
          }                
          task_icon_moving_item (grouping_icon,icon,k->data);
          priv->grouping_list = g_list_prepend (priv->grouping_list,k->data);
        }
        j = NULL;
      }
    }
  }
}

static void
task_manager_regroup (TaskManager * manager)
{
  GSList * i;

  TaskManagerPrivate * priv = manager->priv;
  /*
   find the permanent Launchers and regroup them*/
  for (i=priv->icons; i; i=i->next)
  {
    GtkWidget * launcher;
    TaskIcon  * icon;
    icon = i->data;
    launcher = GTK_WIDGET(task_icon_get_launcher (icon));

    if (launcher)
    {
      task_manager_regroup_launcher_icon (manager,icon);      
    }    
  }

  /* Find the ephemeral launchers and regroup them */
  for (i=priv->icons; i; i=i->next)
  {
    GtkWidget * launcher;
    TaskIcon  * icon;
    icon = i->data;
    launcher = GTK_WIDGET(task_icon_get_launcher (icon));
    if (launcher )
    {
      task_manager_regroup_launcher_icon (manager,icon);      
    }
  }
  /* Find the icons that do not have a launcher. */
  for (i=priv->icons; i; i=i->next)
  {
    TaskIcon  * icon;
    icon = i->data;
    if ( !task_icon_contains_launcher (icon) )
    {
      GSList *items = task_icon_get_items(icon);
      if (items)
      {
        task_manager_regroup_nonlauncher_icon (manager,icon);
      }
    }
  }
  g_list_free (priv->grouping_list);
  priv->grouping_list = NULL;
}

static void
task_manager_set_grouping (TaskManager *manager, 
                                gboolean  grouping)
{
  g_return_if_fail (TASK_IS_MANAGER (manager));

  TaskManagerPrivate *priv = manager->priv;
  priv->grouping = grouping;
  if (grouping)
  {
    task_manager_regroup(manager);
  }
  g_signal_emit (manager,_taskman_signals[GROUPING_CHANGED],0,grouping);
}

/*
 Returns the full list of TaskIcons.  Neither the list nor it's contents are
 owned by the caller.
 */
const GSList *
task_manager_get_icons (TaskManager * manager)
{
  g_return_val_if_fail (TASK_IS_MANAGER (manager),NULL);

  TaskManagerPrivate *priv = manager->priv;
  return priv->icons;
}

/*
 Returns a list of TaskIcons that have a matching resource or class name.
 The caller owns the list and should free it with g_slist_free ().  The caller
 does not own the contents of the list.
 */

GSList * 
task_manager_get_icons_by_wmclass (TaskManager * manager, const gchar * name)
{
  g_return_val_if_fail (TASK_IS_MANAGER (manager),NULL);

  GSList * l = NULL;
  GSList * i;
  GSList * j;
  TaskManagerPrivate *priv = manager->priv;

  for (i = priv->icons; i ; i = i->next)
  {
    GSList * items = task_icon_get_items (i->data);
    for ( j = items; j ; j = j->next)
    {
      WnckWindow * win;
      gchar * res_name = NULL;
      gchar * class_name = NULL;
      if (!TASK_IS_WINDOW(j->data))
      {
        continue;
      }
      win = task_window_get_window (j->data);
      _wnck_get_wmclass (wnck_window_get_xid (win),&res_name, &class_name);
      if ( (g_strcmp0 (res_name, name) == 0) || (g_strcmp0 (class_name, name) == 0) )
      {
        l = g_slist_append (l, i->data);
        g_free (res_name);
        g_free (class_name);
        break;
      }
      g_free (res_name);
      g_free (class_name);
    }
  }
  return l;
}

/*
 Returns a list of TaskIcons that have a matching desktop file.
 The caller owns the list and should free it with g_slist_free ().  The caller
 does not own the contents of the list.
 */

GSList *
task_manager_get_icons_by_desktop (TaskManager * manager,const gchar * desktop)
{
  g_return_val_if_fail (TASK_IS_MANAGER (manager),NULL);
  
  TaskManagerPrivate *priv;
  priv = manager->priv;
  GSList * l = NULL;
  
  for (GSList *i = priv->icons; i ;i = i->next)
  {
    const TaskItem * launcher = task_icon_get_launcher (i->data);
    if (launcher)
    {
      if (g_strcmp0 (desktop, task_launcher_get_desktop_path(TASK_LAUNCHER(launcher)))==0)
      {
        l = g_slist_append (l, i->data);
      }
    }
  }
  return l;
}

/*
 Returns a list of TaskIcons that have a matching PID.
 The caller owns the list and should free it with g_slist_free ().  The caller
 does not own the contents of the list.
 */

GSList *
task_manager_get_icons_by_pid (TaskManager * manager, int pid)
{
  g_return_val_if_fail (TASK_IS_MANAGER (manager),NULL);
  g_return_val_if_fail (pid,NULL);
  
  TaskManagerPrivate *priv;
  priv = manager->priv;
  GSList * l = NULL;
  
  for (GSList *i = priv->icons; i ;i = i->next)
  {
    GSList * items = task_icon_get_items (i->data);
    for (GSList *j = items; j ; j = j->next)
    {
      /*
       Look through all the items in the TaskIcon.  In most cases it's going
       to be a shared PID but not all so we need to check all the windows*/
      if (!TASK_IS_WINDOW(j->data))
      {
        continue;
      }
      if (task_window_get_pid (j->data) == pid)
      {
        l = g_slist_append (l, i->data);
        break;
      }
    }
  }
  return l;
}
/*
 Returns the TaskIcon that contains a TaskWindow with a matching xid.
 */
const TaskIcon *
task_manager_get_icon_by_xid (TaskManager * manager, gint64 xid)
{
  g_return_val_if_fail (TASK_IS_MANAGER (manager),NULL);
  g_return_val_if_fail (xid,NULL);
  
  TaskManagerPrivate *priv;
  priv = manager->priv;
  
  for (GSList *i = priv->icons; i ;i = i->next)
  {
    GSList * items = task_icon_get_items (i->data);
    for (GSList *j = items; j ; j = j->next)
    {
      /*
       Look through all the items in the TaskIcon for our XID.  There should
       only be one instance*/
      if (!TASK_IS_WINDOW(j->data))
      {
        continue;
      }
      if ( (gint64)task_window_get_xid (j->data) == xid)
      {
        return i->data;
      }
    }
  }
  return NULL;
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
    gchar * res_name = NULL;
    gchar * class_name = NULL;

    if (!TASK_IS_WINDOW (taskwindow)) 
    {
      continue;
    }

    _wnck_get_wmclass (task_window_get_xid (taskwindow),&res_name, &class_name);
    if ( (g_strcmp0 (window, res_name) == 0) ||(g_strcmp0 (window, class_name) == 0) )
    {
      g_free (res_name);
      g_free (class_name);
      return taskwindow;
    }
    g_free (res_name);
    g_free (class_name);
    
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

static void
task_manager_check_for_panel_instance_intersection (TaskManager * manager,
                                     TaskManagerAwnPanelInfo * panel_info,
                                     WnckWorkspace * space,
                                     WnckApplication * app)
{
  TaskManagerPrivate  *priv;
  GList * windows;
  GList * iter;
  gboolean  intersect = FALSE;
  GdkRectangle awn_rect;
  GdkRegion * updated_region;
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  gdk_error_trap_push ();

  gdk_window_get_position (panel_info->foreign_window,&awn_rect.x,&awn_rect.y);
  gdk_drawable_get_size (panel_info->foreign_window,&awn_rect.width,&awn_rect.height);
  /*
   gdk_window_get_geometry gives us an x,y or 0,0
   Fix that using get root origin.
   */
  gdk_window_get_root_origin (panel_info->foreign_window,&awn_rect.x,&awn_rect.y);
  /*
   We cache the region for reuse for situations where the input mask is an empty
   region when the panel is hidden.  In that case we reuse the last good 
   region.
   */
  updated_region = xutils_get_input_shape (panel_info->foreign_window);
  g_return_if_fail (updated_region);
  if (gdk_region_empty(updated_region))
  {
    gdk_region_destroy (updated_region);
  }
  else
  {
    if (panel_info->foreign_region)
    {
      gdk_region_destroy (panel_info->foreign_region);
    }
    panel_info->foreign_region = updated_region; 
    gdk_region_offset (panel_info->foreign_region,awn_rect.x,awn_rect.y);    
  }
  
  /*
   Get the list of windows to check for intersection
   */
  switch (panel_info->intellihide_mode)
  {
    case INTELLIHIDE_WORKSPACE:
      windows = wnck_screen_get_windows (priv->screen);
      break;
    case INTELLIHIDE_GROUP:  /*TODO... Implement this for now same as app*/
    case INTELLIHIDE_APP:
    default:
      if (app)
      {
        windows = wnck_application_get_windows (app);
      }
      else
      {
        windows = wnck_screen_get_windows (priv->screen);
      }
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
    if ( wnck_window_get_window_type (iter->data) == WNCK_WINDOW_DOCK )
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

    if (gdk_region_rect_in (panel_info->foreign_region, &win_rect) != 
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
  if (intersect && panel_info->autohide_cookie)
  {     
    task_manager_panel_connector_uninhibit_autohide (panel_info->connector, panel_info->autohide_cookie);
#ifdef DEBUG
    g_debug ("me eat cookie: %u",panel_info->autohide_cookie);
#endif
    panel_info->autohide_cookie = 0;
  }
  
  /*
   Inhibit Hide if not already doing so
   */
  if (!intersect && !panel_info->autohide_cookie)
  {
    gchar * identifier = g_strdup_printf ("Intellihide:applet_conector=%p",panel_info->connector);
    panel_info->autohide_cookie = task_manager_panel_connector_inhibit_autohide (panel_info->connector, identifier);
    g_free (identifier);
#ifdef DEBUG    
    g_debug ("cookie is %u",panel_info->autohide_cookie);
#endif
  }
  gdk_error_trap_pop();
}

static gboolean
_waiting_for_panel_dbus (TaskManager * manager)
{
  TaskManagerPrivate  *priv;
  
  g_return_val_if_fail (TASK_IS_MANAGER (manager),FALSE);
  priv = manager->priv;
  
  task_manager_check_for_intersection (manager,
                                       wnck_screen_get_active_workspace (priv->screen),
                                       wnck_application_get (wnck_window_get_xid(wnck_screen_get_active_window (priv->screen))));
  return FALSE;
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
  gint64 xid;
  GHashTableIter iter;
  gpointer key,value;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;

  g_hash_table_iter_init (&iter, priv->intellihide_panel_instances);
  while (g_hash_table_iter_next (&iter, &key, &value) )
  {
    TaskManagerAwnPanelInfo * panel_info = value;
    g_object_get (panel_info->connector, "panel-xid", &xid, NULL);
    if (!xid)
    {
      g_timeout_add (1000,(GSourceFunc)_waiting_for_panel_dbus,manager);
    }
    else
    {
      if (!panel_info->foreign_window)
      {
        panel_info->foreign_window = gdk_window_foreign_new ( xid);
      }
      if (panel_info->intellihide_mode)
      {
        task_manager_check_for_panel_instance_intersection(manager,
                                                         panel_info,
                                                         space,
                                                         app);
      }
      else if ( !panel_info->intellihide_mode && panel_info->autohide_cookie)
      {
        task_manager_panel_connector_uninhibit_autohide (panel_info->connector, panel_info->autohide_cookie);
        panel_info->autohide_cookie = 0;
      }
    }                                                           
  }
  return;
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
  
  win = wnck_screen_get_active_window (screen);
  if (!win)
  {
    /*
     This tends to happen when the last window on workspace is moved to a
     different workspace or minimized.  In which case we have a problem if 
     we had intersection and the panel was hidden, it will continue hide.
     So inhibit the autohide if there is no active window.
     */
    task_manager_check_for_intersection (manager,
                                         wnck_screen_get_active_workspace(screen),
                                         NULL);
/*
    if (!priv->autohide_cookie)
    {
      priv->autohide_cookie = awn_applet_inhibit_autohide (AWN_APPLET(manager), "Intellihide");
#ifdef DEBUG    
      g_debug ("%s: cookie is %u",__func__,priv->autohide_cookie);
#endif
    }*/
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

  win = wnck_screen_get_active_window (screen);
  if (!win)
  {
    task_manager_check_for_intersection (manager,
                                         wnck_screen_get_active_workspace(screen),
                                         NULL);
    return;
  }
  
  app = wnck_window_get_application (win);
  space = wnck_screen_get_active_workspace (screen);
  task_manager_check_for_intersection (manager,space,app);
}

static void
task_manager_win_closed_cb (WnckScreen *screen,WnckWindow *window, TaskManager * manager)
{
  TaskManagerPrivate  *priv;
  WnckWindow          *win;
  WnckApplication     *app;
  WnckWorkspace       *space;
  
  g_return_if_fail (TASK_IS_MANAGER (manager));
  priv = manager->priv;
  win = wnck_screen_get_active_window (priv->screen);
  if (!win)
  {
    return;
  }
  app = wnck_window_get_application (win);
  space = wnck_screen_get_active_workspace (priv->screen);
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

static void
task_manager_set_windows_visibility (TaskManager *manager,const gchar * name,gboolean visible)
{
  GSList * iter;
  TaskManagerPrivate *priv;

  priv = manager->priv;
  for (iter = priv->windows;iter; iter=iter->next)
  {
    gchar * res_name=NULL;
    gchar * class_name=NULL;
    TaskItem * item = iter->data;
    
    g_assert (TASK_IS_WINDOW(item));
    task_window_get_wm_class(TASK_WINDOW (item), &res_name, &class_name);    

    if (  (g_strcmp0(name,res_name)==0)||
        (g_strcmp0(name,class_name)==0) )
    {
      task_window_set_hidden (TASK_WINDOW(item),!visible);
    }
    g_free (res_name);
    g_free (class_name);
  }
}

GObject*
task_manager_get_dbus_dispatcher (TaskManager *manager)
{
  TaskManagerPrivate *priv;

  g_return_val_if_fail (TASK_IS_MANAGER (manager), NULL);

  priv = manager->priv;
  return G_OBJECT (priv->dbus_proxy);
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
  else if (G_VALUE_HOLDS_INT (window))
  {
    matched_window = _match_xid (manager, g_value_get_int (window));
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

      TaskItem *item = TASK_ITEM (matched_window);
      task_item_update_overlay (item, key_name, value);

      if (strcmp ("visible", key_name) == 0)
      {
        gboolean visible = g_value_get_boolean (value);
        if (G_VALUE_HOLDS_STRING (window))
        {
          GList * needle = g_list_find_custom (priv->hidden_list,
                                               g_value_get_string(window),
                                               (GCompareFunc)g_strcmp0);
          if (visible)
          {
            if (needle)
            {
              g_free (needle->data);
              priv->hidden_list = g_list_delete_link (priv->hidden_list,needle);
            }
          }
          else
          {
            if (!needle)
            {
              priv->hidden_list = g_list_append (priv->hidden_list, g_value_dup_string(window));
            }
          }
          task_manager_set_windows_visibility (manager,g_value_get_string(window),visible);
        }
        task_window_set_hidden (TASK_WINDOW(matched_window),!visible);
      }
    }
    
    return TRUE;
  }
  else
  {
    GHashTableIter iter;
    gpointer key, value;
    gboolean null_op = TRUE;

    g_hash_table_iter_init (&iter, hints);
    while (g_hash_table_iter_next (&iter, &key, &value)) 
    {
      gchar *key_name = (gchar *)key;
      if (strcmp ("visible", key_name) == 0)
      {
        gboolean visible = g_value_get_boolean (value);
        null_op = FALSE;
        if (G_VALUE_HOLDS_STRING (window))
        {
          GList * needle = g_list_find_custom (priv->hidden_list,
                                               g_value_get_string(window),
                                               (GCompareFunc)g_strcmp0);          
          if (visible)
          {
            if (needle)
            {
              g_free (needle->data);
              priv->hidden_list = g_list_delete_link (priv->hidden_list,needle);
            }
          }
          else
          {
            if (!needle)
            {
              priv->hidden_list = g_list_append (priv->hidden_list, g_value_dup_string(window));
            }
          }
          task_manager_set_windows_visibility (manager,g_value_get_string(window),visible);          
        }
      }
    }
    if (null_op)
    {
      g_set_error (error,
                 task_manager_error_quark (),
                 TASK_MANAGER_ERROR_NO_WINDOW_MATCH,
                 "No matching window found to update.");
      return FALSE;
    }
    return TRUE;
  }
}

void
task_manager_add_icon_show ( TaskManager * manager)
{
  TaskManagerPrivate *priv;
  g_return_if_fail (TASK_IS_MANAGER (manager));
  
  priv = manager->priv;

  if (priv->add_icon)
  {
    gtk_box_reorder_child (GTK_BOX(priv->box),priv->add_icon,-1);
    gtk_widget_show_all (priv->add_icon);
  }
}

gboolean
task_manager_add_icon_hide ( TaskManager * manager)
{
  TaskManagerPrivate *priv;
  g_return_val_if_fail (TASK_IS_MANAGER (manager),FALSE);
  
  priv = manager->priv;
  if (priv->add_icon)
  {
    gtk_widget_hide (priv->add_icon);
    if (priv->add_icon_source)
    {
      g_source_remove (priv->add_icon_source);
      priv->add_icon_source = 0;
    }
  }
  return FALSE;
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

  if (priv->add_icon_source)
  {
    g_source_remove (priv->add_icon_source);
    priv->add_icon_source = 0;
  }

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
  TaskManagerPrivate *priv = TASK_MANAGER_GET_PRIVATE (manager);
  if (priv->add_icon_source)
  {
    g_source_remove (priv->add_icon_source);
  }
  
  priv->add_icon_source = g_timeout_add (4000,(GSourceFunc)task_manager_add_icon_hide,manager);
  /*
  

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
  GList * childs;
  GList * iter;
  const gchar * moved_desktop_file;
  const gchar * following_desktop_file = NULL;
  GValueArray *launcher_paths;
  GValue moving_val = {0,};
  TaskLauncher * launcher = NULL;
  
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

  /*
   Get the name of the desktop file.
   Get the name of the desktop file that next appears after it.
   Search the launcherlist for the moved desktop file.
     If there is no follower then move it to the end.
   Else
    Search the launcherlist for the following desktop file
    Place the moved desktop in front of it.
   */

  if (task_icon_is_ephemeral (priv->dragged_icon) )
  {
    priv->dragged_icon = NULL;
    return;
  }
  
  if ( priv->dragged_icon)
  {
    launcher = TASK_LAUNCHER(task_icon_get_launcher (priv->dragged_icon));
  }  
  
  if (launcher)
  {
    g_assert (TASK_IS_LAUNCHER(launcher));
    moved_desktop_file = task_launcher_get_desktop_path (launcher);
    // get the updated list
    childs = gtk_container_get_children (GTK_CONTAINER(priv->box));
    for (iter = g_list_first (childs);iter;iter=g_list_next(iter))
    {
      if (!TASK_IS_ICON(iter->data))
      {
        continue;
      }
      g_assert (TASK_IS_ICON (iter->data) );      
      if (priv->dragged_icon == iter->data)
      {
        for (iter = g_list_next(iter);iter;iter = g_list_next(iter) )
        {
          if (!iter)
          {
            continue;
          }
          if (!TASK_IS_ICON(iter->data))
          {
            continue;
          }
          g_assert (TASK_IS_ICON (iter->data) );
          if (task_icon_is_ephemeral (iter->data) )
          {
            continue;
          }
          const TaskItem * item = task_icon_get_launcher (iter->data);

          if (!item )
          {
            continue;
          }
          following_desktop_file = task_launcher_get_desktop_path (TASK_LAUNCHER(item));
          break;
        }
        break;
      }
    }
    g_object_get (G_OBJECT (manager), "launcher_paths", &launcher_paths, NULL);
    g_value_init (&moving_val,G_TYPE_STRING);
    g_value_set_string (&moving_val, moved_desktop_file);
    
    for (guint idx = 0; idx < launcher_paths->n_values; idx++)
    {
      gchar *path;
      path = g_value_dup_string (g_value_array_get_nth (launcher_paths, idx));
      if (g_strcmp0 (path,moved_desktop_file)==0)
      {
        g_free (path);
        g_value_array_remove (launcher_paths,idx);
        if (!following_desktop_file)
        {
          g_value_array_append (launcher_paths,&moving_val);
        }
        else
        {
          for (idx = 0; idx < launcher_paths->n_values; idx++)
          {
            path = g_value_dup_string (g_value_array_get_nth (launcher_paths, idx));
            if (g_strcmp0 (path,following_desktop_file)==0)
            {
              g_value_array_insert (launcher_paths,idx,&moving_val);
              g_free (path);
              break;
            }
          }
        }
        g_object_set (G_OBJECT (manager), "launcher_paths", launcher_paths, NULL);                            
        break;
      }
      g_free (path);
      task_manager_refresh_launcher_paths (manager, launcher_paths);
    }
    g_value_unset (&moving_val);    
    g_value_array_free (launcher_paths);
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


static void 
copy_over (const gchar *src, const gchar *dest)
{
  DesktopAgnosticVFSFile *from = NULL;
  DesktopAgnosticVFSFile *to = NULL;
  GError                 *error = NULL;

  from = desktop_agnostic_vfs_file_new_for_path (src, &error);
  if (error)
  {
    goto copy_over_error;
  }
  to = desktop_agnostic_vfs_file_new_for_path (dest, &error);
  if (error)
  {
    goto copy_over_error;
  }

  desktop_agnostic_vfs_file_copy (from, to, TRUE, &error);

copy_over_error:

  if (error)
  {
    g_warning ("Unable to copy %s to %s: %s", src, dest, error->message);
    g_error_free (error);
  }

  if (to)
  {
    g_object_unref (to);
  }
  if (from)
  {
    g_object_unref (from);
  }
}

static void
_icon_dest_drag_data_received (GtkWidget      *widget,
                                   GdkDragContext *context,
                                   gint            x,
                                   gint            y,
                                   GtkSelectionData *sdata,
                                   guint           info,
                                   guint           time_,
                                   TaskManager     *manager)
{
  TaskManagerPrivate * priv;
  GError          *error;
  GdkAtom         target;
  gchar           *target_name;
  gchar           *sdata_data;
  GStrv           tokens = NULL;
  gchar           ** i;  

  g_return_if_fail (AWN_IS_THEMED_ICON (widget) );

  priv = TASK_MANAGER_GET_PRIVATE (manager);
  task_manager_add_icon_hide (TASK_MANAGER(manager));
  
  target = gtk_drag_dest_find_target (widget, context, NULL);
  target_name = gdk_atom_name (target);

  /* If it is dragging of the task icon, there is actually no data */
  if (g_strcmp0("awn/task-icon", target_name) == 0)
  {
    gtk_drag_finish (context, TRUE, TRUE, time_);
    return;
  }
  sdata_data = (gchar*)gtk_selection_data_get_data (sdata);

  /* If we are dealing with a desktop file, then we want to do something else
   * FIXME: This is a crude way of checking
   */
  if (strstr (sdata_data, ".desktop"))
  {
    /*TODO move into a separate function */
    tokens = g_strsplit  (sdata_data, "\n",-1);    
    for (i=tokens; *i;i++)
    {
      gchar * filename = g_filename_from_uri ((gchar*) *i,NULL,NULL);
      if (!filename && ((gchar *)*i))
      {
        filename = g_strdup ((gchar*)*i);
      }
      if (filename)
      {
        g_strstrip(filename);
      }
      if (filename && strlen(filename) && strstr (filename,".desktop"))
      {
        if (filename)
        {
          gboolean make_copy = FALSE;
          struct stat stat_buf;
          switch (priv->desktop_copy)
          {
            case DESKTOP_COPY_ALL:
              make_copy = TRUE;
              break;
            case DESKTOP_COPY_OWNER:
              g_stat (filename,&stat_buf);
              if ( stat_buf.st_uid == getuid())
              {
                make_copy = TRUE;
              }
              break;
            case DESKTOP_COPY_NONE:
            default:
              break;
          }
          if (make_copy)
          {
            gchar * launcher_dir = NULL;
            gchar * file_basename;
            gchar * dest;
            launcher_dir = g_strdup_printf ("%s/.config/awn/launchers", 
                                              g_get_home_dir ());
            g_mkdir_with_parents (launcher_dir,0755); 
            file_basename = g_path_get_basename (filename);
            dest = g_strdup_printf("%s/%lu-%s",launcher_dir,(gulong)time(NULL),file_basename);
            copy_over (filename,dest);
            g_free (file_basename);
            g_free (filename);
            g_free (launcher_dir);
            filename = dest;
          }
          task_manager_append_launcher (TASK_MANAGER(manager),filename);
        }
      }
      if (filename)
      {
        g_free (filename);
      }
    }
    g_strfreev (tokens);
    gtk_drag_finish (context, TRUE, FALSE, time_);
    return;
  }

  error = NULL;
  gtk_drag_finish (context, TRUE, FALSE, time_);
}
