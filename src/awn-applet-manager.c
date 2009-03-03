/*
 *  Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
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
 *
 */

#include "config.h"

#include <libawn/awn-config-bridge.h>

#include "awn-defines.h"
#include "awn-applet-manager.h"

#include "awn-applet-proxy.h"

G_DEFINE_TYPE (AwnAppletManager, awn_applet_manager, GTK_TYPE_BOX) 

#define AWN_APPLET_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_APPLET_MANAGER, AwnAppletManagerPrivate))

struct _AwnAppletManagerPrivate
{
  AwnConfigClient *client;

  AwnOrientation   orient;
  gint             offset;
  gint             size;
  GSList           *applet_list;

  GHashTable      *applets;
  GQuark           touch_quark;


  /* Current box class */
  GtkWidgetClass  *klass;
};

enum 
{
  PROP_0,

  PROP_CLIENT,
  PROP_ORIENT,
  PROP_OFFSET,
  PROP_SIZE,
  PROP_APPLET_LIST
};


/* 
 * FORWARDS
 */
static void awn_applet_manager_set_size   (AwnAppletManager *manager,
                                           gint              size);
static void awn_applet_manager_set_orient (AwnAppletManager *manager, 
                                           gint              orient);
static void awn_applet_manager_set_offset (AwnAppletManager *manager,
                                           gint              offset);
static void free_list                     (GSList *list);

/*
 * GOBJECT CODE 
 */
static void
awn_applet_manager_constructed (GObject *object)
{
  AwnAppletManager        *manager;
  AwnAppletManagerPrivate *priv;
  AwnConfigBridge         *bridge;
  
  priv = AWN_APPLET_MANAGER_GET_PRIVATE (object);
  manager = AWN_APPLET_MANAGER (object);

  /* Hook everything up the config client */
  bridge = awn_config_bridge_get_default ();

  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_ORIENT,
                          object, "orient");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_SIZE,
                          object, "size");
  awn_config_bridge_bind (bridge, priv->client,
                          AWN_GROUP_PANEL, AWN_PANEL_OFFSET,
                          object, "offset");
  awn_config_bridge_bind_list (bridge, priv->client,
                               AWN_GROUP_PANEL, AWN_PANEL_APPLET_LIST,
                               AWN_CONFIG_CLIENT_LIST_TYPE_STRING,
                               object, "applet_list");


}

static void
awn_applet_manager_size_request (GtkWidget *widget, GtkRequisition *requisition){
  AwnAppletManagerPrivate *priv = AWN_APPLET_MANAGER (widget)->priv;
  
  priv->klass->size_request (widget, requisition);
}

static void
awn_applet_manager_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  AwnAppletManagerPrivate *priv = AWN_APPLET_MANAGER (widget)->priv;
  
  priv->klass->size_allocate (widget, allocation);
}

