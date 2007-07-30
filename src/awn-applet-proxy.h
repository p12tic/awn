/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
*/

#ifndef __AWN_APPLET_PROXY_H__
#define __AWN_APPLET_PROXY_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _AwnAppletProxy         AwnAppletProxy;
typedef struct _AwnAppletProxyClass	AwnAppletProxyClass;
typedef struct _AwnAppletProxyPrivate  AwnAppletProxyPrivate;

#define AWN_TYPE_APPLET_PROXY		(awn_applet_proxy_get_type ())

#define AWN_APPLET_PROXY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                AWN_TYPE_APPLET_PROXY,\
                                AwnAppletProxy))

#define AWN_APPLET_PROXY_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), \
                                AWN_APPLET_PROXY, \
                                AwnAppletProxyClass))
                                
#define AWN_IS_APPLET_PROXY(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                AWN_TYPE_APPLET_PROXY))

#define AWN_IS_APPLET_PROXY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_APPLET_PROXY))

#define AWN_APPLET_PROXY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                AWN_TYPE_APPLET_PROXY, \
                                AwnAppletProxyClass))

struct _AwnAppletProxy
{
	GtkSocket parent;
};

struct _AwnAppletProxyClass
{
	GtkSocketClass parent_class;
	
	/* Signals */
	void (*applet_deleted) (AwnAppletProxy *applet_proxy, const gchar *uid);

};

GType awn_applet_proxy_get_type (void);

GtkWidget*
awn_applet_proxy_new (const gchar *path, const gchar *uid);

void
awn_applet_proxy_exec (AwnAppletProxy *proxy);

void
awn_applet_proxy_set_orient (AwnAppletProxy *proxy, gint orient);

void
awn_applet_proxy_set_height (AwnAppletProxy *proxy, gint height);


G_END_DECLS

#endif
