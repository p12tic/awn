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
public interface DockManagerDBusInterface: GLib.Object
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

[DBus (name="org.freedesktop.DockItem")]
public interface DockItemDBusInterface: GLib.Object
{
  public abstract int add_menu_item (HashTable<string, Value?> menu_hints) throws DBus.Error;
  public abstract void remove_menu_item (int id) throws DBus.Error;
  public abstract void update_dock_item (HashTable<string, Value?> hints) throws DBus.Error;

  public signal void menu_item_activated (int id);
}

public class TaskManagerDispatcher: GLib.Object, DockManagerDBusInterface
{
  public Task.Manager manager { get; construct; }

  // TODO: remove!
  private TaskIconDispatcher test_item;

  public TaskManagerDispatcher (Task.Manager manager)
  {
    GLib.Object (manager: manager);

    /*
    manager.some_signal.connect ( (m, i) =>
    {
      this.item_added (i);
    });
    */

    var conn = Bus.get (BusType.SESSION);
    string obj_path = "/org/freedesktop/DockManager";
    conn.register_object (obj_path, this);

    test_item = new TaskIconDispatcher ();
  }

  public string[] get_capabilities () throws DBus.Error
  {
    string[] capabilities = {""};
    return capabilities;
  }

  public ObjectPath[] get_items () throws DBus.Error
  {
    ObjectPath[] paths = new ObjectPath[1];
    paths[0] = new ObjectPath (test_item.object_path);
    return paths;
  }

  public ObjectPath[] get_items_by_name (string name) throws DBus.Error
  {
    return null;
  }

  public ObjectPath[] get_items_by_desktop_file (string desktop_file) throws DBus.Error
  {
    unowned SList<Task.Icon> icons = manager.get_icons ();

    foreach (Task.Icon icon in icons)
    {
      unowned Task.Launcher? launcher = icon.get_launcher () as Task.Launcher;
      if (launcher != null)
      {
        if (desktop_file == launcher.get_desktop_path ())
        {
          debug ("desktop_file match: %p", icon);
          // TODO: add to a list
        }
      }
    }
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

public class TaskIconDispatcher: GLib.Object, DockItemDBusInterface
{
  public string object_path { get; set; }

  // FIXME: if TaskManagerDispatcher had only get_default constructor without
  //   parameters, we could just emit its item_added signal in our constructor
  public TaskIconDispatcher ()
  {
    var conn = Bus.get (BusType.SESSION);
    this.object_path = "/org/freedesktop/DockItem/%p".printf (this);
    conn.register_object (this.object_path, this);
  }

  public int add_menu_item (HashTable<string, Value?> menu_hints) throws DBus.Error
  {
    debug ("called add_menu_item method");
    HashTableIter<string, Value?> iter =
      HashTableIter<string, Value?>(menu_hints);
    unowned string key;
    unowned Value? value;
    while (iter.next (out key, out value))
    {
      debug ( "%s: %s", key, value.strdup_contents ());
    }

    int id = 12345;
    return id;
  }

  public void remove_menu_item (int id) throws DBus.Error
  {
  }

  public void update_dock_item (HashTable<string, Value?> hints) throws DBus.Error
  {
    debug ("called update_dock_item method");
    HashTableIter<string, Value?> iter = HashTableIter<string, Value?> (hints);
    unowned string key;
    unowned Value? value;
    while (iter.next (out key, out value))
    {
      debug ( "%s: %s", key, value.strdup_contents ());
    }
  }
}
