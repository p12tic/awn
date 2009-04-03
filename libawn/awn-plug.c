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
 * Authors: Neil J. Patel <njpatel@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-plug.h"

#include "awn-defines.h"
#include "awn-applet.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <string.h>

G_DEFINE_TYPE(AwnPlug, awn_plug, GTK_TYPE_PLUG);

#define AWN_PLUG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                   AWN_TYPE_PLUG, \
                                   AwnPlugPrivate))

static GtkEventBoxClass *parent_class;

/* STRUCTS & ENUMS */

struct _AwnPlugPrivate
{
  AwnApplet        *applet;

  DBusGConnection        *connection;
  DBusGProxy        *proxy;
};

enum
{
  APPLET_DELETED,

  LAST_SIGNAL
};

static guint _plug_signals[LAST_SIGNAL] = { 0 };

/* FORWARDS */

static void awn_plug_class_init(AwnPlugClass *klass);
static void awn_plug_init(AwnPlug *plug);
static void awn_plug_finalize(GObject *obj);

/* CALLBACKS */
void
awn_plug_construct(AwnPlug *plug, GdkNativeWindow socket_id)
{
  gtk_plug_construct(GTK_PLUG(plug), socket_id);
}

static void
on_plug_embedded(GtkPlug *lug, AwnPlug *plug)
{
  AwnPlugPrivate *priv;

  g_return_if_fail(AWN_IS_PLUG(plug));
  priv = AWN_PLUG_GET_PRIVATE(plug);

  /*
    if (!(priv->init_func (AWN_APPLET (priv->applet)))) {
    g_warning ("Unable to embed applet\n");
    return;
   }
  */
  gtk_widget_show_all(GTK_WIDGET(plug));

  // notify the applet that we started
// printf("[AWNPLUG] applet type =  %s\n", g_type_name(G_OBJECT_TYPE(priv->applet)));
  AWN_APPLET_GET_CLASS(priv->applet)->plug_embedded(priv->applet);
}

static void
on_orient_changed(DBusGProxy *proxy, gint orient, AwnPlug *plug)
{
  AwnPlugPrivate *priv;

  g_return_if_fail(AWN_IS_PLUG(plug));
  priv = AWN_PLUG_GET_PRIVATE(plug);

  g_object_set(priv->applet, "orient", orient, NULL);
}

static void
on_height_changed(DBusGProxy *proxy, gint height, AwnPlug *plug)
{
  AwnPlugPrivate *priv;

  g_return_if_fail(AWN_IS_PLUG(plug));
  priv = AWN_PLUG_GET_PRIVATE(plug);

  g_object_set(priv->applet, "height", height, NULL);
}


static void
on_size_changed(DBusGProxy *proxy, gint x, AwnPlug *plug)
{
  AwnPlugPrivate *priv = AWN_PLUG_GET_PRIVATE(plug);
/* Disable size_changed (see LP bug https://launchpad.net/bugs/351433 )
  AWN_APPLET_GET_CLASS(priv->applet)->size_changed(priv->applet, x); */
}

static void
on_delete_notify(DBusGProxy *proxy, AwnPlug *plug)
{
  gtk_main_quit();
}

static void
on_destroy_applet(DBusGProxy *proxy, gchar *id, AwnPlug *plug)
{
  AwnPlugPrivate *priv;
  const gchar *uid;
  g_return_if_fail(AWN_IS_PLUG(plug));
  priv = AWN_PLUG_GET_PRIVATE(plug);

  g_object_get(G_OBJECT(priv->applet), "uid", &uid, NULL);

  if (strcmp(uid, id) == 0)
    on_delete_notify(NULL, plug);
}

/*
static void
on_applet_deleted (AwnApplet *applet, gchar *uid, AwnPlug *plug)
{
 ;
}
*/

/*  GOBJECT STUFF */

void
on_alpha_screen_changed(GtkWidget* widget, GdkScreen* oscreen, gpointer null)
{
  GdkScreen* nscreen = gtk_widget_get_screen(widget);
  GdkColormap* colormap = gdk_screen_get_rgba_colormap(nscreen);

  if (!colormap)
    colormap = gdk_screen_get_rgb_colormap(nscreen);

  gtk_widget_set_colormap(widget, colormap);
}

