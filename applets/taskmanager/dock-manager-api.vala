/*
 * Copyright (C) 2010 Michal Hruby <michal.mhr@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * Authored by Michal Hruby <michal.mhr@gmail.com>
 */

using DBus;

[DBus (name="org.freedesktop.DockManager")]
public interface TaskManagerDBusInterface: GLib.Object
{
  public abstract string[] get_capabilities () throws DBus.Error;
  public abstract ObjectPath[] get_items () throws DBus.Error;
  public abstract ObjectPath[] get_items_by_name (string name) throws DBus.Error;
  public abstract ObjectPath[] get_items_by_desktop_file (string desktop_file) throws DBus.Error;
  public abstract ObjectPath[] get_items_by_pid (int pid) throws DBus.Error;
  public abstract ObjectPath get_item_by_xid (int64 xid) throws DBus.Error;

  public signal void item_added (ObjectPath path);
  public signal void item_removed (ObjectPath path);
}

public class TaskManagerDispatcher: GLib.Object, TaskManagerDBusInterface
{
  public TaskManager manager { get; construct; }

  public TaskManagerDispatcher (TaskManager manager)
  {
    GLib.Object (manager: manager);

    /*
    manager.some_signal.connect ( (m, i) =>
    {
      this.item_added (i);
    });
    */

    var conn = Bus.get (BusType.SESSION);
    string obj_path = "/org/freedektop/DockManager";
    conn.register_object (obj_path, this);
  }

  public string[] get_capabilities () throws DBus.Error
  {
    string[] capabilities = {""};
    return capabilities;
  }

  public ObjectPath[] get_items () throws DBus.Error
  {
    return null;
  }

  public ObjectPath[] get_items_by_name (string name) throws DBus.Error
  {
    return null;
  }

  public ObjectPath[] get_items_by_desktop_file (string desktop_file) throws DBus.Error
  {
    return null;
  }

  public ObjectPath[] get_items_by_pid (int pid) throws DBus.Error
  {
    return null;
  }

  public ObjectPath get_item_by_xid (int64 xid) throws DBus.Error
  {
    return null;
  }
}
