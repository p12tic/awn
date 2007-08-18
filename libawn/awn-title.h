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

#ifndef __AWN_TITLE_H__
#define __AWN_TITLE_H__

#include <gtk/gtk.h>

#include "awn-defines.h"

G_BEGIN_DECLS

typedef struct _AwnTitle AwnTitle;
typedef struct _AwnTitleClass	AwnTitleClass;
typedef struct _AwnTitlePrivate AwnTitlePrivate;

#define AWN_TYPE_TITLE		(awn_title_get_type ())

#define AWN_TITLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj),\
        AWN_TYPE_TITLE,\
        AwnTitle))

#define AWN_TITLE_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), \
        AWN_TYPE_TITLE, \
        AwnTitleClass))
                                
#define AWN_IS_TITLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
        AWN_TYPE_TITLE))

#define AWN_IS_TITLE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), \
        AWN_TYPE_TITLE))

#define AWN_TITLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_TITLE, \
        AwnTitleClass))

struct _AwnTitle
{
	GtkWindow parent;

        AwnTitlePrivate *priv;
};

struct _AwnTitleClass
{
	GtkWindowClass parent_class;

      /* Future padding */
        void (*_title0) (void);
        void (*_title1) (void);
        void (*_title2) (void);
        void (*_title3) (void);
};

GType awn_title_get_type (void);

GtkWidget *
awn_title_get_default (void);

/* This can be markup */
void
awn_title_show (AwnTitle *title, GtkWidget *focus, const gchar *text);

void
awn_title_hide (AwnTitle *title, GtkWidget *focus);

G_END_DECLS

#endif
