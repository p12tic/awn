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
#include "awn-settings.h"

#include "config.h"

#include "awn-config-client.h"

#define FORCE_MONITOR "force_monitor"  /*bool*/
#define MONITOR_WIDTH "monitor_width"  /*int*/
#define MONITOR_HEIGHT "monitor_height" /*int*/
#define PANEL_MODE "panel_mode"  /*bool*/
#define AUTO_HIDE "auto_hide"  /*bool*/
#define AUTO_HIDE_DELAY "auto_hide_delay" /*int*/
#define KEEP_BELOW "keep_below"  /*bool*/
#define SHOW_DIALOG "show_dialog_if_non_composited" /*bool*/

#define BAR  "bar"
#define ROUNDED_CORNERS "rounded_corners" /*bool*/
#define CORNER_RADIUS  "corner_radius"  /*float*/
#define RENDER_PATTERN "render_pattern" /*bool*/
#define EXPAND_BAR "expand_bar"  /*bool*/
#define PATTERN_URI "pattern_uri"  /*string*/
#define PATTERN_ALPHA  "pattern_alpha"  /*float*/
#define GLASS_STEP_1 "glass_step_1"  /*string*/
#define GLASS_STEP_2 "glass_step_2"  /*string*/
#define GLASS_HISTEP_1 "glass_histep_1" /*string*/
#define GLASS_HISTEP_2 "glass_histep_2" /*string*/
#define BORDER_COLOR "border_color"  /*string*/
#define HILIGHT_COLOR "hilight_color"  /*string*/
#define SHOW_SEPARATOR "show_separator" /*bool*/
#define SEP_COLOR "sep_color"  /*string*/
#define BAR_HEIGHT "bar_height"  /*int*/
#define BAR_ANGLE "bar_angle"  /*int, between 0 and 90*/
#define ICON_OFFSET "icon_offset"  /*float*/
#define BAR_POS  "bar_pos"   /*float, between 0 and 1 */
#define NO_BAR_RESIZE_ANI "no_bar_resize_animation" /*bool*/
#define SHOW_SHADOWS "show_shadows"  /*bool*/
#define REFLECTION_OFFSET "reflection_offset"  /*int*/

#define CURVES_SYMMETRY  "curves_symmetry" /*float, between 0 and 1*/
#define CURVINESS   "curviness"  /*float, between 0 and 1*/

#define WINMAN  "window_manager"
#define SHOW_ALL_WINS "show_all_windows" /*bool*/
#define LAUNCHERS "launchers"  /*str list*/

#define APP  "app"
#define ACTIVE_PNG "active_png"  /*string*/
#define USE_PNG  "use_png"  /*bool*/
#define ARROW_COLOR "arrow_color"  /*color*/
#define ARROW_OFFSET "arrow_offset"  /*offset*/
#define TASKS_H_ARROWS "tasks_have_arrows" /*bool*/
#define NAME_CHANGE_NOTIFY "name_change_notify" /*bool*/
#define ALPHA_EFFECT "alpha_effect"  /*bool*/
#define ICON_EFFECT "icon_effect"  /*int*/
#define HOVER_BOUNCE_EFFECT "hover_bounce_effect" /*bool*/
#define ICON_ALPHA "icon_alpha"  /*float*/
#define FRAME_RATE "frame_rate"  /*int*/
#define ICON_DEPTH_ON "icon_depth_on" /*bool*/
#define REFLECT_ALPHA_MULT "reflection_alpha_multiplier"  /*float*/

#define TITLE  "title"
#define TEXT_COLOR "text_color"  /*color*/
#define SHADOW_COLOR "shadow_color"  /*color*/
#define BACKGROUND "background"  /*color*/
#define FONT_FACE "font_face"  /*string*/

/* globals */
static AwnSettings *settings  = NULL;
static AwnConfigClient *client   = NULL;

/* prototypes */
static void awn_load_bool(AwnConfigClient *client, const gchar *group,
                          const gchar* key, gboolean *data, gboolean def);
