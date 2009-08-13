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

#include <libawn/libawn.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "task-manager.h"
#include "task-manager-api-wrapper.h"

AwnApplet* awn_applet_factory_initp (gchar* name, gchar* uid, gint panel_id);

AwnApplet*
awn_applet_factory_initp (gchar* name, gchar* uid, gint panel_id)
{
  AwnApplet       *applet;
  DBusGConnection *connection;
  DBusGProxy      *proxy;
  GError          *error = NULL;
  guint32          ret;

  // init DBus
  dbus_g_thread_init ();

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (connection == NULL)
  {
    g_warning ("Unable to make connection to the D-Bus session bus: %s",
               error->message);
    g_error_free (error);
    return NULL;
  }

  // prepare to request unique name
  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);
  if (!org_freedesktop_DBus_request_name (proxy,
                                          "org.awnproject.Applet.Taskmanager",
                                          0, &ret, &error))
  {
    g_warning ("There was an error requesting the D-Bus name:%s\n",
               error->message);
    g_error_free (error);
    g_object_unref (proxy);
    dbus_g_connection_unref (connection);
    return NULL;
  }
  // check return value
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
    g_warning ("Another instance of Taskmanager is running\n");
    g_object_unref (proxy);
    dbus_g_connection_unref (connection);
    connection = NULL;
    // return NULL; // FIXME: what to do in this case?
  }

  applet = task_manager_new (name, uid, panel_id);
  
  // We're non-first instance, don't do DBus registering
  if (connection == NULL) return applet;

  dbus_g_connection_register_g_object (connection,
                                       "/org/awnproject/Applet/Taskmanager",
                                       G_OBJECT (applet));

  /* Now expose also the old API */
  if (!org_freedesktop_DBus_request_name (proxy,
                                          "com.google.code.Awn",
                                          0, &ret, &error))
  {
    g_warning ("There was an error requesting the D-Bus name:%s\n",
               error->message);
    g_error_free (error);
    connection = NULL;
  }
  // check return value
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
    g_warning ("Another instance of Taskmanager is running\n");
    connection = NULL;
  }

  if (connection)
  {
    GObject *old_manager = 
      G_OBJECT (task_manager_api_wrapper_new (TASK_MANAGER (applet)));
    dbus_g_connection_register_g_object (connection,
                                         "/com/google/code/Awn",
                                         old_manager);
  }

  return applet;
}
