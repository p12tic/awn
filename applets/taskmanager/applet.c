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

#include <libawn/libawn.h>

#include "task-manager.h"

AwnApplet* awn_applet_factory_initp (gchar* uid, gint panel_id);

AwnApplet*
awn_applet_factory_initp (gchar* uid, gint panel_id)
{
  AwnApplet *applet;

  applet = task_manager_new (uid, panel_id);

  return applet;
}
