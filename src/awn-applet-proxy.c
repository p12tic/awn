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
#include <glib/gi18n.h>
#include <libawn/awn-effects.h>

#include "awn-applet-proxy.h"
#include "awn-utils.h"
#include "awn-throbber.h"

G_DEFINE_TYPE (AwnAppletProxy, awn_applet_proxy, GTK_TYPE_SOCKET) 

#define AWN_APPLET_PROXY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_APPLET_PROXY, AwnAppletProxyPrivate))

#define APPLET_EXEC "awn-applet-activation -p %s -u %s -w %lld -o %d -f %d -s %d"

struct _AwnAppletProxyPrivate
{
  gchar *path;
  gchar *uid;
  gint   orient;
  gint   offset;
  gint   size;

  gboolean running;
  gboolean crashed;
  GtkWidget *throbber;
};

enum
{
  PROP_0,
  PROP_PATH,
  PROP_UID,
  PROP_ORIENT,
  PROP_OFFSET,
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
static void     on_plug_added   (AwnAppletProxy *proxy, gpointer user_data);
static gboolean on_plug_removed (AwnAppletProxy *proxy, gpointer user_data);
static void     on_child_exit   (GPid pid, gint status, gpointer user_data);

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
    case PROP_OFFSET:
      g_value_set_int (value, priv->offset);
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
      awn_throbber_set_orientation (AWN_THROBBER (priv->throbber),
                                    priv->orient);
      break;
    case PROP_OFFSET:
      priv->offset = g_value_get_int (value);
      awn_throbber_set_offset (AWN_THROBBER (priv->throbber), priv->offset);
      break;
    case PROP_SIZE:
      priv->size = g_value_get_int (value);
      awn_throbber_set_size (AWN_THROBBER (priv->throbber), priv->size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
awn_applet_proxy_dispose (GObject *object)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE (object);

  g_free (priv->path);
  g_free (priv->uid);

  priv->path = NULL;
  priv->uid = NULL;

  if (priv->throbber)
  {
    gtk_widget_destroy (priv->throbber);
    priv->throbber = NULL;
  }

  G_OBJECT_CLASS (awn_applet_proxy_parent_class)->dispose (object);
}

static void
awn_applet_proxy_class_init (AwnAppletProxyClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  obj_class->dispose      = awn_applet_proxy_dispose;
  obj_class->set_property = awn_applet_proxy_set_property;
  obj_class->get_property = awn_applet_proxy_get_property;

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
        0, 3, AWN_ORIENTATION_BOTTOM,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (obj_class,
      PROP_OFFSET,
      g_param_spec_int ("offset",
        "Offset",
        "The panel icon offset",
        0, G_MAXINT, 0,
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

static gboolean
throbber_click (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE(user_data);

  if (!priv->running)
  {
    priv->crashed = FALSE;
    awn_throbber_set_text (AWN_THROBBER (priv->throbber),
                           _("Loading applet..."));
    awn_throbber_set_type (AWN_THROBBER (priv->throbber),
                           AWN_THROBBER_TYPE_NORMAL);
    awn_applet_proxy_execute (AWN_APPLET_PROXY(user_data));
  }
  return FALSE;
}

static gboolean
throbber_mouse_over (GtkWidget *widget,
                     GdkEventCrossing *event, gpointer user_data)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE(user_data);

  if (priv->crashed)
  {
    AwnEffects *fx = awn_throbber_get_effects (AWN_THROBBER (priv->throbber));
    awn_effects_start (fx, AWN_EFFECT_HOVER);
  }
  return FALSE;
}

static gboolean
throbber_mouse_out (GtkWidget *widget,
                    GdkEventCrossing *event, gpointer user_data)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE(user_data);

  AwnEffects *fx = awn_throbber_get_effects (AWN_THROBBER (priv->throbber));
  awn_effects_stop (fx, AWN_EFFECT_HOVER);

  return FALSE;
}

static void
awn_applet_proxy_init (AwnAppletProxy *proxy)
{
  AwnAppletProxyPrivate *priv;

  priv = proxy->priv = AWN_APPLET_PROXY_GET_PRIVATE (proxy);

  /* Connect to the socket signals */
  g_signal_connect (proxy, "plug-added", G_CALLBACK (on_plug_added), NULL);
  g_signal_connect (proxy, "plug-removed", G_CALLBACK (on_plug_removed), NULL);
  awn_utils_ensure_tranparent_bg (GTK_WIDGET (proxy));
  /* Rest is for the crash notification window */
  priv->running = TRUE;
  priv->crashed = FALSE;

  priv->throbber = awn_throbber_new ();

  awn_throbber_set_text (AWN_THROBBER (priv->throbber), _("Loading applet..."));

  g_signal_connect (priv->throbber, "button-release-event",
                    G_CALLBACK (throbber_click), proxy);
  g_signal_connect (priv->throbber, "enter-notify-event",
                    G_CALLBACK (throbber_mouse_over), proxy);
  g_signal_connect (priv->throbber, "leave-notify-event",
                    G_CALLBACK (throbber_mouse_out), proxy);
}

GtkWidget*
awn_applet_proxy_get_throbber(AwnAppletProxy *proxy)
{
  g_return_val_if_fail(AWN_IS_APPLET_PROXY(proxy), NULL);

  return proxy->priv->throbber;
}

GtkWidget *     
awn_applet_proxy_new (const gchar *path,
    const gchar *uid,
    gint         orient,
    gint         offset,
    gint         size)
{
  GtkWidget *proxy;

  proxy = g_object_new (AWN_TYPE_APPLET_PROXY,
      "path", path,
      "uid", uid,
      "orient", orient,
      "offset", offset,
      "size", size,
      NULL);
  return proxy;
}

/*
 * GtkSocket callbacks
 */
static void 
on_plug_added (AwnAppletProxy *proxy, gpointer user_data)
{
  g_return_if_fail (AWN_IS_APPLET_PROXY (proxy));

  gtk_widget_hide (GTK_WIDGET (proxy->priv->throbber));
  gtk_widget_show (GTK_WIDGET (proxy));
}

static gboolean
on_plug_removed (AwnAppletProxy *proxy, gpointer user_data)
{
  AwnAppletProxyPrivate *priv;

  g_return_val_if_fail (AWN_IS_APPLET_PROXY (proxy), FALSE);
  priv = proxy->priv;

  g_signal_emit (proxy, _proxy_signals[APPLET_DELETED], 0, priv->uid);

  gtk_widget_hide (GTK_WIDGET (proxy));
  /* indicate that the applet crashed and allow restart */
  priv->running = FALSE;
  priv->crashed = TRUE;
  awn_throbber_set_text (AWN_THROBBER (priv->throbber),
    _("Whoops! The applet crashed. Click to restart it."));
  awn_throbber_set_type (AWN_THROBBER (priv->throbber),
                         AWN_THROBBER_TYPE_SAD_FACE);
  gtk_widget_show (priv->throbber);

  return TRUE;
}

static void
on_child_exit (GPid pid, gint status, gpointer user_data)
{
  if (AWN_IS_APPLET_PROXY (user_data))
  {
    AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE (user_data);

    /* FIXME: we could do something with the status var... nice error messages?! */
    /*
    switch (status)
    {
      case ???:
        awn_throbber_set_text (AWN_THROBBER (priv->throbber), _("..."));
        break;
      default:
        awn_throbber_set_text (AWN_THROBBER (priv->throbber), _("..."));
        break;
    }
    */

    priv->running = FALSE;
    priv->crashed = TRUE;

    awn_throbber_set_type (AWN_THROBBER (priv->throbber),
                           AWN_THROBBER_TYPE_SAD_FACE);
    awn_throbber_set_text (AWN_THROBBER (priv->throbber),
      _("Whoops! The applet crashed. Click to restart it."));
    /* we won't call gtk_widget_show - on_plug_removed does that
     * and if the plug wasn't even added, the throbber widget is still visible
     */
  }

  g_spawn_close_pid(pid); /* doesn't do anything on UNIX, but let's have it */
}

void
awn_applet_proxy_execute (AwnAppletProxy *proxy)
{
  AwnAppletProxyPrivate *priv;
  GdkScreen             *screen;
  GError                *error = NULL;
  gchar                 *exec;
  gchar                **argv = NULL;
  GPid                   pid;
  GSpawnFlags            flags = G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD;

  priv = AWN_APPLET_PROXY_GET_PRIVATE (proxy);

  gtk_widget_realize (GTK_WIDGET (proxy));

  g_debug ("Loading Applet: %s %s", priv->path, priv->uid);

  /* FIXME: update tooltip with name of the applet?! */

  /* Load the applet */
  screen = gtk_widget_get_screen (GTK_WIDGET (proxy));
  exec = g_strdup_printf (APPLET_EXEC,
                          priv->path,
                          priv->uid, 
                          (long long)gtk_socket_get_id (GTK_SOCKET (proxy)),
                          priv->orient,
                          priv->offset,
                          priv->size);

  
  g_shell_parse_argv(exec, NULL, &argv, &error);
  g_warn_if_fail(error == NULL);

  if (gdk_spawn_on_screen (
        screen, NULL, argv, NULL, flags, NULL, NULL, &pid, &error))
  {
    priv->running = TRUE;
    g_child_watch_add(pid, on_child_exit, proxy);
  }
  else
  {
    g_warning ("Unable to load applet %s: %s", priv->path, error->message);
    g_error_free (error);
    g_signal_emit (proxy, _proxy_signals[APPLET_DELETED], 0, priv->uid);
  }

  g_strfreev (argv);
  g_free (exec);
}


