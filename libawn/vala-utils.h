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
#include <cstdint>

namespace awn {

void vala_send_dbus_error_message(DBusConnection* connection,
                                  DBusMessage* message, GError* error);

void vala_set_dbus_error(const DBusError& dbus_error, GError** error);

void vala_array_free(gpointer array, gint array_length, GDestroyNotify destroy_func);

void vala_dbus_get_gvalue(DBusMessageIter* super_it, GValue* val);

void vala_dbus_append_gvalue(DBusMessageIter* it, const char* key, GValue* val);

inline dbus_bool_t vala_dbus_iter_append_bool(DBusMessageIter *iter, bool val)
{
    dbus_bool_t dv = val;
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &dv);
}

inline dbus_bool_t vala_dbus_iter_append_uint8(DBusMessageIter *iter, std::uint8_t val)
{
    guint8 dv = val;
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE, &dv);
}

inline dbus_bool_t vala_dbus_iter_append_int32(DBusMessageIter *iter, std::int32_t val)
{
    dbus_int32_t dv = val;
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &dv);
}

inline dbus_bool_t vala_dbus_iter_append_uint32(DBusMessageIter *iter, std::uint32_t val)
{
    dbus_uint32_t dv = val;
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &dv);
}

inline dbus_bool_t vala_dbus_iter_append_int64(DBusMessageIter *iter, std::int64_t val)
{
    dbus_int64_t dv = val;
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_INT64, &dv);
}

inline dbus_bool_t vala_dbus_iter_append_uint64(DBusMessageIter *iter, std::uint64_t val)
{
    dbus_uint64_t dv = val;
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &dv);
}

inline dbus_bool_t vala_dbus_iter_append_double(DBusMessageIter *iter, double val)
{
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_DOUBLE, &val);
}

inline dbus_bool_t vala_dbus_iter_append_string(DBusMessageIter *iter, const char* s)
{
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &s);
}

inline dbus_bool_t vala_dbus_iter_append_obj_path(DBusMessageIter *iter, const char* s)
{
    return dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &s);
}


} // namespace awn

#endif

