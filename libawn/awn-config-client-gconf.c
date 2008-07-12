/*
 *  Copyright (C) 2007, 2008 Mark Lee <avant-wn@lazymalevolence.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the
 *  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gconf/gconf-client.h>

#if GLIB_CHECK_VERSION(2,15,0)
#include <glib/gchecksum.h>
#else
#include "egg/eggchecksum.h"
#endif

#include "libawn/awn-config-client.h"
#include "libawn/awn-defines.h"
#include "awn-config-client-shared.c"

struct _AwnConfigClient
{
	GConfClient *client;
	gchar *path; /* key prefix */
};

typedef struct {
	AwnConfigClient *client;
	AwnConfigClientNotifyFunc callback;
	gpointer data;
} AwnConfigClientNotifyData;

/* returns a newly allocated string */
static gchar *awn_config_client_generate_key (AwnConfigClient *client, const gchar *group, const gchar *key)
{
	if (key == NULL) {
		if (strcmp (group, AWN_CONFIG_CLIENT_DEFAULT_GROUP) == 0) {
			return g_strdup (client->path);
		} else {
			return g_strconcat (client->path, "/", group, NULL);
		}
	} else {
		if (strcmp (group, AWN_CONFIG_CLIENT_DEFAULT_GROUP) == 0) {
			return g_strconcat (client->path, "/", key, NULL);
		} else {
			return g_strconcat (client->path, "/", group, "/", key, NULL);
		}
	}
}

static void awn_config_client_notify_proxy (GConfClient *client, guint cid, GConfEntry *entry, AwnConfigClientNotifyData* notify)
{
	AwnConfigClientNotifyEntry *awn_entry = g_new (AwnConfigClientNotifyEntry, 1);
	GConfValue *value = NULL;
	value = gconf_entry_get_value (entry);
	if (value) {
		gchar **exploded = g_strsplit (gconf_entry_get_key (entry), "/", 0);
		guint exploded_len = g_strv_length (exploded);
		gchar **base_exploded = g_strsplit (notify->client->path, "/", 0);
		guint base_exploded_len = g_strv_length (base_exploded);
		g_strfreev (base_exploded);
		g_return_if_fail (exploded_len >= 2);
		awn_entry->client = notify->client;
		if (exploded_len - base_exploded_len == 1) { /* special case: top-level dock/applet keys */
			awn_entry->group = g_strdup (AWN_CONFIG_CLIENT_DEFAULT_GROUP);
		} else {
			awn_entry->group = g_strdup (exploded[exploded_len - 2]);
		}
		awn_entry->key = g_strdup (exploded[exploded_len - 1]);
		g_strfreev (exploded);
		switch (value->type) {
			case GCONF_VALUE_BOOL:
				awn_entry->value.bool_val = gconf_value_get_bool (value);
				break;
			case GCONF_VALUE_FLOAT:
				awn_entry->value.float_val = (gfloat)gconf_value_get_float (value);
				break;
			case GCONF_VALUE_INT:
				awn_entry->value.int_val = gconf_value_get_int (value);
				break;
			case GCONF_VALUE_LIST:
				awn_entry->value.list_val = gconf_client_get_list (client, entry->key, gconf_value_get_list_type (value), NULL);
				break;
			case GCONF_VALUE_STRING:
				awn_entry->value.str_val = (gchar*)gconf_value_get_string (value);
				break;
			default:
				/* FIXME do an error? */
				g_warning ("Invalid GConf value type!");
				break;
		}
		(notify->callback) (awn_entry, notify->data);
		g_free (awn_entry->group);
		g_free (awn_entry->key);
	}
	g_free (awn_entry);
}

