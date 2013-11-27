/*  This file is part of avant-window-navigator

    Copyright (C) 2013  Povilas Kanapickas <tir5c3@yahoo.co.uk>

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/.
 */

#ifndef LIBAWN_VALA_UTILS_H
#define LIBAWN_VALA_UTILS_H

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib.h>

namespace awn {

void vala_send_dbus_error_message(DBusConnection* connection,
                                  DBusMessage* message, GError* error);

void vala_set_dbus_error(const DBusError& dbus_error, GError** error);

void vala_array_free(gpointer array, gint array_length, GDestroyNotify destroy_func);

void vala_dbus_get_gvalue(DBusMessageIter* super_it, GValue* val);

} // namespace awn

#endif