static gboolean
awn_plug_expose_event(GtkWidget *widget, GdkEventExpose *expose)
{
  cairo_t *cr = NULL;

  if (!GDK_IS_DRAWABLE(widget->window))
    return FALSE;

  cr = gdk_cairo_create(widget->window);

  if (!cr)
    return FALSE;

  /* Clear the background to transparent */
  cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.0f);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);

  cairo_paint(cr);

  /* Clean up */
  cairo_destroy(cr);

  gtk_container_propagate_expose(GTK_CONTAINER(widget),
                                 gtk_bin_get_child(GTK_BIN(widget)),
                                 expose);

  return FALSE;
}

static void
awn_plug_class_init(AwnPlugClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  parent_class = g_type_class_peek_parent(klass);

  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = awn_plug_finalize;

  widget_class = GTK_WIDGET_CLASS(klass);
  widget_class->expose_event = awn_plug_expose_event;

  _plug_signals[APPLET_DELETED] =
    g_signal_new("applet_deleted",
                 G_OBJECT_CLASS_TYPE(klass),
                 G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(AwnPlugClass, applet_deleted),
                 NULL, NULL,
                 g_cclosure_marshal_VOID__STRING,
                 G_TYPE_NONE,
                 1, G_TYPE_STRING);

  g_type_class_add_private(gobject_class, sizeof(AwnPlugPrivate));
}

static void
awn_plug_init(AwnPlug *plug)
{
  AwnPlugPrivate *priv;
  GError *error;

  priv = AWN_PLUG_GET_PRIVATE(plug);

  gtk_widget_set_app_paintable(GTK_WIDGET(plug), TRUE);
  on_alpha_screen_changed(GTK_WIDGET(plug), NULL, NULL);

  error = NULL;
  priv->connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

  if (error)
  {
    g_error_free(error);
  }

  if (priv->connection != NULL)
  {
    priv->proxy = dbus_g_proxy_new_for_name(
                    priv->connection,
                    "com.google.code.Awn.AppletManager",
                    "/com/google/code/Awn/AppletManager",
                    "com.google.code.Awn.AppletManager");

    if (!priv->proxy)
    {
      g_warning("Could not connect to mothership! Bailing\n");
      return;
    }

    dbus_g_proxy_add_signal(priv->proxy, "OrientChanged",

                            G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal(priv->proxy, "HeightChanged",
                            G_TYPE_INT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal(priv->proxy, "DestroyNotify",
                            G_TYPE_INVALID);
    dbus_g_proxy_add_signal(priv->proxy, "DestroyApplet",
                            G_TYPE_STRING, G_TYPE_INVALID);
    dbus_g_proxy_add_signal(priv->proxy, "SizeChanged",
                            G_TYPE_INT, G_TYPE_INVALID);

    dbus_g_proxy_connect_signal(priv->proxy,
                                "OrientChanged",
                                G_CALLBACK(on_orient_changed),
                                (gpointer)plug,
                                NULL);
    dbus_g_proxy_connect_signal(priv->proxy,
                                "HeightChanged",
                                G_CALLBACK(on_height_changed),
                                (gpointer)plug,
                                NULL);
    dbus_g_proxy_connect_signal(priv->proxy,
                                "DestroyNotify",
                                G_CALLBACK(on_delete_notify),
                                (gpointer)plug,
                                NULL);

    dbus_g_proxy_connect_signal(priv->proxy,
                                "DestroyApplet",
                                G_CALLBACK(on_destroy_applet),
                                (gpointer)plug,
                                NULL);
    dbus_g_proxy_connect_signal(priv->proxy,
                                "SizeChanged",
                                G_CALLBACK(on_size_changed),
                                (gpointer)plug,
                                NULL);
  }
}



static void
awn_plug_finalize(GObject *obj)
{
  AwnPlug *plug;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(AWN_IS_PLUG(obj));

  plug = AWN_PLUG(obj);

  if (G_OBJECT_CLASS(parent_class)->finalize)
    G_OBJECT_CLASS(parent_class)->finalize(obj);
}

GtkWidget *
awn_plug_new(AwnApplet *applet)
{
  AwnPlugPrivate *priv;

  GtkWidget *plug = g_object_new(AWN_TYPE_PLUG, NULL);

  priv = AWN_PLUG_GET_PRIVATE(plug);

  priv->applet = applet;
  g_signal_connect(GTK_PLUG(plug), "embedded",
                   G_CALLBACK(on_plug_embedded), (gpointer)plug);
  return plug;
}