static void awn_load_string(AwnConfigClient *client, const gchar *group,
                            const gchar* key, gchar **data, const gchar *def);
static void awn_load_float(AwnConfigClient *client, const gchar *group,
                           const gchar* key, gfloat *data, gfloat def);
static void awn_load_int(AwnConfigClient *client, const gchar *group,
                         const gchar* key, gint *data, gint def);
static void awn_load_color(AwnConfigClient *client, const gchar *group,
                           const gchar* key, AwnColor *color, const gchar * def);
static void awn_load_string_list(AwnConfigClient *client, const gchar *group,
                                 const gchar* key, GSList **data, GSList *def);

static void awn_notify_bool(AwnConfigClientNotifyEntry *entry, gboolean* data);
static void awn_notify_string(AwnConfigClientNotifyEntry *entry, gchar** data);
static void awn_notify_float(AwnConfigClientNotifyEntry *entry, gfloat* data);
static void awn_notify_int(AwnConfigClientNotifyEntry *entry, gint* data);
static void awn_notify_color(AwnConfigClientNotifyEntry *entry, AwnColor *data);

static void hex2float(gchar* HexColor, gfloat* FloatColor);

AwnSettings* awn_get_settings(void)
{
  //assert(settings != NULL);
  if (settings) return settings;
  else return awn_settings_new();
}

