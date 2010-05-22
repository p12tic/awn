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

public extern bool execute_wrapper (string cmd_line) throws GLib.Error;

public extern bool execute_python (string module_path,
                                   string args) throws GLib.Error;

const string dummy = Build.GETTEXT_PACKAGE;

string? path;
string? uid;
int64 window_xid = 0;
int panel_id = 1;
bool standalone = false;

const OptionEntry[] options = {
  {
    "path", 'p', 0, OptionArg.STRING,
    out path, "Path to the Awn applets desktop file.", ""
  },
  {
    "uid", 'u', 0, OptionArg.STRING,
    out uid, "The UID of this applet.", ""
  },
  {
    "window", 'w', 0, OptionArg.INT64,
    out window_xid, "The window to embed in.", ""
  },
  {
    "panel-id", 'i', 0, OptionArg.INT,
    out panel_id, "AwnPanel ID for the DBus connection.", ""
  },
  {
    "standalone", 's', 0, OptionArg.NONE,
    out standalone, "Force applet to run in standalone process", ""
  },
  {
    null
  }
};

namespace Awn
{
  [DBus (name = "org.awnproject.Awn.Panel")]
  public interface PanelDBusInterface: GLib.Object
  {
    public abstract void add_applet (string desktop_file) throws DBus.Error;
  }

  [DBus (name = "org.awnproject.Awn.AppletLauncher")]
  public interface DBusAppletLauncher: GLib.Object
  {
    public abstract void run_applet (AppletLaunchInfo applet) throws DBus.Error;
  }

  public struct AppletLaunchInfo
  {
    string path;
    string uid;
    int64 window_xid;
    int panel_id;
  }

  static bool i18n_initialized = false;

  class AppletStarter : GLib.Object, DBusAppletLauncher
  {
    public static int do_dbus_call ()
    {
      try
      {
        string desktop_file_path = path;
        if (!Path.is_absolute (desktop_file_path))
        {
          desktop_file_path = "%s/%s".printf (Environment.get_current_dir (),
                                              desktop_file_path);
        }

        var f = DesktopAgnostic.VFS.file_new_for_path (desktop_file_path);
        if (!f.exists ())
        {
          warning ("Desktop file '%s' does not exist!", desktop_file_path);
          return 1;
        }

        var connection = DBus.Bus.get (DBus.BusType.SESSION);
        var awn_panel = (PanelDBusInterface) connection.get_object (
          "org.awnproject.Awn",
          "/org/awnproject/Awn/Panel%d".printf (panel_id),
          "org.awnproject.Awn.Panel");
        awn_panel.add_applet (desktop_file_path);
      }
      catch (GLib.Error err)
      {
        return 1;
      }
      return 0;
    }

    public static void check_launch_info (AppletLaunchInfo info)
    {
      if (info.path == null || info.path == "")
      {
        error ("You need to provide path to desktop file");
      }

      if (info.uid == null && info.window_xid == 0)
      {
        do_dbus_call ();
      }

      if (info.uid == null || info.uid == "" || info.uid == "None")
      {
        error ("You need to provide a valid UID for this applet");
      }

      if (info.window_xid == 0)
      {
        // FIXME: this will be ok for dbus applets
        error ("You need to specify window ID for this applet!");
      }
    }

    public static AppletLaunchInfo get_launch_info ()
    {
      AppletLaunchInfo applet_info = AppletLaunchInfo ();
      applet_info.path = path;
      applet_info.uid = uid;
      applet_info.window_xid = window_xid;
      applet_info.panel_id = panel_id;

      check_launch_info (applet_info);

      return applet_info;
    }

    public static Awn.Applet? create_applet (string name, string exec,
                                             string uid, int panel_id)
    {
      var module = Module.open (exec,
                                ModuleFlags.BIND_LAZY | ModuleFlags.BIND_LOCAL);
      if (module == null)
      {
        error ("Unable to load module '%s'", exec);
      }

      void* module_func;
      if (module.symbol ("awn_applet_factory_init", out module_func))
      {
        Awn.AppletInitFunc init_func = (Awn.AppletInitFunc) module_func;
        var applet = new Awn.Applet (name, uid, panel_id);
        if (!init_func (applet))
        {
          warning ("Unable to create applet from factory!");
          return null;
        }
        module.make_resident ();
        return applet;
      }
      else if (module.symbol ("awn_applet_factory_initp", out module_func))
      {
        Awn.AppletInitPFunc initp_func = (Awn.AppletInitPFunc) module_func;
        module.make_resident ();
        return initp_func (name, uid, panel_id);
      }
      else
      {
        error ("No applet init function found!");
      }
      return null;
    }

