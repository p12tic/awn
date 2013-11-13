/* task-manager-panel-connector.c */

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <X11/Xlib.h>
#include <math.h>

#include "libawn/awn-defines.h"
#include "libawn/libawn-marshal.h"

#include "task-manager-panel-connector.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


G_DEFINE_TYPE (TaskManagerPanelConnector, task_manager_panel_connector, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TASK_MANAGER_TYPE_PANEL_CONNECTOR, TaskManagerPanelConnectorPrivate))

typedef struct _TaskManagerPanelConnectorPrivate TaskManagerPanelConnectorPrivate;

struct _TaskManagerPanelConnectorPrivate 
{
  gint panel_id;
  gint64 panel_xid;
  
  DBusGConnection *connection;
  DBusGProxy      *proxy;
};


enum
{
  PROP_0,
  PROP_PANEL_ID,
  PROP_PANEL_XID,
};



static void
on_prop_changed (DBusGProxy *proxy, const gchar *prop_name,
                 GValue *value, TaskManagerPanelConnector * conn)
{
  TaskManagerPanelConnectorPrivate * priv;
  g_return_if_fail (TASK_MANAGER_IS_PANEL_CONNECTOR (conn));
  priv = GET_PRIVATE(conn);

  /*
   Should probably just support all the props of panel
   */
  if ( (g_strcmp0 (prop_name,"panel-id")==0) ||
        (g_strcmp0 (prop_name,"panel-xid")==0) )
  {
    g_debug ("Setting %s",prop_name);
    g_object_set_property (G_OBJECT (conn), prop_name, value);
  }
}


static void
task_manager_panel_connector_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  TaskManagerPanelConnectorPrivate * priv = GET_PRIVATE(object);
  switch (property_id)
  {
    case PROP_PANEL_ID:
      g_value_set_int (value, priv->panel_id);
      break;

    case PROP_PANEL_XID:
      g_value_set_int64 (value, priv->panel_xid);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);      
  }
}

static void
task_manager_panel_connector_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  TaskManagerPanelConnectorPrivate * priv = GET_PRIVATE(object);  
  switch (property_id)
  {
    case PROP_PANEL_ID:
      priv->panel_id = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
task_manager_panel_connector_dispose (GObject *object)
{
  G_OBJECT_CLASS (task_manager_panel_connector_parent_class)->dispose (object);
}

static void
task_manager_panel_connector_finalize (GObject *object)
{
  TaskManagerPanelConnectorPrivate * priv = GET_PRIVATE(object);
  if (priv->connection)
  {
    if (priv->proxy) 
    {
      g_object_unref (priv->proxy);
    }
    dbus_g_connection_unref (priv->connection);
    priv->connection = NULL;
    priv->proxy = NULL;
  }
  
  G_OBJECT_CLASS (task_manager_panel_connector_parent_class)->finalize (object);
}

static void
task_manager_panel_connector_init (TaskManagerPanelConnector *self)
{
  TaskManagerPanelConnectorPrivate * priv = GET_PRIVATE(self);
  GError         *error = NULL;
  
  priv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
  priv->proxy = NULL;
  priv->panel_xid = 0;
  
  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free(error);
  }
}

static gboolean
task_manager_panel_connector_do_connect_dbus (GObject * conn)
{
  TaskManagerPanelConnectorPrivate * priv = GET_PRIVATE(conn);

  gchar *object_path = g_strdup_printf ("/org/awnproject/Awn/Panel%d",
                                        priv->panel_id);
  if (!priv->proxy)
  {
    priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                           "org.awnproject.Awn",
                                           object_path,
                                           "org.awnproject.Awn.Panel");
  }
  if (!priv->proxy)
  {
    g_warning("Could not connect to mothership! Bailing\n");
    return TRUE;
  }

  dbus_g_object_register_marshaller (
    libawn_marshal_VOID__STRING_BOXED,
    G_TYPE_NONE, G_TYPE_STRING, G_TYPE_VALUE,
    G_TYPE_INVALID
  );

  dbus_g_proxy_add_signal (priv->proxy, "PropertyChanged",
                           G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (priv->proxy, "PropertyChanged",
                               G_CALLBACK (on_prop_changed), conn,
                               NULL);
  // get prop values from Panel
  DBusGProxy *prop_proxy = dbus_g_proxy_new_from_proxy (
    priv->proxy, "org.freedesktop.DBus.Properties", NULL
  );

  if (!prop_proxy)
  {
    g_warning("Could not get property values! Bailing\n");
  }

  GError *error = NULL;

#if HAVE_DBUS_GLIB_080
  GHashTable *all_props = NULL;

  // doing GetAll reduces DBus lag significantly
  dbus_g_proxy_call (prop_proxy, "GetAll", &error,
                     G_TYPE_STRING, "org.awnproject.Awn.Panel",
                     G_TYPE_INVALID,
                     dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, 
                                          G_TYPE_VALUE), &all_props,
                     G_TYPE_INVALID);

  if (error) goto crap_out;

  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, all_props);
  while (g_hash_table_iter_next (&iter, &key, &value))
  {
    if (strcmp (key, "PanelXid") == 0)
    {
      priv->panel_xid = g_value_get_int64 (value);
    }
  }

