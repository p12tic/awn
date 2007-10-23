/*
 *  Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
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

#ifdef USE_GCONF
#include <string.h>
#include <gconf/gconf-client.h>
#define AWN_GCONF_KEY_PREFIX "/apps/avant-window-navigator"
#else
#define CALL_NOTIFY_FUNCS(type) \
	AwnConfigClientNotifyEntry *entry = g_new0 (AwnConfigClientNotifyEntry, 1); \
	entry->client = client; \
	entry->group = (gchar*)group; \
	entry->key = (gchar*)key; \
	AwnConfigClientNotifyValue *nvalue = g_malloc (sizeof (AwnConfigClientNotifyValue)); \
	nvalue->type##_val = value; \
	entry->value = *nvalue; \
	GSList *callbacks = g_datalist_get_data (&(client->notify_funcs), full_key); \
	GSList *notify_iter; \
	for (notify_iter = callbacks; notify_iter != NULL; notify_iter = g_slist_next (notify_iter)) { \
		AwnConfigClientNotifyData *notify = (AwnConfigClientNotifyData*)notify_iter->data; \
		(notify->callback) (entry, notify->data); \
	} \
	g_free (nvalue); \
	g_free (entry);
#endif

#include "libawn/awn-config-client.h"

struct _AwnConfigClient
{
#ifdef USE_GCONF
	GConfClient *client;
#else
	GKeyFile *client;
	GData *notify_funcs;
#endif
	gchar *path; /* GConf: key prefix; GKeyFile: .ini path */
};

/* global */
AwnConfigClient *awn_dock_config;

static AwnConfigClient *awn_config_client_new_with_path (gchar *path)
{
	AwnConfigClient *client = g_new (AwnConfigClient, 1);
	client->path = path;
#ifdef USE_GCONF
	client->client = gconf_client_get_default ();
#else
	client->client = g_key_file_new ();
	GError *err = NULL;
	if (!g_key_file_load_from_file (client->client, client->path,
	                                G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
	                                &err)) {
		g_warning("Config file not found.  It will be created for you.");
	}
	g_datalist_init (&(client->notify_funcs));
#endif
	return client;
}

static gpointer
_awn_config_client_copy (gpointer boxed)
{
	return boxed;
}

static void
_awn_config_client_free (gpointer boxed)
{
	awn_config_client_unref (AWN_CONFIG_CLIENT (boxed));
}

GType awn_config_client_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		type = g_boxed_type_register_static ("AwnConfigClient",
						     _awn_config_client_copy,
						     _awn_config_client_free);
	}

	return type;
}

/* singleton */
AwnConfigClient *awn_config_client_new ()
{
	if (!awn_dock_config) {
#ifdef USE_GCONF
		awn_dock_config = awn_config_client_new_with_path (AWN_GCONF_KEY_PREFIX);
#else
		awn_dock_config = awn_config_client_new_with_path (g_build_filename (g_get_user_config_dir (), "awn", "awn.ini", NULL));
	}
	return awn_dock_config;
#endif
}

AwnConfigClient *awn_config_client_new_for_applet (gchar *name)
{
#ifdef USE_GCONF
	return awn_config_client_new_with_path (g_strconcat (AWN_GCONF_KEY_PREFIX, "/applets/", name, NULL));
#else
	return awn_config_client_new_with_path (g_build_filename (g_get_user_config_dir (), "awn", "applets", g_strconcat(name, ".ini", NULL), NULL));
#endif
}

