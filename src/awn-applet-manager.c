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

#include <libawn/libawn.h>
#include <libawn/awn-utils.h>
#include "libawn/gseal-transition.h"

#include "awn-defines.h"
#include "awn-applet-manager.h"
#include "awn-ua-alignment.h"
#include "awn-applet-proxy.h"
#include "awn-throbber.h"
#include "awn-separator.h"
#include "xutils.h"

#define MAX_UA_LIST_ENTRIES 50

G_DEFINE_TYPE (AwnAppletManager, awn_applet_manager, AWN_TYPE_BOX) 

#define AWN_APPLET_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_APPLET_MANAGER, AwnAppletManagerPrivate))

struct _AwnAppletManagerPrivate
{
  DesktopAgnosticConfigClient *client;

  GtkPositionType  position;
  gint             offset;
  gint             size;
  
  GSList          *applet_list;

  /*ua_list does not serve the same purpose as the applet_list
   It's a list of unique UA names plus their position in the panel
   */ 
  GSList          *ua_list;
  GSList          *ua_active_list;
  
  gboolean         docklet_mode;
  GtkWidget       *docklet_widget;

  gboolean         expands;
  gint             expander_count;

  GHashTable      *applets;
  GHashTable      *extra_widgets;
  GQuark           touch_quark;
  GQuark           visibility_quark;
  GQuark           shape_mask_quark;
};

enum 
{
  PROP_0,

  PROP_CLIENT,
  PROP_POSITION,
  PROP_OFFSET,
  PROP_SIZE,
  PROP_APPLET_LIST,
  PROP_UA_LIST,
  PROP_UA_ACTIVE_LIST,
  PROP_EXPANDS
};

enum
{
  APPLET_EMBEDDED,
  APPLET_REMOVED,
  SHAPE_MASK_CHANGED,

  LAST_SIGNAL
};
static guint _applet_manager_signals[LAST_SIGNAL] = { 0 };

/* 
 * FORWARDS
 */
static void awn_applet_manager_set_size     (AwnAppletManager *manager,
                                             gint              size);
static void awn_applet_manager_set_pos_type (AwnAppletManager *manager, 
                                             GtkPositionType   position);
static void awn_applet_manager_set_offset   (AwnAppletManager *manager,
                                             gint              offset);
static void on_icon_size_alloc              (GtkWidget *widget,
                                             GtkAllocation *alloc,
                                             AwnAppletManager *manager);
static void free_list                       (GSList **list);

/*
 * GOBJECT CODE 
 */
