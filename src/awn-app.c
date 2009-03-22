/*
 * Copyright (C) 2008 Neil J. Patel <njpatel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

/*
 * AwnApp is the 'controller' for the panel. It will launch the panel with the
 * correct settings.
 *
 * In the future, AwnApp will be responsible for launching each Awn panel 
 * separately, and dealing with the configuration backend for each  panel.
 */

#include <glib.h>

#include <stdio.h>
#include <string.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libawn/awn-config-client.h>

#include "awn-app.h"
#include "awn-defines.h"
#include "awn-panel.h"
#include "awn-app-glue.h"
#include <libawn/awn-utils.h>


G_DEFINE_TYPE (AwnApp, awn_app, G_TYPE_OBJECT)

#define AWN_APP_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
        AWN_TYPE_APP, AwnAppPrivate))

struct _AwnAppPrivate
{
  DBusGConnection *connection;
  AwnConfigClient *client;

  GHashTable *panels;
};

/* GObject functions */
static void
awn_app_finalize (GObject *app)
{
  AwnAppPrivate *priv;
  
  g_return_if_fail (AWN_IS_APP (app));
  priv = AWN_APP (app)->priv;

  awn_config_client_free (priv->client);

  G_OBJECT_CLASS (awn_app_parent_class)->finalize (app);
}

static void
awn_app_class_init (AwnAppClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->finalize = awn_app_finalize;

  g_type_class_add_private (obj_class, sizeof (AwnAppPrivate));
  
  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass), 
                                   &dbus_glib_awn_app_object_info);
}

static void
awn_app_init (AwnApp *app)
{
  AwnAppPrivate *priv;
  GtkWidget *panel;
  GError *error = NULL;
    
  priv = app->priv = AWN_APP_GET_PRIVATE (app);

  priv->panels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  priv->client = awn_config_client_new ();

  /* Grab a connection to the bus */
  priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (priv->connection == NULL)
  {
    g_warning ("Unable to make connection to the D-Bus session bus: %s",
               error->message);
    g_error_free (error);
    gtk_main_quit ();
  }

  panel = awn_panel_new_from_config (priv->client);

  gchar *object_path = g_strdup_printf (AWN_DBUS_PANEL_PATH "%u",
                                        g_hash_table_size (priv->panels) + 1);

  dbus_g_connection_register_g_object (priv->connection, 
                                       object_path, G_OBJECT (panel));

  g_hash_table_insert (priv->panels, object_path, panel);

  gtk_widget_show (panel);
}

gboolean
awn_app_get_panels (AwnApp *app, GPtrArray **panels)
{
  AwnAppPrivate *priv;
  GList *list, *l;
  guint size;

  priv = app->priv;

  *panels = g_ptr_array_sized_new (g_hash_table_size (priv->panels));

  list = l = g_hash_table_get_keys (priv->panels); // list should be freed

  while (list)
  {
    g_ptr_array_add (*panels, g_strdup (list->data));

    list = list->next;
  }

  g_list_free (l);

  return TRUE;
}

AwnApp*
awn_app_get_default (void)
{
  static AwnApp *app = NULL;
  
  if (app == NULL)
    app = g_object_new (AWN_TYPE_APP, 
                         NULL);

  return app;
}

