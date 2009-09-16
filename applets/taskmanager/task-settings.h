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

#ifndef _TASK_SETTINGS_H
#define _TASK_SETTINGS_H

#include <glib.h>

#include <libawn/libawn.h>

typedef struct
{
  gint  panel_size;
  gint  position;
  gint  offset;
  
} TaskSettings;

TaskSettings * task_settings_get_default (AwnApplet * applet);

#endif /* _TASK_SETTINGS_H */