static void
awn_applet_manager_constructed (GObject *object)
{
  AwnAppletManager        *manager;
  AwnAppletManagerPrivate *priv;
  GValueArray *empty_array;
  
  priv = AWN_APPLET_MANAGER_GET_PRIVATE (object);
  manager = AWN_APPLET_MANAGER (object);

  /* Hook everything up to the config client */

  // FIXME: at least "size" should be set by AwnPanel, don't read it from config
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_POSITION,
                                       object, "position", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_SIZE,
                                       object, "size", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_OFFSET,
                                       object, "offset", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_APPLET_LIST,
                                       object, "applet_list", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_UA_LIST,
                                       object, "ua_list", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  desktop_agnostic_config_client_bind (priv->client,
                                       AWN_GROUP_PANEL, AWN_PANEL_UA_ACTIVE_LIST,
                                       object, "ua_active_list", TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       NULL);
  /*
  ua_active_list should be empty when awn starts...
   */
  empty_array = g_value_array_new (0);
  desktop_agnostic_config_client_set_list (priv->client,
                                           AWN_GROUP_PANEL, AWN_PANEL_UA_ACTIVE_LIST,
                                           empty_array, NULL);
  g_value_array_free (empty_array);
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
      g_value_set_object (value, priv->client);
      break;
    case PROP_POSITION:
      g_value_set_int (value, priv->position);
      break;
    case PROP_OFFSET:
      g_value_set_int (value, priv->offset);
      break;
    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;
    case PROP_APPLET_LIST:
      g_value_take_boxed (value, awn_utils_gslist_to_gvaluearray (priv->applet_list));
      break;
    case PROP_UA_LIST:
      g_value_take_boxed (value, awn_utils_gslist_to_gvaluearray (priv->ua_list));
      break;
    case PROP_UA_ACTIVE_LIST:
      g_value_take_boxed (value, awn_utils_gslist_to_gvaluearray (priv->ua_active_list));
      break;
    case PROP_EXPANDS:
      g_value_set_boolean (value, awn_applet_manager_get_expands (AWN_APPLET_MANAGER (object)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

/**
 * Sets a an GSList<string> property from a GValueArray.
 */
static void
set_list_property (const GValue *value, GSList **list)
{
  GValueArray *array;

  free_list (list);
  array = (GValueArray*)g_value_get_boxed (value);
  if (array)
  {
    for (guint i = 0; i < array->n_values; i++)
    {
      GValue *val = g_value_array_get_nth (array, i);
      *list = g_slist_append (*list, g_value_dup_string (val));
    }
    // don't free array, it will be done automatically
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
      priv->client =  g_value_get_object (value);
      break;
    case PROP_POSITION:
      awn_applet_manager_set_pos_type (manager, g_value_get_int (value));
      break;
    case PROP_OFFSET:
      awn_applet_manager_set_offset (manager, g_value_get_int (value));
      break;
    case PROP_SIZE:
      awn_applet_manager_set_size (manager, g_value_get_int (value));
      break;
    case PROP_APPLET_LIST:
      set_list_property (value, &priv->applet_list);
      awn_applet_manager_refresh_applets (manager);
      break;
    case PROP_UA_LIST:
      set_list_property (value, &priv->ua_list);
      awn_applet_manager_refresh_applets (manager);
      break;
    case PROP_UA_ACTIVE_LIST:
      set_list_property (value, &priv->ua_active_list);
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

  if (priv->applets)
  {
    g_hash_table_destroy (priv->applets);
    priv->applets = NULL;
  }

  if (priv->extra_widgets)
  {
    g_hash_table_destroy (priv->extra_widgets);
    priv->extra_widgets = NULL;
  }

  desktop_agnostic_config_client_unbind_all_for_object (priv->client,
                                                        object, NULL);

  G_OBJECT_CLASS (awn_applet_manager_parent_class)->dispose (object);
}

static void
awn_applet_manager_class_init (AwnAppletManagerClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  
  obj_class->constructed   = awn_applet_manager_constructed;
  obj_class->dispose       = awn_applet_manager_dispose;
  obj_class->get_property  = awn_applet_manager_get_property;
  obj_class->set_property  = awn_applet_manager_set_property;

  /* Add properties to the class */
  g_object_class_install_property (obj_class,
    PROP_CLIENT,
    g_param_spec_object ("client",
                         "Client",
                         "The configuration client",
                         DESKTOP_AGNOSTIC_CONFIG_TYPE_CLIENT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_POSITION,
    g_param_spec_int ("position",
                      "Position",
                      "The position of the panel",
                      0, 3, GTK_POS_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_OFFSET,
    g_param_spec_int ("offset",
                      "Offset",
                      "The icon offset of the panel",
                      0, G_MAXINT, 0,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "Size",
                      "The size of the panel",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_APPLET_LIST,
    g_param_spec_boxed ("applet-list",
                        "Applet List",
                        "The list of applets for this panel",
                        G_TYPE_VALUE_ARRAY,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_UA_LIST,
    g_param_spec_boxed ("ua-list",
                        "UA List",
                        "The remembered screenlet positions for this panel",
                        G_TYPE_VALUE_ARRAY,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_UA_ACTIVE_LIST,
    g_param_spec_boxed ("ua-active-list",
                        "UA Active List",
                        "The list of active screenlets for this panel",
                        G_TYPE_VALUE_ARRAY,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_EXPANDS,
    g_param_spec_boolean ("expands",
                          "Expands",
                          "Determines whether there's an expander",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /* Class signals */
  _applet_manager_signals[APPLET_EMBEDDED] =
    g_signal_new("applet-embedded",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletManagerClass, applet_embedded),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
 
  _applet_manager_signals[APPLET_REMOVED] =
    g_signal_new("applet-removed",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletManagerClass, applet_removed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
 
  _applet_manager_signals[SHAPE_MASK_CHANGED] =
    g_signal_new("shape-mask-changed",
                 G_OBJECT_CLASS_TYPE(obj_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET(AwnAppletManagerClass, shape_mask_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);
 
  g_type_class_add_private (obj_class, sizeof (AwnAppletManagerPrivate));
}

static void
awn_applet_manager_init (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv;

  priv = manager->priv = AWN_APPLET_MANAGER_GET_PRIVATE (manager);

  priv->touch_quark = g_quark_from_string ("applets-touch-quark");
  priv->visibility_quark = g_quark_from_string ("visibility-quark");
  priv->shape_mask_quark = g_quark_from_string ("shape-mask-quark");
  priv->applets = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, NULL);
  priv->extra_widgets = g_hash_table_new (g_direct_hash, g_direct_equal);

  gtk_widget_show_all (GTK_WIDGET (manager));
}

GtkWidget *
awn_applet_manager_new_from_config (DesktopAgnosticConfigClient *client)
{
  GtkWidget *manager;
  
  manager = g_object_new (AWN_TYPE_APPLET_MANAGER,
                         "homogeneous", FALSE,
                         "spacing", 0,
                         "client", client,
                         NULL);
  return manager;
}

gchar*
awn_applet_manager_generate_uid (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv;
  gchar *new_uid = NULL;

  g_return_val_if_fail (AWN_IS_APPLET_MANAGER (manager), NULL);

  priv = manager->priv;

  GTimeVal time_val;
  g_get_current_time (&time_val);

  new_uid = g_strdup_printf ("%ld", time_val.tv_sec);
  while (g_hash_table_lookup (priv->applets, new_uid) != NULL)
  {
    gchar *non_unique_uid = new_uid;
    new_uid = g_strdup_printf ("%s%d", non_unique_uid, rand() % 10);
    g_free (non_unique_uid);
  }

  return new_uid;
}

void
awn_applet_manager_set_applet_flags (AwnAppletManager *manager,
                                     const gchar *uid,
                                     AwnAppletFlags flags)
{
  AwnAppletProxy *applet;
  AwnAppletManagerPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));

  priv = manager->priv;

  applet = g_hash_table_lookup (priv->applets, uid);
  if (applet)
  {
    if ((flags & AWN_APPLET_IS_EXPANDER) && AWN_IS_APPLET_PROXY (applet))
    {
      // dummy widget that will expand
      GtkWidget *image = gtk_image_new ();
      gtk_widget_show (image);
      gtk_box_pack_start (GTK_BOX (manager), image, TRUE, TRUE, 0);

      priv->expander_count++;

      g_hash_table_replace (priv->applets, g_strdup (uid), image);
      gtk_widget_destroy (GTK_WIDGET (applet));
      applet = NULL;

      awn_applet_manager_refresh_applets (manager);
    }
    if ((flags & AWN_APPLET_IS_SEPARATOR) && AWN_IS_APPLET_PROXY (applet))
    {
      GtkWidget *image = awn_separator_new_from_config_with_values (
          priv->client, priv->position, priv->size, priv->offset);
      gtk_widget_show (image);
      gtk_box_pack_start (GTK_BOX (manager), image, FALSE, TRUE, 0);

      g_hash_table_replace (priv->applets, g_strdup (uid), image);
      gtk_widget_destroy (GTK_WIDGET (applet));
      applet = NULL;

      awn_applet_manager_refresh_applets (manager);
    }

    if (applet)
    {
      if (flags & AWN_APPLET_HAS_SHAPE_MASK)
      {
        GdkWindow *win = GTK_WIDGET (applet)->window;
        g_object_set_qdata_full (G_OBJECT (applet), priv->shape_mask_quark,
                                 xutils_get_input_shape (win),
                                 (GDestroyNotify) gdk_region_destroy);
        g_signal_emit (manager, _applet_manager_signals[SHAPE_MASK_CHANGED], 0);
      }
      else
      {
        gpointer region = g_object_get_qdata (G_OBJECT (applet),
                                              priv->shape_mask_quark);
        if (region)
        {
          g_object_set_qdata (G_OBJECT (applet), priv->shape_mask_quark, NULL);
          g_signal_emit (manager, _applet_manager_signals[SHAPE_MASK_CHANGED],
                         0);
        }
      }
    }
  }
  else
  {
    g_warning ("Cannot find applet with UID: %s", uid);
  }
}

gboolean
awn_applet_manager_get_expands (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv;
  g_return_val_if_fail (AWN_IS_APPLET_MANAGER (manager), FALSE);

  priv = manager->priv;

  if (priv->docklet_mode)
  {
    gint w = -1, h = -1;
    // docklet will have size request set if the request specified
    //   expand == false
    gtk_widget_get_size_request (priv->docklet_widget, &w, &h);

    return (w == -1 && h == -1);
  }

  return manager->priv->expands;
}

/*
 * PROPERTY SETTERS
 */

static void
awn_manager_set_applets_size (gpointer key,
                              GtkWidget *applet,
                              AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet) &&
        g_object_class_find_property (G_OBJECT_GET_CLASS (applet), "size"))
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
  if (G_IS_OBJECT (applet) &&
        g_object_class_find_property (G_OBJECT_GET_CLASS (applet), "offset"))
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
awn_manager_set_applets_position (gpointer key,
                                GtkWidget *applet,
                                AwnAppletManager *manager)
{
  if (G_IS_OBJECT (applet) &&
        g_object_class_find_property (G_OBJECT_GET_CLASS (applet), "position"))
  {
    g_object_set (applet, "position", manager->priv->position, NULL);
  }
}

/*
 * Update the box class
 */
static void 
awn_applet_manager_set_pos_type (AwnAppletManager *manager, 
                                 GtkPositionType   position)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  
  priv->position = position;

  awn_box_set_orientation_from_pos_type (AWN_BOX (manager), position);

  /* update position on all running applets (if they'd crash) */
  g_hash_table_foreach(priv->applets,
                       (GHFunc)awn_manager_set_applets_position, manager);
}

/*
 * UTIL
 */
static void
free_list (GSList **list)
{
  GSList *l;

  for (l = *list; l; l = l->next)
  {
    g_free (l->data);
  }
  if (*list)
  {
    g_slist_free (*list);
  }
  *list = NULL;
}

/*
 * APPLET CONTROL
 */
static void
_applet_plug_added (AwnAppletManager *manager, GtkWidget *applet)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));

  if (AWN_IS_APPLET_PROXY (applet))
  {
    AwnAppletProxy *proxy = AWN_APPLET_PROXY (applet);
    if (manager->priv->docklet_mode == FALSE)
    {
      gtk_widget_show (applet);
    }
    gtk_widget_hide (awn_applet_proxy_get_throbber (proxy));
  }

  g_signal_emit (manager, _applet_manager_signals[APPLET_EMBEDDED], 0, applet);
}

static void
_applet_crashed (AwnAppletManager *manager, AwnAppletProxy *proxy)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));

  gtk_widget_hide (GTK_WIDGET (proxy));
  if (manager->priv->docklet_mode == FALSE)
  {
    gtk_widget_show (awn_applet_proxy_get_throbber (proxy));
  }
}

static GtkWidget *
create_applet (AwnAppletManager *manager, 
               const gchar      *path,
               const gchar      *uid)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GtkWidget               *applet;
  GtkWidget               *widget;
  gboolean                 expand = FALSE;
  gboolean                 fill = FALSE;

  /* Exception cases, i.e. separators */
  if (g_str_has_suffix (path, "/separator.desktop"))
  {
    widget = applet = awn_separator_new_from_config_with_values (priv->client,
        priv->position, priv->size, priv->offset);
    g_signal_connect (widget, "size-allocate", 
                      G_CALLBACK (on_icon_size_alloc), manager);
    fill = TRUE;
  }
  else if (g_str_has_suffix (path, "/expander.desktop"))
  {
    widget = applet = gtk_image_new ();

    priv->expander_count++;

    expand = TRUE;
    fill = TRUE;
  }
  else
  {
    // standard applet
    applet = awn_applet_proxy_new (path, uid, priv->position,
                                   priv->offset, priv->size);
    g_signal_connect_swapped (applet, "plug-added",
                              G_CALLBACK (_applet_plug_added), manager);
    g_signal_connect_swapped (applet, "applet-crashed",
                              G_CALLBACK (_applet_crashed), manager);
    widget = awn_applet_proxy_get_throbber (AWN_APPLET_PROXY (applet));
    g_signal_connect (widget, "size-allocate", 
                      G_CALLBACK (on_icon_size_alloc), manager);

    gtk_box_pack_start (GTK_BOX (manager), applet, FALSE, FALSE, 0);

    awn_applet_proxy_schedule_execute (AWN_APPLET_PROXY (applet));
  }

  gtk_box_pack_start (GTK_BOX (manager), widget, expand, fill, 0);

  if (priv->docklet_mode == FALSE)
  {
    gtk_widget_show (widget);
  }
  else
  {
    // set as visible when show_applets will be called
    g_object_set_qdata (G_OBJECT (widget),
                        priv->visibility_quark, GINT_TO_POINTER (1));
  }

  g_object_set_qdata (G_OBJECT (applet),
                      priv->touch_quark, GINT_TO_POINTER (0));
  g_hash_table_insert (priv->applets, g_strdup (uid), applet);

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

static gboolean
delete_applets (gpointer key, GtkWidget *applet, AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  const gchar             *uid;
  gint                     touched;
  
  if (!G_IS_OBJECT (applet))
    return TRUE;
  
  touched = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (applet),
                                                 priv->touch_quark));

  if (!touched)
  {
    if (AWN_IS_APPLET_PROXY (applet))
    {
      g_object_get (applet, "uid", &uid, NULL);
      g_signal_emit (manager, _applet_manager_signals[APPLET_REMOVED],
                     0, applet);
    }
    else if (GTK_IS_IMAGE (applet) && !AWN_IS_SEPARATOR (applet)) // expander
    {
      priv->expander_count--;
      if (priv->expander_count == 0 && priv->expands)
      {
        priv->expands = FALSE;
        g_object_notify (G_OBJECT (manager), "expands");
      }
    }
    gtk_widget_destroy (applet);

    return TRUE; /* remove from hash table */
  }

  return FALSE;
}

void    
awn_applet_manager_refresh_applets  (AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv = manager->priv;
  GSList                  *a;
  GList                   *w;
  gint                     i = 0;
  gint                     applet_num = 0;

  if (!gtk_widget_get_realized (GTK_WIDGET (manager)))
    return;

  if (priv->applet_list == NULL)
  {
    g_debug ("No applets");
    g_hash_table_foreach (priv->applets, (GHFunc)zero_applets, manager);
    g_hash_table_foreach_remove (priv->applets, (GHRFunc)delete_applets, 
                                 manager);
    return;
  }

  guint applet_count = g_slist_length (priv->applet_list);
  gint back_pos = (gint)applet_count;
  back_pos = -back_pos;

  /* Set each of the current apps as "untouched" */
  g_hash_table_foreach (priv->applets, (GHFunc)zero_applets, manager);

  GList *widgets = g_hash_table_get_keys (priv->extra_widgets);
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
    if (applet == NULL)
    {
      applet = create_applet (manager, tokens[0], tokens[1]);
      if (!applet)
      {
        g_strfreev (tokens);
        continue;
      }
    }

    /* insert extra widgets on correct position */
    w = widgets;
    while (w)
    {
      gint pos = GPOINTER_TO_INT (g_hash_table_lookup (priv->extra_widgets,
                                                       w->data));

      if (pos == applet_num || pos == back_pos)
      {
        gtk_box_reorder_child (GTK_BOX (manager), w->data, i++);
        /* widget placed, remove from list */
        widgets = g_list_remove (widgets, w->data);
        w = widgets;
        continue;
      }

      w = w->next;
    }
    applet_num++; back_pos++;

    /* Order the applet correctly */
    gtk_box_reorder_child (GTK_BOX (manager), applet, i++);
    if (AWN_IS_APPLET_PROXY (applet))
    {
      gtk_box_reorder_child (GTK_BOX (manager),
               awn_applet_proxy_get_throbber (AWN_APPLET_PROXY (applet)), i++);
    }
    
    /* Make sure we don't kill it during clean up */
    g_object_set_qdata (G_OBJECT (applet), 
                        priv->touch_quark, GINT_TO_POINTER (1));

    g_strfreev (tokens);
  }

  g_list_free (widgets);

  /* Delete applets that have been removed from the list */
  g_hash_table_foreach_remove (priv->applets, (GHRFunc)delete_applets, manager);

  if (!priv->expands && priv->expander_count > 0)
  {
    priv->expands = TRUE;
    g_object_notify (G_OBJECT (manager), "expands");
  }
}

void
awn_applet_manager_add_widget (AwnAppletManager *manager,
                               GtkWidget *widget, gint pos)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));
  gpointer key, val;
  AwnAppletManagerPrivate *priv = manager->priv;

  if (!g_hash_table_lookup_extended (priv->extra_widgets, widget, &key, &val))
  {
    gtk_box_pack_start (GTK_BOX (manager), widget, FALSE, FALSE, 0);
    /* caller is supposed to call gtk_widget_show! */
  }
  g_hash_table_replace (priv->extra_widgets, widget, GINT_TO_POINTER (pos));

  awn_applet_manager_refresh_applets (manager);
}

