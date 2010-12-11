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

/* task-manager-dialog.c */

#include "task-manager-dialog.h"

G_DEFINE_TYPE (TaskManagerDialog, task_manager_dialog, AWN_TYPE_DIALOG)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TASK_TYPE_MANAGER_DIALOG, TaskManagerDialogPrivate))

typedef struct _TaskManagerDialogPrivate TaskManagerDialogPrivate;

struct _TaskManagerDialogPrivate {
    int dummy;
};

static void
task_manager_dialog_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
task_manager_dialog_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
task_manager_dialog_dispose (GObject *object)
{
  G_OBJECT_CLASS (task_manager_dialog_parent_class)->dispose (object);
}

static void
task_manager_dialog_finalize (GObject *object)
{
  G_OBJECT_CLASS (task_manager_dialog_parent_class)->finalize (object);
}

static void
task_manager_dialog_class_init (TaskManagerDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (TaskManagerDialogPrivate));

  object_class->get_property = task_manager_dialog_get_property;
  object_class->set_property = task_manager_dialog_set_property;
  object_class->dispose = task_manager_dialog_dispose;
  object_class->finalize = task_manager_dialog_finalize;
}

static void
task_manager_dialog_init (TaskManagerDialog *self)
{
}

TaskManagerDialog*
task_manager_dialog_new (GtkWidget * widget, AwnApplet * applet)
{
  return g_object_new (TASK_TYPE_MANAGER_DIALOG, 
                        "anchor", widget,
                        "anchor-applet", applet,
                        NULL);
}

