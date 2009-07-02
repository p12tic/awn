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
 */

#ifndef _TASK_MANAGER_API_WRAPPER_H_
#define _TASK_MANAGER_API_WRAPPER_H_

#include <glib-object.h>
#include <libawn/libawn.h>

#include "task-manager.h"

#define TASK_TYPE_MANAGER_API_WRAPPER (task_manager_api_wrapper_get_type ())

#define TASK_MANAGER_API_WRAPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	TASK_TYPE_MANAGER_API_WRAPPER, TaskManagerApiWrapper))

#define TASK_MANAGER_API_WRAPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	TASK_TYPE_MANAGER_API_WRAPPER, TaskManagerApiWrapperClass))

#define TASK_IS_MANAGER_API_WRAPPER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	TASK_TYPE_MANAGER_API_WRAPPER))

#define TASK_IS_MANAGER_API_WRAPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	TASK_TYPE_MANAGER_API_WRAPPER))

#define TASK_MANAGER_API_WRAPPER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	TASK_TYPE_MANAGER_API_WRAPPER, TaskManagerApiWrapperClass))

typedef struct _TaskManagerApiWrapper        TaskManagerApiWrapper;
typedef struct _TaskManagerApiWrapperClass   TaskManagerApiWrapperClass;
typedef struct _TaskManagerApiWrapperPrivate TaskManagerApiWrapperPrivate;
 
struct _TaskManagerApiWrapper
{
  GObject parent;	

  TaskManagerApiWrapperPrivate *priv;
};

struct _TaskManagerApiWrapperClass
{
  GObjectClass parent_class;
};

GType       task_manager_api_wrapper_get_type (void) G_GNUC_CONST;

TaskManagerApiWrapper * task_manager_api_wrapper_new (TaskManager *manager);

/* name variants */

gboolean
task_manager_api_wrapper_set_task_icon_by_name (TaskManagerApiWrapper *wrapper,
                                            		gchar     *name,
                                             		gchar     *icon_path,
                                                GError    **error);

gboolean
task_manager_api_wrapper_unset_task_icon_by_name (TaskManagerApiWrapper *wrapper,
			                                        		gchar     *name,
			                                        		GError   	**error);

gboolean
task_manager_api_wrapper_set_info_by_name (TaskManagerApiWrapper *wrapper,
                                           gchar    *name,
                                           gchar    *info,
                                           GError   **error);

gboolean
task_manager_api_wrapper_unset_info_by_name (TaskManagerApiWrapper *wrapper,
                                             gchar     *name,
                                             GError    **error);

gboolean
task_manager_api_wrapper_set_progress_by_name (TaskManagerApiWrapper *wrapper,
                                               gchar    *name,
                                               gint     progress,
                                               GError   **error);

/*     XID variants     */

gboolean
task_manager_api_wrapper_set_task_icon_by_xid (TaskManagerApiWrapper *wrapper,
                                              gint64    xid,
                                              gchar     *icon_path,
                                              GError    **error);

gboolean
task_manager_api_wrapper_unset_task_icon_by_xid (TaskManagerApiWrapper *wrapper,
                                                 gint64    xid,
                                                 GError    **error);

gboolean
task_manager_api_wrapper_set_info_by_xid (TaskManagerApiWrapper *wrapper,
                                          gint64    xid,
                                          gchar     *info,
                                          GError    **error);

gboolean
task_manager_api_wrapper_unset_info_by_xid (TaskManagerApiWrapper *wrapper,
                                            gint64    xid,
                                            GError    **error);

gboolean
task_manager_api_wrapper_set_progress_by_xid (TaskManagerApiWrapper *wrapper,
                                              gint64    xid,
                                              gint      progress,
                                              GError    **error);

#endif /* _TASK_MANAGER_API_WRAPPER_H_ */

