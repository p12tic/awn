/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 *  Author : Michal Hruby <michal.mhr@gmail.com>
*/

#ifndef	_AWN_BACKGROUND_EDGY_H
#define	_AWN_BACKGROUND_EDGY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "awn-background-flat.h"

G_BEGIN_DECLS

#define AWN_TYPE_BACKGROUND_EDGY (awn_background_edgy_get_type())

#define AWN_BACKGROUND_EDGY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_BACKGROUND_EDGY, \
  AwnBackgroundEdgy))

#define AWN_BACKGROUND_EDGY_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), AWN_BACKGROUND_EDGY, \
  AwnBackgroundEdgyClass))

#define AWN_IS_BACKGROUND_EDGY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_BACKGROUND_EDGY))

#define AWN_IS_BACKGROUND_EDGY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
  AWN_TYPE_BACKGROUND_EDGY))

#define AWN_BACKGROUND_EDGY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), \
  AWN_TYPE_BACKGROUND_EDGY, AwnBackgroundEdgyClass))

typedef struct _AwnBackgroundEdgy AwnBackgroundEdgy;
typedef struct _AwnBackgroundEdgyClass AwnBackgroundEdgyClass;
typedef struct _AwnBackgroundEdgyPrivate AwnBackgroundEdgyPrivate;

struct _AwnBackgroundEdgy
{
  AwnBackgroundFlat parent;

  gint size_offset;

  AwnBackgroundEdgyPrivate *priv;
};

struct _AwnBackgroundEdgyClass 
{
  AwnBackgroundFlatClass parent_class;
};

GType           awn_background_edgy_get_type (void) G_GNUC_CONST;

AwnBackground * awn_background_edgy_new      (DesktopAgnosticConfigClient *client,
                                              AwnPanel        *panel);

G_END_DECLS

#endif /* _AWN_BACKGROUND_EDGY_H */

