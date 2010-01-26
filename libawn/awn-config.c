/*
 * Copyright (C) 2009 Mark Lee <avant-wn@lazymalevolence.com>
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
 * Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "awn-config.h"

/**
 * SECTION: awn-config
 * @short_description: Convenience functions for handling dock/applet
 * configuration.
 * @see_also: #DesktopAgnosticConfigClient
 * @since: 0.4.0
 *
 * Functions used by the dock, applets, and preferences dialogs to
 * associate configuration options with the first two categories. Also
 * handles their memory management.
 */

#define SCHEMADIR PKGDATADIR "/schemas"

/* The config client cache. */
static GData* awn_config_clients = NULL;


static void
on_config_destroy (gpointer data)
{
  DesktopAgnosticConfigClient* cfg = (DesktopAgnosticConfigClient*)data;
  g_object_unref (cfg);
}


/**
 * awn_config_get_default:
 * @panel_id: The ID of the panel that is associated with the configuration
 * client.
 * @error: The address of the #GError object, if an error occurs.
 *
 * Looks up or creates a configuration client that is associated with the
 * panel specified.
 *
 * Returns: A borrowed reference to a configuration client object.
 */
DesktopAgnosticConfigClient*
awn_config_get_default (gint panel_id, GError** error)
{
  char *instance_id = NULL;
  DesktopAgnosticConfigClient *client = NULL;

  if (awn_config_clients == NULL)
  {
    /* initialize datalist */
    g_datalist_init (&awn_config_clients);
  }
  instance_id = g_strdup_printf ("panel-%d", panel_id);
  client = (DesktopAgnosticConfigClient*)g_datalist_get_data (&awn_config_clients,
                                                              instance_id);
  if (client == NULL)
  {
    char *schema_filename;

    schema_filename = g_build_filename (SCHEMADIR, "avant-window-navigator.schema-ini", NULL);
    if (panel_id != 0)
    {
      client =
        desktop_agnostic_config_client_new_for_instance (schema_filename,
                                                         instance_id,
                                                         error);
    }
    else
    {
      client = desktop_agnostic_config_client_new (schema_filename);
    }
    g_free (schema_filename);
    if (error && *error != NULL)
    {
      if (client != NULL)
      {
        g_object_unref (client);
      }
      g_free (instance_id);
      return NULL;
    }
    g_datalist_set_data_full (&awn_config_clients, instance_id, client,
                              on_config_destroy);
  }
  g_free (instance_id);
  return client;
}


/**
 * awn_config_free:
 *
 * Properly frees all of the config clients in the cache.
 *
 * Should be called on dock shutdown.
 */
void
awn_config_free (void)
{
  g_datalist_clear (&awn_config_clients);
}


/**
 * awn_config_get_default_for_applet:
 * @applet: The applet.
 * @error: The address of the #GError object, if an error occurs.
 *
 * Looks up or creates a configuration client that is associated with the
 * given applet.
 *
 * Returns: A borrowed reference to the configuration client associated with
 * the applet specified via the metadata.
 */
DesktopAgnosticConfigClient*
awn_config_get_default_for_applet (AwnApplet *applet, GError **error)
{
  g_return_val_if_fail (applet != NULL, NULL);

  gchar *canonical_name = NULL;
  gchar *uid = NULL;
  DesktopAgnosticConfigClient *client = NULL;

  g_object_get (G_OBJECT (applet),
                "canonical-name", &canonical_name,
                "uid", &uid,
                NULL);

  g_return_val_if_fail (canonical_name != NULL, NULL);

  client = awn_config_get_default_for_applet_by_info (canonical_name,
                                                      uid,
                                                      error);

  if (uid != NULL)
  {
    g_free (uid);
  }

  g_free (canonical_name);

  return client;
}


/**
 * awn_config_get_default_for_applet_by_info:
 * @name: The canonical applet name.
 * @uid: The UID of the applet (may not be %NULL).
 * @error: The address of the #GError object, if an error occurs.
 *
 * Looks up or creates a configuration client that is associated with the
 * canonical name of an applet and an optional UID.
 * Should only be used by code where the #AwnApplet object is not present,
 * such as the dock's preferences dialog.
 *
 * Returns: A borrowed reference to the configuration client associated with
 * the applet specified via the metadata.
 */
DesktopAgnosticConfigClient*
awn_config_get_default_for_applet_by_info (const gchar  *name,
                                           const gchar  *uid,
                                           GError      **error)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (uid != NULL, NULL);

  gchar *instance_id;
  DesktopAgnosticConfigClient *client = NULL;

  if (awn_config_clients == NULL)
  {
    /* initialize datalist */
    g_datalist_init (&awn_config_clients);
  }

  instance_id = g_strdup_printf ("awn-applet-%s-%s", name, uid);

  client = (DesktopAgnosticConfigClient*)g_datalist_get_data (&awn_config_clients,
                                                              instance_id);

  if (client == NULL)
  {
    gchar *schema_basename;
    gchar *schema_filename;
    DesktopAgnosticConfigSchema *schema;

    schema_basename = g_strdup_printf ("awn-applet-%s.schema-ini", name);

    schema_filename = g_build_filename (SCHEMADIR, schema_basename, NULL);

    g_free (schema_basename);

    schema = desktop_agnostic_config_schema_new (schema_filename, error);

    g_free (schema_filename);

    if (error && *error != NULL)
    {
      if (schema != NULL)
      {
        g_object_unref (schema);
      }
      g_free (instance_id);
      return NULL;
    }

    client = desktop_agnostic_config_client_new_for_schema (schema,
                                                            instance_id,
                                                            error);

    g_object_unref (schema);

    if (error && *error != NULL)
    {
      if (client != NULL)
      {
        g_object_unref (client);
      }
      g_free (instance_id);
      return NULL;
    }
    g_datalist_set_data_full (&awn_config_clients, instance_id, client,
                              on_config_destroy);
  }
  g_free (instance_id);
  return client;
}
