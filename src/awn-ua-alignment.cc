/*
 *  Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 *
 */

/* awn-ua-alignment.c */
#include <stdlib.h>
#include <libawn/awn-utils.h>
#include "libawn/gseal-transition.h"
#include "awn-ua-alignment.h"
#include "awn-defines.h"

G_DEFINE_TYPE (AwnUAAlignment, awn_ua_alignment, GTK_TYPE_ALIGNMENT)

#define AWN_UA_ALIGNMENT_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_UA_ALIGNMENT, AwnUAAlignmentPrivate))

typedef struct _AwnUAAlignmentPrivate AwnUAAlignmentPrivate;

struct _AwnUAAlignmentPrivate
{
  GtkWidget * socket;
  GtkWidget * applet_manager;
  
  gchar       *ua_list_entry;
  double      ua_ratio;
  
  guint       notify_size_id;
  guint       notify_position_id;
  guint       notify_offset_id;
  guint       notify_ua_list_id;
};

enum
{
  PROP_0,
  PROP_APPLET_MANAGER,
  PROP_UA_LIST_ENTRY,
  PROP_UA_RATIO
};


static gboolean awn_ua_alignment_plug_removed (GtkWidget * socket,
                                               AwnUAAlignment * self);
static void awn_ua_alignment_list_change(GObject *object,
                                         GParamSpec *param_spec,
                                         gpointer user_data);
static void awn_ua_alignment_position_change(GObject *object,
                                           GParamSpec *param_spec,
                                           gpointer user_data);
static void awn_ua_alignment_size_change(GObject *object,
                                         GParamSpec *param_spec,
                                         gpointer user_data);
static void awn_ua_alignment_offset_change(GObject *object,
                                           GParamSpec *param_spec,
                                           gpointer user_data);


static void
awn_ua_alignment_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  AwnUAAlignmentPrivate * priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (object);
  
  switch (property_id) 
  {
    case PROP_APPLET_MANAGER:
      g_value_set_object (value,priv->applet_manager);
      break;
    case PROP_UA_LIST_ENTRY:
      g_value_set_string (value,priv->ua_list_entry);
      break;
    case PROP_UA_RATIO:
      g_value_set_double (value,priv->ua_ratio);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_ua_alignment_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  AwnUAAlignmentPrivate * priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (object);
  switch (property_id) 
  {
    case PROP_APPLET_MANAGER:
      priv->applet_manager = g_value_get_object (value);
      break;
    case PROP_UA_LIST_ENTRY:
      g_free (priv->ua_list_entry);
      priv->ua_list_entry = g_value_dup_string (value);
      break;
    case PROP_UA_RATIO:
      priv->ua_ratio = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_ua_alignment_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_ua_alignment_parent_class)->dispose (object);
}

static void
awn_ua_alignment_finalize (GObject *object)
{
  AwnUAAlignmentPrivate * priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (object);  
  g_debug ("%s",__func__);
  if (priv->ua_list_entry)
  {
    g_free (priv->ua_list_entry);
  }
  G_OBJECT_CLASS (awn_ua_alignment_parent_class)->finalize (object);
}

static void
awn_ua_alignment_constructed (GObject *object)
{
  AwnUAAlignmentPrivate * priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (object);

  if (G_OBJECT_CLASS (awn_ua_alignment_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_ua_alignment_parent_class)->constructed (object);
  }

  gtk_container_add (GTK_CONTAINER(object),priv->socket);
  awn_ua_alignment_position_change (NULL,NULL,object);  
  priv->notify_offset_id = g_signal_connect (priv->applet_manager,
                                            "notify::offset",
                                            G_CALLBACK(awn_ua_alignment_offset_change),
                                            object);
  priv->notify_position_id = g_signal_connect_after (priv->applet_manager,
                                            "notify::position",
                                            G_CALLBACK(awn_ua_alignment_position_change),
                                            object);
  priv->notify_size_id = g_signal_connect_after (priv->applet_manager,
                                            "notify::size",
                                            G_CALLBACK(awn_ua_alignment_size_change),
                                            object);
  priv->notify_ua_list_id = g_signal_connect_after (priv->applet_manager,
                                           "notify::ua-active-list",
                                           G_CALLBACK(awn_ua_alignment_list_change),
                                           object);
}

