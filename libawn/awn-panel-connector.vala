/*
 * Copyright (C) 2010 Michal Hruby <michal.mhr@gmail.com>
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
 * Authored by Michal Hruby <michal.mhr@gmail.com>
 *
 */

namespace Awn
{
  public interface PanelConnector: Object
  {
    public void connect (DBus.Connection con, out DBus.Object proxy) throws DBus.Error
    {
      if (this.panel_id > 0)
      {
        dynamic DBus.Object panel =
          con.get_object ("org.awnproject.Awn",
                          "/org/awnproject/Awn/Panel%d".printf (this.panel_id),
                          "org.awnproject.Awn.Panel");

        panel.PropertyChanged += this.on_property_changed;
        panel.DestroyApplet += this.on_applet_destroy;

        proxy = (owned)panel;
        // FIXME: uncomment once in official Vala release
        //proxy.destroy.connect (this.on_proxy_destroyed);

        // initialize properties
        dynamic DBus.Object props =
          con.get_object (proxy.get_bus_name (),
                          proxy.get_path (),
                          "org.freedesktop.DBus.Properties");

        HashTable<string, Value?> table = 
          props.GetAll ("org.awnproject.Awn.Panel");
        HashTableIter<string, Value?> iter = 
          HashTableIter<string, Value?>(table);
        unowned string key;
        unowned Value? value;
        while (iter.next (out key, out value))
        {
          this.property_changed (key, value);
        }
      }
      else
      {
        error ("Panel-id is not set to correct value!");
      }
    }

    public abstract void property_changed (string prop_name, Value value);

    private void on_property_changed (dynamic DBus.Object proxy,
                                      string prop_name, Value value)
    {
      this.property_changed (prop_name, value);
    }

    private void on_applet_destroy (dynamic DBus.Object proxy,
                                    string uid)
    {
      if (this.uid == uid)
      {
        this.applet_deleted ();
      }
    }

    public virtual void on_proxy_destroyed ()
    {
      Gtk.main_quit ();
    }

    public uint inhibit_autohide (string reason)
    {
      string app_name = "%s:%d".printf (Environment.get_prgname (),
                                        Posix.getpid ());
      dynamic unowned DBus.Object panel = this.panel_proxy;
      return panel.InhibitAutohide (app_name, reason);
    }

    public void uninhibit_autohide (uint cookie)
    {
      dynamic unowned DBus.Object panel = this.panel_proxy;
      panel.UninhibitAutohide (cookie);
    }

    public int64 bus_docklet_request (int min_size, bool shrink, bool expand)
    {
      int64 window_id;
      dynamic unowned DBus.Object panel = this.panel_proxy;

      try
      {
        window_id = panel.DockletRequest (min_size, shrink, expand);
      }
      catch
      {
        window_id = 0;
      }

      return window_id;
    }

    public void bus_set_behavior (AppletFlags behavior)
    {
      dynamic unowned DBus.Object panel = this.panel_proxy;

      panel.SetAppletFlags (this.uid, (int)behavior);
    }

    public abstract int panel_id { get; set construct; }
    public abstract DBus.Object panel_proxy { get; }
    public abstract string uid { get; set construct; }
    public abstract Gtk.PositionType position { get; set; }
    public abstract int offset { get; set; }
    public abstract int size { get; set; }

    public signal void applet_deleted ();
  }
}