static void
awn_applet_manager_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  AwnAppletManagerPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (object));
  priv = AWN_APPLET_MANAGER (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      g_value_set_pointer (value, priv->client);
      break;
    case PROP_ORIENT:
      g_value_set_int (value, priv->orient);
      break;
    case PROP_OFFSET:
      g_value_set_int (value, priv->offset);
      break;
    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;

    case PROP_APPLET_LIST:
      g_value_set_pointer (value, priv->applet_list);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_applet_manager_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  AwnAppletManager *manager = AWN_APPLET_MANAGER (object);
  AwnAppletManagerPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (object));
  priv = AWN_APPLET_MANAGER (object)->priv;

  switch (prop_id)
  {
    case PROP_CLIENT:
      priv->client =  g_value_get_pointer (value);
      break;
    case PROP_ORIENT:
      awn_applet_manager_set_orient (manager, g_value_get_int (value));
      break;
    case PROP_OFFSET:
      awn_applet_manager_set_offset (manager, g_value_get_int (value));
      break;
    case PROP_SIZE:
      awn_applet_manager_set_size (manager, g_value_get_int (value));
      break;
    case PROP_APPLET_LIST:
      free_list (priv->applet_list);
      priv->applet_list = g_value_get_pointer (value);
      awn_applet_manager_refresh_applets (manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
awn_applet_manager_dispose (GObject *object)
{ 
  AwnAppletManagerPrivate *priv = AWN_APPLET_MANAGER_GET_PRIVATE (object);

  g_hash_table_destroy (priv->applets);

  G_OBJECT_CLASS (awn_applet_manager_parent_class)->dispose (object);
}

static void
awn_applet_manager_class_init (AwnAppletManagerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
  
  obj_class->constructed   = awn_applet_manager_constructed;
  obj_class->dispose       = awn_applet_manager_dispose;
  obj_class->get_property  = awn_applet_manager_get_property;
  obj_class->set_property  = awn_applet_manager_set_property;

  wid_class->size_request  = awn_applet_manager_size_request;
  wid_class->size_allocate = awn_applet_manager_size_allocate;
    
  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_pointer ("client",
                          "Client",
                          "The AwnConfigClient",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_ORIENT,
    g_param_spec_int ("orient",
                      "Orient",
                      "The orientation of the panel",
                      0, 3, AWN_ORIENTATION_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_OFFSET,
    g_param_spec_int ("offset",
                      "Offset",
                      "The icon offset of the panel",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "Size",
                      "The size of the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_APPLET_LIST,
    g_param_spec_pointer ("applet_list",
                          "Applet List",
                          "The list of applets for this panel",
                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
 
  g_type_class_add_private (obj_class, sizeof (AwnAppletManagerPrivate));
}

static void
awn_applet_manager_init (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv;

  priv = manager->priv = AWN_APPLET_MANAGER_GET_PRIVATE (manager);

  priv->touch_quark = g_quark_from_string ("applets-touch-quark");
  priv->applets = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, NULL);

  gtk_widget_show_all (GTK_WIDGET (manager));
}

GtkWidget *
awn_applet_manager_new_from_config (AwnConfigClient *client)
{
  GtkWidget *manager;
  
  manager = g_object_new (AWN_TYPE_APPLET_MANAGER,
                         "homogeneous", FALSE,
                         "spacing", 0,
                         "client", client,
                         NULL);
  return manager;
}

/*
 * PROPERTY SETTERS
 */

static void
awn_manager_set_applets_size (gpointer key,
                              GtkWidget *applet,
                              AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet))
  {
    g_object_set (applet, "size", manager->priv->size, NULL);
  }
}

static void
awn_applet_manager_set_size (AwnAppletManager *manager,
                             gint              size)
{
  AwnAppletManagerPrivate *priv = manager->priv;

  priv->size = size;

  /* update size on all running applets (if they'd crash) */
  g_hash_table_foreach(priv->applets,
                       (GHFunc)awn_manager_set_applets_size, manager);
}

static void
awn_manager_set_applets_offset (gpointer key,
                                GtkWidget *applet,
                                AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet))
  {
    g_object_set (applet, "offset", manager->priv->offset, NULL);
  }
}

static void
awn_applet_manager_set_offset (AwnAppletManager *manager,
                               gint              offset)
{
  AwnAppletManagerPrivate *priv = manager->priv;

  priv->offset = offset;

  /* update size on all running applets (if they'd crash) */
  g_hash_table_foreach(priv->applets,
                       (GHFunc)awn_manager_set_applets_offset, manager);
}

static void
awn_manager_set_applets_orient (gpointer key,
                                GtkWidget *applet,
                                AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet))
  {
    g_object_set (applet, "orient", manager->priv->orient, NULL);
  }
}

/*
 * Update the box class
 */
static void 
awn_applet_manager_set_orient (AwnAppletManager *manager, 
                               gint              orient)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  
  priv->orient = orient;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      priv->klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_HBOX));
      break;
    
    case AWN_ORIENTATION_RIGHT:
    case AWN_ORIENTATION_LEFT:
      priv->klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_VBOX));
      break;

    default:
      g_assert_not_reached ();
      priv->klass = NULL;
      break;
  }

  /* update orientation on all running applets (if they'd crash) */
  g_hash_table_foreach(priv->applets,
                       (GHFunc)awn_manager_set_applets_orient, manager);
}

/*
 * UTIL
 */
static void
free_list (GSList *list)
{
  GSList *l;

  for (l = list; l; l = l->next)
  {
    g_free (l->data);
  }
  g_slist_free (list);
}

/*
 * APPLET CONTROL
 */
static GtkWidget *
create_applet (AwnAppletManager *manager, 
               const gchar      *path,
               const gchar      *uid)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GtkWidget               *applet;
  GtkWidget               *notifier;

  /*FIXME: Exception cases, i.e. separators */
  
  applet = awn_applet_proxy_new (path, uid, priv->orient,
                                 priv->offset, priv->size);
  notifier = awn_applet_proxy_get_throbber(AWN_APPLET_PROXY(applet));
  gtk_box_pack_start (GTK_BOX (manager), applet, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (manager), notifier, FALSE, FALSE, 0);
  gtk_widget_show(notifier);

  g_object_set_qdata (G_OBJECT (applet), 
                      priv->touch_quark, GINT_TO_POINTER (0));
  g_hash_table_insert (priv->applets, g_strdup (uid), applet);

  awn_applet_proxy_execute (AWN_APPLET_PROXY (applet));

  return applet;
}

