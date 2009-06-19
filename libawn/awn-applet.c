/* -*- Mode: C; tab-width: 2; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/*
 * Copyright (C) 2007 Neil J. Patel <njpatel@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <X11/Xlib.h>
#include <math.h>

#include "awn-defines.h"
#include "awn-applet.h"
#include "awn-utils.h"
#include "awn-enum-types.h"
#include "gseal-transition.h"
#include "libawn-marshal.h"

G_DEFINE_TYPE (AwnApplet, awn_applet, GTK_TYPE_PLUG)

#define AWN_APPLET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                     AWN_TYPE_APPLET, AwnAppletPrivate))

struct _AwnAppletPrivate
{
  gchar *uid;
  gchar *gconf_key;
  AwnOrientation orient;
  AwnPathType path_type;
  gint offset;
  gfloat offset_modifier;
  guint size;

  gboolean show_all_on_embed;
  gboolean quit_on_delete;

  gint origin_x, origin_y;
  gint pos_x, pos_y;
  gint panel_width, panel_height;

  AwnAppletFlags flags;

  DBusGConnection *connection;
  DBusGProxy      *proxy;

  GdkWindow *panel_window;
};

enum
{
  PROP_0,
  PROP_UID,
  PROP_ORIENT,
  PROP_OFFSET,
  PROP_OFFSET_MOD,
  PROP_SIZE,
  PROP_PATH_TYPE
};

enum
{
  ORIENT_CHANGED,
  OFFSET_CHANGED,
  SIZE_CHANGED,
  PANEL_CONFIGURE,
  ORIGIN_CHANGED,
  DELETED,
  MENU_CREATION,
  FLAGS_CHANGED,

  LAST_SIGNAL
};
static guint _applet_signals[LAST_SIGNAL] = { 0 };

static void
on_orient_changed (DBusGProxy *proxy, gint orient, AwnApplet *applet)
{
  g_return_if_fail (AWN_IS_APPLET (applet));

  awn_applet_set_orientation (applet, orient);
}

static void
on_offset_changed (DBusGProxy *proxy, gint offset, AwnApplet *applet)
{
  g_return_if_fail (AWN_IS_APPLET (applet));

  awn_applet_set_offset (applet, offset);
}

static void
on_size_changed (DBusGProxy *proxy, gint size, AwnApplet *applet)
{
  g_return_if_fail (AWN_IS_APPLET (applet));

  awn_applet_set_size (applet, size);
}

static void
on_prop_changed (DBusGProxy *proxy, const gchar *uid, const gchar *prop_name,
                 GValue *value, AwnApplet *applet)
{
  AwnAppletPrivate *priv;
  g_return_if_fail (AWN_IS_APPLET (applet) && uid);
  priv = applet->priv;

  if (strcmp (priv->uid, uid) == 0 || uid[0] == '\0')
  {
    g_object_set_property (G_OBJECT (applet), prop_name, value);
  }
}

static void
on_delete_notify (DBusGProxy *proxy, AwnApplet *applet)
{
  gtk_main_quit ();
}

static void
on_proxy_destroyed (GObject *object)
{
  gtk_main_quit ();
}

static void
awn_applet_plug_embedded (AwnApplet *applet)
{
  g_return_if_fail (AWN_IS_APPLET (applet));

  AwnAppletPrivate *priv = applet->priv;

  if (priv->show_all_on_embed) gtk_widget_show_all (GTK_WIDGET (applet));
}

static gboolean
on_plug_deleted (GObject *object)
{
  AwnAppletPrivate *priv = AWN_APPLET_GET_PRIVATE (object);

  if (priv->quit_on_delete)
  {
    gtk_main_quit ();

    return TRUE;
  }

  return FALSE;
}

static void
on_destroy_applet (DBusGProxy *proxy, gchar *id, AwnApplet *applet)
{
  AwnAppletPrivate *priv;
  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  if (strcmp (priv->uid, id) == 0)
    on_delete_notify (NULL, applet);
}

static GdkFilterReturn
on_client_message (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
  g_return_val_if_fail (AWN_IS_APPLET (data), GDK_FILTER_CONTINUE);

  AwnAppletPrivate *priv = AWN_APPLET_GET_PRIVATE (data);
  GdkWindow *window;

  window = gtk_widget_get_window (GTK_WIDGET (data));

  if (!window)
  {
    return GDK_FILTER_CONTINUE;
  }

  /* Panel sends us our relative position on it */
  XEvent *xe = (XEvent*) xevent;
  gint pos_x = xe->xclient.data.l[0], pos_y = xe->xclient.data.l[1];

  if (priv->pos_x != pos_x || priv->pos_y != pos_y)
  {
    priv->pos_x = xe->xclient.data.l[0];
    priv->pos_y = xe->xclient.data.l[1];

    if (priv->path_type != AWN_PATH_LINEAR)
    {
      g_signal_emit (data, _applet_signals[OFFSET_CHANGED], 0, priv->offset);
    }
  }

  gint x, y;
  gdk_window_get_origin (window, &x, &y);

  if (priv->origin_x == x && priv->origin_y == y) return GDK_FILTER_REMOVE;

  priv->origin_x = x;
  priv->origin_y = y;

  GdkRectangle rect = { .x = x, .y = y };
  g_signal_emit (data, _applet_signals[ORIGIN_CHANGED], 0, &rect);

  return GDK_FILTER_REMOVE;
}