static AwnConfigClient *awn_config_client_new_with_path (gchar *path, gchar *name)
{
	AwnConfigClient *client = g_new (AwnConfigClient, 1);
	client->path = path;
	client->client = gconf_client_get_default ();
	if (!gconf_client_dir_exists (client->client, client->path, NULL)) {
		GError *err = NULL;
		awn_config_client_load_defaults_from_schema (client, &err);
		if (err) {
			g_error ("Error loading the schema: '%s'", err->message);
			g_error_free (err);
		}
	}
	gconf_client_add_dir (client->client, AWN_GCONF_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
	return client;
}

AwnConfigClient *awn_config_client_new ()
{
	static AwnConfigClient *awn_dock_config = NULL;
	if (!awn_dock_config) {
		awn_dock_config = awn_config_client_new_with_path (AWN_GCONF_PATH, NULL);
	}
	return awn_dock_config;
}

AwnConfigClient *awn_config_client_new_for_applet (gchar *name, gchar *uid)
{
	AwnConfigClient *client;
	gchar *gconf_key = NULL;
	if (uid) {
		gconf_key = g_strconcat (AWN_GCONF_PATH, "/applets/", uid, NULL);
	} else {
		gconf_key = g_strconcat (AWN_GCONF_PATH, "/applets/", name, NULL);
	}
	client = awn_config_client_new_with_path (g_strdup (gconf_key), name);
	g_free (gconf_key);
	return client;
}

/**
 * awn_config_client_query_backend :
 *
 * Returns: An enum value indicating the backend in use.
 */
AwnConfigBackend  awn_config_client_query_backend (void)
{
  return AWN_CONFIG_CLIENT_GKEYFILE;  
}


void awn_config_client_clear (AwnConfigClient *client, GError **err)
{
	/* only do it for applets on gconf */
	if (strcmp (AWN_GCONF_PATH, client->path) != 0) {
		gconf_client_remove_dir (client->client, client->path, err);
	}
}

void awn_config_client_ensure_group (AwnConfigClient *client, const gchar *group)
{
	gchar *gconf_key = awn_config_client_generate_key (client, group, NULL);
	if (!gconf_client_dir_exists (client->client, gconf_key, NULL)) {
		gconf_client_add_dir (client->client, gconf_key, GCONF_CLIENT_PRELOAD_NONE, NULL);
	}
	g_free (gconf_key);
}

void awn_config_client_notify_add (AwnConfigClient *client, const gchar *group, 
				   const gchar *key, AwnConfigClientNotifyFunc callback,
				   gpointer user_data)
{
	AwnConfigClientNotifyData *notify = g_new0 (AwnConfigClientNotifyData, 1);
	notify->callback = callback;
	notify->data = user_data;
	gchar *full_key = awn_config_client_generate_key (client, group, key);
	notify->client = client;
	GError *err = NULL;
	guint notify_id = gconf_client_notify_add (client->client, full_key, (GConfClientNotifyFunc)awn_config_client_notify_proxy, notify, NULL, &err);
	if (notify_id == 0 || err) {
		g_warning ("Something went wrong when we tried to add a notification callback: %s", err->message);
		g_error_free (err);
	}
	g_free (full_key);
}

gboolean awn_config_client_entry_exists (AwnConfigClient *client, const gchar *group, const gchar *key)
{
	GConfValue *value = NULL;
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	value = gconf_client_get (client->client, gconf_key, NULL);
	g_free (gconf_key);
	return value != NULL;
}

void awn_config_client_load_defaults_from_schema (AwnConfigClient *client, GError **err)
{
	(void)g_spawn_command_line_async ("killall -HUP gconfd-2", err);
}

AwnConfigValueType awn_config_client_get_value_type (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
	AwnConfigValueType value_type;
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	GConfValue *value = gconf_client_get (client->client, gconf_key, err);
	if (value) {
		switch (value->type) {
			case GCONF_VALUE_BOOL:
				value_type = AWN_CONFIG_VALUE_TYPE_BOOL;
				break;
			case GCONF_VALUE_FLOAT:
				value_type = AWN_CONFIG_VALUE_TYPE_FLOAT;
				break;
			case GCONF_VALUE_INT:
				value_type = AWN_CONFIG_VALUE_TYPE_INT;
				break;
			case GCONF_VALUE_STRING:
				value_type = AWN_CONFIG_VALUE_TYPE_STRING;
				break;
			case GCONF_VALUE_LIST: {
				switch (gconf_value_get_list_type (value)) {
					case GCONF_VALUE_BOOL:
						value_type = AWN_CONFIG_VALUE_TYPE_LIST_BOOL;
						break;
					case GCONF_VALUE_FLOAT:
						value_type = AWN_CONFIG_VALUE_TYPE_LIST_FLOAT;
						break;
					case GCONF_VALUE_INT:
						value_type = AWN_CONFIG_CLIENT_LIST_TYPE_INT;
						break;
					case GCONF_VALUE_STRING:
						value_type = AWN_CONFIG_VALUE_TYPE_LIST_STRING;
						break;
					default:
						value_type = AWN_CONFIG_VALUE_TYPE_NULL;
						break;
				}
				break;
			} default:
				value_type = AWN_CONFIG_VALUE_TYPE_NULL;
				break;
		}
	} else {
		value_type = AWN_CONFIG_VALUE_TYPE_NULL;
	}
	g_free (gconf_key);
	return value_type;
}

gboolean awn_config_client_get_bool (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
    gchar *gconf_key = awn_config_client_generate_key (client, group, key);
    gboolean value = gconf_client_get_bool (client->client, gconf_key, err);
    g_free (gconf_key);
    return value;
}

void awn_config_client_set_bool (AwnConfigClient *client, const gchar *group, const gchar *key, gboolean value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
	gconf_client_set_bool (client->client, full_key, value, err);
	g_free (full_key);
}

gfloat awn_config_client_get_float (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	gfloat value = gconf_client_get_float (client->client, gconf_key, err);
	g_free (gconf_key);
	return value;
}

void awn_config_client_set_float (AwnConfigClient *client, const gchar *group, const gchar *key, gfloat value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
	gconf_client_set_float (client->client, full_key, value, err);
	g_free (full_key);
}

gint awn_config_client_get_int (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	gint value = gconf_client_get_int (client->client, gconf_key, err);
	g_free (gconf_key);
	return value;
}

void awn_config_client_set_int (AwnConfigClient *client, const gchar *group, const gchar *key, gint value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
	gconf_client_set_int (client->client, full_key, value, err);
	g_free (full_key);
}

gchar *awn_config_client_get_string (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	gchar *value = gconf_client_get_string (client->client, gconf_key, err);
	g_free (gconf_key);
	return value;
}

void awn_config_client_set_string (AwnConfigClient *client, const gchar *group, const gchar *key, gchar *value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
	gconf_client_set_string (client->client, full_key, value, err);
	g_free (full_key);
}

GSList *awn_config_client_get_list (AwnConfigClient *client, const gchar *group, const gchar *key, AwnConfigListType list_type, GError **err)
{
	GConfValueType value_type = GCONF_VALUE_INVALID;
	switch (list_type) {
		case AWN_CONFIG_CLIENT_LIST_TYPE_BOOL:
			value_type = GCONF_VALUE_BOOL;
			break;
		case AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT:
			value_type = GCONF_VALUE_FLOAT;
			break;
		case AWN_CONFIG_CLIENT_LIST_TYPE_INT:
			value_type = GCONF_VALUE_INT;
			break;
		case AWN_CONFIG_CLIENT_LIST_TYPE_STRING:
			value_type = GCONF_VALUE_STRING;
			break;
	}
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	GSList *value = gconf_client_get_list (client->client, gconf_key, value_type, err);
	g_free (gconf_key);
	return value;
}

void awn_config_client_set_list (AwnConfigClient *client, const gchar *group, const gchar *key, AwnConfigListType list_type, GSList *value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
	GConfValueType value_type = GCONF_VALUE_INVALID;
	switch (list_type) {
		case AWN_CONFIG_CLIENT_LIST_TYPE_BOOL:
			value_type = GCONF_VALUE_BOOL;
			break;
		case AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT:
			value_type = GCONF_VALUE_FLOAT;
			break;
		case AWN_CONFIG_CLIENT_LIST_TYPE_INT:
			value_type = GCONF_VALUE_INT;
			break;
		case AWN_CONFIG_CLIENT_LIST_TYPE_STRING:
			value_type = GCONF_VALUE_STRING;
			break;
	}
	gconf_client_set_list (client->client, full_key, value_type, value, err);
	g_free (full_key);
}

void awn_config_client_free (AwnConfigClient *client)
{
	g_object_unref (client->client);
	if (strcmp (AWN_GCONF_PATH, client->path) != 0) {
		g_free (client->path);
	}
	g_free (client);
}

/*  vim: set noet ts=8 sw=8 sts=8 : */
