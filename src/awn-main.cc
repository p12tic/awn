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

#include <gtk/gtk.h>
#include <libintl.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libdesktop-agnostic/vfs.h>

#include "awn-app.h"
#include "awn-defines.h"

static gboolean version = FALSE;
static gboolean is_startup = FALSE;
static gint startup_delay = 0;

GOptionEntry entries[] = 
{
  {
    "version", 'v', 
    0, G_OPTION_ARG_NONE, 
    &version, 
    "Prints the version number", NULL 
  },
  {
    "startup", 0,
    0, G_OPTION_ARG_NONE,
    &is_startup,
    "Hint the panel that this is start of the session", NULL
  },
  {
    "startup-delay", 0,
    0, G_OPTION_ARG_INT,
    &startup_delay,
    "Number of seconds to wait before starting", NULL
  },
  {
    NULL 
  }
};

static gboolean
startup_timeout (gpointer data)
{
  GMainLoop *loop = (GMainLoop*) data;

  g_main_loop_quit (loop);

  return FALSE;
}

gint 
main (gint argc, gchar *argv[])
{
  AwnApplication  *app;
  GOptionContext  *context;
  DBusGConnection *connection;
  DBusGProxy      *proxy;
  GError          *error = NULL;
  guint32          ret;
 
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

  desktop_agnostic_vfs_init (&error);
  if (error)
  {
    g_critical ("Error initializing VFS subsystem: %s", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

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
    g_object_unref (proxy); 
    dbus_g_connection_unref (connection);  
    return EXIT_FAILURE;
  }
  /* Check the returned value */
  if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
    g_warning ("Another instance of Awn is running\n");
    g_object_unref (proxy); 
    dbus_g_connection_unref (connection);
    return EXIT_SUCCESS;
  }

  /* Set localization stuff */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  /* wait for the compositor */
  if (is_startup)
  {
    GMainLoop *wait_loop = g_main_loop_new (NULL, FALSE);
    g_timeout_add (2000 + (startup_delay * 1000), startup_timeout, wait_loop);
    g_main_loop_run (wait_loop);

    if (startup_delay == 0)
    {
      // wait up to 25 seconds for the compositor
      int i = 0;
      GdkScreen *screen = gdk_screen_get_default ();
      while (i++ < 25)
      {
        if (gdk_screen_is_composited (screen))
        {
          break;
        }
        g_timeout_add (1000, startup_timeout, wait_loop);
        g_main_loop_run (wait_loop);
      }
    }
    g_main_loop_unref (wait_loop);
  }

  /* Launch Awn */
  app = awn_application_get_default ();

  g_unsetenv ("DESKTOP_AUTOSTART_ID");
  gtk_main ();

  g_object_unref (app);
  g_object_unref (proxy);
  dbus_g_connection_unref (connection);

  desktop_agnostic_vfs_shutdown (&error);
  if (error)
  {
    g_critical ("Error shutting down VFS subsystem: %s", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
