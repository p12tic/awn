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
  public struct ImageStruct
  {
    int width;
    int height;
    int rowstride;
    bool has_alpha;
    int bits_per_sample;
    int num_channels;
    char[] pixel_data;
  }

  [DBus (name="org.awnproject.Awn.Panel")]
  public interface PanelDBusInterface: Object
  {
    public abstract void add_applet (string desktop_file) throws DBus.Error;
    public abstract void delete_applet (string uid) throws DBus.Error;

    public abstract int64 docklet_request (int min_size,
                                           bool shrink,
                                           bool expand) throws DBus.Error;

    public abstract string[] get_inhibitors () throws DBus.Error;
    public abstract ImageStruct get_snapshot () throws DBus.Error;

    public abstract uint inhibit_autohide (DBus.BusName sender,
                                           string app_name,
                                           string reason) throws DBus.Error;
    public abstract void uninhibit_autohide (uint cookie) throws DBus.Error;

    // FIXME: do we really need this?
    public abstract void set_applet_flags (string uid,
                                           int flags) throws DBus.Error;

    public abstract void set_glow (DBus.BusName sender,
                                   bool activate) throws DBus.Error;

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
    public unowned Panel panel { get; construct; }

    public PanelDispatcher (Panel panel)
    {
      Object (panel: panel);

      panel.size_changed.connect ( (p, s) =>
      {
        this.property_changed ("size", s);
      });
      panel.position_changed.connect ( (p, pos) =>
      {
        this.property_changed ("position", pos);
      });
      panel.offset_changed.connect ( (p, o) =>
      {
        this.property_changed ("offset", o);
      });
      panel.property_changed.connect ( (p, pn, v) =>
      {
        this.property_changed (pn, v);
      });

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

    // FIXME: break this API and add the docklet flags from
    //        set_applet_flags () method here
    public int64 docklet_request (int min_size, bool shrink, bool expand) throws DBus.Error
    {
      return panel.docklet_request (min_size, shrink, expand);
    }

    public string[] get_inhibitors () throws DBus.Error
    {
      string[] reasons = panel.get_inhibitors ();

      return reasons;
    }

    public ImageStruct get_snapshot () throws DBus.Error
    {
      var result = ImageStruct ();
      panel.get_snapshot (out result.width,
                          out result.height,
                          out result.rowstride,
                          out result.has_alpha,
                          out result.bits_per_sample,
                          out result.num_channels,
                          out result.pixel_data);

      return result;
    }

    public uint inhibit_autohide (DBus.BusName sender,
                                  string app_name,
                                  string reason) throws DBus.Error
    {
      return panel.inhibit_autohide (sender, app_name, reason);
    }

    public void uninhibit_autohide (uint cookie) throws DBus.Error
    {
      panel.uninhibit_autohide (cookie);
    }

    public void set_applet_flags (string uid, int flags) throws DBus.Error
    {
      panel.set_applet_flags (uid, flags);
    }

    public void set_glow (DBus.BusName sender, bool activate) throws DBus.Error
    {
      panel.set_glow (sender, activate);
    }

    public double offset_modifier
    {
      get
      {
        return panel.offset_modifier;
      }
    }

    public int max_size
    {
      get
      {
        return panel.max_size;
      }
    }

    public int offset
    {
      get
      {
        return panel.offset;
      }
      set
      {
        panel.offset = value;
      }
    }

    public int path_type
    {
      get
      {
        return panel.path_type;
      }
    }

    public int position
    {
      get
      {
        return panel.position;
      }
      set
      {
        panel.position = value;
      }
    }

    public int size
    {
      get
      {
        return panel.size;
      }
      set
      {
        panel.size = value;
      }
    }

    public int64 panel_xid
    {
      get
      {
        return panel.panel_xid;
      }
    }
  }
}
