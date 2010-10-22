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

using DBus;

namespace Awn
{
  [DBus (name="org.awnproject.Awn.App")]
  public interface AppDBusInterface: GLib.Object
  {
    public abstract string[] get_panels () throws DBus.Error;
    public abstract void remove_panel (int panel_id) throws DBus.Error;
    public signal void panel_added (int panel_id);
    public signal void panel_removed (int panel_id);
  }

  public class Application: GLib.Object, AppDBusInterface
  {
    private static Application instance;

    private HashTable<string, Panel> panels;
    private Connection connection;
    private DesktopAgnostic.Config.Client client;

    private Application ()
    {
      this.panels = new HashTable<string, Panel>.full (str_hash, str_equal,
                                                       g_free, g_object_unref);
      this.client = Config.get_default (0);

      Gtk.Window.set_default_icon_name ("avant-window-navigator");

      this.connection = DBus.Bus.get (DBus.BusType.SESSION);
      this.connection.register_object ("/org/awnproject/Awn", this);

      try
      {
        var panel_ids = this.client.get_list ("panels", "panel_list");

        if (panel_ids.n_values == 0)
        {
          bool is_gconf = false;
          Type config_type = DesktopAgnostic.Config.get_type ();
          if (config_type > 0)
          {
            is_gconf = config_type.name ().str ("GConf") != null;
          }
          error ("No panels to create! %s", is_gconf ?
            "\n**  Please check that the gconf-schema is installed." +
            "\n**  You might want to try running `killall gconfd-2`." : "");
        }

        foreach (Value val in panel_ids)
        {
          int panel_id = val.get_int ();
          string path = "/org/awnproject/Awn/Panel%d".printf (panel_id);

          Panel panel = new Panel.with_panel_id (panel_id);

          this.panels.insert ((owned)path, panel);

          panel.show ();
        }

        this.client.notify_add ("panels", "panel_list", this.panels_changed);
      }
      catch (GLib.Error e)
      {
        error ("Unable to retrieve panels config value: %s", e.message);
      }
    }

    private void panels_changed (string key, string group, Value val)
    {
      List<unowned Panel> untouched_panels = new List<unowned Panel> ();
      foreach (var p in this.panels.get_values ())
      {
        untouched_panels.append (p);
      }

      unowned ValueArray arr = (ValueArray) val.get_boxed ();
      foreach (Value v in arr)
      {
        int panel_id = v.get_int ();
        string path = "/org/awnproject/Awn/Panel%d".printf (panel_id);
        unowned Panel? p = this.panels.lookup (path);

        if (p == null)
        {
          var panel = new Panel.with_panel_id (panel_id);
          this.panels.insert ((owned)path, panel);

          panel.show ();
          this.panel_added (panel_id);
        }
        else
        {
          untouched_panels.remove (p);
        }
      }

      foreach (unowned Panel p in untouched_panels)
      {
        int panel_id = p.panel_id;
        string path = "/org/awnproject/Awn/Panel%d".printf (p.panel_id);
        this.panels.remove (path);
        this.panel_removed (panel_id);
        p.destroy ();
      }
    }

    public string[] get_panels () throws DBus.Error
    {
      var keys = this.panels.get_keys ();
      keys.sort (strcmp);

      string[] paths = new string[keys.length ()];
      int i = 0;
      foreach (unowned string path in keys)
      {
        paths[i++] = path;
      }
      return paths;
    }

    public void remove_panel (int panel_id) throws DBus.Error
    {
      if (panel_id == 1)
      {
        Gtk.main_quit ();
      }
      else
      {
        // TODO
      }
    }

    public static unowned Application get_default ()
    {
      if (instance == null)
      {
        instance = new Application ();
      }
      return instance;
    }
  }
}

