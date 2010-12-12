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
#include "task-window.h"
#include "task-launcher.h"

G_DEFINE_TYPE (TaskManagerDialog, task_manager_dialog, AWN_TYPE_DIALOG)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TASK_TYPE_MANAGER_DIALOG, TaskManagerDialogPrivate))

typedef struct _TaskManagerDialogPrivate TaskManagerDialogPrivate;

enum
{
  PROP_0,
  PROP_DIALOG_MODE,
  PROP_APPLET
};

struct _TaskManagerDialogPrivate 
{
  int dialog_mode;
	long * data;
	GdkAtom kde_a;
  DesktopAgnosticConfigClient *client;
  AwnApplet * applet;
};

static void
task_manager_dialog_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_DIALOG_MODE:
      g_value_set_int (value, priv->dialog_mode); 
      break;
    case PROP_APPLET:
      g_value_set_object (value,priv->applet);
      break;  
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
task_manager_dialog_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_DIALOG_MODE:
      priv->dialog_mode = g_value_get_int (value);
      break;
    case PROP_APPLET:
      priv->applet = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
task_manager_dialog_dispose (GObject *object)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (object);
  if (priv->client)
  {
    desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                        object,
                                                        NULL);
    priv->client=NULL;
  }
  G_OBJECT_CLASS (task_manager_dialog_parent_class)->dispose (object);
}

static void
task_manager_dialog_constructed (GObject *object)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (object);
  GError             *error=NULL;
  
  G_OBJECT_CLASS (task_manager_dialog_parent_class)->constructed (object);

  priv->client = awn_config_get_default_for_applet (priv->applet, &error);
  if (error)
  {
    g_debug ("%s: %s",__func__,error->message);
    g_error_free (error);
    error=NULL;
  }
  
  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "dialog_mode",
                                       object, "dialog mode", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);

}

static void
task_manager_dialog_finalize (GObject *object)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (object);
  if (priv->data)
  {
    g_free (priv->data);
    priv->data = NULL;
  }
  G_OBJECT_CLASS (task_manager_dialog_parent_class)->finalize (object);
}

static void
task_manager_dialog_class_init (TaskManagerDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec     *pspec;
  
  object_class->get_property = task_manager_dialog_get_property;
  object_class->set_property = task_manager_dialog_set_property;
  object_class->dispose = task_manager_dialog_dispose;
  object_class->finalize = task_manager_dialog_finalize;
  object_class->constructed = task_manager_dialog_constructed;

  pspec = g_param_spec_int ("dialog_mode",
                            "dialog_mode",
                            "Dialog mode",
                            0,
                            2,
                            1,
                            G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DIALOG_MODE, pspec);

  pspec = g_param_spec_object ("applet",
                               "Applet",
                               "AwnApplet this icon belongs to",
                               AWN_TYPE_APPLET,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_APPLET, pspec);
  
  g_type_class_add_private (klass, sizeof (TaskManagerDialogPrivate));


}


static void
task_manager_dalog_disp_preview (TaskManagerDialog *dialog) 
{
	gint height;
	gint data_length;

  gint win_x,win_y,win_width,win_height;
  GtkAllocation allocation;
  GtkWidget * container = NULL;
  GList * children = NULL;
  GList * iter = NULL;
  gint win_count = 0;
  int i = 0;
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);

  container = awn_dialog_get_content_area (AWN_DIALOG(dialog));
  children = gtk_container_get_children (GTK_CONTAINER(container));
  
  for (iter = g_list_first(children); iter; iter=iter->next)
  {
    if (TASK_IS_WINDOW(iter->data))
    {
      win_count++;
    }
  }
  if (priv->data)
  {
    g_free (priv->data);
  }
	data_length =  win_count*6 +1;
  priv->data = g_new0 (long, data_length);
  priv->data[0] = (long) win_count;
    
  for (iter = g_list_first(children); iter; iter=iter->next)
  {
    if (TASK_IS_WINDOW(iter->data))
    {
      g_debug ("window");
      wnck_window_get_geometry (task_window_get_window (iter->data),
                                &win_x,
                                &win_y,
                                &win_width,
                                &win_height);
      
      gtk_widget_get_allocation (GTK_WIDGET(iter->data), &allocation);
      height = ((float)win_height) / ((float)win_width) * allocation.width;
      gtk_widget_set_size_request (GTK_WIDGET(iter->data), -1, height);
      	
	    priv->data[i*6+1] = (long) 5;
	    priv->data[i*6+2] = (long) task_window_get_xid (TASK_WINDOW(iter->data));
	    priv->data[i*6+3] = (long) allocation.x;
	    priv->data[i*6+4] = (long) allocation.y;
	    priv->data[i*6+5] = (long) allocation.width;
	    priv->data[i*6+6] = (long) height;
      i++;
    }
  }
  g_list_free (children);

	gdk_property_change ((GTK_WIDGET(dialog))->window, 
					priv->kde_a,
 					priv->kde_a,
					32, 
					GDK_PROP_MODE_REPLACE, 
					(guchar*) priv->data,
					data_length);
}


static gboolean 
task_manager_dialog_expose (GtkWidget *dialog,GdkEventExpose *event,gpointer nul)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);
  GList * iter = NULL;
  GList * children = NULL;
  GtkWidget * container; 
  
  g_message ("%s",__func__);
  
  switch (priv->dialog_mode)
  {
    case 2:
      task_manager_dalog_disp_preview (TASK_MANAGER_DIALOG(dialog));
      break;
    default:
      if (priv->data)
      {
        g_free (priv->data);
        priv->data = g_new0 (long,1);
        priv->data[0] = 0;
	      gdk_property_change ((GTK_WIDGET(dialog))->window, 
					priv->kde_a,
 					priv->kde_a,
					32, 
					GDK_PROP_MODE_REPLACE, 
					(guchar*) priv->data,
					1);
      }
      container = awn_dialog_get_content_area (AWN_DIALOG(dialog));
      children = gtk_container_get_children (GTK_CONTAINER(container));
      for (iter = g_list_first(children); iter; iter=iter->next)
      {
        if (TASK_IS_WINDOW(iter->data))
        {
          gtk_widget_set_size_request (GTK_WIDGET(iter->data), -1, -1);
        }
      }
      g_list_free (children);
      break;  
  }

  return FALSE;
}

static void
task_manager_dialog_init (TaskManagerDialog *self)
{
  g_message ("%s",__func__);
  TaskManagerDialogPrivate * priv = GET_PRIVATE (self);
  priv->dialog_mode = 1;
  priv->data = NULL;
	priv->kde_a = gdk_atom_intern_static_string ("_KDE_WINDOW_PREVIEW");
  
  g_signal_connect (self,"expose-event",G_CALLBACK(task_manager_dialog_expose),NULL);
  
}

GtkWidget*
task_manager_dialog_new (GtkWidget * widget, AwnApplet * applet)
{
  g_message ("%s",__func__);  
  return g_object_new (TASK_TYPE_MANAGER_DIALOG, 
                        "anchor", widget,
                        "anchor-applet", applet,
                        "applet",applet,
                        NULL);
}

