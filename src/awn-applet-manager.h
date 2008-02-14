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

#ifndef __AWN_APPLET_MANAGER_H__
#define __AWN_APPLET_MANAGER_H__

#include <gtk/gtk.h>

#include <libawn/awn-settings.h>

G_BEGIN_DECLS

#define AWN_TYPE_APPLET_MANAGER		(awn_applet_manager_get_type ())

#define AWN_APPLET_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
        AWN_TYPE_APPLET_MANAGER, \
        AwnAppletManager))

#define AWN_APPLET_MANAGER_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj),\
        AWN_APPLET_MANAGER, \
        AwnAppletManagerClass))

#define AWN_IS_APPLET_MANAGER(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
        AWN_TYPE_APPLET_MANAGER))

#define AWN_IS_APPLET_MANAGER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj),\
        AWN_TYPE_APPLET_MANAGER))

#define AWN_APPLET_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),\
        AWN_TYPE_APPLET_MANAGER, \
        AwnAppletManagerClass))

typedef struct _AwnAppletManager		AwnAppletManager;
typedef struct _AwnAppletManagerClass	AwnAppletManagerClass;

struct _AwnAppletManager
{
	GtkHBox parent;

	/* < private > */
};

struct _AwnAppletManagerClass
{
	GtkHBoxClass parent_class;
        
        /* Signals */
        void (*orient_changed) (AwnAppletManager *manager, gint orient);
        void (*height_changed) (AwnAppletManager *manager, gint height);
        void (*destroy_notify) (AwnAppletManager *manager);
        void (*destroy_applet) (AwnAppletManager *manager, const gchar *uid);
        void (*size_changed)   (AwnAppletManager *manager, gint x);
        
        /* Future padding */
        void (*_applet_manager0) (void);
        void (*_applet_manager1) (void);
        void (*_applet_manager2) (void);
        void (*_applet_manager3) (void);
};

GType awn_applet_manager_get_type (void);

GtkWidget *awn_applet_manager_new (AwnSettings *settings);

void
awn_applet_manager_load_applets (AwnAppletManager *manager);

void
awn_applet_manager_quit (AwnAppletManager *manager);

void
awn_applet_manager_height_changed (AwnAppletManager *manager);

void
on_awn_applet_manager_size_allocate (GtkWidget *widget, GtkAllocation *allocation, AwnAppletManager *manager);

G_END_DECLS

#endif

