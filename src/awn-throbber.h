/*
 * Copyright (C) 2009 Michal Hruby
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
 * Authored by Michal Hruby <michal.mhr@gmail.com>
 *
 */

#ifndef _AWN_THROBBER_H_
#define _AWN_THROBBER_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include <libawn/libawn.h>

G_BEGIN_DECLS

#define AWN_TYPE_THROBBER (awn_throbber_get_type ())

#define AWN_THROBBER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj),\
	AWN_TYPE_THROBBER, AwnThrobber))

#define AWN_THROBBER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass),\
	AWN_TYPE_THROBBER, AwnThrobberClass))

#define AWN_IS_THROBBER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj),\
	AWN_TYPE_THROBBER))

#define AWN_IS_THROBBER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),\
	AWN_TYPE_THROBBER))

#define AWN_THROBBER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),\
	AWN_TYPE_THROBBER, AwnThrobberClass))

typedef struct _AwnThrobber        AwnThrobber;
typedef struct _AwnThrobberClass   AwnThrobberClass;
typedef struct _AwnThrobberPrivate AwnThrobberPrivate;
 
struct _AwnThrobber
{
  AwnIcon parent;

  AwnThrobberPrivate *priv;
};

struct _AwnThrobberClass
{
  AwnIconClass parent_class;
};

typedef enum {
  AWN_THROBBER_TYPE_NORMAL,
  AWN_THROBBER_TYPE_SAD_FACE,
  AWN_THROBBER_TYPE_ARROW_1,
  AWN_THROBBER_TYPE_ARROW_2,
  AWN_THROBBER_TYPE_CLOSE_BUTTON
} AwnThrobberType;

GType         awn_throbber_get_type         (void) G_GNUC_CONST;

GtkWidget *   awn_throbber_new              (void);

GtkWidget*    awn_throbber_new_with_config  (DesktopAgnosticConfigClient *client);

void          awn_throbber_set_type         (AwnThrobber *throbber,
                                             AwnThrobberType type);

void          awn_throbber_set_size         (AwnThrobber *throbber, gint size);

G_END_DECLS

#endif /* _AWN_THROBBER_H_ */

