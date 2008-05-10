/*
 * Copyright (C) 2008 Neil Jagdish Patel
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#ifndef _HAVE_AWN_APP_H
#define _HAVE_AWN_APP_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AWN_TYPE_APP (awn_app_get_type ())

#define AWN_APP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        AWN_TYPE_APP, AwnApp))

#define AWN_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        AWN_TYPE_APP, AwnAppClass))

#define AWN_IS_APP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        AWN_TYPE_APP))

#define AWN_IS_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        AWN_TYPE_APP))

#define AWN_APP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_APP, AwnAppClass))

typedef struct _AwnApp AwnApp;
typedef struct _AwnAppClass AwnAppClass;
typedef struct _AwnAppPrivate AwnAppPrivate;


struct _AwnApp
{
  GObject         parent;

  /*< private >*/
  AwnAppPrivate   *priv;
};

struct _AwnAppClass 
{
  /*< private >*/
  GObjectClass    parent_class;
  
 /*< private >*/
  void (*_awn_app_1) (void);
  void (*_awn_app_2) (void);
  void (*_awn_app_3) (void);
  void (*_awn_app_4) (void);
};

GType   awn_app_get_type    (void) G_GNUC_CONST;

AwnApp* awn_app_get_default (void);


G_END_DECLS

#endif /* _HAVE_AWN_APP_H */
