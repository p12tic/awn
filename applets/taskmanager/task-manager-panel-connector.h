/* task-manager-panel-connector.h */

#ifndef _TASK_MANAGER_PANEL_CONNECTOR
#define _TASK_MANAGER_PANEL_CONNECTOR

#include <glib-object.h>

G_BEGIN_DECLS

#define TASK_MANAGER_TYPE_PANEL_CONNECTOR task_manager_panel_connector_get_type()

#define TASK_MANAGER_PANEL_CONNECTOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TASK_MANAGER_TYPE_PANEL_CONNECTOR, TaskManagerPanelConnector))

#define TASK_MANAGER_PANEL_CONNECTOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TASK_MANAGER_TYPE_PANEL_CONNECTOR, TaskManagerPanelConnectorClass))

#define TASK_MANAGER_IS_PANEL_CONNECTOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TASK_MANAGER_TYPE_PANEL_CONNECTOR))

#define TASK_MANAGER_IS_PANEL_CONNECTOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), TASK_MANAGER_TYPE_PANEL_CONNECTOR))

#define TASK_MANAGER_PANEL_CONNECTOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TASK_MANAGER_TYPE_PANEL_CONNECTOR, TaskManagerPanelConnectorClass))

typedef struct {
  GObject parent;
} TaskManagerPanelConnector;

typedef struct {
  GObjectClass parent_class;
} TaskManagerPanelConnectorClass;

GType task_manager_panel_connector_get_type (void);

guint task_manager_panel_connector_inhibit_autohide (TaskManagerPanelConnector *conn, const gchar *reason);
void task_manager_panel_connector_uninhibit_autohide (TaskManagerPanelConnector *conn, guint cookie);

TaskManagerPanelConnector* task_manager_panel_connector_new (gint id);

G_END_DECLS

#endif /* _TASK_MANAGER_PANEL_CONNECTOR */
