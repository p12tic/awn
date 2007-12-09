/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifndef __AWN_PLUG_H__
#define __AWN_PLUG_H__

#include <gtk/gtk.h>
#include "awn-applet.h"

G_BEGIN_DECLS

typedef struct _AwnPlug         AwnPlug;
typedef struct _AwnPlugClass	AwnPlugClass;
typedef struct _AwnPlugPrivate  AwnPlugPrivate;

#define AWN_TYPE_PLUG		(awn_plug_get_type ())

#define AWN_PLUG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                AWN_TYPE_PLUG,\
                                AwnPlug))

#define AWN_PLUG_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), \
                                AWN_PLUG, \
                                AwnPlugClass))
                                
#define AWN_IS_PLUG(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                AWN_TYPE_PLUG))

#define AWN_IS_PLUG_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_PLUG))

#define AWN_PLUG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                AWN_TYPE_PLUG, \
                                AwnPlugClass))

struct _AwnPlug
{
	GtkPlug parent;
};

struct _AwnPlugClass
{
	GtkPlugClass parent_class;
	
	/* Signals */
	void (*applet_deleted) (AwnPlug *plug, const gchar *uid);

};

GType awn_plug_get_type (void);

GtkWidget *
awn_plug_new (AwnApplet *applet);

void
awn_plug_construct (AwnPlug *plug, GdkNativeWindow socket_id);

G_END_DECLS

#endif
