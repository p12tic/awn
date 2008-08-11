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

#include "awn-applet-proxy.h"

G_DEFINE_TYPE (AwnAppletProxy, awn_applet_proxy, GTK_TYPE_EVENT_BOX) 

#define AWN_APPLET_PROXY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_APPLET_PROXY, AwnAppletProxyPrivate))

#define APPLET_EXEC "awn-applet-activation -p %s -u %s -w %lld -o %d -h %d"

struct _AwnAppletProxyPrivate
{
  GtkWidget *socket;

  gchar *path;
  gchar *uid;
  gint   orient;
  gint   size;
};

enum
{
  PROP_0,
  PROP_PATH,
  PROP_UID,
  PROP_ORIENT,
  PROP_SIZE
};

enum
{
  APPLET_DELETED,

  LAST_SIGNAL
};
static guint _proxy_signals[LAST_SIGNAL] = { 0 };

/* 
 * FORWARDS
 */

/*
 * GOBJECT CODE 
 */
static void
awn_applet_proxy_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  AwnAppletProxyPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_PROXY (object));
  priv = AWN_APPLET_PROXY (object)->priv;

  switch (prop_id)
  {
    case PROP_PATH:
      g_value_set_string (value, priv->path);
      break;
    case PROP_UID:
      g_value_set_string (value, priv->path);
      break;
    case PROP_ORIENT:
      g_value_set_int (value, priv->orient);
      break;
    case PROP_SIZE:
      g_value_set_int (value, priv->size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_applet_proxy_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  AwnAppletProxyPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_PROXY (object));
  priv = AWN_APPLET_PROXY (object)->priv;

  switch (prop_id)
  {
    case PROP_PATH:
      priv->path = g_value_dup_string (value);
      break;
    case PROP_UID:
      priv->uid = g_value_dup_string (value);
      break;
    case PROP_ORIENT:
      priv->orient = g_value_get_int (value);
      break;
    case PROP_SIZE:
      priv->size = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static gboolean
awn_applet_proxy_expose (GtkWidget *widget, GdkEventExpose *event)
{
  AwnAppletProxyPrivate *priv;
  cairo_t         *cr;
  GtkWidget       *child;

  priv = AWN_APPLET_PROXY_GET_PRIVATE (widget);

  if (!GDK_IS_DRAWABLE (widget->window))
  {
    g_debug ("!GDK_IS_DRAWABLE (widget->window) failed");
    return FALSE;
  }

  /* Get our ctx */
  cr = gdk_cairo_create (widget->window);
  if (!cr)
  {
    g_debug ("Unable to create cairo context\n");
    return FALSE;
  }

  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);
  
  /* The actual drawing of the background */
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
 
  /* Pass on the expose event to the child */
  child = gtk_bin_get_child (GTK_BIN (widget));
  if (!GTK_IS_WIDGET (child))
    return TRUE;

  if (1)
  {
    GdkRegion *region;
    
    gdk_cairo_set_source_pixmap (cr, child->window,
                                 child->allocation.x, 
                                 child->allocation.y);

    region = gdk_region_rectangle (&child->allocation);
    gdk_region_intersect (region, event->region);
    gdk_cairo_region (cr, region);
    cairo_clip (cr);
    
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_paint (cr);
  }

  cairo_destroy (cr);

  gtk_container_propagate_expose (GTK_CONTAINER (widget),
                                  child,
                                  event);

  return TRUE;
}

static void
awn_applet_proxy_dispose (GObject *object)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE (object);

  g_free (priv->path);
  g_free (priv->uid);

  G_OBJECT_CLASS (awn_applet_proxy_parent_class)->dispose (object);
}

