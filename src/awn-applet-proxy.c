/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Neil J. Patel <njpatel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Neil J. Patel <njpatel@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-applet-proxy.h"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include <libawn/awn-defines.h>
#include <libawn/awn-applet.h>

G_DEFINE_TYPE (AwnAppletProxy, awn_applet_proxy, GTK_TYPE_SOCKET);

#define AWN_APPLET_PROXY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
	AWN_TYPE_APPLET_PROXY, \
	AwnAppletProxyPrivate))

static GtkEventBoxClass *parent_class;

/* STRUCTS & ENUMS */
struct _AwnAppletProxyPrivate
{
	DBusGConnection *connection;
	DBusGProxy 	*proxy;

	gchar		*path;
	gchar 		*uid;
	gchar 		*gconf_key;
	AwnOrientation 	 orient;
	guint 		 height;
	
};

enum
{
	PROP_0,
	PROP_PATH,
	PROP_UID,
	PROP_ORIENT,
	PROP_HEIGHT
};


enum
{
	SIGNAL_0,
	APPLET_DELETED,

	LAST_SIGNAL
};

static guint _proxy_signals[LAST_SIGNAL] = { 0 };

/* FORWARDS */

static void awn_applet_proxy_class_init(AwnAppletProxyClass *klass);
static void awn_applet_proxy_init(AwnAppletProxy *applet_proxy);
static void awn_applet_proxy_finalize(GObject *obj);

/* CALLBACKS */

void
awn_applet_proxy_exec (AwnAppletProxy *proxy)
{
	AwnAppletProxyPrivate *priv;
	GError *error = NULL;
	gchar *exec = NULL;

	g_return_if_fail (AWN_IS_APPLET_PROXY (proxy));
	priv = AWN_APPLET_PROXY_GET_PRIVATE(proxy);

	exec = g_strdup_printf 
		("awn-applet-activation -p %s -u %s -w %lld -o %d -h %d", 
		priv->path,
		priv->uid,
		(long long)gtk_socket_get_id (GTK_SOCKET (proxy)),
		priv->orient,
		priv->height);
	
	g_spawn_command_line_async (exec, &error);
	
	if (error) {
		g_warning ("%s\n", error->message);
		g_error_free (error);
		g_signal_emit (proxy, _proxy_signals[APPLET_DELETED], 
			       0, priv->uid);
	}
	
	g_free (exec);
}

static void
on_awn_applet_proxy_plug_added (GtkSocket *proxy, gpointer null)
{
	gtk_widget_show_all (GTK_WIDGET (proxy));
}

static void
on_awn_applet_proxy_plug_removed (GtkSocket *socket, gpointer null)
{
	AwnAppletProxyPrivate *priv;
	
	g_return_if_fail (AWN_IS_APPLET_PROXY (socket));
	priv = AWN_APPLET_PROXY_GET_PRIVATE(socket);
	
	g_signal_emit (socket, _proxy_signals[APPLET_DELETED], 0, priv->uid);
}

void
awn_applet_proxy_set_orient (AwnAppletProxy *proxy, gint orient)
{
	AwnAppletProxyPrivate *priv;
	
	g_return_if_fail (AWN_IS_APPLET_PROXY (proxy));
	priv = AWN_APPLET_PROXY_GET_PRIVATE(proxy);	
	
	priv->orient = orient;
}

void
awn_applet_proxy_set_height (AwnAppletProxy *proxy, gint height)
{
	AwnAppletProxyPrivate *priv;
	
	g_return_if_fail (AWN_IS_APPLET_PROXY (proxy));
	priv = AWN_APPLET_PROXY_GET_PRIVATE(proxy);	
	
	priv->height = height;
}

/*  GOBJECT STUFF */

static void
awn_applet_proxy_set_property (GObject      *object, 
			       guint         prop_id,
			       const GValue *value, 
			       GParamSpec   *pspec)
{
	AwnAppletProxyPrivate *priv;

	g_return_if_fail (AWN_IS_APPLET_PROXY (object));
	priv = AWN_APPLET_PROXY_GET_PRIVATE(object);

	switch (prop_id) {
		case PROP_PATH:
			if (priv->path != NULL)
				g_free (priv->path);
			priv->path = g_strdup (g_value_get_string (value));
			break;
		case PROP_UID:
			if (priv->uid != NULL)
				g_free (priv->uid);
			priv->uid = g_strdup (g_value_get_string (value));
			break;
		case PROP_ORIENT:
			awn_applet_proxy_set_orient (AWN_APPLET_PROXY (object),
						     g_value_get_int (value));
			break;	
		case PROP_HEIGHT:
			awn_applet_proxy_set_height (AWN_APPLET_PROXY (object),
						     g_value_get_int (value));
			break;
		
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, 
							   pspec);
			break;
	}
}