static void
awn_ua_alignment_class_init (AwnUAAlignmentClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec   *pspec;

  object_class->get_property = awn_ua_alignment_get_property;
  object_class->set_property = awn_ua_alignment_set_property;
  object_class->dispose = awn_ua_alignment_dispose;
  object_class->finalize = awn_ua_alignment_finalize;
  object_class->constructed = awn_ua_alignment_constructed;  
  
  pspec = g_param_spec_object ("applet-manager",
                               "Awn Applet Manager",
                               "Awn Applet Manager",
                               AWN_TYPE_APPLET_MANAGER,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_APPLET_MANAGER, pspec);   
  
  pspec = g_param_spec_string ("ua-list-entry",
                               "UA List entry",
                               "UA List entry",
                               NULL,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_UA_LIST_ENTRY, pspec);   
  
  pspec = g_param_spec_double ("ua-ratio",
                               "UA Ratio",
                               "UA Ratio",
                               0.0,
                               100.0,
                               1.0,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                               G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class, PROP_UA_RATIO, pspec);   

  g_type_class_add_private (klass, sizeof (AwnUAAlignmentPrivate));  
}

static void
awn_ua_alignment_init (AwnUAAlignment *self)
{
  AwnUAAlignmentPrivate *priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  priv->socket = gtk_socket_new ();
}

GtkWidget*
awn_ua_alignment_new (AwnAppletManager *manager,gchar * ua_list_entry,double ua_ratio)
{
  return g_object_new (AWN_TYPE_UA_ALIGNMENT,
                       "applet-manager",manager,
                       "ua-list-entry",ua_list_entry,
                       "ua-ratio",ua_ratio,
                       NULL);
}

GtkWidget*
awn_ua_alignment_get_socket (AwnUAAlignment *self)
{
  AwnUAAlignmentPrivate *priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  return priv->socket;
}


GdkWindow *
awn_ua_alignment_add_id (AwnUAAlignment *self,GdkNativeWindow native_window)
{
  GdkWindow * plugwin;
  AwnUAAlignmentPrivate *priv;

  priv =  AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  gtk_socket_add_id (GTK_SOCKET(priv->socket), native_window);
  plugwin = gtk_socket_get_plug_window (GTK_SOCKET(priv->socket));
  g_signal_connect (priv->socket,"plug-removed",
                    G_CALLBACK(awn_ua_alignment_plug_removed),self);
  gtk_widget_realize (priv->socket);
  gtk_widget_show_all (priv->socket);
  return plugwin;
}


static gboolean
awn_ua_alignment_plug_removed (GtkWidget * socket,AwnUAAlignment * self)
{

  GSList * search;
  GSList * ua_active_list;
  GSList * orig_active_list;
  GSList * iter;
  DesktopAgnosticConfigClient *client;
  GValueArray *array;
  
  AwnUAAlignmentPrivate *priv = AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  
  g_signal_handler_disconnect (priv->applet_manager,priv->notify_size_id);
  g_signal_handler_disconnect (priv->applet_manager,priv->notify_position_id);
  g_signal_handler_disconnect (priv->applet_manager,priv->notify_offset_id);
  g_signal_handler_disconnect (priv->applet_manager,priv->notify_ua_list_id);

  g_object_get ( priv->applet_manager,
                "ua_active_list",&orig_active_list,
                "client",&client,
                NULL);
  ua_active_list = g_slist_copy (orig_active_list);
  for (iter = ua_active_list;iter;iter = iter->next)
  {
    iter->data = g_strdup(iter->data);
  }
  search = g_slist_find_custom (ua_active_list,priv->ua_list_entry,
                                (GCompareFunc)g_strcmp0);
  if (search)
  {
    ua_active_list = g_slist_delete_link (ua_active_list,search);
  } 

  array = awn_utils_gslist_to_gvaluearray (ua_active_list);

  desktop_agnostic_config_client_set_list (client,
                                           AWN_GROUP_PANEL,
                                           AWN_PANEL_UA_ACTIVE_LIST,
                                           array, NULL);

  g_value_array_free (array);

  g_object_set (priv->applet_manager,
                "ua-active-list",ua_active_list,
                NULL);
  awn_applet_manager_remove_widget(AWN_APPLET_MANAGER(priv->applet_manager), 
                               GTK_WIDGET (self));        
  return FALSE;
}

/*UA*/
gint
awn_ua_alignment_list_cmp (gconstpointer a, gconstpointer b)
{
  const gchar * str1 = a;
  const gchar * str2 = b;
  gchar * search = NULL;
  GStrv tokens = g_strsplit (str1,"::",2);
  g_return_val_if_fail (tokens,-1);
  
  search = g_strstr_len (str2,-1,tokens[0]);
  g_strfreev (tokens);
  
  if (!search)
  {
    return -1;
  };
  return 0;
}

