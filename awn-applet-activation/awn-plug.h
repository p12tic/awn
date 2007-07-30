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

#ifndef __AWN_PLUG_H__
#define __AWN_PLUG_H__

#include <gtk/gtk.h>

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
awn_plug_new (const gchar *path, 
              const gchar *uid,
              gint         orient,
              gint         height);

G_END_DECLS

#endif