    public static bool launch_applet_with (string app, string module,
                                           AppletLaunchInfo info)
    {
      string exec;
      if (Path.is_absolute (module))
      {
        exec = module;
      }
      else
      {
        exec = Path.build_filename (Path.get_dirname (info.path), module, null);
      }

      if (app == "python")
      {
        string fmt = "--uid=%s --panel-id=%d --window=%" + int64.FORMAT;
        string cmd = fmt.printf (info.uid, info.panel_id, info.window_xid);

        execute_python (exec, cmd);
      }
      else
      {
        string fmt = "%s %s --uid=%s --panel-id=%d --window=%" + int64.FORMAT;
        string cmd = fmt.printf (app, exec, info.uid,
                                 info.panel_id, info.window_xid);

        execute_wrapper (cmd);
      }
      return true;
    }

    public static bool launch_applet (AppletLaunchInfo info)
    {
      var f = DesktopAgnostic.VFS.file_new_for_path (info.path);
      if (f == null || !f.exists ())
      {
        error ("The desktop file '%s' does not exist.", info.path);
      }

      var de = DesktopAgnostic.FDO.desktop_entry_new_for_file (f);
      if (de == null)
      {
        error ("Unable to load desktop entry '%s'", info.path);
      }

      var exec = de.get_string ("X-AWN-AppletExec");
      if (exec == null || exec == "")
      {
        error ("No X-AWN-AppletExec key found in desktop file '%s', exiting.",
               info.path);
      }
      var name = de.name;
      var type = de.get_string ("X-AWN-AppletType");
      if (type == null || type == "")
      {
        error ("No X-AWN-AppletType key found in desktop file '%s', exiting.",
               info.path);
      }

      if (!i18n_initialized)
      {
        Intl.bindtextdomain (Build.GETTEXT_PACKAGE, Build.LOCALEDIR);
        i18n_initialized = true;
      }

      if (type == "Python" || type == "Mono")
      {
        launch_applet_with (type.down (), exec, info);
        return true;
      }
      else
      {
        // proceed with native C/Vala applet
        unowned string? canonical_name = exec.rstr ("/");
        canonical_name = canonical_name == null ? 
          exec : canonical_name.offset (1);
        unowned string? dot = canonical_name.rstr (".");
        string canon_name = canonical_name.ndup (dot != null ? 
          (char*)dot - (char*)canonical_name : canonical_name.size ());

        var applet = create_applet (canon_name, exec, info.uid, info.panel_id);
        if (info.window_xid != 0)
        {
          applet.@construct ((Gdk.NativeWindow)info.window_xid);
        }
      }

      return true;
    }

    public void run ()
    {
      AppletLaunchInfo applet = get_launch_info ();
      debug ("launching %s, %s", applet.path, applet.uid);

      var connection = Bus.get (BusType.SESSION);
      dynamic DBus.Object bus = connection.get_object ("org.freedesktop.DBus",
                                                       "/org/freedesktop/DBus",
                                                       "org.freedesktop.DBus");

      uint request_name_result = bus.request_name ("org.awnproject.Awn.AppletLauncher", (uint) NameFlag.DO_NOT_QUEUE);
      if (request_name_result == RequestNameReply.PRIMARY_OWNER)
      {
        connection.register_object ("/org/awnproject/Awn/AppletLauncher", 
                                    this);
            
        this.run_applet (applet);

        Gtk.main ();
      }
      else
      {
        var launcher = (DBusAppletLauncher)
          connection.get_object ("org.awnproject.Awn.AppletLauncher",
                                 "/org/awnproject/Awn/AppletLauncher",
                                 "org.awnproject.Awn.AppletLauncher");

        launcher.run_applet (applet);

        // FIXME: panel will display crash icon if this process exits
        //Gtk.main ();
      }
    }

    public void run_applet (AppletLaunchInfo applet_info)
    {
      launch_applet (applet_info);
    }

    public static int main (string[] argv)
    {
      var context = new OptionContext (" - Awn Applet Activation Options");
      context.add_main_entries (options, null);
      context.parse (ref argv);

      DesktopAgnostic.VFS.init ();
      Gtk.init (ref argv);

      var launcher = new AppletStarter ();
      launcher.run ();

      DesktopAgnostic.VFS.shutdown ();

      return 0;
    }
  }
}

