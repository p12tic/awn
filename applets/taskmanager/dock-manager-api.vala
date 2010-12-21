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

[DBus (name="net.launchpad.DockManager")]
public interface DockManagerDBusInterface: GLib.Object
{
  public abstract string[] get_capabilities () throws DBus.Error;
  public abstract ObjectPath[] get_items () throws DBus.Error;
  public abstract ObjectPath[] get_items_by_name (string name) throws DBus.Error;
  public abstract ObjectPath[] get_items_by_desktop_file (string desktop_file) throws DBus.Error;
  public abstract ObjectPath[] get_items_by_pid (int pid) throws DBus.Error;
  public abstract ObjectPath get_item_by_xid (int64 xid) throws DBus.Error;

  // Awn-specific methods
  public abstract void awn_set_visibility (string win_name, bool visible) throws DBus.Error;
  public abstract ObjectPath awn_register_proxy_item (string desktop_file, string uri) throws DBus.Error;

  public signal void item_added (ObjectPath path);
  public signal void item_removed (ObjectPath path);
}

[DBus (name="net.launchpad.DockItem")]
public interface DockItemDBusInterface: GLib.Object
{
  public abstract int add_menu_item (HashTable<string, Value?> menu_hints) throws DBus.Error;
  public abstract void remove_menu_item (int id) throws DBus.Error;
  public abstract void update_dock_item (HashTable<string, Value?> hints) throws DBus.Error;

  public abstract string desktop_file { owned get; }
  public abstract string uri { owned get; }

  public signal void menu_item_activated (int id);
}

public class TaskManagerDispatcher: GLib.Object, DockManagerDBusInterface
{
  public Task.Manager manager { get; construct; }

  public TaskManagerDispatcher (Task.Manager manager)
  {
    GLib.Object (manager: manager);

    /*
    manager.some_signal.connect ( (m, i) =>
    {
      this.item_added (i);
    });
    */

    var conn = DBus.Bus.get (DBus.BusType.SESSION);
    string obj_path = "/net/launchpad/DockManager";
    conn.register_object (obj_path, this);
  }

  private static ObjectPath[] list_to_object_path_array (SList<Task.Icon> list)
  {
    ObjectPath[] result = new ObjectPath[list.length ()];
    int i = 0;
    foreach (unowned Task.Icon icon in list)
    {
      unowned TaskIconDispatcher dispatcher;
      dispatcher = icon.get_dbus_dispatcher () as TaskIconDispatcher;
      result[i++] = new ObjectPath (dispatcher.object_path);
    }

    return result;
  }

  public string[] get_capabilities () throws DBus.Error
  {
    string[] capabilities =
    {
      "dock-item-badge",
      "dock-item-message",
      "dock-item-progress",
      "dock-item-icon-file",
      "menu-item-container-title",
      "menu-item-with-label",
      "menu-item-icon-name",
      "menu-item-icon-file",
      "x-awn-set-visibility"
      // "x-awn-register-proxy-item" // TODO: later
    };
    return capabilities;
  }

  public ObjectPath[] get_items () throws DBus.Error
  {
    return list_to_object_path_array (this.manager.get_icons ());
  }

  public ObjectPath[] get_items_by_name (string name) throws DBus.Error
  {
    unowned SList<unowned Task.Icon> icons = this.manager.get_icons ();
    SList<unowned Task.Icon> matches = new SList<unowned Task.Icon> ();

    foreach (unowned Task.Icon icon in icons)
    {
      foreach (unowned Task.Item item in icon.get_items ())
      {
        unowned Task.Window? window = item as Task.Window;
        if (window != null)
        {
          if (window.matches_wmclass (name))
          {
            matches.append (icon);
            break;
          }
        }
      }
    }

    return list_to_object_path_array (matches);
  }

  public ObjectPath[] get_items_by_desktop_file (string desktop_file) throws DBus.Error
  {
    unowned SList<unowned Task.Icon> icons = this.manager.get_icons ();
    SList<unowned Task.Icon> matches = new SList<unowned Task.Icon> ();

    foreach (unowned Task.Icon icon in icons)
    {
      unowned Task.Launcher? launcher = icon.get_launcher () as Task.Launcher;
      if (launcher != null)
      {
        if (launcher.get_desktop_path ().has_suffix (desktop_file))
        {
          matches.append (icon);
        }
      }
    }

    return list_to_object_path_array (matches);
  }

  public ObjectPath[] get_items_by_pid (int pid) throws DBus.Error
  {
    unowned SList<unowned Task.Icon> icons = this.manager.get_icons ();
    SList<unowned Task.Icon> matches = new SList<unowned Task.Icon> ();

    foreach (unowned Task.Icon icon in icons)
    {
      foreach (unowned Task.Item item in icon.get_items ())
      {
        if (item is Task.Window)
        {
          unowned Task.Window window = item as Task.Window;
          if (window.get_pid () == pid)
          {
            matches.append (icon);
            break;
          }
        }
      }
    }

    return list_to_object_path_array (matches);
  }

