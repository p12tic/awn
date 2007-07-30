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

#ifndef __AWN_APPLET_H__
#define __AWN_APPLET_H__

#include <gtk/gtk.h>

#include "awn-defines.h"

G_BEGIN_DECLS

typedef struct _AwnApplet AwnApplet;
typedef struct _AwnAppletClass	AwnAppletClass;
typedef struct _AwnAppletPrivate AwnAppletPrivate;

#define AWN_TYPE_APPLET		(awn_applet_get_type ())

#define AWN_APPLET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
                                AWN_TYPE_APPLET,\
                                AwnApplet))

#define AWN_APPLET_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), \
                                AWN_TYPE_APPLET, \
                                AwnAppletClass))
                                
#define AWN_IS_APPLET(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
                                AWN_TYPE_APPLET))

#define AWN_IS_APPLET_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_APPLET))

#define AWN_APPLET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                AWN_TYPE_APPLET, \
                                AwnAppletClass))

struct _AwnApplet
{
	GtkEventBox parent;
};

struct _AwnAppletClass
{
	GtkEventBoxClass parent_class;

	void (*plug_embedded)  (AwnApplet *applet);
	void (*orient_changed) (AwnApplet *applet, AwnOrientation oreint);
	void (*height_changed) (AwnApplet *applet, guint height);
	void (*deleted)        (AwnApplet *applet, const gchar *uid);
	void (*size_changed)   (AwnApplet *applet, const gint x);
        /* Future padding */
        void (*_applet0) (void);
        void (*_applet1) (void);
        void (*_applet2) (void);
        void (*_applet3) (void);
};

GType awn_applet_get_type (void);

/* Hook to have an AWN Applet built for you */
typedef gboolean (*AwnAppletInitFunc) ( AwnApplet *applet );
/* Hook to build your own AWN Applet */
typedef AwnApplet* (*AwnAppletInitPFunc) ( const gchar* uid, gint orient, gint height );


GtkWidget *
awn_applet_new ( const gchar* uid, gint orient, gint height );

AwnOrientation
awn_applet_get_orientation (AwnApplet *applet);

/* This is the *visible* height, the height of the bar is always double this */
guint
awn_applet_get_height (AwnApplet *applet);

gchar*
awn_applet_get_preferences_key (AwnApplet *applet);

void                
awn_applet_add_preferences (AwnApplet *applet, 
                            const gchar *schema_dir,
                            GError **opt_error);

GtkWidget*
awn_applet_create_default_menu (AwnApplet *applet);


G_END_DECLS

#endif
