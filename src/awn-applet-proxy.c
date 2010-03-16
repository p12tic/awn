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
#include <gdk/gdkx.h>
#include <libawn/libawn.h>
#include <libawn/awn-utils.h>

#include "awn-applet-proxy.h"
#include "awn-throbber.h"
#include "libawn/gseal-transition.h"

G_DEFINE_TYPE (AwnAppletProxy, awn_applet_proxy, GTK_TYPE_SOCKET) 

#define AWN_APPLET_PROXY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, \
  AWN_TYPE_APPLET_PROXY, AwnAppletProxyPrivate))

#define APPLET_EXEC "awn-applet -p %s -u %s -w %" G_GINT64_FORMAT " -i %d"

#define DEBUG_APPLET_EXEC "gdb -ex run -ex bt --batch --args " APPLET_EXEC

#define APPLY_SIZE_MULTIPLIER(x)	(x)*6/5

struct _AwnAppletProxyPrivate
{
  gchar *path;
  gchar *uid;
  gint   position;
  gint   offset;
  gint   size;

  gboolean running;
  gboolean crashed;
  gboolean size_req_initialized;
  GtkWidget *throbber;

  gint old_x, old_y, old_w, old_h;
  guint idle_id;
};

enum
{
  PROP_0,
  PROP_PATH,
  PROP_UID,
  PROP_POSITION,
  PROP_OFFSET,
  PROP_SIZE
};

enum
{
  APPLET_CRASHED,

  LAST_SIGNAL
};
static guint _proxy_signals[LAST_SIGNAL] = { 0 };

/* 
 * FORWARDS
 */
static gboolean on_plug_removed (AwnAppletProxy *proxy, gpointer user_data);
static void     on_size_alloc   (AwnAppletProxy *proxy, GtkAllocation *a);
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
      g_value_set_string (value, priv->uid);
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
    case PROP_POSITION:
      priv->position = g_value_get_int (value);
      awn_icon_set_pos_type (AWN_ICON (priv->throbber), priv->position);
      break;
    case PROP_OFFSET:
      priv->offset = g_value_get_int (value);
      awn_icon_set_offset (AWN_ICON (priv->throbber), priv->offset);
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
awn_applet_proxy_size_request (GtkWidget *widget, GtkRequisition *req)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE (widget);

  // call base.size_request()
  GTK_WIDGET_CLASS (awn_applet_proxy_parent_class)->size_request (widget, req);

  if (!priv->size_req_initialized && req->width == 1 && req->height == 1)
  {
    // to prevent flicker we set the size request to the same value 
    //   as AwnThrobber uses
    switch (priv->position)
    {
      case GTK_POS_LEFT:
      case GTK_POS_RIGHT:
        req->height = APPLY_SIZE_MULTIPLIER (priv->size);
        break;
      case GTK_POS_BOTTOM:
      case GTK_POS_TOP:
      default:
        req->width = APPLY_SIZE_MULTIPLIER (priv->size);
        break;
    }
  }
  else if (!priv->size_req_initialized)
  {
    priv->size_req_initialized = TRUE;
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

  if (priv->idle_id)
  {
    g_source_remove (priv->idle_id);
    priv->idle_id = 0;
  }

  G_OBJECT_CLASS (awn_applet_proxy_parent_class)->dispose (object);
}