void
awn_applet_manager_remove_widget (AwnAppletManager *manager, GtkWidget *widget)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));
  AwnAppletManagerPrivate *priv = manager->priv;

  if (g_hash_table_remove (priv->extra_widgets, widget))
  {
    gtk_container_remove (GTK_CONTAINER (manager), widget);
  }
}

static void
on_icon_size_alloc (GtkWidget *widget, GtkAllocation *alloc,
                    AwnAppletManager *manager)
{
  AwnAppletManagerPrivate *priv;
  GtkAllocation manager_alloc;
  AwnPathType path_type;
  gfloat offset_modifier;

  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));

  priv = manager->priv;

  if (!AWN_IS_SEPARATOR (widget) && !AWN_IS_ICON (widget)) return;

  gtk_widget_get_allocation (GTK_WIDGET (manager), &manager_alloc);
  g_object_get (gtk_widget_get_toplevel (GTK_WIDGET (manager)),
                "path-type", &path_type, "offset-modifier", &offset_modifier,
                NULL);

  // get curve offset
  gfloat temp = awn_utils_get_offset_modifier_by_path_type (path_type,
             priv->position, priv->offset, offset_modifier,
             alloc->x + alloc->width / 2 - manager_alloc.x,
             alloc->y + alloc->height / 2 - manager_alloc.y,
             manager_alloc.width,
             manager_alloc.height);
  gint offset = round (temp);

  if (AWN_IS_ICON (widget))
  {
    awn_icon_set_offset (AWN_ICON (widget), offset);
  }
  else if (AWN_IS_SEPARATOR (widget))
  {
    g_object_set (widget, "offset", offset, NULL);
  }
}