/*  GOBJECT STUFF */

static void
on_alpha_screen_changed (GtkWidget* widget, GdkScreen* oscreen, gpointer null)
{
  GdkScreen* nscreen = gtk_widget_get_screen (widget);
  GdkColormap* colormap = gdk_screen_get_rgba_colormap (nscreen);

  if (!colormap)
    colormap = gdk_screen_get_rgb_colormap (nscreen);

  gtk_widget_set_colormap (widget, colormap);
}

static void
awn_applet_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  AwnApplet *applet = AWN_APPLET (object);
  
  switch (prop_id)
  {
    case PROP_UID:
      awn_applet_set_uid (applet, g_value_get_string (value));
      break;
    case PROP_ORIENT:
      awn_applet_set_orientation (applet, g_value_get_int (value));
      break;
    case PROP_OFFSET:
      awn_applet_set_offset (applet, g_value_get_int (value));
      break;
    case PROP_OFFSET_MOD:
      // FIXME: method!
      AWN_APPLET_GET_PRIVATE(applet)->offset_modifier = g_value_get_float (value);
      break;
    case PROP_SIZE:
      awn_applet_set_size (applet, g_value_get_int (value));
      break;
    case PROP_PATH_TYPE:
      awn_applet_set_path_type (applet, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_applet_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  AwnAppletPrivate *priv;

  g_return_if_fail(AWN_IS_APPLET(object));
  priv = AWN_APPLET_GET_PRIVATE(object);

  switch (prop_id)
  {
    case PROP_UID:
      g_value_set_string (value, priv->uid);
      break;

    case PROP_ORIENT:
      g_value_set_int (value, priv->orient);
      break;

    case PROP_OFFSET:
      g_value_set_int (value, priv->offset);
      break;

    case PROP_OFFSET_MOD:
      g_value_set_float (value, priv->offset_modifier);
      break;

    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;

    case PROP_PATH_TYPE:
      g_value_set_int (value, priv->path_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_applet_dispose (GObject *obj)
{
  G_OBJECT_CLASS (awn_applet_parent_class)->dispose(obj);
}

static void
awn_applet_finalize (GObject *obj)
{
  AwnAppletPrivate *priv = AWN_APPLET_GET_PRIVATE (obj);

  if (priv->connection)
  {
    dbus_g_connection_unref (priv->connection);
    g_object_unref (priv->proxy);
    priv->connection = NULL;
    priv->proxy = NULL;
  }

  G_OBJECT_CLASS (awn_applet_parent_class)->finalize (obj);
}

static void
awn_applet_size_request (GtkWidget *widget, GtkRequisition *req)
{
  AwnAppletPrivate *priv = AWN_APPLET_GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (awn_applet_parent_class)->size_request (widget, req);

  gint modifier = 0;
  if (priv->flags & AWN_APPLET_EXPAND_MINOR) modifier += 1; // + 25% size
  if (priv->flags & AWN_APPLET_EXPAND_MAJOR) modifier += 2; // + 50% size

  if (modifier > 0)
  {
    switch (priv->orient)
    {
      case AWN_ORIENTATION_BOTTOM:
      case AWN_ORIENTATION_TOP:
        req->width += req->width * modifier / 4;
        break;
      default:
        req->height += req->height * modifier / 4;
        break;
    }
  }
}

static void
awn_applet_class_init (AwnAppletClass *klass)
{
  GObjectClass *g_object_class;
  GtkWidgetClass *gtk_widget_class;

  g_object_class = G_OBJECT_CLASS (klass);
  g_object_class->dispose = awn_applet_dispose;
  g_object_class->finalize = awn_applet_finalize;
  g_object_class->get_property = awn_applet_get_property;
  g_object_class->set_property = awn_applet_set_property;

  gtk_widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class->size_request = awn_applet_size_request;

  /* Class properties */
  g_object_class_install_property (g_object_class,
    PROP_UID,
    g_param_spec_string ("uid",
                         "wid",
                         "Awn's Unique ID for this applet instance",
                         NULL,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class,
   PROP_ORIENT,
   g_param_spec_int ("orient",
                     "Orientation",
                     "The current bar orientation",
                     0, 3, AWN_ORIENTATION_BOTTOM,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class,
   PROP_OFFSET,
   g_param_spec_int ("offset",
                     "Offset",
                     "Icon offset set on the bar",
                     0, G_MAXINT, 0,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class,
   PROP_OFFSET_MOD,
   g_param_spec_float ("offset-modifier",
                       "Offset modifier",
                       "Offset modifier for non-linear path types",
                       -G_MAXFLOAT, G_MAXFLOAT, 1.0,
                       G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class,
   PROP_SIZE,
   g_param_spec_int ("size",
                     "Size",
                     "The current visible size of the bar",
                     0, G_MAXINT, 48,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class,
   PROP_PATH_TYPE,
   g_param_spec_int ("path-type",
                     "Path type",
                     "Path used on the panel",
                     0, AWN_PATH_LAST-1, AWN_PATH_LINEAR,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  /* Class signals */
  _applet_signals[ORIENT_CHANGED] =
    g_signal_new("orientation-changed",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, orient_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__ENUM,
                 G_TYPE_NONE, 1, AWN_TYPE_ORIENTATION);

  _applet_signals[OFFSET_CHANGED] =
    g_signal_new("offset-changed",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, offset_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);

  _applet_signals[SIZE_CHANGED] =
    g_signal_new("size-changed",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, size_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);

  _applet_signals[PANEL_CONFIGURE] =
    g_signal_new("panel-configure-event",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET (AwnAppletClass, panel_configure),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__BOXED,
                 G_TYPE_NONE, 1,
                 GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

  _applet_signals[ORIGIN_CHANGED] =
    g_signal_new("origin-changed",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET (AwnAppletClass, origin_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__BOXED,
                 G_TYPE_NONE, 1, GDK_TYPE_RECTANGLE);

  _applet_signals[DELETED] =
    g_signal_new("applet-deleted",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, deleted),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__STRING,
                 G_TYPE_NONE, 1, G_TYPE_STRING);

  _applet_signals[MENU_CREATION] =
    g_signal_new("menu-creation",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, menu_creation),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__OBJECT,
                 G_TYPE_NONE, 1, GTK_TYPE_MENU);

  _applet_signals[FLAGS_CHANGED] =
    g_signal_new("flags-changed",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, flags_changed),
								 NULL, NULL,
                 g_cclosure_marshal_VOID__INT,
                 G_TYPE_NONE, 1, G_TYPE_INT);



  g_type_class_add_private (g_object_class, sizeof (AwnAppletPrivate));
}

static void
awn_applet_init (AwnApplet *applet)
{
  AwnAppletPrivate *priv;
  GError         *error;

  priv = applet->priv = AWN_APPLET_GET_PRIVATE(applet);

  gtk_widget_set_app_paintable(GTK_WIDGET(applet), TRUE);
  on_alpha_screen_changed(GTK_WIDGET(applet), NULL, NULL);

  priv->flags = AWN_APPLET_FLAGS_NONE;
  priv->offset_modifier = 1.0;
  // FIXME: turn into proper properties
  priv->show_all_on_embed = TRUE;
  priv->quit_on_delete = TRUE;

  error = NULL;
  priv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free(error);
    gtk_main_quit ();
  }
  
  priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                           "org.awnproject.Awn",
                                           "/org/awnproject/Awn/Panel1",
                                           "org.awnproject.Awn.Panel");
  if (!priv->proxy)
  {
    g_warning("Could not connect to mothership! Bailing\n");
    gtk_main_quit ();
  }

  dbus_g_object_register_marshaller (libawn_marshal_VOID__STRING_STRING_BOXED,
                                     G_TYPE_NONE, G_TYPE_STRING,
                                     G_TYPE_STRING, G_TYPE_VALUE,
                                     G_TYPE_INVALID);

  dbus_g_proxy_add_signal (priv->proxy, "OrientChanged",
                           G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "OffsetChanged",
                           G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "SizeChanged",
                           G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "PropertyChanged",
                           G_TYPE_STRING, G_TYPE_STRING,
                           G_TYPE_VALUE, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "DestroyNotify",
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "DestroyApplet",
                           G_TYPE_STRING, G_TYPE_INVALID);
  
  dbus_g_proxy_connect_signal (priv->proxy, "OrientChanged",
                               G_CALLBACK (on_orient_changed), applet, 
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "OffsetChanged",
                               G_CALLBACK (on_offset_changed), applet, 
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "SizeChanged",
                               G_CALLBACK (on_size_changed), applet,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "PropertyChanged",
                               G_CALLBACK (on_prop_changed), applet,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "DestroyNotify",
                               G_CALLBACK (on_delete_notify), applet,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "DestroyApplet",
                               G_CALLBACK (on_destroy_applet), applet,
                               NULL);

  g_signal_connect (priv->proxy, "destroy",
                    G_CALLBACK (on_proxy_destroyed), NULL);

  g_signal_connect (applet, "embedded",
                    G_CALLBACK (awn_applet_plug_embedded), NULL);
  g_signal_connect (applet, "delete-event",
                    G_CALLBACK (on_plug_deleted), NULL);

  awn_utils_ensure_transparent_bg (GTK_WIDGET (applet));

  GdkAtom atom = gdk_atom_intern_static_string ("_AWN_APPLET_POS_CHANGE");
  gdk_display_add_client_message_filter (gdk_display_get_default (),
                                         atom,
                                         on_client_message,
                                         applet);
}

AwnApplet *
awn_applet_new (const gchar* uid, gint orient, gint offset, gint size)
{
  AwnApplet *applet = g_object_new (AWN_TYPE_APPLET,
                                    "uid", uid,
                                    "orient", orient,
                                    "offset", offset,
                                    "size", size,
                                    NULL);
  return applet;
}

/*
 * Public funcs
 */
/*
 * Callback to start awn-manager.  See awn_applet_create_default_menu()
 */
static gboolean
_start_awn_manager (GtkMenuItem *menuitem, gpointer null)
{
  GError *err = NULL;
  
  g_spawn_command_line_async("awn-manager-mini", &err);

  if (err)
  {
    g_warning("Failed to start awn-manager: %s\n", err->message);
    g_error_free(err);
  }

  /* FIXME: Call a method on AwnPanel to popup it's prefs dialog */

  return TRUE;
}

/*create a Dock Preferences menu item */
GtkWidget *
awn_applet_create_pref_item (void)
{
	GtkWidget *item;
  
  item = gtk_image_menu_item_new_with_label (_("Dock Preferences"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
                                 gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,
                                                          GTK_ICON_SIZE_MENU));
  gtk_widget_show_all (item);
  g_signal_connect (item, "activate", 
                    G_CALLBACK (_start_awn_manager), NULL);
  return item;
}

GtkWidget*
awn_applet_create_default_menu (AwnApplet *applet)
{
  AwnAppletPrivate *priv;
  GtkWidget *menu = NULL;
  GtkWidget *item = NULL;
  
  g_return_val_if_fail (AWN_IS_APPLET (applet), menu);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  menu = gtk_menu_new ();

  /* The preferences (awn-manager) menu item  */
  item = awn_applet_create_pref_item ();
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

  /* And Neil said: "Let there be customisation of thy menu" */
  g_signal_emit (applet, _applet_signals[MENU_CREATION], 0, menu);

  /* The second separator  */
  item = gtk_separator_menu_item_new ();
  gtk_widget_show_all (item);
  gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);

  return menu;
}

AwnOrientation
awn_applet_get_orientation (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), AWN_ORIENTATION_BOTTOM);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->orient;
}

void
awn_applet_set_orientation (AwnApplet *applet, AwnOrientation orient)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->orient = orient;

  g_signal_emit (applet, _applet_signals[ORIENT_CHANGED], 0, orient);
}

AwnPathType
awn_applet_get_path_type (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), AWN_PATH_LINEAR);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->path_type;
}

void
awn_applet_set_path_type (AwnApplet *applet, AwnPathType path)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  if (priv->path_type != path)
  {
    priv->path_type = path;

    g_signal_emit (applet, _applet_signals[OFFSET_CHANGED], 0, priv->offset);
  }
}

gint
awn_applet_get_offset (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), 0);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->offset;
}