/* returns a newly allocated string */
static gchar *awn_config_client_generate_key (AwnConfigClient *client, const gchar *group, const gchar *key)
{
	if (key == NULL) {
		return g_strconcat(client->path, "/", group, NULL);
	} else {
		return g_strconcat(client->path, "/", group, "/", key, NULL);
	}
}
#ifdef USE_GCONF
static void awn_config_client_notify_proxy (GConfClient *client, guint cid, GConfEntry *entry, AwnConfigClientNotifyData* notify)
{
	AwnConfigClientNotifyEntry *awn_entry = g_new (AwnConfigClientNotifyEntry, 1);
	GConfValue *value = NULL;
	value = gconf_entry_get_value (entry);
	if (value) {
		gchar **exploded = g_strsplit (gconf_entry_get_key (entry), "/", 0);
		guint exploded_len = g_strv_length (exploded);
		g_return_if_fail (exploded_len < 2);
		awn_entry->client = client;
		awn_entry->group = g_strdup (exploded[exploded_len - 2]);
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
				awn_entry->value.list_val = gconf_value_get_list (value);
				break;
			case GCONF_VALUE_STRING:
				awn_entry->value.str_val = gconf_value_get_string (value);
				break;
		}
		(notify->callback) (awn_entry, notify->data);
		g_free (awn_entry->group);
		g_free (awn_entry->key);
	}
	g_free (awn_entry);
}
#else
static void awn_config_client_free_gslist (GSList *list)
{
	g_slist_foreach (list, (GFunc)g_free, NULL);
	g_slist_free (list);
}
static void awn_config_client_save (AwnConfigClient *client, GError **err)
{
	gchar *data;
	gsize len;
	data = g_key_file_to_data (client->client, &len, err);
	if (!err || *err != NULL) {
		g_file_set_contents (client->path, data, len, err);
		g_free (data);
	}
}
#endif

/* removes all of the options */
void awn_config_client_clear (AwnConfigClient *client, GError **err)
{
#ifdef USE_GCONF
	/* only do it for applets on gconf */
	if (strcmp (AWN_GCONF_KEY_PREFIX, client->path) != 0) {
		GConfClient *client = gconf_client_get_default ();
		gconf_client_remove_dir (client->client, client->path, err);
	}
#else
	gsize i, group_len;
	gchar **group_list = g_key_file_get_groups (client->client, &group_len);
	for (i = 0; i < group_len; i++) {
		gsize j, key_len;
		gchar **key_list = g_key_file_get_keys (client->client, group_list[i], &key_len, NULL);
		for (j = 0; j < key_len; j++) {
			g_key_file_remove_key (client->client, group_list[i], key_list[j], NULL);
		}
		g_strfreev (key_list);
		g_key_file_remove_group (client->client, group_list[i], NULL);
	}
	g_strfreev (group_list);
	awn_config_client_save (client, err);
#endif
}

void awn_config_client_ensure_group (AwnConfigClient *client, const gchar *group)
{
#ifdef USE_GCONF
	gchar *gconf_key = awn_config_client_generate_key (client, group, NULL);
	if (!gconf_client_dir_exists (client->client, gconf_key, NULL)) {
		gconf_client_add_dir (client->client, gconf_key, GCONF_CLIENT_PRELOAD_NONE, NULL);
	}
	g_free (gconf_key);
#else
	if (!g_key_file_has_group (client->client, group)) {
		g_warning ("The configuration file does not currently contain the group '%s'.  It will be created when a configuration option is set in that group.", group);
	}
#endif
}

void awn_config_client_notify_add (AwnConfigClient *client, const gchar *group, 
				   const gchar *key, AwnConfigClientNotifyFunc callback,
				   gpointer data)
{
	AwnConfigClientNotifyData *notify = g_new0 (AwnConfigClientNotifyData, 1);
	notify->callback = callback;
	notify->data = data;
	gchar *full_key = awn_config_client_generate_key (client, group, key);
#ifdef USE_GCONF
	gconf_client_notify_add (client->client, gconf_key, (GConfClientNotifyFunc)awn_config_client_notify_proxy, notify, NULL, NULL);
#else
	GQuark quark = g_quark_from_string (full_key);
	GSList *funcs = g_datalist_id_get_data (&(client->notify_funcs), quark);
	funcs = g_slist_append (funcs, notify);
	g_datalist_id_set_data_full (&(client->notify_funcs), quark, funcs, (GDestroyNotify)awn_config_client_free_gslist);
#endif
	g_free (full_key);
}
gboolean awn_config_client_entry_exists (AwnConfigClient *client, const gchar *group, const gchar *key)
{
#ifdef USE_GCONF
	GConfValue *value = NULL;
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	value = gconf_client_get(client->client, gconf_key, NULL);
	g_free (gconf_key);
	return value != NULL;
#else
	return g_key_file_has_key (client->client, group, key, NULL);
#endif
}