/*DBUS*/
/*
  Description of this dbus interface.
 
 	@action(IFACE)
	def add_applet (self, id, plug_id, width, height, size_type):
		"""
		Add an applet.
		
		id: A unique string used to identify the applet.
		plug_id: The applet's gtk.Plug's xid.
		width: A recommended width. This will be interpreted according to size_type.
		height A recommended height. This will be interpreted according to size_type.
		size_type: Determines the meaning of width and height.
			May be one of the following values:
			"scalable"- The applet may be resized as long as the width/height ratio is kept.
			"static"- The applet should be displayed at exactly the size requested.
			"static-width"-	The applet's width should remain static, and the server may change the height.
			"static-height"- The applet's height should remain static, and the server may change the width.
			"dynamic"- The applet may be resized to any size.
		desktop_path: Path to the desktop file.
		"""
		# NOTE: Melange currently ignores the size_type parameter.
		container = ToplevelContainer(plug_id, id, self, width, height,
			size_type, backend=self.backend)
		self.containers.append(container)
*/
gboolean
awn_ua_add_applet ( AwnAppletManager *manager,
                    gchar *name, glong xid,
                    gint width, gint height,
                    gchar *size_type,
                    GError **error)
{
  g_return_val_if_fail (AWN_IS_APPLET_MANAGER (manager),FALSE);
  g_return_val_if_fail ( (g_strcmp0(size_type,"scalable")==0 ) || 
                     (g_strcmp0(size_type,"dynamic")==0 ), FALSE );
  
  GdkWindow* plugwin; 
  AwnAppletManagerPrivate *priv = manager->priv;  
  gint pos = g_slist_length (priv->applet_list);  
  GdkNativeWindow native_window = (GdkNativeWindow) xid;
  gchar * tmp = g_strdup_printf ("%s::%d",name,pos);
  gchar * ua_list_entry = NULL;
  GtkWidget  *ua_alignment;
  double ua_ratio;  
  GValueArray *array;

  /*
   Is there an entry in ua_list for this particular screenlet instance(name).
   The comparision function used ignores the position.
   */
  GSList * search = g_slist_find_custom (priv->ua_list,tmp,awn_ua_alignment_list_cmp);
  if (search)
  {
    /*     There's already an entry in ua_list so use that.     */
    GStrv tokens;
    ua_list_entry = g_strdup (search->data) ;
    g_free (tmp);
    /*    Get the position where the screenlet should be placed*/
    tokens = g_strsplit (search->data,"::",2);
    if (tokens && tokens[1])
    {
      pos = atoi (tokens[1]);
    }
    g_strfreev (tokens);
    /* remove the link... that data will be appended at to the list*/
    g_free (search->data);
    priv->ua_list = g_slist_delete_link (priv->ua_list,search);
    search = NULL;
  }
  else
  {
    /*
     This screenlet instance is not recorded in ua_list.  It will end up being
     placed at the end of the bar
     */
    ua_list_entry = tmp;
  }
  
  /*
   Calculated here and passed to the awn_ua_alignment_new().  AwnUAAlignment
   could recalculate the ratio on bar resizes based on the, then current,
   dimensions of the widget but over time the amount of error in the the 
   calcs would increase
   */
  ua_ratio = width / (double) height;
  ua_alignment = awn_ua_alignment_new(manager,ua_list_entry,ua_ratio); 

  g_signal_connect_swapped (awn_ua_alignment_get_socket(AWN_UA_ALIGNMENT(ua_alignment)), 
                            "plug-added",
                            G_CALLBACK (_applet_plug_added),
                            manager);

  awn_applet_manager_add_widget(manager, GTK_WIDGET (ua_alignment), pos);
  gtk_widget_show_all (ua_alignment);

  plugwin = awn_ua_alignment_add_id (AWN_UA_ALIGNMENT(ua_alignment),native_window);
  
  if (!plugwin)
  {
    g_warning ("UA Plug was not created within socket.");
    gtk_widget_destroy (ua_alignment);
    return FALSE;
  }

  /*
   Either add the new entry into ua_list or move an existing entry to the 
   end of ua_list_entry
   */
  priv->ua_list = g_slist_append (priv->ua_list,g_strdup(ua_list_entry));

  /* Keep the length of ua_list reasonable */
  if (g_slist_length (priv->ua_list) > MAX_UA_LIST_ENTRIES)
  {
    GSList * iter;
    int i =  g_slist_length (priv->ua_list) - MAX_UA_LIST_ENTRIES;
    for(iter = priv->ua_list; i && iter ; iter = priv->ua_list )
    {
      g_free (iter->data);
      priv->ua_list = g_slist_delete_link (priv->ua_list,iter);
      i--;
    }
  }
  array = awn_utils_gslist_to_gvaluearray (priv->ua_list);
  desktop_agnostic_config_client_set_list (priv->client,
                                           AWN_GROUP_PANEL, AWN_PANEL_UA_LIST,
                                           array, NULL);
  g_value_array_free (array);
  /*Add our newly active screenlet to thend of the active list */
  priv->ua_active_list = g_slist_append (priv->ua_active_list,g_strdup(ua_list_entry));  
  array = awn_utils_gslist_to_gvaluearray (priv->ua_active_list);
  desktop_agnostic_config_client_set_list (priv->client,
                                           AWN_GROUP_PANEL,
                                           AWN_PANEL_UA_ACTIVE_LIST,
                                           array, NULL);
  g_value_array_free (array);
  
  return TRUE;
}

