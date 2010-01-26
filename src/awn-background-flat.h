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

#ifndef	_AWN_BACKGROUND_FLAT_H
#define	_AWN_BACKGROUND_FLAT_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "awn-background.h"

G_BEGIN_DECLS

#define AWN_TYPE_BACKGROUND_FLAT (awn_background_flat_get_type())

#define AWN_BACKGROUND_FLAT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_BACKGROUND_FLAT, \
  AwnBackgroundFlat))

#define AWN_BACKGROUND_FLAT_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_BACKGROUND_FLAT, \
  AwnBackgroundFlatClass))

#define AWN_IS_BACKGROUND_FLAT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_BACKGROUND_FLAT))

#define AWN_IS_BACKGROUND_FLAT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
  AWN_TYPE_BACKGROUND_FLAT))

#define AWN_BACKGROUND_FLAT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
  AWN_TYPE_BACKGROUND_FLAT, AwnBackgroundFlatClass))

typedef struct _AwnBackgroundFlat AwnBackgroundFlat;
typedef struct _AwnBackgroundFlatClass AwnBackgroundFlatClass;

struct _AwnBackgroundFlat 
{
  AwnBackground  parent;
};

struct _AwnBackgroundFlatClass 
{
  AwnBackgroundClass parent_class;
};

GType           awn_background_flat_get_type (void) G_GNUC_CONST;

AwnBackground * awn_background_flat_new      (DesktopAgnosticConfigClient *client,
                                              AwnPanel        *panel);

G_END_DECLS

#endif /* _AWN_BACKGROUND_FLAT_H */