  public ObjectPath get_item_by_xid (int64 xid) throws DBus.Error
  {
    unowned Task.Icon? icon = this.manager.get_icon_by_xid (xid);

    if (icon != null)
    {
      unowned TaskIconDispatcher dispatcher;
      dispatcher = icon.get_dbus_dispatcher () as TaskIconDispatcher;

      return new ObjectPath (dispatcher.object_path);
    }

    return null;
  }

  public void 
  awn_set_visibility (string win_name, bool visible) throws DBus.Error
  {
    HashTable<string, Value?> hints;
    hints = new HashTable<string, Value?> (str_hash, str_equal);
    hints.insert ("visible", visible);

    this.manager.update (win_name, hints);
  }

  public ObjectPath 
  awn_register_proxy_item (string desktop_file, string uri) throws DBus.Error
  {
    // TODO: implement
    return new ObjectPath ("/not/yet/implemented");
  }
}

public class TaskIconDispatcher: GLib.Object, DockItemDBusInterface
{
  private unowned Task.Icon icon;

  public string object_path { get; set; }

  public string desktop_file
  {
    owned get
    {
      string path = "";
      unowned Task.Launcher? launcher;
      launcher = this.icon.get_launcher () as Task.Launcher;
      if (launcher != null)
      {
        path = launcher.get_desktop_path ();
        path = path.replace ("//", "/");
      }
      return path;
    }
  }

  public string uri
  {
    owned get
    {
      return "";
    }
  }

  static int counter = 1;

  public TaskIconDispatcher (Task.Icon icon)
  {
    this.icon = icon;

    var conn = DBus.Bus.get (DBus.BusType.SESSION);
    this.object_path = "/net/launchpad/DockManager/Item%d".printf (counter++);
    conn.register_object (this.object_path, this);

    this.emit_item_added ();
  }

  private unowned TaskManagerDispatcher? get_manager_proxy ()
  {
    unowned TaskManagerDispatcher? proxy;
    unowned Task.Manager manager = this.icon.get_applet () as Task.Manager;
    proxy = manager.get_dbus_dispatcher () as TaskManagerDispatcher;

    return proxy;
  }

  private void emit_item_added ()
  {
    unowned TaskManagerDispatcher? proxy = this.get_manager_proxy ();

    if (proxy != null)
    {
      proxy.item_added (new ObjectPath(this.object_path));
    }
  }

  ~TaskIconDispatcher ()
  {
    unowned TaskManagerDispatcher? proxy = this.get_manager_proxy ();

    if (proxy != null)
    {
      proxy.item_removed (new ObjectPath(this.object_path));
    }
  }

  public int add_menu_item (HashTable<string, Value?> menu_hints) throws DBus.Error
  {
    Gtk.ImageMenuItem? item = null;
    Gtk.Image? image = null;
    string? group = null;

    HashTableIter<string, Value?> iter =
      HashTableIter<string, Value?>(menu_hints);
    unowned string key;
    unowned Value? value;
    while (iter.next (out key, out value))
    {
      if (key == "label")
      {
        item = new Gtk.ImageMenuItem.with_label (value.get_string ());
      }
      else if (key == "icon-name")
      {
        image = new Gtk.Image.from_icon_name (value.get_string (),
                                              Gtk.IconSize.MENU);
      }
      else if (key == "icon-file")
      {
        Gdk.Pixbuf pixbuf;
        int w, h;
        Gtk.icon_size_lookup (Gtk.IconSize.MENU, out w, out h);
        try
        {
          pixbuf = new Gdk.Pixbuf.from_file_at_size (value.get_string (),
                                                     w, h);
          image = new Gtk.Image.from_pixbuf (pixbuf);
        }
        catch (GLib.Error err) { warning ("%s", err.message); }
      }
      else if (key == "container-title")
      {
        group = value.get_string ();
      }
      else if (key == "uri")
      {
        // TODO: implement
      }
    }

    if (item != null)
    {
      if (image != null) item.set_image (image);
      int id = this.icon.add_menu_item (item,group);
      item.show ();

      item.activate.connect ((w) =>
      {
        this.menu_item_activated (id);
      });

      return id;
    }

    return 0;
  }

  public void remove_menu_item (int id) throws DBus.Error
  {
    this.icon.remove_menu_item (id);
  }

  public void update_dock_item (HashTable<string, Value?> hints) throws DBus.Error
  {
    HashTableIter<string, Value?> iter = HashTableIter<string, Value?> (hints);
    unowned string key;
    unowned Value? value;
    while (iter.next (out key, out value))
    {
      unowned SList<unowned Task.Item> items = this.icon.get_items ();
      foreach (unowned Task.Item item in items)
      {
        if (item is Task.Launcher) continue;
        item.update_overlay (key, value);
      }
    }
  }
}

