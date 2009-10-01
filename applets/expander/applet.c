/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 */

#include <libawn/libawn.h>

gboolean awn_applet_factory_init (AwnApplet *applet);

static gboolean after_embed (gpointer user_data)
{
  AwnApplet *applet = (AwnApplet*)user_data;

  awn_applet_set_behavior (applet, AWN_APPLET_IS_EXPANDER);

  return FALSE;
}

static void on_embedded (AwnApplet *applet)
{
  g_idle_add (after_embed, applet);
}

gboolean
awn_applet_factory_init (AwnApplet *applet)
{
  // we should let the applet fully initialize, then we can set the flag
  g_signal_connect (applet, "embedded", G_CALLBACK (on_embedded), NULL);

  return TRUE;
}

