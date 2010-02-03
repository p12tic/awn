/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 */

#ifndef _AWN_DBUS_WATCHER_H
#define _AWN_DBUS_WATCHER_H

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define AWN_TYPE_DBUS_WATCHER (awn_dbus_watcher_get_type ())

#define AWN_DBUS_WATCHER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
        AWN_TYPE_DBUS_WATCHER, AwnDBusWatcher))

#define AWN_DBUS_WATCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
        AWN_TYPE_DBUS_WATCHER, AwnDBusWatcherClass))

#define AWN_IS_DBUS_WATCHER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
        AWN_TYPE_DBUS_WATCHER))

#define AWN_IS_DBUS_WATCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
        AWN_TYPE_DBUS_WATCHER))

#define AWN_DBUS_WATCHER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
        AWN_TYPE_DBUS_WATCHER, AwnDBusWatcherClass))

typedef struct _AwnDBusWatcher AwnDBusWatcher;
typedef struct _AwnDBusWatcherClass AwnDBusWatcherClass;
typedef struct _AwnDBusWatcherPrivate AwnDBusWatcherPrivate;


struct _AwnDBusWatcher
{
  GObject         parent;

  /*< private >*/
  AwnDBusWatcherPrivate   *priv;
};

struct _AwnDBusWatcherClass 
{
  GObjectClass    parent_class;

  /*< signals >*/
  void (*name_appeared) (AwnDBusWatcher* watcher,
                             gchar* name);
  void (*name_disappeared) (AwnDBusWatcher* watcher,
                             gchar* name);
  void (*_awn_dbus_watcher_1) (void);
  void (*_awn_dbus_watcher_2) (void);
};

GType    awn_dbus_watcher_get_type    (void) G_GNUC_CONST;

AwnDBusWatcher * awn_dbus_watcher_get_default (void);
gboolean awn_dbus_watcher_has_name (AwnDBusWatcher* self, const gchar* name);

G_END_DECLS

#endif /* _AWN_DBUS_WATCHER_H */

