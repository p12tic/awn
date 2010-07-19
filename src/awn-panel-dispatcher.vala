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
 *
 */

namespace Awn
{
  [DBus (name="org.awnproject.Awn.Panel")]
  public interface PanelDBusInterface: Object
  {
    public abstract void add_applet (string desktop_file) throws DBus.Error;
    public abstract void delete_applet (string uid) throws DBus.Error;

    public abstract void docklet_request (int min_size,
                                          bool shrink,
                                          bool expand) throws DBus.Error;

    public abstract string[] get_inhibitors () throws DBus.Error;
    public abstract Value get_snapshot () throws DBus.Error;

    public abstract uint inhibit_autohide (DBus.BusName sender,
                                           string app_name,
                                           string reason) throws DBus.Error;
    public abstract void uninhibit_autohide (uint cookie) throws DBus.Error;

    // FIXME: do we really need this?
    public abstract void set_applet_flags (string uid,
                                           int flags) throws DBus.Error;

    public abstract double offset_modifier { get; }
    public abstract int max_size { get; }
    public abstract int offset { get; set; }
    public abstract int path_type { get; }
    public abstract int position { get; set; }
    public abstract int size { get; set; }
    public abstract int64 panel_xid { get; }

    public signal void destroy_applet (string uid);
    public signal void destroy_notify ();
    public signal void property_changed (string prop_name, Value value);
  }

  public class PanelDispatcher: Object, PanelDBusInterface
  {
    public Panel panel { get; construct; }

    public PanelDispatcher (Panel panel)
    {
      Object (panel: panel);

      var conn = DBus.Bus.get (DBus.BusType.SESSION);
      string obj_path = "/org/awnproject/Awn/Panel%d".printf(panel.panel_id);
      conn.register_object (obj_path, this);
    }

    public void add_applet (string desktop_file) throws DBus.Error
    {
      panel.add_applet (desktop_file);
    }
    public void delete_applet (string uid) throws DBus.Error
    {
      panel.delete_applet (uid);
    }

    public void docklet_request (int min_size, bool shrink, bool expand) throws DBus.Error
    {
    }

    public string[] get_inhibitors () throws DBus.Error
    {
      string[] reasons;
      panel.get_inhibitors (out reasons);

      return reasons;
    }

    public Value get_snapshot () throws DBus.Error
    {
      Value val;
      panel.get_snapshot (out val);

      return val;
    }

    public uint inhibit_autohide (DBus.BusName sender, string app_name, string reason) throws DBus.Error
    {
      return panel.inhibit_autohide (sender, app_name, reason);
    }

    public void uninhibit_autohide (uint cookie) throws DBus.Error
    {
      panel.uninhibit_autohide (cookie);
    }

    public void set_applet_flags (string uid, int flags) throws DBus.Error
    {
    }

    public double offset_modifier { get { return panel.offset_modifier; } }
    public int max_size { get { return panel.max_size; } }
    public int offset { get { return panel.offset; } set { panel.offset = value;} }
    public int path_type { get { return panel.path_type; } }
    public int position { get { return panel.position; } set { panel.position = value; } }
    public int size { get { return panel.size; } set { panel.size = value; } }
    public int64 panel_xid { get { return panel.panel_xid; } }
  }
  /*
  public class Test: Object, PanelDBusInterface
  {
    public static void main (string[] args)
    {
      MainLoop ml = new MainLoop ();

      Test t = new Test ();

      var conn = DBus.Bus.get (DBus.BusType.SESSION);
      conn.register_object ("/org/example/test", t);

      ml.run ();
    }

    public void add_applet (string desktop_file) throws DBus.Error
    {
    }
    public void delete_applet (string uid) throws DBus.Error
    {
    }

    public void docklet_request (int min_size, bool shrink, bool expand) throws DBus.Error
    {
    }
    public string[] get_inhibitors () throws DBus.Error
    {
      string[] inhibitors = {""};

      return inhibitors;
    }
    public Value get_snapshot () throws DBus.Error
    {
      Value v = 3;
      return v;
    }
    public uint inhibit_autohide (DBus.BusName sender, string app_name, string reason) throws DBus.Error
    {
      return 1;
    }
    public void uninhibit_autohide (uint cookie) throws DBus.Error
    {
    }

    public void set_applet_flags (string uid, int flags) throws DBus.Error
    {
    }

    public double offset_modifier { get { return 1.0; } }
    public int max_size { get { return 100; } }
    public int offset { get; set; }
    public int path_type { get { return 1; } }
    public int position { get; set; }
    public int size { get; set; }
    public int64 panel_xid { get { return 1234567890; } }
  }
  */
}
