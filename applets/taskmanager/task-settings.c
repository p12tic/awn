/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "task-settings.h"

/* prototypes */

/*
 TODO hook up to the signals instead of monitoring the keys.
 This works for now... but it'll break once we beging automatic resizing.
 */

static void
cfg_notify_int (const gchar *group, const gchar *key, GValue *value,
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

static void
_size_changed (AwnApplet * applet, gint size, TaskSettings * settings)
{
  settings->panel_size = size;
}

static void
_position_changed (AwnApplet * applet, gint position, TaskSettings * settings)
{
  settings->position = position;
}

static void
_offset_changed (AwnApplet * applet, gint offset, TaskSettings * settings)
{
  settings->offset = offset;
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
task_settings_get_default (AwnApplet * applet)
{
  static TaskSettings *settings  = NULL;
  static DesktopAgnosticConfigClient *client = NULL;

  if (!settings)
  {
    /*
     The first time this function is called applet must be provided...
     after which it can be NULL*/
    g_assert (applet);
    settings = g_new (TaskSettings, 1);

    /* FIXME handle error */
    client = awn_config_get_default (AWN_PANEL_ID_DEFAULT, NULL);

    /* Bar settings
     * FIXME: this should be using AwnApplet properties instead
     */
    cfg_load_int(client, "panel", "size", &(settings->panel_size));
    cfg_load_int(client, "panel", "orient", &(settings->position));
    cfg_load_int(client, "panel", "offset", &(settings->offset));

    g_signal_connect (applet,"size-changed",G_CALLBACK(_size_changed),settings);
    g_signal_connect (applet,"offset-changed",G_CALLBACK(_offset_changed),settings);
    g_signal_connect (applet,"position-changed",G_CALLBACK(_position_changed),settings);
  }

  return settings;
}
