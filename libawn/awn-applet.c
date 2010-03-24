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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#define AWN_SETTINGS_APP "awn-settings"

struct _AwnAppletPrivate
{
  gchar *uid;
  gint panel_id;
  gint64 panel_xid;
  gchar *canonical_name;
	gchar *display_name;
  GtkPositionType position;
  AwnPathType path_type;
  gint offset;
  gfloat offset_modifier;
  gint size;
  gint max_size;

  guint menu_inhibit_cookie;

  gboolean show_all_on_embed;
  gboolean quit_on_delete;

  gint origin_x, origin_y;
  gint pos_x, pos_y;
  gint panel_width, panel_height;

  AwnAppletFlags flags;

  DBusGConnection *connection;
  DBusGProxy      *proxy;
};

enum
{
  PROP_0,
  PROP_UID,
  PROP_PANEL_ID,
  PROP_PANEL_XID,
  PROP_CANONICAL_NAME,
	PROP_DISPLAY_NAME,
  PROP_POSITION,
  PROP_OFFSET,
  PROP_OFFSET_MOD,
  PROP_SIZE,
  PROP_MAX_SIZE,
  PROP_PATH_TYPE,

  PROP_SHOW_ALL_ON_EMBED,
  PROP_QUIT_ON_DELETE,
};

