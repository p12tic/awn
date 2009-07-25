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
 * Authored by Hannes Verschore <hv1989@gmail.com>
 *
 */

#ifndef _TASK_ITEM_PRIVATE_H_
#define _TASK_ITEM_PRIVATE_H_

#include "task-item.h"

#define TASK_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ITEM, \
  TaskItemPrivate))

struct _TaskItemPrivate
{
  GtkWidget *box;
  GtkWidget *name;
  GtkWidget *image;
  GdkPixbuf *icon;
};

#endif