gboolean awn_config_client_get_bool (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
#ifdef USE_GCONF
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	gboolean value = gconf_client_get_bool (client->client, gconf_key, err);
	g_free (gconf_key);
	return value;
#else
	return g_key_file_get_boolean (client->client, group, key, err);
#endif
}

void awn_config_client_set_bool (AwnConfigClient *client, const gchar *group, const gchar *key, gboolean value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
#ifdef USE_GCONF
	gconf_client_set_bool (client->client, full_key, value, err);
#else
	g_key_file_set_boolean (client->client, group, key, value);
	CALL_NOTIFY_FUNCS (bool);
	awn_config_client_save (client, err);
#endif
	g_free (full_key);
}

gfloat awn_config_client_get_float (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
#ifdef USE_GCONF
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	gfloat value = gconf_client_get_float (client->client, gconf_key, err);
	g_free (gconf_key);
	return value;
#else
	return (gfloat)g_key_file_get_double (client->client, group, key, err);
#endif
}

/* note: if you want double precision, use a string. */
void awn_config_client_set_float (AwnConfigClient *client, const gchar *group, const gchar *key, gfloat value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
#ifdef USE_GCONF
	gconf_client_set_float (client->client, full_key, value, err);
#else
	g_key_file_set_double (client->client, group, key, (gdouble)value);
	CALL_NOTIFY_FUNCS (float);
	awn_config_client_save (client, err);
#endif
	g_free (full_key);
}

gint awn_config_client_get_int (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
#ifdef USE_GCONF
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	gint value = gconf_client_get_int (client->client, gconf_key, err);
	g_free (gconf_key);
	return value;
#else
	return g_key_file_get_integer (client->client, group, key, err);
#endif
}

void awn_config_client_set_int (AwnConfigClient *client, const gchar *group, const gchar *key, gint value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
#ifdef USE_GCONF
	gconf_client_set_int (client->client, full_key, value, err);
#else
	g_key_file_set_integer (client->client, group, key, value);
	CALL_NOTIFY_FUNCS (int);
	awn_config_client_save (client, err);
#endif
	g_free (full_key);
}

/* returns a newly allocated string */
gchar *awn_config_client_get_string (AwnConfigClient *client, const gchar *group, const gchar *key, GError **err)
{
#ifdef USE_GCONF
	gchar *gconf_key = awn_config_client_generate_key (client, group, key);
	gchar *value = gconf_client_get_string (client->client, gconf_key, err);
	g_free (gconf_key);
	return value;
#else
	return g_key_file_get_string (client->client, group, key, err);
#endif
}

void awn_config_client_set_string (AwnConfigClient *client, const gchar *group, const gchar *key, gchar *value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
#ifdef USE_GCONF
	gconf_client_set_string (client->client, full_key, value, err);
#else
	g_key_file_set_string (client->client, group, key, value);
	CALL_NOTIFY_FUNCS (str);
	awn_config_client_save (client, err);
#endif
	g_free (full_key);
}

