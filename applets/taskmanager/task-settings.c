/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#include <assert.h>
#include "task-settings.h"

#include "config.h"

/* globals */
static TaskSettings *settings  = NULL;
static AwnConfigClient    *client   = NULL;

/* prototypes */

/*
static void awn_load_bool  (AwnConfigClient *lclient, 
                            const gchar     *group,
                            const gchar     *key, 
                            gboolean        *data, 
                            gboolean         def);
static void awn_load_float (AwnConfigClient *lclient, 
                            const gchar     *group,
                            const gchar     *key, 
                            gfloat          *data, 
                            gfloat           def);
                            */
static void awn_load_int  (AwnConfigClient  *lclient, 
                           const gchar      *group,
                           const gchar      *key, 
                           gint             *data, 
                           gint              def);

/*
static void awn_notify_bool  (AwnConfigClientNotifyEntry *entry, 
                              gboolean                   *data);
static void awn_notify_float (AwnConfigClientNotifyEntry *entry, 
                              gfloat                     *data);
                              */
static void awn_notify_int   (AwnConfigClientNotifyEntry *entry, 
                              gint                       *data);

TaskSettings *
task_settings_get_default (void)
{
  TaskSettings *s;

  if (settings)
    return settings;
  
  s = g_new (TaskSettings, 1);

  settings = s;

  client = awn_config_client_new();

  /* Bar settings */
  awn_config_client_ensure_group(client, "panel");

  awn_load_int(client, "panel", "size", &s->panel_size, 48);
  awn_load_int(client, "panel", "orient", &s->orient, 0);
  awn_load_int(client, "panel", "offset", &s->offset, 0);
 
  return s;
}

/*
static void
awn_notify_bool (AwnConfigClientNotifyEntry *entry, gboolean* data)
{
  *data = entry->value.bool_val;
}

static void
awn_notify_float (AwnConfigClientNotifyEntry *entry, gfloat* data)
{
  *data = entry->value.float_val;
}
*/
static void
awn_notify_int (AwnConfigClientNotifyEntry *entry, gint* data)
{
  *data = entry->value.int_val;
}

/*
static void
awn_load_bool (AwnConfigClient *lclient, 
               const gchar     *group, 
               const gchar     *key, 
               gboolean        *data, 
               gboolean         def)
{
  if (awn_config_client_entry_exists(lclient, group, key))
  {
    *data = awn_config_client_get_bool(lclient, group, key, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_bool(lclient, group, key, def, NULL);
    *data = def;
  }

  awn_config_client_notify_add(lclient, group, key, 
                               (AwnConfigClientNotifyFunc)awn_notify_bool, 
                               data);
}

static void
awn_load_float (AwnConfigClient *lclient, 
                const gchar     *group, 
                const gchar     *key, 
                gfloat          *data, 
                gfloat           def)
{
  if (awn_config_client_entry_exists(lclient, group, key))
  {
    *data = awn_config_client_get_float(lclient, group, key, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_float(lclient, group, key, def, NULL);
    *data = def;
  }

  awn_config_client_notify_add (lclient, group, key, 
                                (AwnConfigClientNotifyFunc)awn_notify_float, 
                                data);
}
*/
static void
awn_load_int (AwnConfigClient *lclient, 
              const gchar     *group, 
              const gchar     *key, 
              gint            *data, 
              gint             def)
{
  if (awn_config_client_entry_exists(lclient, group, key))
  {
    *data = awn_config_client_get_int(lclient, group, key, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_int(lclient, group, key, def, NULL);
    *data = def;
  }

  awn_config_client_notify_add (lclient, group, key, 
                                (AwnConfigClientNotifyFunc)awn_notify_int, 
                                data);
}