void
awn_applet_set_offset (AwnApplet *applet, gint offset)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  if (priv->offset != offset)
  {
    priv->offset = offset;

    g_signal_emit (applet, _applet_signals[OFFSET_CHANGED], 0, offset);
  }
}

gint
awn_applet_get_offset_at (AwnApplet *applet, gint x, gint y)
{
  AwnAppletPrivate *priv;
  gint result;
  gdouble temp;

  g_return_val_if_fail (AWN_IS_APPLET (applet), 0);
  priv = applet->priv;

  switch (priv->path_type)
  {
    case AWN_PATH_ELLIPSE:
      switch (priv->orient)
      {
        case AWN_ORIENTATION_LEFT:
        case AWN_ORIENTATION_RIGHT:
          temp = sin (M_PI * (priv->pos_y + y) / priv->panel_height);
          temp = temp * temp;
          result = round (temp * (priv->offset_modifier * priv->offset));
          break;
        default:
          temp = sin (M_PI * (priv->pos_x + x) / priv->panel_width);
          temp = temp * temp;
          result = round (temp * (priv->offset_modifier * priv->offset));
          break;
      }
/*
      g_debug ("%s: sin(PI*(%d+%d)/%d) * (%d - %d) = %d",
               __func__, priv->pos_x, x, priv->panel_width,
               priv->max_offset, priv->offset, result);
*/
      return result + priv->offset;
    default:
      return priv->offset;
  }
}

