/*
 * Copyright (C) 2008 Neil Jagdish Patel
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifndef _AWN_THEMED_ICON_H_
#define _AWN_THEMED_ICON_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "awn-icon.h"

#define AWN_TYPE_THEMED_ICON (awn_themed_icon_get_type ())

#define AWN_THEMED_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	AWN_TYPE_THEMED_ICON, AwnThemedIcon))

#define AWN_THEMED_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	AWN_TYPE_THEMED_ICON, AwnThemedIconClass))

#define AWN_IS_THEMED_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	AWN_TYPE_THEMED_ICON))

#define AWN_IS_THEMED_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	AWN_TYPE_THEMED_ICON))

#define AWN_THEMED_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	AWN_TYPE_THEMED_ICON, AwnThemedIconClass))

typedef struct _AwnThemedIcon        AwnThemedIcon;
typedef struct _AwnThemedIconClass   AwnThemedIconClass;
typedef struct _AwnThemedIconPrivate AwnThemedIconPrivate;
 
struct _AwnThemedIcon
{
  AwnIcon  parent;	

  AwnThemedIconPrivate *priv;
};

struct _AwnThemedIconClass
{
  AwnIconClass parent_class;
};

GType         awn_themed_icon_get_type (void) G_GNUC_CONST;

GtkWidget *   awn_themed_icon_new      (void);


#endif /* _AWN_THEMED_ICON_H_ */
