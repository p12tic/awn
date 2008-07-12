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

#ifndef _LIBAWN_AWN_CONFIG_CLIENT_H
#define _LIBAWN_AWN_CONFIG_CLIENT_H

#include <glib.h>
#include <glib-object.h>

/**
 * AwnConfigClient:
 *
 * An opaque structure that facilitates having multiple configuration backends
 * available to Awn.
 */
typedef struct _AwnConfigClient AwnConfigClient;

#define AWN_TYPE_CONFIG_CLIENT (awn_config_client_get_type ())
/**
 * AWN_CONFIG_CLIENT:
 * @obj: The variable/value to cast
 *
 * Casts a variable/value to be an #AwnConfigClient.
 */
#define AWN_CONFIG_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_CONFIG_CLIENT, AwnConfigClient))

/**
 * AwnConfigClientValue:
 * @bool_val: The boolean value of the entry, if the entry is defined as taking
 * a boolean value.
 * @float_val: The float value of the entry, if the entry is defined as taking
 * a float value.
 * @int_val: The integer value of the entry, if the entry is defined as taking
 * an integer value.
 * @str_val: The string value of the entry, if the entry is defined as taking
 * a string value.
 * @list_val: The list value of the entry, if the entry is defined as taking
 * a list value.
 *
 * A union used to store a configuration value when it has changed and has
 * notify callbacks associated with it.
 */
typedef union {
	gboolean bool_val;
	gfloat float_val;
	gint int_val;
	gchar *str_val;
	GSList *list_val;
} AwnConfigClientValue;

/**
 * AwnConfigClientNotifyEntry:
 * @client: The client associated with the entry.
 * @group: The group name of the entry.
 * @key: The key name of the entry.
 * @value: The new value of the entry.
 *
 * The structure used to transport data to the notification functions of
 * a configuration entry.
 */
typedef struct {
	AwnConfigClient *client;
	gchar *group;
	gchar *key;
	AwnConfigClientValue value;
} AwnConfigClientNotifyEntry;

/**
 * AwnConfigClientNotifyFunc:
 * @entry: The metadata about the new entry value.
 * @data: Extra data passed to the callback, as defined in the call
 * to awn_config_client_notify_add().
 *
 * The callback template for configuration change notification functions.
 */
typedef void (*AwnConfigClientNotifyFunc) (AwnConfigClientNotifyEntry *entry, gpointer user_data);

/**
 * AWN_CONFIG_CLIENT_DEFAULT_GROUP:
 *
 * In the #GKeyFile backend, the group name with which "top-level" configuration
 * entries are associated.
 */
#define AWN_CONFIG_CLIENT_DEFAULT_GROUP "DEFAULT"

/**
 * AwnConfigValueType:
 * @AWN_CONFIG_VALUE_TYPE_NULL: Indicates that the configuration value type
 * is unknown.
 * @AWN_CONFIG_VALUE_TYPE_BOOL: Indicates that the configuration value type
 * is boolean.
 * @AWN_CONFIG_VALUE_TYPE_FLOAT: Indicates that the configuration value type
 * is float.
 * @AWN_CONFIG_VALUE_TYPE_INT: Indicates that the configuration value type
 * is integer.
 * @AWN_CONFIG_VALUE_TYPE_STRING: Indicates that the configuration value type
 * is string.
 * @AWN_CONFIG_VALUE_TYPE_LIST_BOOL: Indicates that the configuration value
 * type is list whose items are booleans.
 * @AWN_CONFIG_VALUE_TYPE_LIST_FLOAT: Indicates that the configuration value
 * type is list whose items are floats.
 * @AWN_CONFIG_VALUE_TYPE_LIST_INT: Indicates that the configuration value
 * type is list whose items are integers.
 * @AWN_CONFIG_VALUE_TYPE_LIST_STRING: Indicates that the configuration value
 * type is list whose items are strings.
 *
 * Indicates the value type of a particular configuration entry.
 */