static void
zero_applets (gpointer key, GtkWidget *applet, AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet))
  {
    g_object_set_qdata (G_OBJECT (applet), 
                        manager->priv->touch_quark, GINT_TO_POINTER (0));
  }
}

static void
delete_applets (gpointer key, GtkWidget *applet, AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  const gchar             *uid;
  gint                     touched;
  
  if (!G_IS_OBJECT (applet))
    return;
  
  touched = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (applet),
                                                 priv->touch_quark));

  if (!touched) 
  {
    g_object_get (applet, "uid", &uid, NULL);
    /* FIXME: Let the applet know it's about to be deleted ? */
    gtk_widget_destroy (applet);
  }
}

void    
awn_applet_manager_refresh_applets  (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GSList                   *a;
  gint                     i = 0;

  if (!GTK_WIDGET_REALIZED (manager))
    return;

  if (priv->applet_list == NULL)
  {
    g_debug ("No applets");
    return;
  }

  /* Set each of the current apps as "untouched" */
  g_hash_table_foreach (priv->applets, (GHFunc)zero_applets, manager);

  /* Go through the list of applets. Re-order those that are already active, 
   * and create those that are not
   */
  for (a = priv->applet_list; a; a = a->next)
  {
    GtkWidget *applet = NULL;
    gchar     **tokens;

    /* Get the two tokens from the saved string, where:
     * tokens[0] == path to applet desktop file &
     * tokens[1] == uid of applet
     */
    tokens = g_strsplit (a->data, "::", 2);

    if (tokens == NULL || g_strv_length (tokens) != 2)
    {
      g_warning ("Bad applet key: %s", (gchar*)a->data);
      continue;
    }

    /* See if the applet already exists */
    applet = g_hash_table_lookup (priv->applets, tokens[1]);

    /* If not, create it */
    if (!AWN_IS_APPLET_PROXY (applet))
    {
      applet = create_applet (manager, tokens[0], tokens[1]);
      if (!applet)
      {
        g_strfreev (tokens);
        continue;
      }
    }

    /* Order the applet correctly */
    gtk_box_reorder_child (GTK_BOX (manager), applet, i++);
    gtk_box_reorder_child (GTK_BOX (manager),
               awn_applet_proxy_get_throbber(AWN_APPLET_PROXY(applet)), i++);
    
    /* Make sure we don't kill it during clean up */
    g_object_set_qdata (G_OBJECT (applet), 
                        priv->touch_quark, GINT_TO_POINTER (1));
    
    g_strfreev (tokens);
  }

  /* Delete applets that have been removed from the list */
  g_hash_table_foreach (priv->applets, (GHFunc)delete_applets, manager);
}

void 
awn_applet_manager_set_real_size (AwnAppletManager *manager,
                                  gint              width,
                                  gint              height)
{
  GList *children, *c;

  children = gtk_container_get_children (GTK_CONTAINER (manager));
  for (c = children; c; c = c->next)
  {
    /*GtkWidget *widget = c->data;
    gtk_widget_set_size_request (widget, width, height);*/
  }
  g_list_free (children);
}

void 
awn_applet_manager_handle_applet_size_request (AwnAppletManager *manager,
                                               gint              panel_size,
                                               AwnOrientation    orient,
                                               const gchar      *uid,
                                               gint              width, 
                                               gint              height)
{
  AwnAppletManagerPrivate *priv;
  GtkWidget *applet;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));
  priv = manager->priv;

  /* make sure noone calls this and afterwards FIXME: remove */
  g_assert_not_reached();

  /* See if the applet already exists */
  applet = g_hash_table_lookup (priv->applets, uid);

  if (GTK_IS_WIDGET (applet))
  {
    if (orient == AWN_ORIENTATION_BOTTOM || orient == AWN_ORIENTATION_TOP)
      gtk_widget_set_size_request (GTK_WIDGET (applet), width, panel_size);
    else
      gtk_widget_set_size_request (GTK_WIDGET (applet), panel_size, height);
  }
  else
  {
    g_warning ("Unable to find %s for size-request", uid);
  }
}


