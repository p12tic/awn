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
*/

#ifndef	_AWN_APPLET_PROXY_H
#define	_AWN_APPLET_PROXY_H

#include <glib.h>
#include <gtk/gtk.h>

#include "awn-panel.h"

G_BEGIN_DECLS

#define AWN_TYPE_APPLET_PROXY (awn_applet_proxy_get_type())

#define AWN_APPLET_PROXY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_APPLET_PROXY, \
        AwnAppletProxy))

#define AWN_APPLET_PROXY_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_APPLET_PROXY, \
        AwnAppletProxyClass))

#define AWN_IS_APPLET_PROXY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_APPLET_PROXY))

#define AWN_IS_APPLET_PROXY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_APPLET_PROXY))

#define AWN_APPLET_PROXY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_APPLET_PROXY, AwnAppletProxyClass))

typedef struct _AwnAppletProxy AwnAppletProxy;
typedef struct _AwnAppletProxyClass AwnAppletProxyClass;
typedef struct _AwnAppletProxyPrivate AwnAppletProxyPrivate;

struct _AwnAppletProxy 
{
  GtkSocket parent;

  /*< private >*/
  AwnAppletProxyPrivate *priv;
};

struct _AwnAppletProxyClass 
{
  GtkSocketClass parent_class;

  /*< signals >*/
  void (*applet_crashed) (AwnAppletProxy *proxy);
};

GType       awn_applet_proxy_get_type          (void) G_GNUC_CONST;

GtkWidget * awn_applet_proxy_new               (const gchar *path,
                                                const gchar *uid,
                                                gint         position,
                                                gint         offset,
                                                gint         size);
void        awn_applet_proxy_execute           (AwnAppletProxy *proxy);

void        awn_applet_proxy_schedule_execute  (AwnAppletProxy *proxy);

GtkWidget* awn_applet_proxy_get_throbber       (AwnAppletProxy *proxy);

G_END_DECLS


#endif /* _AWN_APPLET_PROXY_H */