static void
awn_ua_alignment_offset_change(GObject *object,GParamSpec *param_spec,gpointer user_data)
{
  AwnUAAlignment * self = user_data;  
  gint offset;
  gint position;
  AwnUAAlignmentPrivate *priv = AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  
  g_object_get ( priv->applet_manager,
                "offset",&offset,
                "position",&position,
                NULL);
  
  switch (position)
  {
    case GTK_POS_TOP:
      gtk_alignment_set_padding (GTK_ALIGNMENT(self), offset, 0, 0, 0);
      break;
    case GTK_POS_BOTTOM:
      gtk_alignment_set_padding (GTK_ALIGNMENT(self), 0, offset, 0, 0);
      break;
    case GTK_POS_LEFT:
      gtk_alignment_set_padding (GTK_ALIGNMENT(self), 0, 0, offset, 0);
      break;
    case GTK_POS_RIGHT:
      gtk_alignment_set_padding (GTK_ALIGNMENT(self), 0, 0, 0, offset);
      break;
    default:
      g_warning ("%s: recieved invalid position %d",__func__,position);
  }
}

static void
awn_ua_alignment_size_change(GObject *object,GParamSpec *param_spec,gpointer user_data)
{
  AwnUAAlignment * self = user_data;  
  gint offset;
  gint position;
  gint size;
  AwnUAAlignmentPrivate *priv = AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  
  g_object_get ( priv->applet_manager,
                "offset",&offset,
                "position",&position,
                "size",&size,
                NULL);
  GtkRequisition  req;

  req.width = req.height = size;
  switch (position)
  {
    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:      
      req.width = size * priv->ua_ratio;
      req.height = size;
      break;
    case GTK_POS_LEFT:
    case GTK_POS_RIGHT:
      req.width = size;
      req.height = size  * 1.0 / priv->ua_ratio;            
      break;
    default:
      g_warning ("%s: received invalid position %d",__func__,position);
  }
  gtk_widget_set_size_request (GTK_WIDGET(self),req.width,req.height);
}

static void
awn_ua_alignment_position_change(GObject *object,GParamSpec *param_spec,gpointer user_data)
{
  
  AwnUAAlignment * self = user_data;  
  gint position;
  AwnUAAlignmentPrivate *priv = AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  
  g_object_get ( priv->applet_manager,
                "position",&position,
                NULL);
  switch (position)
  {
    case GTK_POS_TOP:
      gtk_alignment_set (GTK_ALIGNMENT(self), 0.0, 0.0, 1.0, 0.5);
      break;
    case GTK_POS_BOTTOM:
      gtk_alignment_set (GTK_ALIGNMENT(self), 0.0, 1.0, 1.0, 0.5);
      break;
    case GTK_POS_LEFT:
      gtk_alignment_set (GTK_ALIGNMENT(self), 0.0, 0.0, 0.5, 1.0);
      break;
    case GTK_POS_RIGHT:
      gtk_alignment_set (GTK_ALIGNMENT(self), 1.0, 0.0, 0.5, 1.0);
      break;
    default:
      g_warning ("%s: received invalid position %d",__func__,position);
  }
  awn_ua_alignment_offset_change (object,param_spec,self);
  awn_ua_alignment_size_change (object,param_spec,self);
}

static void
awn_ua_alignment_list_change(GObject *object,GParamSpec *param_spec,gpointer user_data)
{
  AwnUAAlignment * self = user_data;  
  gint position;
  AwnUAAlignmentPrivate *priv = AWN_UA_ALIGNMENT_GET_PRIVATE (self); 
  GSList * ua_active_list;
  
  g_object_get ( priv->applet_manager,
                "position",&position,
                "ua_active_list",&ua_active_list,
                NULL);

  GSList * search = g_slist_find_custom (ua_active_list,
                                         priv->ua_list_entry,
                                         (GCompareFunc)g_strcmp0);  
  if (search)
  {
    g_debug ("Found... do not need to update %s",priv->ua_list_entry);
  }
  else
  {
    search = g_slist_find_custom (ua_active_list,priv->ua_list_entry,awn_ua_alignment_list_cmp);
    if (search)
    {
      g_debug ("Moving %s to %s",priv->ua_list_entry,(gchar*)search->data);
      GStrv tokens;
      gint pos = -1;
      g_free (priv->ua_list_entry);
      priv->ua_list_entry = g_strdup(search->data);
      tokens = g_strsplit (search->data,"::",2);
      if (tokens && tokens[1])
      {
        pos = atoi (tokens[1]);
      }
      g_strfreev (tokens);
      if (pos != -1)
      {
        awn_applet_manager_add_widget(AWN_APPLET_MANAGER(priv->applet_manager),
                                      GTK_WIDGET (self), 
                                      pos);
      }
    }
    else
    {
      g_debug ("looks like  %s was removed from panel/ua_list",priv->ua_list_entry);
      /*
       aantn as expected this does not kill the screenlet.  What is the best way to
       tell it that we want it to go away?
       */
      awn_applet_manager_remove_widget(AWN_APPLET_MANAGER(priv->applet_manager), 
                                   GTK_WIDGET (self));        
      gtk_widget_destroy (priv->socket);
    } 
  }  
}
