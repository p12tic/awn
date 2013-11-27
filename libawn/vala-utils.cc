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

#include <libawn/vala-utils.h>
#include <cstring>

namespace awn {

void vala_send_dbus_error_message(DBusConnection* connection, DBusMessage* message, GError* error)
{
    DBusMessage* reply;
    if (error->domain == DBUS_GERROR) {
        switch (error->code) {
        case DBUS_GERROR_FAILED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Failed", error->message);
            break;
        case DBUS_GERROR_NO_MEMORY:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.NoMemory", error->message);
            break;
        case DBUS_GERROR_SERVICE_UNKNOWN:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.ServiceUnknown", error->message);
            break;
        case DBUS_GERROR_NAME_HAS_NO_OWNER:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.NameHasNoOwner", error->message);
            break;
        case DBUS_GERROR_NO_REPLY:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.NoReply", error->message);
            break;
        case DBUS_GERROR_IO_ERROR:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.IOError", error->message);
            break;
        case DBUS_GERROR_BAD_ADDRESS:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.BadAddress", error->message);
            break;
        case DBUS_GERROR_NOT_SUPPORTED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.NotSupported", error->message);
            break;
        case DBUS_GERROR_LIMITS_EXCEEDED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.LimitsExceeded", error->message);
            break;
        case DBUS_GERROR_ACCESS_DENIED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.AccessDenied", error->message);
            break;
        case DBUS_GERROR_AUTH_FAILED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.AuthFailed", error->message);
            break;
        case DBUS_GERROR_NO_SERVER:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.NoServer", error->message);
            break;
        case DBUS_GERROR_TIMEOUT:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Timeout", error->message);
            break;
        case DBUS_GERROR_NO_NETWORK:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.NoNetwork", error->message);
            break;
        case DBUS_GERROR_ADDRESS_IN_USE:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.AddressInUse", error->message);
            break;
        case DBUS_GERROR_DISCONNECTED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Disconnected", error->message);
            break;
        case DBUS_GERROR_INVALID_ARGS:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.InvalidArgs", error->message);
            break;
        case DBUS_GERROR_FILE_NOT_FOUND:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.FileNotFound", error->message);
            break;
        case DBUS_GERROR_FILE_EXISTS:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.FileExists", error->message);
            break;
        case DBUS_GERROR_UNKNOWN_METHOD:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.UnknownMethod", error->message);
            break;
        case DBUS_GERROR_TIMED_OUT:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.TimedOut", error->message);
            break;
        case DBUS_GERROR_MATCH_RULE_NOT_FOUND:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.MatchRuleNotFound", error->message);
            break;
        case DBUS_GERROR_MATCH_RULE_INVALID:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.MatchRuleInvalid", error->message);
            break;
        case DBUS_GERROR_SPAWN_EXEC_FAILED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Spawn.ExecFailed", error->message);
            break;
        case DBUS_GERROR_SPAWN_FORK_FAILED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Spawn.ForkFailed", error->message);
            break;
        case DBUS_GERROR_SPAWN_CHILD_EXITED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Spawn.ChildExited", error->message);
            break;
        case DBUS_GERROR_SPAWN_CHILD_SIGNALED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Spawn.ChildSignaled", error->message);
            break;
        case DBUS_GERROR_SPAWN_FAILED:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.Spawn.Failed", error->message);
            break;
        case DBUS_GERROR_UNIX_PROCESS_ID_UNKNOWN:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.UnixProcessIdUnknown", error->message);
            break;
        case DBUS_GERROR_INVALID_SIGNATURE:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.InvalidSignature", error->message);
            break;
        case DBUS_GERROR_INVALID_FILE_CONTENT:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.InvalidFileContent", error->message);
            break;
        case DBUS_GERROR_SELINUX_SECURITY_CONTEXT_UNKNOWN:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.SELinuxSecurityContextUnknown", error->message);
            break;
        case DBUS_GERROR_REMOTE_EXCEPTION:
            reply = dbus_message_new_error(message, "org.freedesktop.DBus.Error.RemoteException", error->message);
            break;
        default:
            return;
        }
        dbus_connection_send(connection, reply, NULL);
        dbus_message_unref(reply);
    }
    // FIXME: what to do in this case?
}

void vala_set_dbus_error(const DBusError& dbus_error, GError** error)
{
    GQuark edomain = 0;
    gint ecode = 0;
    if (std::strstr(dbus_error.name, "org.freedesktop.DBus.Error") == dbus_error.name) {
        const char* _tmp5_;
        edomain = DBUS_GERROR;
        _tmp5_ = dbus_error.name + 27;
        if (std::strcmp(_tmp5_, "Failed") == 0) {
            ecode = DBUS_GERROR_FAILED;
        } else if (std::strcmp(_tmp5_, "NoMemory") == 0) {
            ecode = DBUS_GERROR_NO_MEMORY;
        } else if (std::strcmp(_tmp5_, "ServiceUnknown") == 0) {
            ecode = DBUS_GERROR_SERVICE_UNKNOWN;
        } else if (std::strcmp(_tmp5_, "NameHasNoOwner") == 0) {
            ecode = DBUS_GERROR_NAME_HAS_NO_OWNER;
        } else if (std::strcmp(_tmp5_, "NoReply") == 0) {
            ecode = DBUS_GERROR_NO_REPLY;
        } else if (std::strcmp(_tmp5_, "IOError") == 0) {
            ecode = DBUS_GERROR_IO_ERROR;
        } else if (std::strcmp(_tmp5_, "BadAddress") == 0) {
            ecode = DBUS_GERROR_BAD_ADDRESS;
        } else if (std::strcmp(_tmp5_, "NotSupported") == 0) {
            ecode = DBUS_GERROR_NOT_SUPPORTED;
        } else if (std::strcmp(_tmp5_, "LimitsExceeded") == 0) {
            ecode = DBUS_GERROR_LIMITS_EXCEEDED;
        } else if (std::strcmp(_tmp5_, "AccessDenied") == 0) {
            ecode = DBUS_GERROR_ACCESS_DENIED;
        } else if (std::strcmp(_tmp5_, "AuthFailed") == 0) {
            ecode = DBUS_GERROR_AUTH_FAILED;
        } else if (std::strcmp(_tmp5_, "NoServer") == 0) {
            ecode = DBUS_GERROR_NO_SERVER;
        } else if (std::strcmp(_tmp5_, "Timeout") == 0) {
            ecode = DBUS_GERROR_TIMEOUT;
        } else if (std::strcmp(_tmp5_, "NoNetwork") == 0) {
            ecode = DBUS_GERROR_NO_NETWORK;
        } else if (std::strcmp(_tmp5_, "AddressInUse") == 0) {
            ecode = DBUS_GERROR_ADDRESS_IN_USE;
        } else if (std::strcmp(_tmp5_, "Disconnected") == 0) {
            ecode = DBUS_GERROR_DISCONNECTED;
        } else if (std::strcmp(_tmp5_, "InvalidArgs") == 0) {
            ecode = DBUS_GERROR_INVALID_ARGS;
        } else if (std::strcmp(_tmp5_, "FileNotFound") == 0) {
            ecode = DBUS_GERROR_FILE_NOT_FOUND;
        } else if (std::strcmp(_tmp5_, "FileExists") == 0) {
            ecode = DBUS_GERROR_FILE_EXISTS;
        } else if (std::strcmp(_tmp5_, "UnknownMethod") == 0) {
            ecode = DBUS_GERROR_UNKNOWN_METHOD;
        } else if (std::strcmp(_tmp5_, "TimedOut") == 0) {
            ecode = DBUS_GERROR_TIMED_OUT;
        } else if (std::strcmp(_tmp5_, "MatchRuleNotFound") == 0) {
            ecode = DBUS_GERROR_MATCH_RULE_NOT_FOUND;
        } else if (std::strcmp(_tmp5_, "MatchRuleInvalid") == 0) {
            ecode = DBUS_GERROR_MATCH_RULE_INVALID;
        } else if (std::strcmp(_tmp5_, "Spawn.ExecFailed") == 0) {
            ecode = DBUS_GERROR_SPAWN_EXEC_FAILED;
        } else if (std::strcmp(_tmp5_, "Spawn.ForkFailed") == 0) {
            ecode = DBUS_GERROR_SPAWN_FORK_FAILED;
        } else if (std::strcmp(_tmp5_, "Spawn.ChildExited") == 0) {
            ecode = DBUS_GERROR_SPAWN_CHILD_EXITED;
        } else if (std::strcmp(_tmp5_, "Spawn.ChildSignaled") == 0) {
            ecode = DBUS_GERROR_SPAWN_CHILD_SIGNALED;
        } else if (std::strcmp(_tmp5_, "Spawn.Failed") == 0) {
            ecode = DBUS_GERROR_SPAWN_FAILED;
        } else if (std::strcmp(_tmp5_, "UnixProcessIdUnknown") == 0) {
            ecode = DBUS_GERROR_UNIX_PROCESS_ID_UNKNOWN;
        } else if (std::strcmp(_tmp5_, "InvalidSignature") == 0) {
            ecode = DBUS_GERROR_INVALID_SIGNATURE;
        } else if (std::strcmp(_tmp5_, "InvalidFileContent") == 0) {
            ecode = DBUS_GERROR_INVALID_FILE_CONTENT;
        } else if (std::strcmp(_tmp5_, "SELinuxSecurityContextUnknown") == 0) {
            ecode = DBUS_GERROR_SELINUX_SECURITY_CONTEXT_UNKNOWN;
        } else if (std::strcmp(_tmp5_, "RemoteException") == 0) {
            ecode = DBUS_GERROR_REMOTE_EXCEPTION;
        }
    }
    g_set_error(error, edomain, ecode, "%s", dbus_error.message);
}

void vala_dbus_get_gvalue(DBusMessageIter* super_it, GValue* val)
{
    val = {0};
    DBusMessageIter it;
    dbus_message_iter_recurse(super_it, &it);

    if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_BYTE) {
        guint8 tmp;
        dbus_message_iter_get_basic(&it, &tmp);
        g_value_init(val, G_TYPE_UCHAR);
        g_value_set_uchar(val, tmp);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_BOOLEAN) {
        dbus_bool_t tmp;
        dbus_message_iter_get_basic(&it, &tmp);
        g_value_init(val, G_TYPE_BOOLEAN);
        g_value_set_boolean(val, tmp);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_INT16) {
        dbus_int16_t _tmp15_;
        dbus_message_iter_get_basic(&it, &_tmp15_);
        g_value_init(val, G_TYPE_INT);
        g_value_set_int(val, _tmp15_);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_UINT16) {
        dbus_uint16_t _tmp16_;
        dbus_message_iter_get_basic(&it, &_tmp16_);
        g_value_init(val, G_TYPE_UINT);
        g_value_set_uint(val, _tmp16_);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_INT32) {
        dbus_int32_t _tmp17_;
        dbus_message_iter_get_basic(&it, &_tmp17_);
        g_value_init(val, G_TYPE_INT);
        g_value_set_int(val, _tmp17_);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_UINT32) {
        dbus_uint32_t _tmp18_;
        dbus_message_iter_get_basic(&it, &_tmp18_);
        g_value_init(val, G_TYPE_UINT);
        g_value_set_uint(val, _tmp18_);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_INT64) {
        dbus_int64_t _tmp19_;
        dbus_message_iter_get_basic(&it, &_tmp19_);
        g_value_init(val, G_TYPE_INT64);
        g_value_set_int64(val, _tmp19_);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_UINT64) {
        dbus_uint64_t _tmp20_;
        dbus_message_iter_get_basic(&it, &_tmp20_);
        g_value_init(val, G_TYPE_UINT64);
        g_value_set_uint64(val, _tmp20_);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_DOUBLE) {
        double _tmp21_;
        dbus_message_iter_get_basic(&it, &_tmp21_);
        g_value_init(val, G_TYPE_DOUBLE);
        g_value_set_double(val, _tmp21_);
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_STRING) {
        const char* _tmp22_;
        dbus_message_iter_get_basic(&it, &_tmp22_);
        g_value_init(val, G_TYPE_STRING);
        g_value_take_string(val, g_strdup(_tmp22_));
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_OBJECT_PATH) {
        const char* _tmp23_;
        dbus_message_iter_get_basic(&it, &_tmp23_);
        g_value_init(val, G_TYPE_STRING);
        g_value_take_string(val, g_strdup(_tmp23_));
    } else if (dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_SIGNATURE) {
        const char* _tmp24_;
        dbus_message_iter_get_basic(&it, &_tmp24_);
        g_value_init(val, G_TYPE_STRING);
        g_value_take_string(val, g_strdup(_tmp24_));
    } else if ((dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_ARRAY) &&
               (dbus_message_iter_get_element_type(&it) == DBUS_TYPE_STRING)) {
        const gchar** data = g_new(const gchar*, 5);
        int data_length = 0;
        int data_size = 4;
        int data_length1 = 0;

        DBusMessageIter m_it;
        dbus_message_iter_recurse(&it, &m_it);
        for (; dbus_message_iter_get_arg_type(&m_it); data_length1++) {
            if (data_size == data_length) {
                data_size = 2 * data_size;
                data = g_renew(const gchar*, data, data_size + 1);
            }
            const char* str;
            dbus_message_iter_get_basic(&m_it, &str);
            dbus_message_iter_next(&m_it);
            data[data_length++] = g_strdup(str);
        }
        data[data_length] = NULL;
        g_value_init(val, G_TYPE_STRV);
        g_value_take_boxed(val, data);
    }
}

