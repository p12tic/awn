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


#ifndef	_AWN_HOTSPOT_H
#define	_AWN_HOTSPOT_H

#include <glib.h>
#include <gtk/gtk.h>

#include <libawn/awn-settings.h>

G_BEGIN_DECLS

#define AWN_HOTSPOT_TYPE      (awn_hotspot_get_type())
#define AWN_HOTSPOT(o)        (G_TYPE_CHECK_INSTANCE_CAST((o), AWN_HOTSPOT_TYPE, AwnHotspot))
#define AWN_HOTSPOT_CLASS(c)  (G_TYPE_CHECK_CLASS_CAST((c), AWN_HOTSPOT_TYPE, AwnHotspotClass))
#define IS_AWN_HOTSPOT(o)     (G_TYPE_CHECK_INSTANCE_TYPE((o), AWN_HOTSPOT_TYPE))
#define IS_AWN_HOTSPOT_CLASS  (G_TYPE_INSTANCE_GET_CLASS((o), AWN_HOTSPOT_TYPE, AwnHotspotClass))

typedef struct _AwnHotspot AwnHotspot;
typedef struct _AwnHotspotClass AwnHotspotClass;

struct _AwnHotspot {
        GtkWindow win;
           
};

struct _AwnHotspotClass {
        GtkWindowClass parent_class;
        

};

GType awn_hotspot_get_type (void);

GtkWidget *awn_hotspot_new (AwnSettings *sets);

G_END_DECLS


#endif /* _AWN_HOTSPOT_H */