GSList *awn_config_client_get_list (AwnConfigClient *client, const gchar *group, const gchar *key, AwnConfigListType list_type, GError **err)
{
#ifdef USE_GCONF
	GConfValueType value_type;
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
#else
	GSList *slist = NULL;
	gsize list_len;
	switch (list_type) {
		case AWN_CONFIG_CLIENT_LIST_TYPE_BOOL: {
			gboolean *list = g_key_file_get_boolean_list (client->client, group, key, &list_len, err);
			if (list) {
				gsize i;
				for (i = 0; i < list_len; i++) {
					slist = g_slist_append (slist, (gpointer)(&list[i]));
				}
			}
			break;
		} case AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT: {
			gdouble *list = g_key_file_get_double_list (client->client, group, key, &list_len, err);
			if (list) {
				gsize i;
				for (i = 0; i < list_len; i++) {
					slist = g_slist_append (slist, (gpointer)(&list[i]));
				}
			}
			break;
		} case AWN_CONFIG_CLIENT_LIST_TYPE_INT: {
			gint *list = g_key_file_get_integer_list (client->client, group, key, &list_len, err);
			if (list) {
				gsize i;
				for (i = 0; i < list_len; i++) {
					slist = g_slist_append (slist, (gpointer)(&list[i]));
				}
			}
			break;
		} case AWN_CONFIG_CLIENT_LIST_TYPE_STRING: {
			gchar **list = g_key_file_get_string_list (client->client, group, key, &list_len, err);
			if (list) {
				gsize i;
				for (i = 0; i < list_len; i++) {
					slist = g_slist_append (slist, (gpointer)(list[i]));
				}
			}
			break;
		}
	}
	return slist;
#endif
}

void awn_config_client_set_list (AwnConfigClient *client, const gchar *group, const gchar *key, AwnConfigListType list_type, GSList *value, GError **err)
{
	gchar *full_key = awn_config_client_generate_key (client, group, key);
#ifdef USE_GCONF
	GConfValueType value_type;
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
#else
	gsize list_len = g_slist_length (value);
	gsize i;
	GSList *iter = NULL;
	switch (list_type) {
		case AWN_CONFIG_CLIENT_LIST_TYPE_BOOL: {
			gboolean *list = g_malloc (sizeof(gboolean) * list_len);
			for (iter = value, i = 0; iter != NULL; iter = g_slist_next(iter), i++) {
				list[i] = *(gboolean*)(iter->data);
			}
			g_key_file_set_boolean_list (client->client, group, key, list, list_len);
			g_free (list);
			break;
		} case AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT: {
			gdouble *list = g_malloc (sizeof(gdouble) * list_len);
			for (iter = value, i = 0; iter != NULL; iter = g_slist_next(iter), i++) {
				list[i] = *(gdouble*)(iter->data);
			}
			g_key_file_set_double_list (client->client, group, key, list, list_len);
			g_free (list);
			break;
		} case AWN_CONFIG_CLIENT_LIST_TYPE_INT: {
			gint *list = g_malloc (sizeof(gint) * list_len);
			for (iter = value, i = 0; iter != NULL; iter = g_slist_next(iter), i++) {
				list[i] = *(gint*)(iter->data);
			}
			g_key_file_set_integer_list (client->client, group, key, list, list_len);
			g_free (list);
			break;
		} case AWN_CONFIG_CLIENT_LIST_TYPE_STRING: {
			gchar **list = g_malloc (sizeof(gchar*) * list_len);
			for (iter = value, i = 0; iter != NULL; iter = g_slist_next(iter), i++) {
				list[i] = (gchar*)(iter->data);
			}
			g_key_file_set_string_list (client->client, group, key, (const gchar * const *)list, list_len);
			g_free (list);
			break;
		}
	}
	CALL_NOTIFY_FUNCS (list);
	awn_config_client_save (client, err);
#endif
	g_free (full_key);
}

void awn_config_client_unref (AwnConfigClient *client)
{
#ifdef USE_GCONF
	g_object_unref (client->client);
	if (strcmp (AWN_GCONF_KEY_PREFIX, client->path) != 0) {
		g_free (client->path);
	}
#else
	g_datalist_clear (&(client->notify_funcs));
	g_key_file_free (client->client);
	g_free (client->path);
#endif
	g_free (client);
}

/*  vim: set noet ts=8 sw=8 sts=8 : */