guint
awn_applet_get_size (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), 48);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->size;
}

void
awn_applet_set_size (AwnApplet *applet, gint size)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->size = size;

  g_signal_emit (applet, _applet_signals[SIZE_CHANGED], 0, size);
}

const gchar *
awn_applet_get_uid (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->uid;
}

void
awn_applet_set_uid (AwnApplet *applet, const gchar *uid)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  g_return_if_fail (uid);
  priv = applet->priv;

  priv->uid = g_strdup (uid);
}

void
awn_applet_set_flags (AwnApplet *applet, AwnAppletFlags flags)
{
  AwnAppletPrivate *priv;
  GError *error = NULL;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->flags = flags;

  dbus_g_proxy_call (priv->proxy, "SetAppletFlags",
                        &error,
                        G_TYPE_STRING, awn_applet_get_uid (AWN_APPLET (applet)),
                        G_TYPE_INT, flags,
                        G_TYPE_INVALID, G_TYPE_INVALID);

  if (flags & (AWN_APPLET_EXPAND_MINOR | AWN_APPLET_EXPAND_MAJOR))
  {
    gtk_widget_queue_resize (GTK_WIDGET (applet));
  }

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }
}

AwnAppletFlags 
awn_applet_get_flags (AwnApplet *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET (applet), AWN_APPLET_FLAGS_NONE);

  return applet->priv->flags;
}

