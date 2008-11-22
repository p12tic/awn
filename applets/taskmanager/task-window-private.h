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

#define TASK_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_WINDOW, \
  TaskWindowPrivate))

struct _TaskWindowPrivate
{
  WnckWindow *window;
  GSList     *utilities;

  /* Properties */
  gchar   *message;
  gfloat   progress;
  gboolean hidden;
  gboolean needs_attention;
};


enum
{
  NAME_CHANGED,
  ICON_CHANGED,
  ACTIVE_CHANGED,
  NEEDS_ATTENTION,
  WORKSPACE_CHANGED,
  MESSAGE_CHANGED,
  PROGRESS_CHANGED,
  HIDDEN_CHANGED,

  LAST_SIGNAL
};
static guint32 _window_signals[LAST_SIGNAL] = { 0 };
