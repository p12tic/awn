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
 * Authored by Michal Hruby <michal.mhr@gmail.com>
 *
 */

#ifndef _AWN_SEPARATOR_H_
#define _AWN_SEPARATOR_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AWN_TYPE_SEPARATOR awn_separator_get_type()

#define AWN_SEPARATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_SEPARATOR, AwnSeparator))

#define AWN_SEPARATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_SEPARATOR, AwnSeparatorClass))

#define AWN_IS_SEPARATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_SEPARATOR))

#define AWN_IS_SEPARATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_SEPARATOR))

#define AWN_SEPARATOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_SEPARATOR, AwnSeparatorClass))

typedef struct _AwnSeparatorPrivate AwnSeparatorPrivate;

typedef struct {
  GtkImage parent;

  AwnSeparatorPrivate *priv;
} AwnSeparator;

typedef struct {
  GtkImageClass parent_class;
} AwnSeparatorClass;

GType
awn_separator_get_type        (void);

GtkWidget*
awn_separator_new_from_config (DesktopAgnosticConfigClient *client);

GtkWidget*
awn_separator_new_from_config_with_values (DesktopAgnosticConfigClient *client,
                                           GtkPositionType pos,
                                           gint size, gint offset);

G_END_DECLS

#endif /* _AWN_SEPARATOR_H_ */

