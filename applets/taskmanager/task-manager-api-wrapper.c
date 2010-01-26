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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "task-manager-api-wrapper.h"
#include "task-manager-api-wrapper-glue.h"

#include <libawn/libawn.h>

G_DEFINE_TYPE (TaskManagerApiWrapper, task_manager_api_wrapper, G_TYPE_OBJECT)

#define TASK_MANAGER_API_WRAPPER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_MANAGER_API_WRAPPER, \
  TaskManagerApiWrapperPrivate))

struct _TaskManagerApiWrapperPrivate
{
  TaskManager *manager;
};

enum
{
  PROP_0,
  PROP_MANAGER
};

/* GObject stuff */

static void
task_manager_api_wrapper_set_manager (TaskManagerApiWrapper *wrapper, 
                                      TaskManager *manager)
{
  g_return_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper));
  g_return_if_fail (TASK_IS_MANAGER (manager));

  wrapper->priv->manager = manager;
}

static void
task_manager_api_wrapper_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  TaskManagerApiWrapper *wrapper = TASK_MANAGER_API_WRAPPER (object);

  switch (prop_id)
  {
    case PROP_MANAGER:
      g_value_set_object (value, wrapper->priv->manager); 
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_manager_api_wrapper_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  TaskManagerApiWrapper *wrapper = TASK_MANAGER_API_WRAPPER (object);

  switch (prop_id)
  {
    case PROP_MANAGER:
      task_manager_api_wrapper_set_manager (wrapper, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

/* GObject stuff */
static void
task_manager_api_wrapper_class_init (TaskManagerApiWrapperClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->set_property = task_manager_api_wrapper_set_property;
  obj_class->get_property = task_manager_api_wrapper_get_property;
  
  /* Install properties */
  pspec = g_param_spec_object ("manager",
                               "Manager",
                               "TaskManager",
                               TASK_TYPE_MANAGER,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_MANAGER, pspec);  
  
  g_type_class_add_private (obj_class, sizeof (TaskManagerApiWrapperPrivate));

  dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                              &dbus_glib_task_manager_api_wrapper_object_info);
    
}

static void
task_manager_api_wrapper_init (TaskManagerApiWrapper *wrapper)
{
  TaskManagerApiWrapperPrivate *priv;

  /* get and save private struct */
  priv = wrapper->priv = TASK_MANAGER_API_WRAPPER_GET_PRIVATE (wrapper);
  priv->manager = NULL;
}


TaskManagerApiWrapper *
task_manager_api_wrapper_new (TaskManager *manager)
{
  TaskManagerApiWrapper *wrapper = NULL;

  wrapper = g_object_new (TASK_TYPE_MANAGER_API_WRAPPER,
                          "manager", manager,
                          NULL);

  return wrapper;
}

gboolean
task_manager_api_wrapper_set_task_icon_by_name (TaskManagerApiWrapper *wrapper,
                                            		gchar     *name,
                                             		gchar     *icon_path,
                                                GError    **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "icon-file";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_STRING);
  g_value_set_string (&window, name);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, icon_path);
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_unset_task_icon_by_name (TaskManagerApiWrapper *wrapper,
			                                        		gchar     *name,
			                                        		GError   	**error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "icon-file";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_STRING);
  g_value_set_string (&window, name);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, "");
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_set_info_by_name (TaskManagerApiWrapper *wrapper,
                                           gchar    *name,
                                           gchar    *info,
                                           GError   **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "message";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_STRING);
  g_value_set_string (&window, name);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, info);
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_unset_info_by_name (TaskManagerApiWrapper *wrapper,
                                             gchar     *name,
                                             GError    **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "message";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_STRING);
  g_value_set_string (&window, name);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, "");
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_set_progress_by_name (TaskManagerApiWrapper *wrapper,
                                               gchar    *name,
                                               gint     progress,
                                               GError   **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "progress";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_STRING);
  g_value_set_string (&window, name);

  //difference between old and new api
  //unsetting progress indicator by setting progress to -1;
  if (progress == 100) progress = -1;

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, progress);
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

/*     XID variants     */

gboolean
task_manager_api_wrapper_set_task_icon_by_xid (TaskManagerApiWrapper *wrapper,
                                              gint64    xid,
                                              gchar     *icon_path,
                                              GError    **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "icon-file";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_INT64);
  g_value_set_int64 (&window, xid);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, icon_path);
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_unset_task_icon_by_xid (TaskManagerApiWrapper *wrapper,
                                                 gint64    xid,
                                                 GError    **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "icon-file";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_INT64);
  g_value_set_int64 (&window, xid);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, "");
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_set_info_by_xid (TaskManagerApiWrapper *wrapper,
                                          gint64    xid,
                                          gchar     *info,
                                          GError    **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "message";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_INT64);
  g_value_set_int64 (&window, xid);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, info);
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_unset_info_by_xid (TaskManagerApiWrapper *wrapper,
                                            gint64    xid,
                                            GError    **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "message";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_INT64);
  g_value_set_int64 (&window, xid);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_string (&value, "");
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}

gboolean
task_manager_api_wrapper_set_progress_by_xid (TaskManagerApiWrapper *wrapper,
                                              gint64    xid,
                                              gint      progress,
                                              GError    **error)
{
  TaskManagerApiWrapperPrivate *priv;
  gboolean succeeded;
  GValue window = {0};
  GHashTable *hints;
  const gchar *key = "progress";
  GValue value = {0};
  
  g_return_val_if_fail (TASK_IS_MANAGER_API_WRAPPER (wrapper), FALSE);

  priv = wrapper->priv;
  
  g_value_init (&window, G_TYPE_INT64);
  g_value_set_int64 (&window, xid);

  //difference between old and new api
  //unsetting progress indicator by setting progress to -1;
  if (progress == 100) progress = -1;

  g_value_init (&value, G_TYPE_INT);
  g_value_set_int (&value, progress);
  
  hints = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (hints, (gpointer)key, &value);
  
  succeeded = task_manager_update (priv->manager,
                                   &window,
                                   hints,
                                   error);
  g_clear_error (error);

  g_value_unset (&window);
  g_value_unset (&value);
  g_hash_table_destroy (hints);

  return TRUE;
}
