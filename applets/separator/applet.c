/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 */

#include <libawn/libawn.h>
#include <dbus/dbus-glib.h>

AwnApplet* awn_applet_factory_initp (gchar* name, gchar* uid, gint panel_id);

AwnApplet*
awn_applet_factory_initp (gchar* name, gchar* uid, gint panel_id)
{
  DBusGProxy *proxy;
  GError *error = NULL;

  gchar *object_path = g_strdup_printf ("/org/awnproject/Awn/Panel%d",
                                        panel_id);
  proxy = dbus_g_proxy_new_for_name (dbus_g_bus_get (DBUS_BUS_SESSION, &error),
                                     "org.awnproject.Awn",
                                     object_path,
                                     "org.awnproject.Awn.Panel");

  dbus_g_proxy_call (proxy, "SetAppletFlags",
                     &error,
                     G_TYPE_STRING, uid,
                     G_TYPE_INT, AWN_APPLET_IS_SEPARATOR,
                     G_TYPE_INVALID, G_TYPE_INVALID);

  return NULL;
}

