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
  PROP_DIALOG_SCALE,
  PROP_APPLET
};

struct _TaskManagerDialogPrivate 
{
  int dialog_mode;
  int current_dialog_mode;
	long * data;
  gulong wm_change_id;
  
  gboolean analyzed;
	GdkAtom kde_a;
  DesktopAgnosticConfigClient *client;
  AwnApplet * applet;
  gdouble dialog_scale;

  
  GtkWidget * main_box;
  GtkWidget * items_box;
  
  GList * children;
};

static void
task_manager_dialog_analyze_wm (TaskManagerDialog * dialog);

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
    case PROP_DIALOG_SCALE:
      g_value_set_double (value, priv->dialog_scale);
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
    case PROP_DIALOG_SCALE:
      priv->dialog_scale = g_value_get_double (value);
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
  if ( priv->wm_change_id )
  {
    g_signal_handler_disconnect (wnck_screen_get_default(),priv->wm_change_id);
    priv->wm_change_id = 0;
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

  desktop_agnostic_config_client_bind (priv->client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "dialog_scale",
                                       object, "dialog scale", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  priv->children = NULL;
  priv->main_box = gtk_vbox_new (FALSE,3);
  priv->items_box = gtk_hbox_new (FALSE,3);
  priv->current_dialog_mode = priv->dialog_mode;
  gtk_container_add (GTK_CONTAINER (priv->main_box),priv->items_box);
  gtk_container_add (GTK_CONTAINER (object),priv->main_box);
  gtk_widget_show_all (priv->main_box);

  task_manager_dialog_analyze_wm (TASK_MANAGER_DIALOG(object));
  priv->wm_change_id = g_signal_connect_swapped (wnck_screen_get_default(),"window-manager-changed",
                            G_CALLBACK(task_manager_dialog_analyze_wm),object);
  
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
  g_list_free (priv->children);
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

  pspec = g_param_spec_double ("dialog_scale",
                            "dialog_scale",
                            "Dialog mode",
                            0.05,
                            1.0,
                            0.3,
                            G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_DIALOG_SCALE, pspec);

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
  gint width;
  gint data_length;

  gint win_x,win_y,win_width,win_height;
  GtkAllocation allocation;
  GList * iter = NULL;
  gint win_count = 0;
  int i = 0;
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);
  GtkPositionType pos_type = awn_applet_get_pos_type (priv->applet);
  GtkOrientation current_orientation;
  gdouble scale;
  glong total_width = 0;
  glong screen_width = gdk_screen_get_width (gdk_screen_get_default ());
  glong screen_height = gdk_screen_get_height (gdk_screen_get_default ());
  
  
  current_orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (priv->items_box));
  if ( (current_orientation == GTK_ORIENTATION_VERTICAL) &&
      (( pos_type == GTK_POS_BOTTOM) || (pos_type == GTK_POS_TOP)) )
  {
    gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->items_box),GTK_ORIENTATION_HORIZONTAL);
  }
  else if ( (current_orientation == GTK_ORIENTATION_HORIZONTAL)&&
      (( pos_type == GTK_POS_LEFT) || (pos_type == GTK_POS_RIGHT)) )
  {
    gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->items_box),GTK_ORIENTATION_VERTICAL);
  }
  for (iter = g_list_first(priv->children); iter; iter=iter->next)
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
  scale = priv->dialog_scale;

