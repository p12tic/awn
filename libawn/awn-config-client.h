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

#ifndef _LIBAWN_AWN_CONFIG_CLIENT_H
#define _LIBAWN_AWN_CONFIG_CLIENT_H

#include <glib.h>
#include <glib-object.h>

typedef struct _AwnConfigClient AwnConfigClient;

#define AWN_TYPE_CONFIG_CLIENT (awn_config_client_get_type ())
#define AWN_CONFIG_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_CONFIG_CLIENT, AwnConfigClient))

typedef union {
	gboolean bool_val;
	gfloat float_val;
	gint int_val;
	gchar *str_val;
	GSList *list_val;
} AwnConfigClientValue;

typedef struct {
	AwnConfigClient *client;
	gchar *group;
	gchar *key;
	AwnConfigClientValue value;
} AwnConfigClientNotifyEntry;

typedef void (*AwnConfigClientNotifyFunc) (AwnConfigClientNotifyEntry *entry, gpointer data);

typedef struct {
	AwnConfigClientNotifyFunc callback;
	gpointer data;
} AwnConfigClientNotifyData;

#define AWN_CONFIG_CLIENT_DEFAULT_GROUP "DEFAULT"

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

typedef enum {
	AWN_CONFIG_CLIENT_LIST_TYPE_BOOL,
	AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT,
	AWN_CONFIG_CLIENT_LIST_TYPE_INT,
	AWN_CONFIG_CLIENT_LIST_TYPE_STRING
} AwnConfigListType;

GType            awn_config_client_get_type (void);

AwnConfigClient *awn_config_client_new ();
AwnConfigClient *awn_config_client_new_for_applet (gchar *name);

void             awn_config_client_clear (AwnConfigClient *client, GError **err);

void             awn_config_client_ensure_group (AwnConfigClient *client, const gchar *group);

void             awn_config_client_notify_add (AwnConfigClient *client, const gchar *group,
                                               const gchar *key, AwnConfigClientNotifyFunc callback,
                                               gpointer data);
gboolean         awn_config_client_entry_exists (AwnConfigClient *client, const gchar *group,
                                                 const gchar *key);

gboolean         awn_config_client_get_bool (AwnConfigClient *client, const gchar *group,
                                             const gchar *key, GError **err);
void             awn_config_client_set_bool (AwnConfigClient *client, const gchar *group,
                                             const gchar *key, gboolean value, GError **err);
gfloat           awn_config_client_get_float (AwnConfigClient *client, const gchar *group,
                                              const gchar *key, GError **err);
void             awn_config_client_set_float (AwnConfigClient *client, const gchar *group,
                                              const gchar *key, gfloat value, GError **err);
gint             awn_config_client_get_int (AwnConfigClient *client, const gchar *group,
                                            const gchar *key, GError **err);
void             awn_config_client_set_int (AwnConfigClient *client, const gchar *group,
                                            const gchar *key, gint value, GError **err);
gchar           *awn_config_client_get_string (AwnConfigClient *client, const gchar *group,
                                               const gchar *key, GError **err);
void             awn_config_client_set_string (AwnConfigClient *client, const gchar *group,
                                               const gchar *key, gchar *value, GError **err);
GSList          *awn_config_client_get_list (AwnConfigClient *client, const gchar *group,
                                             const gchar *key, AwnConfigListType list_type,
                                             GError **err);
void             awn_config_client_set_list (AwnConfigClient *client, const gchar *group,
                                             const gchar *key, AwnConfigListType list_type,
                                             GSList *value, GError **err);
void             awn_config_client_unref (AwnConfigClient *client);

#endif /* _LIBAWN_AWN_CONFIG_CLIENT_H */
/* vim: set noet ts=8 sw=8 sts=8 : */
