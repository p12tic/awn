/*
 * Copyright (C) 2008 Neil Jagdish Patel
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
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifndef _AWN_ICON_BOX_H_
#define _AWN_ICON_BOX_H_

#include <gtk/gtk.h>

#include "awn-defines.h"
#include "awn-applet.h"
#include "awn-box.h"

G_BEGIN_DECLS

#define AWN_TYPE_ICON_BOX (awn_icon_box_get_type ())

#define AWN_ICON_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	AWN_TYPE_ICON_BOX, AwnIconBox))

#define AWN_ICON_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	AWN_TYPE_ICON_BOX, AwnIconBoxClass))

#define AWN_IS_ICON_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	AWN_TYPE_ICON_BOX))

#define AWN_IS_ICON_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	AWN_TYPE_ICON_BOX))

#define AWN_ICON_BOX_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	AWN_TYPE_ICON_BOX, AwnIconBoxClass))

typedef struct _AwnIconBox        AwnIconBox;
typedef struct _AwnIconBoxClass   AwnIconBoxClass;
typedef struct _AwnIconBoxPrivate AwnIconBoxPrivate;
 
struct _AwnIconBox
{
  AwnBox  parent;	

  AwnIconBoxPrivate *priv;
};

struct _AwnIconBoxClass
{
  AwnBoxClass parent_class;
};

GType         awn_icon_box_get_type         (void) G_GNUC_CONST;

GtkWidget *   awn_icon_box_new              (void);

GtkWidget *   awn_icon_box_new_for_applet   (AwnApplet *applet);

void          awn_icon_box_set_pos_type  (AwnIconBox     *icon_box,
                                             GtkPositionType  position);

void          awn_icon_box_set_offset       (AwnIconBox *icon_box,
                                             gint        offset);

G_END_DECLS

#endif /* _AWN_ICON_BOX_H_ */