scaled_down:
  total_width = 0;
  if (screen_width && screen_height)
  {
    for (iter = g_list_first(priv->children); iter; iter=iter->next)
    {
      if (TASK_IS_WINDOW(iter->data))
      {
        wnck_window_get_geometry (task_window_get_window (iter->data),
                                  &win_x,
                                  &win_y,
                                  &win_width,
                                  &win_height);

        gtk_widget_get_allocation (GTK_WIDGET(iter->data), &allocation);
        if ((pos_type == GTK_POS_TOP) || (pos_type == GTK_POS_BOTTOM))
        {
          /*conditional operator alert*/
          height = gdk_screen_height () / (win_count?1.0/scale:1.0/scale+(win_count-2));

          width = ((float)win_width) / ((float)win_height) * height;
          total_width = total_width + width;
          if (total_width > screen_width * 0.9)
          {
            scale = scale * 0.9;
            goto scaled_down;
          }
        }
        else
        {
          /*conditional operator alert*/
          width = gdk_screen_width () / (win_count<4?1.0/scale+1:1.0/scale+1+(win_count-3));
          height = ((float)win_height) / ((float)win_width) * width;
          total_width = total_width + height;
          if (total_width > screen_height * 0.9)
          {
            scale = scale * 0.9;
            goto scaled_down;
          }
        }
      }
    }
  }
  
  for (iter = g_list_first(priv->children); iter; iter=iter->next)
  {
    if (TASK_IS_WINDOW(iter->data))
    {
      wnck_window_get_geometry (task_window_get_window (iter->data),
                                &win_x,
                                &win_y,
                                &win_width,
                                &win_height);

      gtk_widget_get_allocation (GTK_WIDGET(iter->data), &allocation);
      gtk_widget_set_tooltip_text (GTK_WIDGET (iter->data),
                                   task_window_get_name(TASK_WINDOW(iter->data)));
      /* Change these calculations. Ultimately we can be much smarter about
       layout.  After a certain point it will involve adding some more
       containers in our container....*/
      if ((pos_type == GTK_POS_TOP) || (pos_type == GTK_POS_BOTTOM))
      {
        /*conditional operator alert*/
        height = gdk_screen_height () / (win_count?1.0/scale:1.0/scale+(win_count-2));

        width = ((float)win_width) / ((float)win_height) * height;
        gtk_widget_set_size_request (GTK_WIDGET(iter->data), width, height);
      }
      else
      {
        /*conditional operator alert*/
        width = gdk_screen_width () / (win_count<4?1.0/scale+1:1.0/scale+1+(win_count-3));
        height = ((float)win_height) / ((float)win_width) * width;
        gtk_widget_set_size_request (GTK_WIDGET(iter->data), width, height);
      }      	
	    priv->data[i*6+1] = (long) 5;
	    priv->data[i*6+2] = (long) task_window_get_xid (TASK_WINDOW(iter->data));
	    priv->data[i*6+3] = (long) allocation.x+4;
	    priv->data[i*6+4] = (long) allocation.y+4;
	    priv->data[i*6+5] = (long) width-8;
	    priv->data[i*6+6] = (long) height-8;
      i++;
    }
  }

	gdk_property_change ((GTK_WIDGET(dialog))->window, 
					priv->kde_a,
 					priv->kde_a,
					32, 
					GDK_PROP_MODE_REPLACE, 
					(guchar*) priv->data,
					data_length);
}

static void
task_manager_dialog_hide (GtkWidget *dialog,gpointer nul)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);

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
}

static gboolean 
task_manager_dialog_expose (GtkWidget *dialog,GdkEventExpose *event,gpointer nul)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);
  GList * iter = NULL;

  if (!priv->analyzed)
  {
    task_manager_dialog_analyze_wm (TASK_MANAGER_DIALOG (dialog));
  }
  switch (priv->current_dialog_mode)
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
      if (gtk_orientable_get_orientation (GTK_ORIENTABLE(priv->items_box))!=GTK_ORIENTATION_VERTICAL)
      {
        gtk_orientable_set_orientation (GTK_ORIENTABLE (priv->items_box),GTK_ORIENTATION_VERTICAL);
      }
      for (iter = g_list_first(priv->children); iter; iter=iter->next)
      {
        if (TASK_IS_WINDOW(iter->data))
        {
          gtk_widget_set_size_request (GTK_WIDGET(iter->data), -1, -1);
          gtk_widget_set_tooltip_text (GTK_WIDGET (iter->data),NULL);
        }
      }
      break;  
  }
  return FALSE;
}

static void
task_manager_dialog_init (TaskManagerDialog *self)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (self);
  priv->data = NULL;
  priv->analyzed = FALSE;
  priv->wm_change_id = 0;
	priv->kde_a = gdk_atom_intern_static_string ("_KDE_WINDOW_PREVIEW");
  
  g_signal_connect (self,"expose-event",G_CALLBACK(task_manager_dialog_expose),NULL);
  g_signal_connect (self,"hide",G_CALLBACK(task_manager_dialog_hide),NULL);
  
}

GtkWidget*
task_manager_dialog_new (GtkWidget * widget, AwnApplet * applet)
{
  return g_object_new (TASK_TYPE_MANAGER_DIALOG, 
                        "anchor", widget,
                        "anchor-applet", applet,
                        "applet",applet,
                        NULL);
}