enum
{
  POS_CHANGED,
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

/* DBus signal callbacks */
static void
on_position_changed (DBusGProxy *proxy, GtkPositionType position,
                     AwnApplet *applet)
{
  g_return_if_fail (AWN_IS_APPLET (applet));

  awn_applet_set_pos_type (applet, position);
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
on_prop_changed (DBusGProxy *proxy, const gchar *prop_name,
                 GValue *value, AwnApplet *applet)
{
  AwnAppletPrivate *priv;
  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  g_object_set_property (G_OBJECT (applet), prop_name, value);
}

static void
on_delete_notify (DBusGProxy *proxy, AwnApplet *applet)
{
  gtk_main_quit ();
}

static void
on_proxy_destroyed (GObject *object, AwnApplet *applet)
{
  AwnAppletPrivate *priv = applet->priv;

  if (priv->quit_on_delete)
  {
    gtk_main_quit ();
  }
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
  {
    g_signal_emit (applet, _applet_signals[DELETED], 0);
    on_delete_notify (NULL, applet);
  }
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
  gint panel_w = xe->xclient.data.l[2], panel_h = xe->xclient.data.l[3];

  if (priv->pos_x != pos_x || priv->pos_y != pos_y ||
      priv->panel_width != panel_w || priv->panel_height != panel_h)
  {
    priv->pos_x = pos_x;
    priv->pos_y = pos_y;
    priv->panel_width = panel_w;
    priv->panel_height = panel_h;

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
    case PROP_CANONICAL_NAME:
      if (g_value_get_string (value) == NULL)
      {
        g_warning ("Canonical name for this applet wasn't set!");
      }
      else
      {
        applet->priv->canonical_name =
          g_strcanon (g_ascii_strdown (g_value_get_string (value), -1),
                      "abcdefghijklmnopqrstuvwxyz0123456789-", '-');
        g_warn_if_fail (strlen (applet->priv->canonical_name) > 0);
      }
      break;
    case PROP_DISPLAY_NAME:
			g_free (applet->priv->display_name);
			applet->priv->display_name = g_value_dup_string (value);
      break;			
    case PROP_PANEL_ID:
      applet->priv->panel_id = g_value_get_int (value);
      break;
    case PROP_POSITION:
      awn_applet_set_pos_type (applet, g_value_get_enum (value));
      break;
    case PROP_OFFSET:
      awn_applet_set_offset (applet, g_value_get_int (value));
      break;
    case PROP_OFFSET_MOD:
      // FIXME: method!
      applet->priv->offset_modifier = g_value_get_float (value);
      break;
    case PROP_SIZE:
      awn_applet_set_size (applet, g_value_get_int (value));
      break;
    case PROP_MAX_SIZE:
      applet->priv->max_size = g_value_get_int (value);
      break;
    case PROP_PATH_TYPE:
      awn_applet_set_path_type (applet, g_value_get_int (value));
      break;
    case PROP_SHOW_ALL_ON_EMBED:
      applet->priv->show_all_on_embed = g_value_get_boolean (value);
      break;
    case PROP_QUIT_ON_DELETE:
      applet->priv->quit_on_delete = g_value_get_boolean (value);
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

    case PROP_CANONICAL_NAME:
      g_value_set_string (value, priv->canonical_name);
      break;

    case PROP_DISPLAY_NAME:
      g_value_set_string (value, priv->display_name);
      break;
			
    case PROP_PANEL_ID:
      g_value_set_int (value, priv->panel_id);
      break;

    case PROP_PANEL_XID:
      g_value_set_int64 (value, priv->panel_xid);
      break;

    case PROP_POSITION:
      g_value_set_enum (value, priv->position);
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

    case PROP_MAX_SIZE:
      g_value_set_int (value, priv->max_size);
      break;

    case PROP_PATH_TYPE:
      g_value_set_int (value, priv->path_type);
      break;

    case PROP_SHOW_ALL_ON_EMBED:
      g_value_set_boolean (value, priv->show_all_on_embed);
      break;

    case PROP_QUIT_ON_DELETE:
      g_value_set_boolean (value, priv->quit_on_delete);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_applet_constructed (GObject *obj)
{
  AwnApplet *applet = AWN_APPLET (obj);
  AwnAppletPrivate *priv = applet->priv;

  if (priv->panel_id > 0)
  {
    gchar *object_path = g_strdup_printf ("/org/awnproject/Awn/Panel%d",
                                          priv->panel_id);
    priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                             "org.awnproject.Awn",
                                             object_path,
                                             "org.awnproject.Awn.Panel");
    if (!priv->proxy)
    {
      g_warning("Could not connect to mothership! Bailing\n");
      gtk_main_quit ();
    }

    dbus_g_object_register_marshaller (
      libawn_marshal_VOID__STRING_BOXED,
      G_TYPE_NONE, G_TYPE_STRING, G_TYPE_VALUE,
      G_TYPE_INVALID
    );

    dbus_g_proxy_add_signal (priv->proxy, "PositionChanged",
                             G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (priv->proxy, "OffsetChanged",
                             G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (priv->proxy, "SizeChanged",
                             G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (priv->proxy, "PropertyChanged",
                             G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (priv->proxy, "DestroyNotify",
                             G_TYPE_INVALID);
    dbus_g_proxy_add_signal (priv->proxy, "DestroyApplet",
                             G_TYPE_STRING, G_TYPE_INVALID);
  
    dbus_g_proxy_connect_signal (priv->proxy, "PositionChanged",
                                 G_CALLBACK (on_position_changed), applet, 
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
                      G_CALLBACK (on_proxy_destroyed), applet);

    // get prop values from Panel
    DBusGProxy *prop_proxy = dbus_g_proxy_new_from_proxy (
      priv->proxy, "org.freedesktop.DBus.Properties", NULL
    );

    if (!prop_proxy)
    {
      g_warning("Could not get property values! Bailing\n");
      gtk_main_quit ();
    }

    GError *error = NULL;

#if HAVE_DBUS_GLIB_080
    GHashTable *all_props = NULL;

    // doing GetAll reduces DBus lag significantly
    dbus_g_proxy_call (prop_proxy, "GetAll", &error,
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_INVALID,
                       dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, 
                                            G_TYPE_VALUE), &all_props,
                       G_TYPE_INVALID);

    if (error) goto crap_out;

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init (&iter, all_props);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
      if (strcmp (key, "PanelXid") == 0)
      {
        priv->panel_xid = g_value_get_int64 (value);
      }
      else if (strcmp (key, "MaxSize") == 0)
      {
        g_object_set_property (obj, "max-size", value);
      }
      else if (strcmp (key, "Position") == 0)
      {
        g_object_set_property (obj, "position", value);
      }
      else if (strcmp (key, "Size") == 0)
      {
        g_object_set_property (obj, "size", value);
      }
      else if (strcmp (key, "Offset") == 0)
      {
        g_object_set_property (obj, "offset", value);
      }
      else if (strcmp (key, "OffsetModifier") == 0)
      {
        g_object_set_property (obj, "offset-modifier", value);
      }
      else if (strcmp (key, "PathType") == 0)
      {
        g_object_set_property (obj, "path-type", value);
      }
      else
      {
        g_warning ("Unknown property: \"%s\"", (char*)key);
      }
    }

#else
    GValue position = {0,}, size = {0,}, max_size = {0,}, offset = {0,};
    GValue panel_xid = {0,}, path_type = {0,}, offset_mod = {0,};

    dbus_g_proxy_call (prop_proxy, "Get", &error, 
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_STRING, "Position",
                       G_TYPE_INVALID,
                       G_TYPE_VALUE, &position,
                       G_TYPE_INVALID);

    if (error) goto crap_out;

    dbus_g_proxy_call (prop_proxy, "Get", &error, 
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_STRING, "Size",
                       G_TYPE_INVALID,
                       G_TYPE_VALUE, &size,
                       G_TYPE_INVALID);

    if (error) goto crap_out;

    dbus_g_proxy_call (prop_proxy, "Get", &error, 
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_STRING, "Offset",
                       G_TYPE_INVALID,
                       G_TYPE_VALUE, &offset,
                       G_TYPE_INVALID);

    if (error) goto crap_out;

    dbus_g_proxy_call (prop_proxy, "Get", &error,
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_STRING, "MaxSize",
                       G_TYPE_INVALID,
                       G_TYPE_VALUE, &max_size,
                       G_TYPE_INVALID);

    if (error) goto crap_out;
    
    dbus_g_proxy_call (prop_proxy, "Get", &error,
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_STRING, "PanelXid",
                       G_TYPE_INVALID,
                       G_TYPE_VALUE, &panel_xid,
                       G_TYPE_INVALID);

    if (error) goto crap_out;

    dbus_g_proxy_call (prop_proxy, "Get", &error,
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_STRING, "OffsetModifier",
                       G_TYPE_INVALID,
                       G_TYPE_VALUE, &offset_mod,
                       G_TYPE_INVALID);

    if (error) goto crap_out;

    dbus_g_proxy_call (prop_proxy, "Get", &error,
                       G_TYPE_STRING, "org.awnproject.Awn.Panel",
                       G_TYPE_STRING, "PathType",
                       G_TYPE_INVALID,
                       G_TYPE_VALUE, &path_type,
                       G_TYPE_INVALID);

    if (error) goto crap_out;

    g_object_set_property (obj, "position", &position);
    g_object_set_property (obj, "size", &size);
    g_object_set_property (obj, "offset", &offset);
    g_object_set_property (obj, "max-size", &max_size);
    g_object_set_property (obj, "offset-modifier", &offset_mod);
    g_object_set_property (obj, "path-type", &path_type);

    if (G_VALUE_HOLDS_INT64 (&panel_xid))
    {
      priv->panel_xid = g_value_get_int64 (&panel_xid);
    }

    g_value_unset (&position);
    g_value_unset (&size);
    g_value_unset (&max_size);
    g_value_unset (&offset);
    g_value_unset (&panel_xid);
    g_value_unset (&offset_mod);
    g_value_unset (&path_type);

#endif
    if (prop_proxy) g_object_unref (prop_proxy);

    g_free (object_path);
    return;

    crap_out:

    g_warning ("%s", error->message);
    g_error_free (error);
    g_assert_not_reached ();
  }
}

static void
awn_applet_dispose (GObject *obj)
{
  //AwnAppletPrivate *priv = AWN_APPLET_GET_PRIVATE (obj);

  G_OBJECT_CLASS (awn_applet_parent_class)->dispose(obj);
}

static void
awn_applet_finalize (GObject *obj)
{
  AwnAppletPrivate *priv = AWN_APPLET_GET_PRIVATE (obj);

  if (priv->connection)
  {
    if (priv->proxy) g_object_unref (priv->proxy);
    dbus_g_connection_unref (priv->connection);
    priv->connection = NULL;
    priv->proxy = NULL;
  }

  if (priv->canonical_name)
  {
    g_free (priv->canonical_name);
    priv->canonical_name = NULL;
  }

  if (priv->display_name)
  {
    g_free (priv->display_name);
    priv->display_name = NULL;
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
    switch (priv->position)
    {
      case GTK_POS_BOTTOM:
      case GTK_POS_TOP:
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
  g_object_class->constructed = awn_applet_constructed;
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
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS));

  /**
   * AwnApplet:canonical-name:
   *
   * The canonical name of the applet. The format should be considered the
   * same as a GObject property name: [a-zA-Z][a-zA-Z0-9_\-]
   * In English, the first character must be a lowercase letter of the English
   * alphabet, and the following character(s) can be one or more lowercase
   * English letters, numbers, and/or minus characters.
   *
   * For all applets in the Awn Extras project, this name should be the same as
   * the main directory as the applet sources.
   *
   * <note>For Python applets, it should also be the same name as the main
   * applet script.</note>
   */
  g_object_class_install_property (g_object_class,
    PROP_CANONICAL_NAME,
    g_param_spec_string ("canonical-name",
                         "Canonical name",
                         "Canonical name for the applet, this should be also "
                         "be the name of the directory the applet is in",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS));

 /**
 * AwnApplet:display:
 *
 * The friendly name of the applet.  Used for display in menu text if available
 */

  g_object_class_install_property (g_object_class,
    PROP_DISPLAY_NAME,
    g_param_spec_string ("display-name",
                         "Display name",
                         "Display name for the applet.",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
 * AwnApplet:panel-id:
 *
 * The id of the Awn Panel the applet connects to.
 */
	
  g_object_class_install_property (g_object_class,
    PROP_PANEL_ID,
    g_param_spec_int ("panel-id",
                      "Panel ID",
                      "The id of AwnPanel this applet connects to",
                      0, G_MAXINT, 0,
                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
                      G_PARAM_STATIC_STRINGS));
 /**
 * AwnApplet:panel-xid:
 *
 * The XID of the awn panel the applet is connected to.
 */

  g_object_class_install_property (g_object_class,
    PROP_PANEL_XID,
    g_param_spec_int64 ("panel-xid",
                        "Panel XID",
                        "The XID of AwnPanel this applet is connected to",
                        G_MININT64, G_MAXINT64, 0,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

 /**
 * AwnApplet:position:
 *
 * The current bar position.
 */
	
  g_object_class_install_property (g_object_class,
   PROP_POSITION,
   g_param_spec_enum ("position",
                      "Position",
                      "The current bar position",
                      GTK_TYPE_POSITION_TYPE,
                      GTK_POS_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
 * AwnApplet:offset:
 *
 * The icon offset of the bar.
 */
	
  g_object_class_install_property (g_object_class,
   PROP_OFFSET,
   g_param_spec_int ("offset",
                     "Offset",
                     "Icon offset set on the bar",
                     0, G_MAXINT, 0,
                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

 /**
 * AwnApplet:offset-modifier:
 *
 * The offset modifier for non-linear path types.
 */

  g_object_class_install_property (g_object_class,
   PROP_OFFSET_MOD,
   g_param_spec_float ("offset-modifier",
                       "Offset modifier",
                       "Offset modifier for non-linear path types",
                       -G_MAXFLOAT, G_MAXFLOAT, 1.0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
 /**
 * AwnApplet:size:
 *
 * The current visible size of the bar.  
 */

  g_object_class_install_property (g_object_class,
   PROP_SIZE,
   g_param_spec_int ("size",
                     "Size",
                     "The current visible size of the bar",
                     0, G_MAXINT, 48,
                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
 /**
 * AwnApplet:max-size:
 *
 * The maximum visible size of the applet.
 */

  g_object_class_install_property (g_object_class,
   PROP_MAX_SIZE,
   g_param_spec_int ("max-size",
                     "Max Size",
                     "The maximum visible size of the applet",
                     0, G_MAXINT, 48,
                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (g_object_class,
   PROP_PATH_TYPE,
   g_param_spec_int ("path-type",
                     "Path type",
                     "Path used on the panel",
                     0, AWN_PATH_LAST-1, AWN_PATH_LINEAR,
                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE |
                     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (g_object_class,
   PROP_SHOW_ALL_ON_EMBED,
   g_param_spec_boolean ("show-all-on-embed",
                         "Show all on Embed",
                         "The applet will automatically call show_all when "
                         "it's embedded in the socket",
                         TRUE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
 /**
 * AwnApplet:quit-on-delete:
 *
 * Whether the applet quits when it's socket is destroyed.
 */
	
  g_object_class_install_property (g_object_class,
   PROP_QUIT_ON_DELETE,
   g_param_spec_boolean ("quit-on-delete",
                         "Quit on delete",
                         "Quit the applet when it's socket is destroyed",
                         TRUE,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* Class signals */
  _applet_signals[POS_CHANGED] =
    g_signal_new("position-changed",
                 G_OBJECT_CLASS_TYPE (g_object_class),
                 G_SIGNAL_RUN_FIRST,
                 G_STRUCT_OFFSET (AwnAppletClass, position_changed),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__ENUM,
                 G_TYPE_NONE, 1, GTK_TYPE_POSITION_TYPE);

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
                 g_cclosure_marshal_VOID__VOID,
                 G_TYPE_NONE, 0);

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

  // provide defaults (these aren't constructed)
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

/**
 * awn_applet_new:
 * @canonical_name: canonical name of the applet.
 * @uid: unique ID of the applet.
 * @panel_id: ID of AwnPanel associated with the applet.
 *
 * Creates a new AwnApplet which tries to connect via DBus to AwnPanel
 * with the given ID. You can pass zero @panel_id to not connect to any panel.
 *
 * Returns: the new AwnApplet.
 */
AwnApplet *
awn_applet_new (const gchar* canonical_name, const gchar* uid, gint panel_id)
{
  AwnApplet *applet = g_object_new (AWN_TYPE_APPLET,
                                    "canonical-name", canonical_name,
                                    "uid", uid,
                                    "panel-id", panel_id,
                                    NULL);
  return applet;
}

/*
 * Public funcs
 */

/**
 * awn_applet_get_canonical_name:
 * @applet: The #AwnApplet.
 *
 * Retrieve the applet's canonical name.
 * Returns: the applet's canonical name.
 */
const gchar*
awn_applet_get_canonical_name (AwnApplet *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);

  return applet->priv->canonical_name;
}

/*
 * Callback to start the settings App. See awn_applet_create_default_menu().
 */
static gboolean
_start_awn_settings (GtkMenuItem *menuitem, gpointer null)
{
  GError *err = NULL;
  
  g_spawn_command_line_async(AWN_SETTINGS_APP, &err);

  if (err)
  {
    g_warning("Failed to start %s: %s\n", AWN_SETTINGS_APP, err->message);
    g_error_free(err);
  }

  /* FIXME: Call a method on AwnPanel to popup it's prefs dialog */

  return TRUE;
}

/**
 * awn_applet_create_pref_item:
 *
 * Create a Dock Preferences menu item.
 * Returns: A #GtkImageMenuItem for the Dock Preferences that can be added to
 * an applet icon's context menu.
 */

GtkWidget *
awn_applet_create_pref_item (void)
{
	GtkWidget *item;
  
  item = gtk_image_menu_item_new_with_label (dgettext (GETTEXT_PACKAGE,
                                                       "Dock Preferences"));
#if GTK_CHECK_VERSION (2,16,0)	
	g_object_set (item,"always-show-image",TRUE,NULL);  
#endif
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
                                 gtk_image_new_from_icon_name ("avant-window-navigator",
                                                          GTK_ICON_SIZE_MENU));
  gtk_widget_show_all (item);
  g_signal_connect (item, "activate", 
                    G_CALLBACK (_start_awn_settings), NULL);
  return item;
}

static void
_show_about_dialog (GtkMenuItem *menuitem,
                    GtkWidget   *dialog)
{
  gtk_widget_show_all (dialog);
}

static gboolean
_cleanup_about_dialog (GtkWidget *menuitem,
                       GdkEvent  *event,
                       GtkWidget *dialog)
{
  gtk_widget_destroy (dialog);
  return FALSE;
}

/**
 * awn_applet_create_about_item:
 * @applet: An AwnApplet.
 * @copyright: The copyright holder string.
 * @license: Must be one of the values enumerated in #AwnAppletLicense.
 * @version: Applet version string.
 * @comments: Comment string.
 * @website: Website string.
 * @website_label: Website label string.
 * @icon_name: Icon name.
 * @translator_credits: Translator's credit string.
 * @authors: Array of author strings.
 * @artists: Array of artist strings.
 * @documenters: Array of documentor strings.
 *
 * Creates an about dialog and an associated menu item for use in the applet's
 * context menu. The @copyright and @license parameters are
 * mandatory. The rest are optional. See also #GtkAboutDialog for a description
 * of the parameters other than @license.
 *
 * Returns: An "about applet" #GtkMenuItem
 */
GtkWidget *
awn_applet_create_about_item (AwnApplet         *applet,
                              const gchar       *copyright,
                              AwnAppletLicense   license,
                              const gchar       *version,
                              const gchar       *comments,
                              const gchar       *website,
                              const gchar       *website_label,
                              const gchar       *icon_name,
                              const gchar       *translator_credits,
                              const gchar      **authors,
                              const gchar      **artists,
                              const gchar      **documenters)
{
  g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);
  g_return_val_if_fail (copyright && strlen (copyright) > 8, NULL);

  /* we could use  gtk_show_about_dialog()... but no. */
  GtkAboutDialog *dialog = GTK_ABOUT_DIALOG (gtk_about_dialog_new ());
  GtkWidget *item;
  gchar *item_text = NULL;
  GdkPixbuf *pixbuf = NULL;
  gchar *applet_name;

  if (applet->priv->display_name)
  {
    applet_name = applet->priv->display_name;
  }
  else
  {
    applet_name = applet->priv->canonical_name;
  }
  g_return_val_if_fail (applet_name, NULL);

  gtk_about_dialog_set_copyright (dialog, copyright);

  switch (license) /* FIXME insert more complete license info. */
  {
    case AWN_APPLET_LICENSE_GPLV2:
      gtk_about_dialog_set_license (dialog, "GPLv2");
      break;
    case AWN_APPLET_LICENSE_GPLV3:
      gtk_about_dialog_set_license (dialog, "GPLv3");
      break;
    case AWN_APPLET_LICENSE_LGPLV2_1:
      gtk_about_dialog_set_license (dialog, "LGPLv2.1");
      break;
    case AWN_APPLET_LICENSE_LGPLV3:
      gtk_about_dialog_set_license (dialog, "LGPLv3");
      break;
    default:
      g_warning ("License must be set");
      g_assert_not_reached ();
  }

  gtk_about_dialog_set_program_name (dialog, applet_name);

  if (version) /* FIXME we can probably append some addition build info in here... */
  {
    gtk_about_dialog_set_version (dialog, version);
  }
  if (comments)
  {
    gtk_about_dialog_set_comments (dialog, comments);
  }
  if (website)
  {
    gtk_about_dialog_set_website (dialog, website);
  }
  if (website_label)
  {
    gtk_about_dialog_set_website_label (dialog, website_label);
  }
  if (!icon_name)
  {
    icon_name = "stock_about";
  }
  gtk_about_dialog_set_logo_icon_name (dialog, icon_name);
  pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                     icon_name, 64, 0, NULL);
  if (pixbuf)
  {
    gtk_window_set_icon (GTK_WINDOW (dialog), pixbuf);
    g_object_unref (pixbuf);
  }

  if (translator_credits)
  {
    gtk_about_dialog_set_translator_credits (dialog, translator_credits);
  }
  if (authors)
  {
    gtk_about_dialog_set_authors (dialog, authors);
  }
  if (artists)
  {
    gtk_about_dialog_set_artists (dialog, artists);
  }
  if (documenters)
  {
    gtk_about_dialog_set_documenters (dialog, documenters);
  }
  item_text = g_strdup_printf ("About %s", applet_name);
  item = gtk_image_menu_item_new_with_label (item_text); /* FIXME Add pretty icon */
#if GTK_CHECK_VERSION (2,16,0)	
	g_object_set (item,"always-show-image",TRUE,NULL);  
#endif
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
    gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU));

  g_free (item_text);

  gtk_widget_show_all (item);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (_show_about_dialog), dialog);
  g_signal_connect (G_OBJECT (item), "destroy-event",
                    G_CALLBACK (_cleanup_about_dialog), dialog);
  g_signal_connect_swapped (dialog, "response",
                            G_CALLBACK (gtk_widget_hide), dialog);
  g_signal_connect_swapped (dialog, "delete-event",
                            G_CALLBACK (gtk_widget_hide), dialog);
  return item;
}

/**
 * awn_applet_create_about_item_simple:
 * @applet: An AwnApplet.
 * @copyright: The copyright holder string.
 * @license: Must be one of the values enumerated in #AwnAppletLicense.
 * @version: Applet version string.
 *
 * Creates an about dialog and an associated menu item for use in the applet's
 * context menu. The @copyright and @license parameters are
 * mandatory. See also #GtkAboutDialog for a description
 * of the parameters other than @license.
 *
 * Returns: An "about applet" #GtkMenuItem
 */

GtkWidget *
awn_applet_create_about_item_simple (AwnApplet        *applet,
                                     const gchar      *copyright,
                                     AwnAppletLicense  license,
                                     const gchar      *version)
{
  g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);
  return awn_applet_create_about_item (applet, copyright, license, version, 
                                       NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                       NULL);
}

static void
_menu_showed (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));

  priv = applet->priv;

  if (priv->menu_inhibit_cookie == 0)
  {
    priv->menu_inhibit_cookie = 
      awn_applet_inhibit_autohide (applet, "Displaying applet menu");
  }
}

static void
_menu_hidden (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));

  priv = applet->priv;

  if (priv->menu_inhibit_cookie)
  {
    awn_applet_uninhibit_autohide (applet, priv->menu_inhibit_cookie);
    priv->menu_inhibit_cookie = 0;
  }
}

/**
 * awn_applet_create_default_menu:
 * @applet: An AwnApplet.
 *
 * Creates an default applet context menu. Includes a dock preferences menu
 * item
 *
 * Returns: A default #GtkMenu for #AwnApplet
 */
GtkWidget*
awn_applet_create_default_menu (AwnApplet *applet)
{
  AwnAppletPrivate *priv;
  GtkWidget *menu = NULL;
  GtkWidget *item = NULL;
  
  g_return_val_if_fail (AWN_IS_APPLET (applet), menu);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  menu = gtk_menu_new ();

  /* Inhibit autohide when the menu is shown */
  g_signal_connect_swapped (menu, "show", G_CALLBACK (_menu_showed), applet);
  g_signal_connect_swapped (menu, "hide", G_CALLBACK (_menu_hidden), applet);

  /* The preferences (awn-settings) menu item  */
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

/**
 * awn_applet_get_pos_type:
 * @applet: an #AwnApplet.
 *
 * Gets current position of the applet. See awn_applet_set_pos_type().
 * This value corresponds to the value used by the associated panel.
 *
 * Returns: current position of the applet.
 */
GtkPositionType
awn_applet_get_pos_type (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), GTK_POS_BOTTOM);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->position;
}

/**
 * awn_applet_set_pos_type:
 * @applet: an #AwnApplet.
 * @position: new position of the applet.
 *
 * Sets current position of the applet. Note that setting the position 
 * emits the #AwnApplet::position-changed signal.
 */
void
awn_applet_set_pos_type (AwnApplet *applet, GtkPositionType position)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->position = position;

  g_signal_emit (applet, _applet_signals[POS_CHANGED], 0, position);
}

/**
 * awn_applet_get_path_type:
 * @applet: an #AwnApplet.
 *
 * Gets currently used path type for this applet. This value corresponds
 * to the value used by the associated panel.
 *
 * Returns: currently used path type.
 */
AwnPathType
awn_applet_get_path_type (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), AWN_PATH_LINEAR);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->path_type;
}

/**
 * awn_applet_set_path_type:
 * @applet: an #AwnApplet.
 * @path: path type for this applet.
 *
 * Sets path type used by this applet. See awn_applet_get_offset_at().
 */
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

/**
 * awn_applet_get_offset:
 * @applet: an #AwnApplet.
 *
 * Gets current offset set for the applet. This value corresponds
 * to the value used by the associated panel.
 * @see_also: awn_applet_get_offset_at().
 *
 * Returns: current offset.
 */
gint
awn_applet_get_offset (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), 0);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->offset;
}

/**
 * awn_applet_set_offset:
 * @applet: an #AwnApplet.
 * @offset: new offset for this applet.
 *
 * Sets offset used by this applet. Note that setting the offset emits the
 * #AwnApplet::offset-changed signal.
 */
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

/**
 * awn_applet_get_offset_at:
 * @applet: an #AwnApplet.
 * @x: X-coordinate.
 * @y: Y-coordinate.
 *
 * @see_also: awn_applet_set_path_type().
 *
 * Gets offset for widget with [@x, @y] coordinates with respect to the current
 * path type.
 *
 * Returns: offset which should have the widget with [x, y] coordinates.
 */
gint
awn_applet_get_offset_at (AwnApplet *applet, gint x, gint y)
{
  AwnAppletPrivate *priv;
  gint result;
  gfloat temp;

  g_return_val_if_fail (AWN_IS_APPLET (applet), 0);
  priv = applet->priv;

  temp = awn_utils_get_offset_modifier_by_path_type (priv->path_type,
                                                     priv->position,
                                                     priv->offset,
                                                     priv->offset_modifier,
                                                     priv->pos_x + x,
                                                     priv->pos_y + y,
                                                     priv->panel_width,
                                                     priv->panel_height);
  result = round (temp);
  return result;
}

/**
 * awn_applet_get_size:
 * @applet: an #AwnApplet.
 *
 * Gets the current size set for the applet. This value corresponds
 * to the value used by the associated panel.
 *
 * Returns: current size set for the applet.
 */
gint
awn_applet_get_size (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail(AWN_IS_APPLET (applet), 48);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->size;
}

/**
 * awn_applet_set_size:
 * @applet: an #AwnApplet.
 * @size: new size of the applet.
 *
 * Sets new size for the applet. Note that setting the size emits the
 * #AwnApplet::size-changed signal.
 */
void
awn_applet_set_size (AwnApplet *applet, gint size)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  priv->size = size;

  g_signal_emit (applet, _applet_signals[SIZE_CHANGED], 0, size);
}

/**
 * awn_applet_get_uid:
 * @applet: an #AwnApplet.
 *
 * Gets the unique ID for the applet.
 *
 * Returns: unique ID for the applet.
 */
const gchar *
awn_applet_get_uid (AwnApplet *applet)
{
  AwnAppletPrivate *priv;

  g_return_val_if_fail (AWN_IS_APPLET (applet), NULL);
  priv = AWN_APPLET_GET_PRIVATE (applet);

  return priv->uid;
}

/**
 * awn_applet_set_uid:
 * @applet: an #AwnApplet.
 * @uid: new unique ID for the applet.
 *
 * Sets new unique ID for the applet.
 */
void
awn_applet_set_uid (AwnApplet *applet, const gchar *uid)
{
  AwnAppletPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET (applet));
  g_return_if_fail (uid);
  priv = applet->priv;

  priv->uid = g_strdup (uid);
}

/**
 * awn_applet_set_behavior:
 * @applet: an #AwnApplet.
 * @flags: flags for this applet.
 *
 * Sets behavior flags for this applet. Note that setting the flags to
 * #AWN_APPLET_IS_SEPARATOR or #AWN_APPLET_IS_EXPANDER will send a DBus request
 * to the associated AwnPanel which will destroy the socket used by this applet.
 */
void
awn_applet_set_behavior (AwnApplet *applet, AwnAppletFlags flags)
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

/**
 * awn_applet_get_behavior:
 * @applet: an #AwnApplet.
 *
 * Gets the flags set for this applet.
 *
 * Returns: flags set for this applet.
 */
AwnAppletFlags 
awn_applet_get_behavior (AwnApplet *applet)
{
  g_return_val_if_fail (AWN_IS_APPLET (applet), AWN_APPLET_FLAGS_NONE);

  return applet->priv->flags;
}

/**
 * awn_applet_inhibit_autohide:
 * @applet: an #AwnApplet.
 * @reason: reason for the inhibit.
 *
 * Requests the associated AwnPanel to disable autohide (if the panel is
 * already hidden it will unhide) until a call to
 * awn_applet_uninhibit_autohide() with the returned ID is made.
 *
 * Returns: cookie ID which can be used in awn_applet_uninhibit_autohide().
 */
guint
awn_applet_inhibit_autohide (AwnApplet *applet, const gchar *reason)
{
  AwnAppletPrivate *priv;
  GError *error = NULL;
  guint ret = 0;

  g_return_val_if_fail (AWN_IS_APPLET (applet), 0);
  priv = applet->priv;

  g_return_val_if_fail (priv->proxy, 0);

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

/**
 * awn_applet_uninhibit_autohide:
 * @applet: an #AwnApplet.
 * @cookie: inhibit cookie returned by the call
 *          to awn_applet_inhibit_autohide().
 *
 * Uninhibits autohide of the associated AwnPanel.
 * See awn_applet_inhibit_autohide().
 */
void
awn_applet_uninhibit_autohide (AwnApplet *applet, guint cookie)
{
  AwnAppletPrivate *priv;
  GError *error = NULL;

  g_return_if_fail (AWN_IS_APPLET (applet));
  priv = applet->priv;

  g_return_if_fail (priv->proxy);

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

/**
 * awn_applet_docklet_request:
 * @applet: AwnApplet instance.
 * @min_size: Minimum size required.
 * @shrink: If true and the panel has greater size than requested, it will
 *          shrink to min_size. Otherwise current panel size will be allocated.
 * @expand: If true the embedded window will be allowed to expand, otherwise
 *          the window will be restricted to min_size.
 *
 * Requests docklet mode from the associated AwnPanel - all applets will be
 * hidden and only one window will be shown.
 *
 * Returns: non-zero window XID which can be passed to GtkPlug constructor, 
 * or zero if the call failed (or another application is currently using
 * docklet mode).
 */
GdkNativeWindow
awn_applet_docklet_request (AwnApplet *applet, gint min_size,
                            gboolean shrink, gboolean expand)
{
  AwnAppletPrivate *priv;
  GError *error = NULL;
  gint64 ret = 0;

  g_return_val_if_fail (AWN_IS_APPLET (applet), 0);
  priv = applet->priv;

  g_return_val_if_fail (priv->proxy, 0);

  dbus_g_proxy_call (priv->proxy, "DockletRequest",
                     &error,
                     G_TYPE_INT, min_size,
                     G_TYPE_BOOLEAN, shrink,
                     G_TYPE_BOOLEAN, expand,
                     G_TYPE_INVALID,
                     G_TYPE_INT64, &ret,
                     G_TYPE_INVALID);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  return (GdkNativeWindow)ret;
}

