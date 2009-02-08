/*
 * Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 * Authors: Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <string.h>

#include "awn-plug.h"

#include "awn-defines.h"
#include "awn-applet.h"

G_DEFINE_TYPE(AwnPlug, awn_plug, GTK_TYPE_PLUG);

#define AWN_PLUG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                   AWN_TYPE_PLUG, AwnPlugPrivate))

struct _AwnPlugPrivate
{
  AwnApplet *applet;

  DBusGConnection *connection;
  DBusGProxy      *proxy;
};

enum
{
  APPLET_DELETED,

  LAST_SIGNAL
};
static guint _plug_signals[LAST_SIGNAL] = { 0 };

/* FORWARDS */
static void on_applet_flags_changed (AwnApplet      *applet, 
                                     AwnAppletFlags  flags, 
                                     AwnPlug        *plug);

void
awn_plug_construct (AwnPlug *plug, GdkNativeWindow id)
{
  gtk_plug_construct (GTK_PLUG (plug), id);
}

/* CALLBACKS */
static void
on_plug_embedded (AwnPlug *plug)
{
  AwnPlugPrivate *priv;

  g_return_if_fail (AWN_IS_PLUG(plug));
  priv = plug->priv;

  awn_applet_plug_embedded (priv->applet);

  gtk_widget_show_all (GTK_WIDGET (plug));
}

static void
on_orient_changed (DBusGProxy *proxy, gint orient, AwnPlug *plug)
{
  AwnPlugPrivate *priv;

  g_return_if_fail (AWN_IS_PLUG (plug));
  priv = plug->priv;

  awn_applet_set_orientation (priv->applet, orient);
}

static void
on_size_changed (DBusGProxy *proxy, gint size, AwnPlug *plug)
{
  AwnPlugPrivate *priv;

  g_return_if_fail (AWN_IS_PLUG (plug));
  priv = plug->priv;

  awn_applet_set_size (priv->applet, size);
}

static void
on_delete_notify (DBusGProxy *proxy, AwnPlug *plug)
{
  gtk_main_quit ();
}

static void
on_proxy_destroyed (GObject *object)
{
  gtk_main_quit ();
}

static void
on_destroy_applet (DBusGProxy *proxy, gchar *id, AwnPlug *plug)
{
  AwnPlugPrivate *priv;
  const gchar *uid;
  g_return_if_fail (AWN_IS_PLUG (plug));
  priv = plug->priv;

  uid = awn_applet_get_uid (priv->applet);

  if (strcmp (uid, id) == 0)
    on_delete_notify (NULL, plug);
}

/*  GOBJECT STUFF */
void
on_alpha_screen_changed (GtkWidget* widget, GdkScreen* oscreen, gpointer null)
{
  GdkScreen* nscreen = gtk_widget_get_screen (widget);
  GdkColormap* colormap = gdk_screen_get_rgba_colormap (nscreen);

  if (!colormap)
    colormap = gdk_screen_get_rgb_colormap (nscreen);

  gtk_widget_set_colormap (widget, colormap);
}

static gboolean
awn_plug_make_transparent (GtkWidget *widget, gpointer data)
{
/*  AwnPlugPrivate *priv = AWN_PLUG (widget)->priv;*/

  /*
   * This is how we make sure that widget has transparent background
   * all the time.
   */
  if (gtk_widget_is_composited(widget)) // FIXME: is is_composited correct here?
  {
    static GdkPixmap *pixmap = NULL;
    if (pixmap == NULL)
    {
      pixmap = gdk_pixmap_new(widget->window, 1, 1, -1);
      cairo_t *cr = gdk_cairo_create(pixmap);
      cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(cr);
      cairo_destroy(cr);
    }
    gdk_window_set_back_pixmap(widget->window, pixmap, FALSE);

  }

  return FALSE;
}