gboolean
awn_ua_get_all_server_flags (	AwnAppletManager *manager,
				GHashTable **hash,
				gchar     *name,
				GError   **error)
{
  *hash = g_hash_table_new(g_str_hash,g_str_equal);
/* Future function to return capability of the server
For now, it return nothing*/
  return TRUE;
}


/*End DBUS*/

/* FIXME: get rid of these show/hide methods, it can be made cleaner */
void
awn_applet_manager_show_applets (AwnAppletManager *manager)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));
  AwnAppletManagerPrivate *priv = manager->priv;
  GList *list = gtk_container_get_children (GTK_CONTAINER (manager));

  for (GList *it = list; it != NULL; it = it->next)
  {
    if (AWN_IS_THROBBER (it->data)) continue;
    if (AWN_IS_APPLET_PROXY (it->data))
    {
      AwnAppletProxy *proxy = AWN_APPLET_PROXY (it->data);
      if (gtk_socket_get_plug_window (GTK_SOCKET (proxy)))
        gtk_widget_show (GTK_WIDGET (proxy));
      else
        gtk_widget_show (awn_applet_proxy_get_throbber (proxy));
    }
    else
    {
      int was_visible = GPOINTER_TO_INT (g_object_get_qdata (it->data,
                                         priv->visibility_quark));
      if (was_visible) gtk_widget_show (GTK_WIDGET (it->data));
    }
  }
  priv->docklet_mode = FALSE;
  g_object_notify (G_OBJECT (manager), "expands");

  g_list_free (list);
}