AwnSettings*
awn_settings_new()
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

  AwnSettings *s = NULL;

  s = g_new(AwnSettings, 1);

  settings = s;

  client = awn_config_client_new();

  s->icon_theme = gtk_icon_theme_get_default();

  /* general settings */
  awn_config_client_ensure_group(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP);

  awn_load_bool(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, FORCE_MONITOR, &s->force_monitor, FALSE);

  awn_load_int(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, MONITOR_WIDTH, &s->monitor_width, width);

  awn_load_int(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, MONITOR_HEIGHT, &s->monitor_height, height);

  awn_load_bool(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, PANEL_MODE, &s->panel_mode, FALSE);

  awn_load_bool(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, AUTO_HIDE, &s->auto_hide, FALSE);

  awn_load_int(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, AUTO_HIDE_DELAY, &s->auto_hide_delay, 1000);

  awn_load_bool(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, KEEP_BELOW, &s->keep_below, FALSE);

  awn_load_bool(client, AWN_CONFIG_CLIENT_DEFAULT_GROUP, SHOW_DIALOG, &s->show_dialog, TRUE);

  s->hidden = FALSE;

  /* Bar settings */
  awn_config_client_ensure_group(client, BAR);

  awn_load_int(client, BAR, BAR_HEIGHT, &s->bar_height, 48);
  // make sure bar_height is >= AWN_MIN_BAR_HEIGHT
  if (s->bar_height < AWN_MIN_BAR_HEIGHT) s->bar_height = AWN_MIN_BAR_HEIGHT;

  awn_load_float(client, BAR, BAR_POS, &s->bar_pos, 0.5);

  awn_load_int(client, BAR, BAR_ANGLE, &s->bar_angle, 0);

  awn_load_bool(client, BAR, NO_BAR_RESIZE_ANI, &s->no_bar_resize_ani, FALSE);

  awn_load_int(client, BAR, REFLECTION_OFFSET, &s->reflection_offset, 0);

  awn_load_int(client, BAR, ICON_OFFSET, &s->icon_offset, 10);

  awn_load_bool(client, BAR, ROUNDED_CORNERS, &s->rounded_corners, TRUE);

  awn_load_float(client, BAR, CORNER_RADIUS, &s->corner_radius, 10.0);

  awn_load_bool(client, BAR, RENDER_PATTERN, &s->render_pattern, FALSE);

  awn_load_bool(client, BAR, EXPAND_BAR, &s->expand_bar, FALSE);

  awn_load_string(client, BAR, PATTERN_URI, &s->pattern_uri, "~");

  awn_load_float(client, BAR, PATTERN_ALPHA, &s->pattern_alpha, 1.0);

  awn_load_color(client, BAR, GLASS_STEP_1, &s->g_step_1, "454545C8");

  awn_load_color(client, BAR, GLASS_STEP_2, &s->g_step_2, "010101BE");

  awn_load_color(client, BAR, GLASS_HISTEP_1, &s->g_histep_1, "FFFFFF0B");

  awn_load_color(client, BAR, GLASS_HISTEP_2, &s->g_histep_2, "FFFFFF0A");

  awn_load_color(client, BAR, BORDER_COLOR, &s->border_color, "000000CC");

  awn_load_color(client, BAR, HILIGHT_COLOR, &s->hilight_color, "FFFFFF11");

  awn_load_bool(client, BAR, SHOW_SEPARATOR, &s->show_separator, TRUE);

  awn_load_color(client, BAR, SEP_COLOR, &s->sep_color, "FFFFFF00");

  awn_load_float(client, BAR, CURVES_SYMMETRY, &s->curves_symmetry, 0.7);

  awn_load_float(client, BAR, CURVINESS, &s->curviness, 1.0);

  /* Window Manager settings */
  awn_config_client_ensure_group(client, WINMAN);

  awn_load_bool(client, WINMAN, SHOW_ALL_WINS, &s->show_all_windows, TRUE);

  awn_load_string_list(client, WINMAN, LAUNCHERS, &s->launchers, NULL);

  /* App settings */
  awn_config_client_ensure_group(client, APP);

  awn_load_string(client, APP, ACTIVE_PNG, &s->active_png, "~");

  awn_load_bool(client, APP, USE_PNG, &s->use_png, FALSE);

  awn_load_color(client, APP, ARROW_COLOR, &s->arrow_color, "FFFFFF66");

  awn_load_int(client, APP, ARROW_OFFSET, &s->arrow_offset, 2);

  awn_load_bool(client, APP, TASKS_H_ARROWS, &s->tasks_have_arrows, FALSE);

  awn_load_bool(client, APP, NAME_CHANGE_NOTIFY, &s->name_change_notify, FALSE);

  awn_load_bool(client, APP, ALPHA_EFFECT, &s->alpha_effect, FALSE);

  awn_load_int(client, APP, ICON_EFFECT, &s->icon_effect, 0);

  awn_load_float(client, APP, ICON_ALPHA, &s->icon_alpha, 1.0);

  awn_load_int(client, APP, FRAME_RATE, &s->frame_rate, 25);

  awn_load_bool(client, APP, ICON_DEPTH_ON, &s->icon_depth_on, TRUE);

  awn_load_float(client, APP, REFLECT_ALPHA_MULT, &s->reflection_alpha_mult, 0.33);

  awn_load_bool(client, APP, SHOW_SHADOWS, &s->show_shadows, FALSE);

  /* Title settings */
  awn_config_client_ensure_group(client, TITLE);

  awn_load_color(client, TITLE, TEXT_COLOR, &s->text_color, "FFFFFFFF");

  awn_load_color(client, TITLE, SHADOW_COLOR, &s->shadow_color, "1B3B12E1");

  awn_load_color(client, TITLE, BACKGROUND, &s->background, "000000AA");

  awn_load_string(client, TITLE, FONT_FACE, &s->font_face, "Sans 11");

  s->task_width = 12;
  s->task_width += settings->bar_height >= AWN_MIN_BAR_HEIGHT ?
      settings->bar_height :
      AWN_MIN_BAR_HEIGHT;

  /* make the custom icons directory */
  gchar *path = g_build_filename(g_get_home_dir(),
                                 ".config/awn/custom-icons",
                                 NULL);

  g_mkdir_with_parents(path, 0755);

  g_free(path);

  return s;
}

static void
awn_notify_bool(AwnConfigClientNotifyEntry *entry, gboolean* data)
{
  *data = entry->value.bool_val;

  //if (*data)
  //g_print("%s/%s is true\n", entry->group, entry->key);
}

static void
awn_notify_string(AwnConfigClientNotifyEntry *entry, gchar** data)
{
  *data = entry->value.str_val;

  //g_print("%s/%s is %s\n", entry->group, entry->key, *data);
}

