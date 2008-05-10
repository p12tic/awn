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

#ifndef	_AWN_PANEL_H
#define	_AWN_PANEL_H

#include <glib.h>
#include <gtk/gtk.h>

#include <libawn/awn-config-client.h>

G_BEGIN_DECLS

#define AWN_TYPE_PANEL (awn_panel_get_type())

#define AWN_PANEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_PANEL, \
        AwnPanel))

#define AWN_PANEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_PANEL, \
        AwnPanelClass))

#define AWN_IS_PANEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_PANEL))

#define AWN_IS_PANEL_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_PANEL))

#define AWN_PANEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_PANEL, AwnPanelClass))

typedef struct _AwnPanel AwnPanel;
typedef struct _AwnPanelClass AwnPanelClass;
typedef struct _AwnPanelPrivate AwnPanelPrivate;

struct _AwnPanel 
{
  GtkWindow parent;

  /*< private >*/
  AwnPanelPrivate *priv;
};

struct _AwnPanelClass 
{
  GtkWindowClass parent_class;
};

GType       awn_panel_get_type          (void) G_GNUC_CONST;

GtkWidget * awn_panel_new_from_config (AwnConfigClient *client);

G_END_DECLS


#endif /* _AWN_PANEL_H */