static void
awn_applet_proxy_get_property (GObject    *object, 
			 guint       prop_id,
			 GValue     *value, 
			 GParamSpec *pspec)
{
	AwnAppletProxyPrivate *priv;

	g_return_if_fail (AWN_IS_APPLET_PROXY (object));
	priv = AWN_APPLET_PROXY_GET_PRIVATE(object);

	switch (prop_id) {
		case PROP_PATH:
			g_value_set_string (value, priv->path);
			break;
		case PROP_UID:
			g_value_set_string (value, priv->uid);
			break;
		
		case PROP_ORIENT:
			g_value_set_int (value, priv->orient);
			
		case PROP_HEIGHT:
			g_value_set_int (value, priv->height);
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
							   pspec);
		break;
	} 
}


static void
awn_applet_proxy_class_init(AwnAppletProxyClass *klass)
{
	GObjectClass *gobject_class;
	
	parent_class = g_type_class_peek_parent(klass);

	gobject_class = G_OBJECT_CLASS(klass);
	gobject_class->finalize = awn_applet_proxy_finalize;
	gobject_class->get_property = awn_applet_proxy_get_property;
	gobject_class->set_property = awn_applet_proxy_set_property;	

	/* Class properties */
	g_object_class_install_property 
		(gobject_class,
		 PROP_PATH,
		 g_param_spec_string ("path",
		 "Path",
		 "The path to the desktop file that represents this applet",
		 NULL,
		 G_PARAM_CONSTRUCT|G_PARAM_READWRITE));	

	g_object_class_install_property 
		(gobject_class,
		 PROP_UID,
		 g_param_spec_string ("uid",
		 "UID",
		 "Awn's Unique ID for this applet instance (used for gconf)",
		 NULL,
		 G_PARAM_CONSTRUCT|G_PARAM_READWRITE));	
		 
	g_object_class_install_property 
		(gobject_class,
		 PROP_ORIENT,
		 g_param_spec_int ("orient",
		 "Orientation",
		 "The current bar orientation",
		 0,10,
		 AWN_ORIENTATION_BOTTOM,
		 G_PARAM_CONSTRUCT|G_PARAM_READWRITE));	
		 	
	g_object_class_install_property 
		(gobject_class,
		 PROP_HEIGHT,
		 g_param_spec_int ("height",
		 "Height",
		 "The current visible height of the bar",
		 AWN_MIN_HEIGHT, AWN_MAX_HEIGHT, 48,
		 G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

	/* Class signals */
	_proxy_signals[APPLET_DELETED] =
		g_signal_new ("applet_deleted",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (AwnAppletProxyClass, applet_deleted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
			      1, G_TYPE_STRING);	
	
	g_type_class_add_private (gobject_class, sizeof (AwnAppletProxyPrivate));
}

static void
awn_applet_proxy_init(AwnAppletProxy *applet_proxy)
{
	AwnAppletProxyPrivate *priv;

	g_return_if_fail (AWN_IS_APPLET_PROXY (applet_proxy));
	priv = AWN_APPLET_PROXY_GET_PRIVATE(applet_proxy);
	
	priv->connection = NULL;
	priv->proxy = NULL;	
}



static void
awn_applet_proxy_finalize(GObject *obj)
{
	AwnAppletProxy *applet_proxy;
	
	g_return_if_fail(obj != NULL);
	g_return_if_fail(AWN_IS_APPLET_PROXY(obj));

	applet_proxy = AWN_APPLET_PROXY(obj);
	
	if (G_OBJECT_CLASS(parent_class)->finalize)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}

GtkWidget *
awn_applet_proxy_new(const gchar *path, const gchar *uid)
{
	GtkWidget *applet_proxy = g_object_new(AWN_TYPE_APPLET_PROXY, 
				  	       "path", path,
				  	       "uid", uid,
				  	       NULL);
	
	g_signal_connect (G_OBJECT (applet_proxy), "plug-added",
			  G_CALLBACK (on_awn_applet_proxy_plug_added), NULL);

	g_signal_connect (G_OBJECT (applet_proxy), "plug-removed",
			  G_CALLBACK (on_awn_applet_proxy_plug_removed), NULL);
			  
	return applet_proxy;
}

