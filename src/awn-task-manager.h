/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifndef __AWN_TASK_MANAGER_H__
#define __AWN_TASK_MANAGER_H__

#include <gtk/gtk.h>

#include <libawn/awn-settings.h>

G_BEGIN_DECLS

#define AWN_TYPE_TASK_MANAGER		(awn_task_manager_get_type ())
#define AWN_TASK_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_TASK_MANAGER, AwnTaskManager))
#define AWN_TASK_MANAGER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_TASK_MANAGER, AwnTaskManagerClass))
#define AWN_IS_TASK_MANAGER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_TASK_MANAGER))
#define AWN_IS_TASK_MANAGER_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), AWN_TYPE_TASK_MANAGER))
#define AWN_TASK_MANAGER_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_TASK_MANAGER, AwnTaskManagerClass))

typedef struct _AwnTaskManager		AwnTaskManager;
typedef struct _AwnTaskManagerClass	AwnTaskManagerClass;

struct _AwnTaskManager
{
	GtkHBox parent;

	/* < private > */
};

struct _AwnTaskManagerClass
{
	GtkHBoxClass parent_class;

	void (*menu_item_clicked) (AwnTaskManager *task, guint id);
	void (*check_item_clicked) (AwnTaskManager *task, guint id, gboolean active);		

};

typedef enum 
{
  AWN_ACTIVATE_DEFAULT,
  AWN_ACTIVATE_MOVE_TO_TASK_VIEWPORT,
  AWN_ACTIVATE_MOVE_TASK_TO_ACTIVE_VIEWPORT
} AwnActivateBehavior;

GtkWidget *awn_task_manager_new (AwnSettings *settings);

void awn_task_manager_remove_launcher (AwnTaskManager *task_manager, gpointer task);

void awn_task_manager_remove_task     (AwnTaskManager *task_manager, gpointer task);

gboolean awn_task_manager_refresh_box (AwnTaskManager *task_manager);

gboolean awn_task_manager_get_windows (AwnTaskManager *task_manager, gdouble *number, GError **error);

gboolean awn_task_manager_screen_has_viewports (AwnTaskManager *task_manager);

gint awn_task_manager_get_activate_behavior (AwnTaskManager *task_manager);

G_END_DECLS

#endif