guint
awn_applet_inhibit_autohide (AwnApplet *applet, const gchar *reason)
{
  AwnAppletPrivate *priv;
  GError *error = NULL;
  guint ret = 0;

  g_return_val_if_fail (AWN_IS_APPLET (applet), 0);
  priv = applet->priv;

  gchar *app_name = g_strdup_printf ("%s:%d", g_get_prgname(), getpid());

  dbus_g_proxy_call (priv->proxy, "InhibitAutohide",
                     &error,
                     G_TYPE_STRING, app_name,
                     G_TYPE_STRING, reason,
                     G_TYPE_INVALID, 
                     G_TYPE_UINT, &ret,
                     G_TYPE_INVALID);

  if (app_name) g_free (app_name);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  return ret;
}

void
awn_applet_uninhibit_autohide (AwnApplet *applet, guint cookie)
{
  AwnAppletPrivate *priv;
  GError *error = NULL;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  dbus_g_proxy_call (priv->proxy, "UninhibitAutohide",
                     &error,
                     G_TYPE_UINT, cookie,
                     G_TYPE_INVALID,
                     G_TYPE_INVALID);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }
}

static GdkFilterReturn
_on_panel_configure (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
  g_return_val_if_fail (AWN_IS_APPLET (data), GDK_FILTER_CONTINUE);

  AwnAppletPrivate *priv = AWN_APPLET_GET_PRIVATE (data);
  GdkWindow *window;

  window = gtk_widget_get_window (GTK_WIDGET (data));

  XEvent *xe = (XEvent*)xevent;

  if (xe->type == ConfigureNotify)
  {
    event->type = GDK_CONFIGURE;
    event->configure.send_event = xe->xconfigure.send_event;
    event->configure.x = xe->xconfigure.x;
    event->configure.y = xe->xconfigure.y;
    event->configure.width = xe->xconfigure.width;
    event->configure.height = xe->xconfigure.height;

    priv->panel_width = xe->xconfigure.width;
    priv->panel_height = xe->xconfigure.height;

    g_signal_emit (data, _applet_signals[PANEL_CONFIGURE], 0, event);

    /* Emit offset-changed, so we still have nice ellipsis */
    if (priv->path_type != AWN_PATH_LINEAR)
    {
      g_signal_emit (data, _applet_signals[OFFSET_CHANGED], 0, priv->offset);
    }

    if (window != NULL)
    {
      gint x, y;
      gdk_window_get_origin (window, &x, &y);

      if (priv->origin_x == x && priv->origin_y == y)
        return GDK_FILTER_TRANSLATE;

      priv->origin_x = x;
      priv->origin_y = y;

      GdkRectangle rect = { .x = x, .y = y };
      g_signal_emit (data, _applet_signals[ORIGIN_CHANGED], 0, &rect);
    }

    return GDK_FILTER_TRANSLATE;
  }

  return GDK_FILTER_CONTINUE;
}

void
awn_applet_set_panel_window_id (AwnApplet *applet, GdkNativeWindow anid)
{
  g_return_if_fail (AWN_IS_APPLET (applet) && anid);

  AwnAppletPrivate *priv = applet->priv;

  if (priv->panel_window)
  {
    gdk_window_remove_filter (priv->panel_window, _on_panel_configure, applet);
  }

  priv->panel_window = gdk_window_foreign_new (anid);
  gdk_window_get_geometry (priv->panel_window, NULL, NULL,
                           &priv->panel_width, &priv->panel_height, NULL);

  gdk_window_set_events (priv->panel_window, GDK_STRUCTURE_MASK);
  gdk_window_add_filter (priv->panel_window, _on_panel_configure, applet);
}