static void
awn_applet_proxy_class_init (AwnAppletProxyClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->dispose      = awn_applet_proxy_dispose;
  obj_class->set_property = awn_applet_proxy_set_property;
  obj_class->get_property = awn_applet_proxy_get_property;

  wid_class->expose_event = awn_applet_proxy_expose;

  /* Install class properties */
  g_object_class_install_property (obj_class,
    PROP_PATH,
    g_param_spec_string ("path",
                         "Path",
                         "The path to the applets desktop file",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_UID,
    g_param_spec_string ("uid",
                         "UID",
                         "The unique ID for this applet instance",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_ORIENT,
    g_param_spec_int ("orient",
                      "Orient",
                      "The panel orientation",
                      0, 3, AWN_ORIENT_BOTTOM,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
    PROP_SIZE,
    g_param_spec_int ("size",
                      "size",
                      "The panel size",
                      0, G_MAXINT, 48,
                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  /* Class signals */
  _proxy_signals[APPLET_DELETED] =
		g_signal_new ("applet_deleted",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnAppletProxyClass, applet_deleted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT, 
			      G_TYPE_NONE,
			      1, G_TYPE_INT);
  
     
  g_type_class_add_private (obj_class, sizeof (AwnAppletProxyPrivate));
}

static void
awn_applet_proxy_init (AwnAppletProxy *proxy)
{
  AwnAppletProxyPrivate *priv;

  priv = proxy->priv = AWN_APPLET_PROXY_GET_PRIVATE (proxy);

  priv->socket = gtk_socket_new ();
  gtk_container_add (GTK_CONTAINER (proxy), priv->socket);
}


GtkWidget *     
awn_applet_proxy_new (const gchar *path,
                      const gchar *uid,
                      gint         orient,
                      gint         size)
{
  GtkWidget *proxy;

  proxy = g_object_new (AWN_TYPE_APPLET_PROXY,
                        "path", path,
                        "uid", uid,
                        "orient", orient,
                        "size", size,
                        NULL);
  return proxy;
}

/*
 * GtkSocket callbacks
 */
static void 
on_plug_added (GtkWidget *socket, AwnAppletProxy *proxy)
{
  g_return_if_fail (AWN_IS_APPLET_PROXY (proxy));

  gtk_widget_show_all (GTK_WIDGET (proxy));
}

static void 
on_plug_removed (GtkWidget *socket, AwnAppletProxy *proxy)
{
  AwnAppletProxyPrivate *priv;

  g_return_if_fail (AWN_IS_APPLET_PROXY (proxy));
  priv = proxy->priv;

  g_signal_emit (proxy, _proxy_signals[APPLET_DELETED], 0, priv->uid);
}

void
awn_applet_proxy_execute (AwnAppletProxy *proxy)
{
  AwnAppletProxyPrivate *priv;
  GtkWidget             *socket;
  GdkScreen             *screen;
  GError                *error = NULL;
  gchar                 *exec;

  priv = AWN_APPLET_PROXY_GET_PRIVATE (proxy);

  socket = priv->socket;

  gtk_widget_realize (GTK_WIDGET (socket));
  gdk_window_set_composited (priv->socket->window, TRUE);

  /* Connect to the socket signals */
  g_signal_connect (socket, "plug-added", 
                    G_CALLBACK (on_plug_added), proxy);
  g_signal_connect (socket, "plug-removed", 
                    G_CALLBACK (on_plug_removed),proxy);

  g_debug ("Loading Applet: %s %s", priv->path, priv->uid);

  /* Load the applet */
  screen = gtk_widget_get_screen (GTK_WIDGET (proxy));
  exec = g_strdup_printf (APPLET_EXEC,
                          priv->path,
                          priv->uid, 
                          (long long)gtk_socket_get_id (GTK_SOCKET (socket)),
                          priv->orient,
                          priv->size);
  gdk_spawn_command_line_on_screen (screen, exec, &error);

  if (error)
  {
    g_warning ("Unable to load applet %s: %s", priv->path, error->message);
    g_error_free (error);
    g_signal_emit (proxy, _proxy_signals[APPLET_DELETED], 0, priv->uid);
  }

  g_free (error);
}