#else
  GValue panel_xid = {0,};
  
  dbus_g_proxy_call (prop_proxy, "Get", &error,
                     G_TYPE_STRING, "org.awnproject.Awn.Panel",
                     G_TYPE_STRING, "PanelXid",
                     G_TYPE_INVALID,
                     G_TYPE_VALUE, &panel_xid,
                     G_TYPE_INVALID);

  if (error) goto crap_out;

  if (G_VALUE_HOLDS_INT64 (&panel_xid))
  {
    priv->panel_xid = g_value_get_int64 (&panel_xid);
  }

  g_value_unset (&panel_xid);
#endif
  if (prop_proxy) g_object_unref (prop_proxy);

  g_free (object_path);
  return FALSE;

  crap_out:

  g_warning ("%s", error->message);
  g_error_free (error);
  g_free (object_path);
  if (prop_proxy) 
  {
    g_object_unref (prop_proxy);
  }
  return TRUE;
}

static void
task_manager_panel_connector_constructed (GObject *obj)
{
  TaskManagerPanelConnector *conn = TASK_MANAGER_PANEL_CONNECTOR(obj);
  TaskManagerPanelConnectorPrivate *priv = GET_PRIVATE(conn);

  if (priv->panel_id > 0)
  {
    task_manager_panel_connector_do_connect_dbus (obj);
  }
}

static void
task_manager_panel_connector_class_init (TaskManagerPanelConnectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = task_manager_panel_connector_get_property;
  object_class->set_property = task_manager_panel_connector_set_property;
  object_class->dispose = task_manager_panel_connector_dispose;
  object_class->finalize = task_manager_panel_connector_finalize;
  object_class->constructed = task_manager_panel_connector_constructed;

  g_object_class_install_property (object_class,
    PROP_PANEL_ID,
    g_param_spec_int ("panel-id",
                      "Panel ID",
                      "The id of AwnPanel this connector connects to",
                      0, G_MAXINT, 0,
                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property ( object_class,
    PROP_PANEL_XID,
    g_param_spec_int64 ("panel-xid",
                        "Panel XID",
                        "The XID of AwnPanel this connector is connected to",
                        G_MININT64, G_MAXINT64, 0,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (klass, sizeof (TaskManagerPanelConnectorPrivate));
}

TaskManagerPanelConnector*
task_manager_panel_connector_new (gint id)
{
  return g_object_new (TASK_MANAGER_TYPE_PANEL_CONNECTOR, 
                       "panel_id",id,
                       NULL);
}

guint
task_manager_panel_connector_inhibit_autohide (TaskManagerPanelConnector *conn, const gchar *reason)
{
  TaskManagerPanelConnectorPrivate *priv;
  GError *error = NULL;
  guint ret = 0;
  
  g_return_val_if_fail (TASK_MANAGER_IS_PANEL_CONNECTOR (conn), 0);
  priv = GET_PRIVATE(conn);

  if (!priv->proxy)
  {
    return 0;
  }

  gchar *app_name = g_strdup_printf ("%s:%d", g_get_prgname(), getpid());

  dbus_g_proxy_call (priv->proxy, "InhibitAutohide",
                     &error,
                     G_TYPE_STRING, app_name,
                     G_TYPE_STRING, reason,
                     G_TYPE_INVALID, 
                     G_TYPE_UINT, &ret,
                     G_TYPE_INVALID);

  if (app_name) g_free (app_name);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  return ret;
}

void
task_manager_panel_connector_uninhibit_autohide (TaskManagerPanelConnector *conn, guint cookie)
{
  TaskManagerPanelConnectorPrivate *priv;
  GError *error = NULL;
  
  g_return_if_fail (TASK_MANAGER_IS_PANEL_CONNECTOR (conn));
  priv = GET_PRIVATE(conn);

  if (!priv->proxy)
  {
    return;
  }

  dbus_g_proxy_call (priv->proxy, "UninhibitAutohide",
                     &error,
                     G_TYPE_UINT, cookie,
                     G_TYPE_INVALID,
                     G_TYPE_INVALID);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }
}