typedef enum {
	AWN_CONFIG_VALUE_TYPE_NULL = -1,
	AWN_CONFIG_VALUE_TYPE_BOOL,
	AWN_CONFIG_VALUE_TYPE_FLOAT,
	AWN_CONFIG_VALUE_TYPE_INT,
	AWN_CONFIG_VALUE_TYPE_STRING,
	AWN_CONFIG_VALUE_TYPE_LIST_BOOL,
	AWN_CONFIG_VALUE_TYPE_LIST_FLOAT,
	AWN_CONFIG_VALUE_TYPE_LIST_INT,
	AWN_CONFIG_VALUE_TYPE_LIST_STRING
} AwnConfigValueType;

/**
 * AwnConfigListType:
 * @AWN_CONFIG_CLIENT_LIST_TYPE_BOOL: Indicates that the list value type
 * is boolean.
 * @AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT: Indicates that the list value type
 * is float.
 * @AWN_CONFIG_CLIENT_LIST_TYPE_INT: Indicates that the list value type
 * is integer.
 * @AWN_CONFIG_CLIENT_LIST_TYPE_STRING: Indicates that the list value type
 * is string.
 *
 * Indicates the value type of every item in a configuration entry of
 * type "list".
 */
typedef enum {
	AWN_CONFIG_CLIENT_LIST_TYPE_BOOL,
	AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT,
	AWN_CONFIG_CLIENT_LIST_TYPE_INT,
	AWN_CONFIG_CLIENT_LIST_TYPE_STRING
} AwnConfigListType;

/**
 * AwnConfigBackend:
 * @AWN_CONFIG_CLIENT_GCONF: Indicates the configuration backend in use is 
 * gconf.
 * @AWN_CONFIG_CLIENT_GKEYFILE: Indicates the configuration backend in use is 
 * gkeyfile.
 *
 * Indicates the configuration backend in use.
 */
typedef enum {
  AWN_CONFIG_CLIENT_GCONF,
  AWN_CONFIG_CLIENT_GKEYFILE
} AwnConfigBackend;

GType              awn_config_client_get_type                  (void);

AwnConfigClient   *awn_config_client_new                       ();
AwnConfigClient   *awn_config_client_new_for_applet            (gchar *name, gchar *uid);
AwnConfigBackend  awn_config_client_query_backend              (void);

void               awn_config_client_clear                     (AwnConfigClient *client, GError **err);

void               awn_config_client_ensure_group              (AwnConfigClient *client, const gchar *group);

void               awn_config_client_notify_add                (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, AwnConfigClientNotifyFunc callback,
                                                                gpointer user_data);
gboolean           awn_config_client_entry_exists              (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key);
void               awn_config_client_load_defaults_from_schema (AwnConfigClient *client, GError **err);

int                awn_config_client_key_lock_open             (const gchar *group, const gchar *key);
int                awn_config_client_key_lock                  (int fd, int operation);
int                awn_config_client_key_lock_close            (int fd);

AwnConfigValueType awn_config_client_get_value_type            (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, GError **err);

gboolean           awn_config_client_get_bool                  (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, GError **err);
void               awn_config_client_set_bool                  (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, gboolean value, GError **err);
gfloat             awn_config_client_get_float                 (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, GError **err);
void               awn_config_client_set_float                 (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, gfloat value, GError **err);
gint               awn_config_client_get_int                   (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, GError **err);
void               awn_config_client_set_int                   (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, gint value, GError **err);
gchar             *awn_config_client_get_string                (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, GError **err);
void               awn_config_client_set_string                (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, gchar *value, GError **err);
GSList            *awn_config_client_get_list                  (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, AwnConfigListType list_type,
                                                                GError **err);
void               awn_config_client_set_list                  (AwnConfigClient *client, const gchar *group,
                                                                const gchar *key, AwnConfigListType list_type,
                                                                GSList *value, GError **err);
void               awn_config_client_free                      (AwnConfigClient *client);

#endif /* _LIBAWN_AWN_CONFIG_CLIENT_H */
/* vim: set noet ts=8 sw=8 sts=8 : */