void
task_manager_dialog_add (TaskManagerDialog * dialog,TaskItem * item)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);
  if (TASK_IS_LAUNCHER(item))
  {
    gtk_container_add (GTK_CONTAINER (priv->main_box), GTK_WIDGET (item));
    gtk_box_reorder_child (GTK_BOX(priv->main_box),
                           GTK_WIDGET(item),0);    
  }
  else
  {
    gtk_container_add (GTK_CONTAINER (priv->items_box), GTK_WIDGET (item));
  }
  priv->children = g_list_append (priv->children,item);
}

void
task_manager_dialog_remove (TaskManagerDialog * dialog,TaskItem * item)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);
  gtk_container_remove(GTK_CONTAINER(awn_dialog_get_content_area(AWN_DIALOG(dialog))), GTK_WIDGET(item));
  priv->children = g_list_remove (priv->children,item);
}

static gboolean
task_manager_dialog_fn_true (TaskManagerDialog * dialog)
{
  return TRUE;
}

static gboolean
task_manager_dialog_fn_false (TaskManagerDialog * dialog)
{
  return FALSE;
}

static gboolean
task_manager_dialog_check_kde_preview_active (TaskManagerDialog * dialog)
{
  glong *data;
  GdkAtom actual_type;
  gint format,len;

  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);  
  GdkWindow *root_win = gtk_widget_get_root_window (GTK_WIDGET(dialog));
  if (root_win)
  {
    if (gdk_property_get (root_win,
                      priv->kde_a,
                      priv->kde_a,
                      0,
                      1,
                      0,
                      &actual_type,
                      &format,
                      &len,
                      (guchar **)&data))
    {
      g_free (data);
      return TRUE;
    }
  }
  return FALSE;
}

#define FN_TRUE task_manager_dialog_fn_true
#define FN_FALSE task_manager_dialog_fn_false

enum {
  WM_UNKNOWN,
  WM_COMPIZ,
  WM_KWIN,
  WM_METACITY,
  WM_MUTTER,
  WM_OPENBOX,
  WM_XFWM4,
  WM_END
}WM;

typedef struct
{
  const gchar * wm_name;
  gint  wm_code;
  gboolean (*live_previews) (TaskManagerDialog*);
  gboolean (*min_mapped) (TaskManagerDialog*);
  gboolean (*inactive_ws_mapped) (TaskManagerDialog*);
} WMStrings;

WMStrings wm_strings[]={
  {"compiz",WM_COMPIZ,task_manager_dialog_check_kde_preview_active,FN_FALSE,FN_TRUE},
  {"KWin",WM_KWIN,task_manager_dialog_check_kde_preview_active,FN_TRUE,FN_TRUE},
  {"Metacity",WM_METACITY,FN_FALSE,FN_FALSE,FN_TRUE},
  {"Mutter",WM_MUTTER,FN_TRUE,FN_TRUE,FN_TRUE},
  {"Openbox",WM_OPENBOX,FN_FALSE,FN_FALSE,FN_TRUE},
  {"Xfwm4",WM_XFWM4,FN_FALSE,FN_FALSE,FN_TRUE},
  {NULL,WM_UNKNOWN,FN_FALSE,FN_FALSE,FN_FALSE},
  {NULL,WM_END,FN_FALSE,FN_FALSE,FN_FALSE}
};

static void 
task_manager_dialog_analyze_wm (TaskManagerDialog * dialog)
{
  TaskManagerDialogPrivate * priv = GET_PRIVATE (dialog);
  WnckScreen * wnck_screen = wnck_screen_get_default ();
  gint wm = WM_UNKNOWN;
  
  const gchar * wm_name = wnck_screen_get_window_manager_name (wnck_screen);
//  g_debug ("%s:  %s",__func__,wm_name);
  for (int i = 0; wm_strings[i].wm_code != WM_END ;i++)
  {
//    g_debug ("comp:%d  '%s', '%s'",i,wm_strings[i].wm_name,wm_name);
    if (g_strcmp0 (wm_strings[i].wm_name,wm_name)==0)
    {
      wm = wm_strings[i].wm_code;
 //     g_message ("WM = %s, code = %d",wm_name,wm);
      if ( wm_strings[i].live_previews (dialog))
      {
        if ( (priv->dialog_mode == 0) || (priv->dialog_mode==2))
        {
          priv->current_dialog_mode = 2;
        }
      }
      else
      {
        priv->current_dialog_mode = 1;
      }
      break;
    }
  }
  priv->analyzed = TRUE;
}
