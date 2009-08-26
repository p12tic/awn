/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _AWN_BOX_H_
#define _AWN_BOX_H_

#include <gtk/gtk.h>

#include "awn-defines.h"

G_BEGIN_DECLS

#define AWN_TYPE_BOX (awn_box_get_type ())

#define AWN_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	AWN_TYPE_BOX, AwnBox))

#define AWN_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	AWN_TYPE_BOX, AwnBoxClass))

#define AWN_IS_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	AWN_TYPE_BOX))

#define AWN_IS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	AWN_TYPE_BOX))

#define AWN_BOX_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	AWN_TYPE_BOX, AwnBoxClass))

typedef struct _AwnBox        AwnBox;
typedef struct _AwnBoxClass   AwnBoxClass;
typedef struct _AwnBoxPrivate AwnBoxPrivate;
 
struct _AwnBox
{
  GtkBox  parent;	

  AwnBoxPrivate *priv;
};

struct _AwnBoxClass
{
  GtkBoxClass parent_class;
};

GType          awn_box_get_type         (void) G_GNUC_CONST;

GtkWidget *    awn_box_new              (GtkOrientation orient);

void           awn_box_set_orientation  (AwnBox         *box,
                                         GtkOrientation  orient);

GtkOrientation awn_box_orientation_from_awn_orientation (AwnOrientation orient);

G_END_DECLS

#endif /* _AWN_BOX_H_ */