static void
awn_applet_proxy_class_init (AwnAppletProxyClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  obj_class->dispose      = awn_applet_proxy_dispose;
  obj_class->set_property = awn_applet_proxy_set_property;
  obj_class->get_property = awn_applet_proxy_get_property;

  widget_class->size_request = awn_applet_proxy_size_request;

  /* Install class properties */
  g_object_class_install_property (obj_class,
      PROP_PATH,
      g_param_spec_string ("path",
        "Path",
        "The path to the applets desktop file",
        NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
      PROP_UID,
      g_param_spec_string ("uid",
        "UID",
        "The unique ID for this applet instance",
        NULL,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
      PROP_POSITION,
      g_param_spec_int ("position",
        "Position",
        "The panel position",
        0, 3, GTK_POS_BOTTOM,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
      PROP_OFFSET,
      g_param_spec_int ("offset",
        "Offset",
        "The panel icon offset",
        0, G_MAXINT, 0,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
      PROP_SIZE,
      g_param_spec_int ("size",
        "size",
        "The panel size",
        0, G_MAXINT, 48,
        G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));

  /* Class signals */
  _proxy_signals[APPLET_CRASHED] =
    g_signal_new ("applet-crashed",
        G_OBJECT_CLASS_TYPE (obj_class),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (AwnAppletProxyClass, applet_crashed),
        NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

  g_type_class_add_private (obj_class, sizeof (AwnAppletProxyPrivate));
}

static gboolean
throbber_click (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE(user_data);

  if (!priv->running)
  {
    priv->crashed = FALSE;
    awn_icon_set_tooltip_text (AWN_ICON (priv->throbber),
                               _("Loading applet..."));
    awn_throbber_set_type (AWN_THROBBER (priv->throbber),
                           AWN_THROBBER_TYPE_NORMAL);
    awn_icon_set_hover_effects (AWN_ICON (priv->throbber), FALSE);

    awn_applet_proxy_execute (AWN_APPLET_PROXY (user_data));
  }
  return FALSE;
}

static void
awn_applet_proxy_init (AwnAppletProxy *proxy)
{
  AwnAppletProxyPrivate *priv;

  priv = proxy->priv = AWN_APPLET_PROXY_GET_PRIVATE (proxy);

  /* Connect to the socket signals */
  g_signal_connect (proxy, "plug-removed", G_CALLBACK (on_plug_removed), NULL);
  g_signal_connect (proxy, "size-allocate", G_CALLBACK (on_size_alloc), NULL);
  awn_utils_ensure_transparent_bg (GTK_WIDGET (proxy));
  /* Rest is for the crash notification window */
  priv->running = TRUE;
  priv->crashed = FALSE;

  priv->throbber = awn_throbber_new_with_config (
      awn_config_get_default (0, NULL));

  awn_icon_set_tooltip_text (AWN_ICON (priv->throbber),
                             _("Loading applet..."));

  g_signal_connect (priv->throbber, "button-release-event",
                    G_CALLBACK (throbber_click), proxy);
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
                      gint         position,
                      gint         offset,
                      gint         size)
{
  GtkWidget *proxy;

  proxy = g_object_new (AWN_TYPE_APPLET_PROXY,
      "path", path,
      "uid", uid,
      "position", position,
      "offset", offset,
      "size", size,
      NULL);
  return proxy;
}

/*
 * GtkSocket callbacks
 */
static gboolean
on_plug_removed (AwnAppletProxy *proxy, gpointer user_data)
{
  AwnAppletProxyPrivate *priv;

  g_return_val_if_fail (AWN_IS_APPLET_PROXY (proxy), FALSE);
  priv = proxy->priv;

  /* reset our old position */
  priv->old_x = 0;
  priv->old_y = 0;
  priv->old_w = 0;
  priv->old_h = 0;

  /* indicate that the applet crashed and allow restart */
  priv->running = FALSE;
  priv->crashed = TRUE;
  awn_icon_set_tooltip_text (AWN_ICON (priv->throbber),
    _("Whoops! The applet crashed. Click to restart it."));
  awn_throbber_set_type (AWN_THROBBER (priv->throbber),
                         AWN_THROBBER_TYPE_SAD_FACE);
  awn_icon_set_hover_effects (AWN_ICON (priv->throbber), TRUE);

  g_signal_emit (proxy, _proxy_signals[APPLET_CRASHED], 0);

  return TRUE;
}

/* FIXME: should we schedule the event or not?
static gboolean
schedule_send_client_event (gpointer data)
{
  GdkEvent *event = (GdkEvent*) data;

  gdk_event_send_client_message (event, GDK_WINDOW_XID (event->client.window));

  gdk_event_free (event);

  return FALSE;
}
*/
static void on_size_alloc (AwnAppletProxy *proxy, GtkAllocation *alloc)
{
  AwnAppletProxyPrivate *priv;
  GtkWidget *parent;
  GtkAllocation parent_alloc;
  GdkWindow *plug_win;

  g_return_if_fail (AWN_IS_APPLET_PROXY (proxy));

  priv = proxy->priv;

  parent = gtk_widget_get_parent (GTK_WIDGET (proxy));
  gtk_widget_get_allocation (parent, &parent_alloc);

  gint pos_x = alloc->x;
  gint pos_y = alloc->y;
  gint rel_x = pos_x - parent_alloc.x;
  gint rel_y = pos_y - parent_alloc.y;
  gint parent_w = parent_alloc.width;
  gint parent_h = parent_alloc.height;

  if (pos_x == priv->old_x && pos_y == priv->old_y
      && parent_w == priv->old_w && parent_h == priv->old_h) return;

  priv->old_x = pos_x;
  priv->old_y = pos_y;
  priv->old_w = parent_w;
  priv->old_h = parent_h;

  /* Only directly access the struct member if we have to. */
  plug_win = gtk_socket_get_plug_window (GTK_SOCKET (proxy));
  if (plug_win)
  {
    GdkAtom msg_type = gdk_atom_intern("_AWN_APPLET_POS_CHANGE", FALSE);
    GdkEvent *event = gdk_event_new (GDK_CLIENT_EVENT);
    event->client.window = g_object_ref (plug_win);
    event->client.data_format = 32;
    event->client.message_type = msg_type;
    // first two longs are our relative [x, y]
    event->client.data.l[0] = rel_x;
    event->client.data.l[1] = rel_y;
    // other two longs are our parent's [w, h]
    event->client.data.l[2] = parent_w;
    event->client.data.l[3] = parent_h;

    gdk_event_send_client_message (event, GDK_WINDOW_XID (plug_win));

    gdk_event_free (event);
    /* g_idle_add (schedule_send_client_event, event); */
  }
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
    awn_icon_set_tooltip_text (AWN_ICON (priv->throbber),
      _("Whoops! The applet crashed. Click to restart it."));
    awn_icon_set_hover_effects (AWN_ICON (priv->throbber), TRUE);

    g_signal_emit (user_data, _proxy_signals[APPLET_CRASHED], 0);
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
  gint                   panel_id = AWN_PANEL_ID_DEFAULT;
  gchar                 *exec;
  gchar                **argv = NULL;
  GPid                   pid;
  GSpawnFlags            flags = G_SPAWN_SEARCH_PATH|G_SPAWN_DO_NOT_REAP_CHILD;

  priv = AWN_APPLET_PROXY_GET_PRIVATE (proxy);

  priv->size_req_initialized = FALSE;
  gtk_widget_realize (GTK_WIDGET (proxy));

  /* FIXME: update tooltip with name of the applet?! */

  /* Load the applet */
  screen = gtk_widget_get_screen (GTK_WIDGET (proxy));
  gint64 socket_id = (gint64) gtk_socket_get_id (GTK_SOCKET (proxy));

  g_object_get (G_OBJECT (gtk_widget_get_toplevel (GTK_WIDGET (proxy))),
                "panel-id", &panel_id, NULL);

  if (g_getenv ("AWN_APPLET_GDB"))
  {
    exec = g_strdup_printf (DEBUG_APPLET_EXEC, priv->path, priv->uid,
                            socket_id, panel_id);
  }
  else
  {
    exec = g_strdup_printf (APPLET_EXEC, priv->path, priv->uid, 
                            socket_id, panel_id);
  }

  g_shell_parse_argv (exec, NULL, &argv, &error);
  g_warn_if_fail (error == NULL);

  if (gdk_spawn_on_screen (
        screen, NULL, argv, NULL, flags, NULL, NULL, &pid, &error))
  {
    priv->running = TRUE;
    g_child_watch_add(pid, on_child_exit, proxy);

    gchar *desktop = g_path_get_basename (priv->path);
    g_debug ("Spawned awn-applet[%d] for \"%s\", UID: %s, XID: %" G_GINT64_FORMAT,
       pid, desktop, priv->uid, socket_id);
    g_free (desktop);
  }
  else
  {
    g_warning ("Unable to load applet %s: %s", priv->path, error->message);
    g_error_free (error);
    g_signal_emit (proxy, _proxy_signals[APPLET_CRASHED], 0);
  }

  g_strfreev (argv);
  g_free (exec);
}

static gboolean
awn_applet_proxy_idle_cb (gpointer data)
{
  g_return_val_if_fail (AWN_IS_APPLET_PROXY (data), FALSE);
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE (data);

  awn_applet_proxy_execute (AWN_APPLET_PROXY (data));

  priv->idle_id = 0;

  return FALSE;
}

void
awn_applet_proxy_schedule_execute (AwnAppletProxy *proxy)
{
  AwnAppletProxyPrivate *priv = AWN_APPLET_PROXY_GET_PRIVATE (proxy);

  if (priv->idle_id == 0)
  {
    priv->idle_id = g_idle_add (awn_applet_proxy_idle_cb, proxy);
  }
}