void vala_dbus_append_gvalue(DBusMessageIter* it, const char* key,
                             GValue* val)
{
    DBusMessageIter it2;

    awn::vala_dbus_iter_append_string(it, key);
    if (G_VALUE_TYPE(val) == G_TYPE_UCHAR) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "y", &it2);
        awn::vala_dbus_iter_append_uint8(&it2, g_value_get_uchar(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_BOOLEAN) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "b", &it2);
        awn::vala_dbus_iter_append_bool(&it2, g_value_get_boolean(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_INT) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "i", &it2);
        awn::vala_dbus_iter_append_int32(&it2, g_value_get_int(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_UINT) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "u", &it2);
        awn::vala_dbus_iter_append_uint32(&it2, g_value_get_uint(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_INT64) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "x", &it2);
        awn::vala_dbus_iter_append_int64(&it2, g_value_get_int64(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_UINT64) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "t", &it2);
        awn::vala_dbus_iter_append_uint64(&it2, g_value_get_uint64(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_DOUBLE) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "d", &it2);
        awn::vala_dbus_iter_append_double(&it2, g_value_get_double(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_STRING) {
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "s", &it2);
        awn::vala_dbus_iter_append_string(&it2, g_value_get_string(val));
        dbus_message_iter_close_container(it, &it2);
    } else if (G_VALUE_TYPE(val) == G_TYPE_STRV) {
        DBusMessageIter it3;
        dbus_message_iter_open_container(it, DBUS_TYPE_VARIANT, "as", &it2);
        const char** _tmp74_ = g_value_get_boxed(val);
        dbus_message_iter_open_container(&it2, DBUS_TYPE_ARRAY, "s", &it3);
        for (int i = 0; i < g_strv_length(g_value_get_boxed(val)); i++) {
            awn::vala_dbus_iter_append_string(&it3, *_tmp74_++);
        }
        dbus_message_iter_close_container(&it2, &it3);
        dbus_message_iter_close_container(it, &it2);
    }
}

void vala_array_destroy(gpointer array, gint array_length, GDestroyNotify destroy_func)
{
    if ((array != NULL) && (destroy_func != NULL)) {
        int i;
        for (i = 0; i < array_length; i = i + 1) {
            if (((gpointer*) array)[i] != NULL) {
                destroy_func(((gpointer*) array)[i]);
            }
        }
    }
}

void vala_array_free(gpointer array, gint array_length, GDestroyNotify destroy_func)
{
    vala_array_destroy(array, array_length, destroy_func);
    g_free(array);
}

} // namespace awn