static gboolean
awn_plug_expose_event (GtkWidget *widget, GdkEventExpose *expose)
{
  cairo_t *cr = NULL;

  if (!GDK_IS_DRAWABLE (widget->window))
    return FALSE;

  cr = gdk_cairo_create (widget->window);

  if (!cr)
    return FALSE;

  /* Clear the background to transparent */
  cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.0f);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);

  /* Clean up */
  cairo_destroy (cr);

  gtk_container_propagate_expose(GTK_CONTAINER (widget),
                                 gtk_bin_get_child (GTK_BIN (widget)),
                                 expose);

  return FALSE;
}

static void
awn_plug_finalize(GObject *obj)
{
  AwnPlugPrivate *priv = AWN_PLUG_GET_PRIVATE (obj);

  dbus_g_connection_unref (priv->connection);
  g_object_unref (priv->proxy);

  G_OBJECT_CLASS (awn_plug_parent_class)->finalize (obj);
}


static void
awn_plug_class_init(AwnPlugClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->finalize = awn_plug_finalize;

  wid_class->expose_event = awn_plug_expose_event;

  /* Add signals */
  _plug_signals[APPLET_DELETED] =
    g_signal_new ("applet_deleted",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (AwnPlugClass, applet_deleted),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1, G_TYPE_STRING);

  g_type_class_add_private (obj_class, sizeof (AwnPlugPrivate));
}

static void
awn_plug_init(AwnPlug *plug)
{
  AwnPlugPrivate *priv;
  GError         *error;

  priv = plug->priv = AWN_PLUG_GET_PRIVATE(plug);

  gtk_widget_set_app_paintable(GTK_WIDGET(plug), TRUE);
  on_alpha_screen_changed(GTK_WIDGET(plug), NULL, NULL);

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
                                           "/org/awnproject/Awn/Panel",
                                           "org.awnproject.Awn.Panel");
  if (!priv->proxy)
  {
    g_warning("Could not connect to mothership! Bailing\n");
    gtk_main_quit ();
  }

  dbus_g_proxy_add_signal (priv->proxy, "OrientChanged",
                           G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "SizeChanged",
                           G_TYPE_INT, G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "DestroyNotify",
                           G_TYPE_INVALID);
  dbus_g_proxy_add_signal (priv->proxy, "DestroyApplet",
                           G_TYPE_STRING, G_TYPE_INVALID);
  
  dbus_g_proxy_connect_signal (priv->proxy, "OrientChanged",
                               G_CALLBACK (on_orient_changed), plug, 
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "SizeChanged",
                               G_CALLBACK (on_size_changed), plug,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "DestroyNotify",
                               G_CALLBACK (on_delete_notify), plug,
                               NULL);
  dbus_g_proxy_connect_signal (priv->proxy, "DestroyApplet",
                               G_CALLBACK (on_destroy_applet), plug,
                               NULL);

  g_signal_connect (priv->proxy, "destroy", 
                    G_CALLBACK (on_proxy_destroyed), NULL);
  g_signal_connect (plug, "embedded",
                    G_CALLBACK (on_plug_embedded), NULL);
  g_signal_connect_after(G_OBJECT(plug), "realize",
                         G_CALLBACK(awn_plug_make_transparent), NULL);  
}

GtkWidget *
awn_plug_new (AwnApplet *applet)
{
  AwnPlug *plug;

  plug = g_object_new (AWN_TYPE_PLUG, NULL);

  /* This is ugly, we need to make the applet a property */
  plug->priv->applet = applet;
  g_signal_connect (applet, "flags-changed", 
                    G_CALLBACK (on_applet_flags_changed), plug);

  on_applet_flags_changed (applet, awn_applet_get_flags (applet), plug);

  return GTK_WIDGET (plug);
}

static void
on_applet_flags_changed (AwnApplet *applet, AwnAppletFlags flags, AwnPlug *plug)
{
  AwnPlugPrivate *priv;
  GError *error = NULL;

  g_return_if_fail (AWN_IS_PLUG (plug));
  priv = plug->priv;

  dbus_g_proxy_call (priv->proxy, "SetAppletFlags",
                        &error,
                        G_TYPE_STRING, awn_applet_get_uid (AWN_APPLET (applet)),                        G_TYPE_INT, flags, 
                        G_TYPE_INVALID, G_TYPE_INVALID);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }
}
