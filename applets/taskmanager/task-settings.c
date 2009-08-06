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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "task-settings.h"

/* prototypes */

static void
cfg_notify_int (const gchar *group, const gchar *key, const GValue *value,
                gpointer user_data)
{
  gint* int_value = (gint*)user_data;
  *int_value = g_value_get_int (value);
}

static void
cfg_load_int (DesktopAgnosticConfigClient *cfg,
              const gchar                 *group,
              const gchar                 *key,
              gint                        *data)
{
  /* FIXME handle errors */
  *data = desktop_agnostic_config_client_get_int (cfg, group, key, NULL);
  desktop_agnostic_config_client_notify_add (cfg, group, key,
                                             cfg_notify_int, data, NULL);
}

/**
 * task_settings_get_default:
 *
 * Returns: A singleton
 *
 * Retrieves a structure that holds various values from the panel useful
 * for the applet.
 */
TaskSettings *
task_settings_get_default (void)
{
  static TaskSettings *settings  = NULL;
  static DesktopAgnosticConfigClient *client = NULL;

  if (!settings)
  {
    settings = g_new (TaskSettings, 1);

    /* FIXME handle error */
    client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, NULL);

    /* Bar settings */

    cfg_load_int(client, "panel", "size", &(settings->panel_size));
    cfg_load_int(client, "panel", "orient", &(settings->orient));
    cfg_load_int(client, "panel", "offset", &(settings->offset));
  }

  return settings;
}
