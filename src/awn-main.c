/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libawn/awn-vfs.h>

#include "awn-app.h"
#include "awn-defines.h"

static gboolean version = FALSE;

GOptionEntry entries[] = 
{
  {
    "version", 'v', 
    0, G_OPTION_ARG_NONE, 
    &version, 
    "Prints the version number", NULL 
  },
  { 
    NULL 
  }
};

    
gint 
main (gint argc, gchar *argv[])
{
  AwnApp *app;
  GOptionContext *context;
  DBusGConnection *connection;
  DBusGProxy *proxy;
  GError *error = NULL;
  guint32 ret;
 
  context = g_option_context_new ("- Avant Window Navigator " VERSION);
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_parse (context, &argc, &argv, NULL);
  g_option_context_free(context);

  if (version) 
  {
    g_print("Avant Window Navigator %s\n", VERSION);
    return EXIT_SUCCESS;
  }

  /* Init the world */
  if (!g_thread_supported ()) 
    g_thread_init (NULL);

  dbus_g_thread_init ();
  g_type_init ();
  gtk_init (&argc, &argv);
  awn_vfs_init ();

  /* Single instance checking; first get the D-Bus connection */
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (connection == NULL)
  {
    g_warning ("Unable to make connection to the D-Bus session bus: %s",
               error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

  /* Try and request the Awn namespace */
  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);
  if (!org_freedesktop_DBus_request_name (proxy,
                                          AWN_DBUS_NAMESPACE,
                                          0, &ret, &error))
  {
    g_warning ("There was an error requesting the D-Bus name:%s\n",
               error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }
  /* Check the returned value */
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
    g_warning ("Another instance of Awn is running\n");
    dbus_g_connection_unref (connection);
    return EXIT_SUCCESS;
  }

  /* Launch Awn */
  app = awn_app_get_default ();

  gtk_main ();

  g_object_unref (G_OBJECT (app));
	
  return EXIT_SUCCESS;
}
