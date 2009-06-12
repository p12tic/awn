/*
 *  Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
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
 *  Author : Mark Lee <avant-wn@lazymalevolence.com>
 */
#include <string.h>
#include <libawn/awn-config-client.h>

#define GROUP "test"

#define ERROR_CHECK(err) \
	if (err) { \
		g_warning ("Error: %s", err->message); \
		return 1; \
	}

static int test_option_bool (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_bool...");
	gboolean bool_val = TRUE;
	awn_config_client_set_bool (client, GROUP, "bool", bool_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_bool...");
	bool_val = FALSE;
	bool_val = awn_config_client_get_bool (client, GROUP, "bool", &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	return bool_val ? 0 : 1;
}

static int test_option_float (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_float...");
	gfloat float_val = 0.5;
	awn_config_client_set_float (client, GROUP, "float", float_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_float...");
	float_val = 0;
	float_val = awn_config_client_get_float (client, GROUP, "float", &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	return float_val == 0.5 ? 0 : 1;
}

static int test_option_int (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_int...");
	gint int_val = 4;
	awn_config_client_set_int (client, GROUP, "int", int_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_int...");
	int_val = 0;
	int_val = awn_config_client_get_int (client, GROUP, "int", &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	return int_val == 4 ? 0 : 1;
}

static int test_option_string (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_string...");
	const gchar *str_val = "My dog has fleas.";
	awn_config_client_set_string (client, GROUP, "str", (gchar*)str_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_string...");
	str_val = NULL;
	str_val = awn_config_client_get_string (client, GROUP, "str", &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	return strcmp(str_val, "My dog has fleas.") == 0 ? 0 : 1;
}

static int test_option_list_bool (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_list (bool)...");
	GSList *list_val = NULL;
	gboolean b_t = TRUE;
	gboolean b_f = FALSE;
	list_val = g_slist_append (list_val, &b_t);
	list_val = g_slist_append (list_val, &b_f);
	list_val = g_slist_append (list_val, &b_t);
	awn_config_client_set_list (client, GROUP, "list_bool", AWN_CONFIG_CLIENT_LIST_TYPE_BOOL, list_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_list (bool)...");
	GSList *get_list_val = NULL;
	get_list_val = awn_config_client_get_list (client, GROUP, "list_bool", AWN_CONFIG_CLIENT_LIST_TYPE_BOOL, &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	int retval = 0;
	if (g_slist_length (list_val) != g_slist_length (get_list_val)) {
		retval = 1;
	} else {
		gsize i, len = g_slist_length (list_val);
		for (i = 0; i < len; i++) {
			gboolean set_val = *(gboolean*)g_slist_nth_data (list_val, i);
			gboolean get_val = *(gboolean*)g_slist_nth_data (get_list_val, i);
			if (set_val != get_val) {
				g_warning ("The lists differ at list index %zu; get='%d', set='%d'\n", i, get_val, set_val);
				retval = 1;
			}
		}
	}
	return retval;
}

static int test_option_list_float (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_list (float)...");
	GSList *list_val = NULL;
	gfloat f2 = 0.2;
	gfloat f4 = 0.4;
	gfloat f6 = 0.6;
	list_val = g_slist_append (list_val, &f2);
	list_val = g_slist_append (list_val, &f4);
	list_val = g_slist_append (list_val, &f6);
	awn_config_client_set_list (client, GROUP, "list_float", AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT, list_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_list (float)...");
	GSList *get_list_val = NULL;
	get_list_val = awn_config_client_get_list (client, GROUP, "list_float", AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT, &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	int retval = 0;
	if (g_slist_length (list_val) != g_slist_length (get_list_val)) {
		retval = 1;
	} else {
		gsize i, len = g_slist_length (list_val);
		for (i = 0; i < len; i++) {
			gfloat set_val = *(gfloat*)g_slist_nth_data (list_val, i);
			gfloat get_val = *(gfloat*)g_slist_nth_data (get_list_val, i);
			if (set_val != get_val) {
				g_warning ("The lists differ at list index %zu; get='%f', set='%f'\n", i, get_val, set_val);
				retval = 1;
			}
		}
	}
	return retval;
}

static int test_option_list_int (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_list (int)...");
	GSList *list_val = NULL;
	gint i1 = 1;
	gint i3 = 3;
	gint i5 = 5;
	list_val = g_slist_append (list_val, &i1);
	list_val = g_slist_append (list_val, &i3);
	list_val = g_slist_append (list_val, &i5);
	awn_config_client_set_list (client, GROUP, "list_int", AWN_CONFIG_CLIENT_LIST_TYPE_INT, list_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_list (int)...");
	GSList *get_list_val = NULL;
	get_list_val = awn_config_client_get_list (client, GROUP, "list_int", AWN_CONFIG_CLIENT_LIST_TYPE_INT, &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	int retval = 0;
	if (g_slist_length (list_val) != g_slist_length (get_list_val)) {
		retval = 1;
	} else {
		gsize i, len = g_slist_length (list_val);
		for (i = 0; i < len; i++) {
			gint set_val = *(gint*)g_slist_nth_data (list_val, i);
			gint get_val = *(gint*)g_slist_nth_data (get_list_val, i);
			if (set_val != get_val) {
				g_warning ("The lists differ at list index %zu; get='%d', set='%d'\n", i, get_val, set_val);
				retval = 1;
			}
		}
	}
	return retval;
}

static int test_option_list_string (AwnConfigClient *client)
{
	GError *err = NULL;
	g_print ("* set_list (string)...");
	GSList *list_val = NULL;
	list_val = g_slist_append (list_val, (gpointer)"One");
	list_val = g_slist_append (list_val, (gpointer)"Two");
	list_val = g_slist_append (list_val, (gpointer)"Three");
	awn_config_client_set_list (client, GROUP, "list_str", AWN_CONFIG_CLIENT_LIST_TYPE_STRING, list_val, &err);
	ERROR_CHECK (err);
	g_print ("done.\n* get_list (string)...");
	GSList *get_list_val = NULL;
	get_list_val = awn_config_client_get_list (client, GROUP, "list_str", AWN_CONFIG_CLIENT_LIST_TYPE_STRING, &err);
	ERROR_CHECK (err);
	g_print ("done.\n");
	int retval = 0;
	if (g_slist_length (list_val) != g_slist_length (get_list_val)) {
		retval = 1;
	} else {
		gsize i, len = g_slist_length (list_val);
		for (i = 0; i < len; i++) {
			gchar *set_val = (gchar*)g_slist_nth_data (list_val, i);
			gchar *get_val = (gchar*)g_slist_nth_data (get_list_val, i);
			if (strcmp (set_val, get_val) != 0) {
				g_warning ("The lists differ at list index %zu; get='%s', set='%s'\n", i, get_val, set_val);
				retval = 1;
			}
		}
	}
	return retval;
}

static void test_notify (AwnConfigClientNotifyEntry *entry, gchar* data)
{
	g_print ("The configuration option [%s]%s has been changed to the value '%s'. %s\n",
		 entry->group, entry->key, entry->value.str_val, data);
}

static void test_option_notify (AwnConfigClient *client)
{
	awn_config_client_notify_add (client, GROUP, "str", (AwnConfigClientNotifyFunc)test_notify,
				      (gpointer)"This string is the user_data.");
}

static int test_option_types (AwnConfigClient *client)
{
	int retval = 0;
	retval |= test_option_bool (client);
	retval |= test_option_float (client);
	retval |= test_option_int (client);
	retval |= test_option_string (client);
	retval |= test_option_list_bool (client);
	retval |= test_option_list_float (client);
	retval |= test_option_list_int (client);
	retval |= test_option_list_string (client);
	return retval;
}

static int test_awn_config()
{
	int retval = 0;
	g_print ("Main config client\n");
	AwnConfigClient *client = awn_config_client_new ();
	if (client == NULL) {
		return 1;
	}
	test_option_notify (client);
	retval |= test_option_types (client);
	awn_config_client_free (client);
	return retval;
}

static int test_applet_config()
{
	int retval = 0;
	g_print ("Applet config client\n");
	AwnConfigClient *client = awn_config_client_new_for_applet ("test", NULL);
	if (client == NULL) {
		return 1;
	}
	test_option_notify (client);
	retval |= test_option_types (client);
	awn_config_client_free (client);
	return retval;
}

int main(int argc, char** argv)
{
	int retval = 0;
	g_type_init ();
	retval |= test_awn_config ();
	retval |= test_applet_config ();
	return retval;
}

/* vim: set noet ts=8 sts=8 sw=8 : */
