/* 
 * Copyright (C) 2010 Rodney Cryderman <rcryderman@gmail.com> 
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
 */

/* task-manager-dialog.h */

#ifndef _TASK_MANAGER_DIALOG
#define _TASK_MANAGER_DIALOG

#include "libawn/awn-dialog.h"
#include "task-item.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define TASK_TYPE_MANAGER_DIALOG task_manager_dialog_get_type()

#define TASK_MANAGER_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TASK_TYPE_MANAGER_DIALOG, TaskManagerDialog))

#define TASK_MANAGER_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TASK_TYPE_MANAGER_DIALOG, TaskManagerDialogClass))

#define TASK_IS_MANAGER_DIALOG(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TASK_TYPE_MANAGER_DIALOG))

#define TASK_IS_MANAGER_DIALOG_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TASK_TYPE_MANAGER_DIALOG))

#define TASK_MANAGER_DIALOG_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TASK_TYPE_MANAGER_DIALOG, TaskManagerDialogClass))

typedef struct {
  AwnDialog parent;
} TaskManagerDialog;

typedef struct {
  AwnDialogClass parent_class;
} TaskManagerDialogClass;

GType task_manager_dialog_get_type (void);

GtkWidget* task_manager_dialog_new (GtkWidget * widget, AwnApplet * applet);

void task_manager_dialog_add (TaskManagerDialog * dialog,TaskItem * item);

void task_manager_dialog_remove (TaskManagerDialog * dialog,TaskItem * item);

G_END_DECLS

#endif /* _TASK_MANAGER_DIALOG */