void
awn_applet_manager_hide_applets (AwnAppletManager *manager)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));
  AwnAppletManagerPrivate *priv = manager->priv;
  GList *list = gtk_container_get_children (GTK_CONTAINER (manager));

  for (GList *it = list; it != NULL; it = it->next)
  {
    if (!AWN_IS_THROBBER (it->data) && !AWN_IS_APPLET_PROXY (it->data))
    {
      g_object_set_qdata (it->data, priv->visibility_quark,
                          GINT_TO_POINTER (gtk_widget_get_visible (it->data) ?
                            1 : 0));
    }

    gtk_widget_hide (GTK_WIDGET (it->data));
  }
  priv->docklet_mode = TRUE;
  g_object_notify (G_OBJECT (manager), "expands");

  g_list_free (list);
}

gboolean
awn_applet_manager_get_docklet_mode (AwnAppletManager *manager)
{
  g_return_val_if_fail (AWN_IS_APPLET_MANAGER (manager), FALSE);

  return manager->priv->docklet_mode;
}

void
awn_applet_manager_add_docklet (AwnAppletManager *manager,
                                GtkWidget *docklet)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));
  AwnAppletManagerPrivate *priv = manager->priv;

  priv->docklet_widget = docklet;
  g_object_add_weak_pointer (G_OBJECT (docklet), (gpointer)&priv->docklet_widget);
  awn_applet_manager_add_widget (manager, docklet, 0);
  // we will allow it to expand
  gtk_box_set_child_packing (GTK_BOX (manager), priv->docklet_widget,
                             TRUE, TRUE, 0, GTK_PACK_START);
}