static void
awn_notify_float(AwnConfigClientNotifyEntry *entry, gfloat* data)
{
  *data = entry->value.float_val;
  //g_print("%s/%s is %f\n", entry->group, entry->key, *data);
}

static void
awn_notify_int(AwnConfigClientNotifyEntry *entry, gint* data)
{
  *data = entry->value.int_val;
  //g_print("%s/%s is %f\n", entry->group, entry->key, *data);
}

static void
awn_notify_color(AwnConfigClientNotifyEntry *entry, AwnColor* color)
{
  float colors[4];

  hex2float(entry->value.str_val, colors);

  color->red = colors[0];
  color->green = colors[1];
  color->blue = colors[2];
  color->alpha = colors[3];
}


static void
awn_load_bool(AwnConfigClient *client, const gchar *group, const gchar* key, gboolean *data, gboolean def)
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

  awn_config_client_notify_add(client, group, key, (AwnConfigClientNotifyFunc)awn_notify_bool, data);
}

static void
awn_load_string(AwnConfigClient *client, const gchar *group, const gchar* key, gchar **data, const gchar *def)
{
  if (awn_config_client_entry_exists(client, group, key))
  {
    *data = awn_config_client_get_string(client, group, key, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_string(client, group, key, (gchar*)def, NULL);
    *data = g_strdup(def);
  }

  awn_config_client_notify_add(client, group, key, (AwnConfigClientNotifyFunc)awn_notify_string, data);
}

static void
awn_load_float(AwnConfigClient *client, const gchar *group, const gchar* key, gfloat *data, gfloat def)
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

  awn_config_client_notify_add(client, group, key, (AwnConfigClientNotifyFunc)awn_notify_float, data);
}

static void
awn_load_int(AwnConfigClient *client, const gchar *group, const gchar* key, gint *data, gint def)
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

  awn_config_client_notify_add(client, group, key, (AwnConfigClientNotifyFunc)awn_notify_int, data);
}

static void
awn_load_color(AwnConfigClient *client, const gchar *group, const gchar* key, AwnColor *color, const gchar * def)
{
  float colors[4];

  if (awn_config_client_entry_exists(client, group, key))
  {
    hex2float(awn_config_client_get_string(client, group, key, NULL), colors);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_string(client, group, key, (gchar*)def, NULL);
    hex2float((gchar*)def, colors);
  }

  color->red = colors[0];

  color->green = colors[1];
  color->blue = colors[2];
  color->alpha = colors[3];

  awn_config_client_notify_add(client, group, key, (AwnConfigClientNotifyFunc)awn_notify_color, color);
}

static void
awn_load_string_list(AwnConfigClient *client, const gchar *group, const gchar* key, GSList **data, GSList *def)
{
  if (awn_config_client_entry_exists(client, group, key))
  {
    *data = awn_config_client_get_list(client, group, key, AWN_CONFIG_CLIENT_LIST_TYPE_STRING, NULL);
  }
  else
  {
    g_print("%s unset, setting now\n", key);
    awn_config_client_set_list(client, group, key, AWN_CONFIG_CLIENT_LIST_TYPE_STRING, def, NULL);
    *data = def;
  }
}

static int
getdec(char hexchar)
{
  if ((hexchar >= '0') && (hexchar <= '9')) return hexchar - '0';

  if ((hexchar >= 'A') && (hexchar <= 'F')) return hexchar - 'A' + 10;

  if ((hexchar >= 'a') && (hexchar <= 'f')) return hexchar - 'a' + 10;

  return -1; // Wrong character

}

static void
hex2float(gchar* HexColor, gfloat* FloatColor)
{
  gchar* HexColorPtr = HexColor;

  gint i = 0;

  for (i = 0; i < 4; i++)
  {
    gint IntColor = (getdec(HexColorPtr[0]) * 16) +
                    getdec(HexColorPtr[1]);

    FloatColor[i] = (gfloat) IntColor / 255.0;
    HexColorPtr += 2;
  }

}


