/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#include <assert.h>
#include "awn-effects-settings.h"

#include "config.h"

#include "awn-config-client.h"

/* globals */
static AwnEffectsSettings *settings  = NULL;
static AwnConfigClient    *client   = NULL;

/* prototypes */
static void awn_load_bool  (AwnConfigClient *client, 
                            const gchar     *group,
                            const gchar     *key, 
                            gboolean        *data, 
                            gboolean         def);
static void awn_load_float (AwnConfigClient *client, 
                            const gchar     *group,
                            const gchar     *key, 
                            gfloat          *data, 
                            gfloat           def);
static void awn_load_int  (AwnConfigClient  *client, 
                           const gchar      *group,
                           const gchar      *key, 
                           gint             *data, 
                           gint              def);

static void awn_notify_bool  (AwnConfigClientNotifyEntry *entry, 
                              gboolean                   *data);
static void awn_notify_float (AwnConfigClientNotifyEntry *entry, 
                              gfloat                     *data);
static void awn_notify_int   (AwnConfigClientNotifyEntry *entry, 
                              gint                       *data);

AwnEffectsSettings * 
awn_effects_settings_get_default (void)
{
  GdkScreen *screen;
  gint width = 1024;
  gint height = 768;

  screen = gdk_screen_get_default();

  if (screen)
  {
    width = gdk_screen_get_width(screen);
    height = gdk_screen_get_height(screen);
  }

  if (settings)
    return settings;

  AwnEffectsSettings *s = NULL;

  s = g_new (AwnEffectsSettings, 1);

  settings = s;
  s->bar_angle = 0;

  client = awn_config_client_new();

  /* Bar settings */
  awn_config_client_ensure_group(client, "panel");

  awn_load_int(client, "panel", "size", &s->bar_height, 48);
  awn_load_int(client, "panel", "offset", &s->icon_offset, 0);
 
  /* App settings */
  awn_config_client_ensure_group(client, "effects");
  
  awn_load_int (client, "effects", "reflection_offset", 
                &s->reflection_offset, 0);
  awn_load_int (client, "effects", "icon_effect", &s->icon_effect, 0);
  awn_load_float (client, "effects","icon_alpha", &s->icon_alpha, 1.0);
  awn_load_int (client, "effects", "frame_rate", &s->frame_rate, 25);
  awn_load_bool (client, "effects", "enable_icon_depth", 
                 &s->icon_depth_on, TRUE);
  awn_load_float (client, "effects", "reflection_alpha_multiplier", 
                  &s->reflection_alpha_mult, 0.33);
  awn_load_bool (client, "effects", "show_shadows", &s->show_shadows, FALSE);

  return s;
}

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

static void
awn_notify_int (AwnConfigClientNotifyEntry *entry, gint* data)
{
  *data = entry->value.int_val;
}

static void
awn_load_bool (AwnConfigClient *client, 
               const gchar     *group, 
               const gchar     *key, 
               gboolean        *data, 
               gboolean         def)
{
  if (awn_config_client_entry_exists(client, group, key))
  {
    *data = awn_config_client_get_bool(client, group, key, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_bool(client, group, key, def, NULL);
    *data = def;
  }

  awn_config_client_notify_add(client, group, key, 
                               (AwnConfigClientNotifyFunc)awn_notify_bool, 
                               data);
}

static void
awn_load_float (AwnConfigClient *client, 
                const gchar     *group, 
                const gchar     *key, 
                gfloat          *data, 
                gfloat           def)
{
  if (awn_config_client_entry_exists(client, group, key))
  {
    *data = awn_config_client_get_float(client, group, key, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_float(client, group, key, def, NULL);
    *data = def;
  }

  awn_config_client_notify_add (client, group, key, 
                                (AwnConfigClientNotifyFunc)awn_notify_float, 
                                data);
}

static void
awn_load_int (AwnConfigClient *client, 
              const gchar     *group, 
              const gchar     *key, 
              gint            *data, 
              gint             def)
{
  if (awn_config_client_entry_exists(client, group, key))
  {
    *data = awn_config_client_get_int(client, group, key, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_int(client, group, key, def, NULL);
    *data = def;
  }

  awn_config_client_notify_add (client, group, key, 
                                (AwnConfigClientNotifyFunc)awn_notify_int, 
                                data);
}