void
awn_applet_manager_redraw_throbbers (AwnAppletManager *manager)
{
  g_return_if_fail (AWN_IS_APPLET_MANAGER (manager));
  //AwnAppletManagerPrivate *priv = manager->priv;
  GList *list = gtk_container_get_children (GTK_CONTAINER (manager));

  for (GList *it = list; it != NULL; it = it->next)
  {
    if ((AWN_IS_THROBBER (it->data) || AWN_IS_SEPARATOR (it->data))
        && gtk_widget_get_visible (GTK_WIDGET (it->data)))
    {
      gtk_widget_queue_draw (GTK_WIDGET (it->data));
    }
  }

  g_list_free (list);
}

GdkRegion*
awn_applet_manager_get_mask (AwnAppletManager *manager,
                             AwnPathType path_type,
                             gfloat offset_modifier)
{
  g_return_val_if_fail (AWN_IS_APPLET_MANAGER (manager), NULL);
  AwnAppletManagerPrivate *priv = manager->priv;

  GdkRegion *region = gdk_region_new ();
  GList *children = gtk_container_get_children (GTK_CONTAINER (manager));

  for (GList *iter = children; iter != NULL; iter = g_list_next (iter))
  {
    GtkWidget *widget = (GtkWidget*)iter->data;
    if (gtk_widget_get_visible (widget) && gtk_widget_get_has_window (widget))
    {
      gpointer mask = g_object_get_qdata (G_OBJECT (widget),
                                          priv->shape_mask_quark);
      if (mask)
      {
        GdkRegion *temp_region = gdk_region_copy (mask);
        GtkAllocation alloc;
        
        gtk_widget_get_allocation (widget, &alloc);
        gdk_region_offset (temp_region, alloc.x, alloc.y);
        gdk_region_union (region, temp_region);
        gdk_region_destroy (temp_region);
      }
      else
      {
        // GtkAllocation and GdkRectangle are the same, we can do this
        GtkAllocation manager_alloc;
        GdkRectangle rect;

        gtk_widget_get_allocation (GTK_WIDGET (manager), &manager_alloc);
        gtk_widget_get_allocation (widget, &rect);
        // get curve offset
        gfloat temp = awn_utils_get_offset_modifier_by_path_type (path_type,
                   priv->position, priv->offset, offset_modifier,
                   rect.x + rect.width / 2 - manager_alloc.x,
                   rect.y + rect.height / 2 - manager_alloc.y,
                   manager_alloc.width,
                   manager_alloc.height);
        gint offset = round (temp);

        gint size = priv->size + offset;

        switch (priv->position)
        {
          case GTK_POS_BOTTOM:
            rect.y += rect.height - size;
            // no break!
          case GTK_POS_TOP:
            rect.height = size;
            break;
          case GTK_POS_RIGHT:
            rect.x += rect.width - size;
            // no break!
          case GTK_POS_LEFT:
            rect.width = size;
            break;
        }
        gdk_region_union_with_rect (region, &rect);
      }
    }
  }

  g_list_free (children);

  return region;
}

