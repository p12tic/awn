/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "awn-dbus-watcher.h"

static AwnDBusWatcher *singleton_instance = NULL;

G_DEFINE_TYPE (AwnDBusWatcher, awn_dbus_watcher, G_TYPE_OBJECT)

#define AWN_DBUS_WATCHER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        AWN_TYPE_DBUS_WATCHER, AwnDBusWatcherPrivate))

struct _AwnDBusWatcherPrivate
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
};

enum
{
  NAME_APPEARED,
  NAME_DISAPPEARED,

  LAST_SIGNAL
};

static guint _dbus_watcher_signals[LAST_SIGNAL] = { 0 };

static void
on_name_owner_changed (DBusGProxy *proxy,
                       gchar *name, gchar *old_owner, gchar *new_owner,
                       AwnDBusWatcher *watcher)
{
  g_return_if_fail (old_owner && new_owner);

#if 0
  g_debug ("NameOwnerChanged (%s): '%s' -> '%s'",
           name, old_owner, new_owner);
#endif

  if (old_owner[0] == '\0' || new_owner[0] == '\0')
  {
    // there's either a new name on the bus, or a name disappeared
    guint signal_id = _dbus_watcher_signals[old_owner[0] == '\0' ?
                                            NAME_APPEARED : NAME_DISAPPEARED];
    GQuark owner_quark = g_quark_try_string (name);
    // if there isn't quark created for the dbus name, it means that
    // noone called g_signal_connect with this detail name, and we don't need
    // to create it
    g_signal_emit (watcher, signal_id, owner_quark, name);
  }
}

static void
awn_dbus_watcher_init (AwnDBusWatcher *watcher)
{
  AwnDBusWatcherPrivate *priv = AWN_DBUS_WATCHER_GET_PRIVATE (watcher);
  GError *error = NULL;

  watcher->priv = priv;

  priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (error != NULL)
  {
    g_warning ("Unable to make connection to the D-Bus session bus: %s",
               error->message);
    g_error_free (error);
    return;
  }

  priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                           "org.freedesktop.DBus", 
                                           "/org/freedesktop/DBus",
                                           "org.freedesktop.DBus");

  g_return_if_fail (priv->proxy);

  dbus_g_proxy_add_signal (priv->proxy, "NameOwnerChanged",
                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->proxy, "NameOwnerChanged",
                               G_CALLBACK (on_name_owner_changed), watcher,
                               NULL);
}

static void
awn_dbus_watcher_finalize (GObject *object)
{
  AwnDBusWatcherPrivate *priv = AWN_DBUS_WATCHER_GET_PRIVATE (object);

  if (priv->proxy)
  {
    g_object_unref (priv->proxy);
    priv->proxy = NULL;
  }

  if (priv->connection)
  {
    dbus_g_connection_unref (priv->connection);
    priv->connection = NULL;
  }

  singleton_instance = NULL;

  G_OBJECT_CLASS (awn_dbus_watcher_parent_class)->finalize (object);
}

static void
awn_dbus_watcher_class_init (AwnDBusWatcherClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = awn_dbus_watcher_finalize;

  /* Add signals to the class */
  _dbus_watcher_signals[NAME_APPEARED] =
                g_signal_new ("name-appeared",
                              G_OBJECT_CLASS_TYPE (obj_class),
                              G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                              G_STRUCT_OFFSET (AwnDBusWatcherClass,
                                               name_appeared),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

  _dbus_watcher_signals[NAME_DISAPPEARED] =
                g_signal_new ("name-disappeared",
                              G_OBJECT_CLASS_TYPE (obj_class),
                              G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                              G_STRUCT_OFFSET (AwnDBusWatcherClass,
                                               name_disappeared),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

  g_type_class_add_private (obj_class, sizeof (AwnDBusWatcherPrivate));
}

AwnDBusWatcher*
awn_dbus_watcher_get_default (void)
{
  if (singleton_instance == NULL)
  {
    singleton_instance = g_object_new (AWN_TYPE_DBUS_WATCHER, NULL);
  }

  return singleton_instance;
}


gboolean 
awn_dbus_watcher_has_name (AwnDBusWatcher* self, const gchar* name)
{
  AwnDBusWatcherPrivate *priv = AWN_DBUS_WATCHER_GET_PRIVATE (self);
  GError *error = NULL;
  gboolean success, has_owner;
  
  success = dbus_g_proxy_call (priv->proxy, "NameHasOwner", &error, G_TYPE_STRING, name, G_TYPE_INVALID, G_TYPE_BOOLEAN, &has_owner, G_TYPE_INVALID);
  
  if (!success) 
  {
    g_warning ("Unable to make get dbus connections: %s",
               error->message);
    g_error_free (error);
    return FALSE;
  }
  
  return has_owner;  
}




