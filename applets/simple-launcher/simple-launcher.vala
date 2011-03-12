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
 * Author : Michal Hruby <michal.mhr@gmail.com>
 */

using Awn;
using DesktopAgnostic;
//using Zeitgeist;

class SimpleLauncher : Applet
{
  static const string DESKTOP_ENTRY = "desktop-entry-object";

  //private Zeitgeist.Log zg_log = new Zeitgeist.Log ();
  private IconBox icon_box;
  private Awn.ThemedIcon add_icon;
  private Gtk.Menu menu;
  private Gtk.Menu add_icon_menu;
  private Gtk.MenuItem remove_menu_item;
  private Gtk.MenuItem edit_menu_item;
  private DesktopAgnostic.Config.Client client;
  private string config_dir;
  private GenericArray<Awn.ThemedIcon> launchers;
  private uint timer_id = 0;

  private ValueArray _launcher_list = new ValueArray (4);
  public ValueArray launcher_list
  {
    get
    {
      return _launcher_list;
    }
    set
    {
      _launcher_list = value.copy ();
    }
  }

  const Gtk.TargetEntry[] targets = {
    { "text/uri-list", 0, 0 },
    { "text/plain",    0, 0 }
  };

  public SimpleLauncher (string canonical_name, string uid, int panel_id)
  {
    Object (canonical_name: canonical_name, uid: uid, panel_id: panel_id);

    init_widgets ();

    this.notify["launcher-list"].connect (this.launchers_changed);
    launchers_changed ();
  }

  private void init_widgets ()
  {
    // initialize popup menus
    var about_item1 = 
      this.create_about_item_simple ("Copyright © 2010 Michal Hruby",
                                     AppletLicense.GPLV2,
                                     Build.VERSION) as Gtk.MenuItem;
    var about_item2 = 
      this.create_about_item_simple ("Copyright © 2010 Michal Hruby",
                                     AppletLicense.GPLV2,
                                     Build.VERSION) as Gtk.MenuItem;

    // launcher menu
    menu = this.create_default_menu () as Gtk.Menu;
    var add_launcher_item1 = new Gtk.MenuItem.with_label ("Add Launcher...");
    add_launcher_item1.activate.connect (this.create_new_launcher);
    menu.append (add_launcher_item1);
    menu.append (new Gtk.SeparatorMenuItem ());
    edit_menu_item = new Gtk.MenuItem.with_label ("Edit Launcher");
    edit_menu_item.activate.connect (this.edit_clicked);
    menu.append (edit_menu_item);
    remove_menu_item = new Gtk.MenuItem.with_label ("Remove Launcher");
    remove_menu_item.activate.connect (this.remove_clicked);
    menu.append (remove_menu_item);
    menu.append (new Gtk.SeparatorMenuItem ());
    menu.append (about_item1);
    menu.show_all ();

    // add icon menu
    add_icon_menu = this.create_default_menu () as Gtk.Menu;
    var add_launcher_item2 = new Gtk.MenuItem.with_label ("Add Launcher...");
    add_launcher_item2.activate.connect (this.create_new_launcher);
    add_icon_menu.append (add_launcher_item2);
    var add_folders_item = new Gtk.MenuItem.with_label ("Add common folders");
    add_folders_item.activate.connect (this.add_folders);
    add_icon_menu.append (add_folders_item);
    add_icon_menu.append (new Gtk.SeparatorMenuItem ());
    add_icon_menu.append (about_item2);
    add_icon_menu.show_all ();

    icon_box = new IconBox.for_applet (this);

    add_icon = new Awn.ThemedIcon ();
    add_icon.drag_and_drop = false;
    add_icon.set_size (this.size);
    Gtk.drag_dest_set (add_icon,
                       Gtk.DestDefaults.MOTION | Gtk.DestDefaults.DROP,
                       targets,
                       Gdk.DragAction.COPY);
    (add_icon as Gtk.Widget).drag_data_received.connect (this.uri_received);
    add_icon.drag_motion.connect_after (() =>
    {
      this.add_icon.get_effects ().start_ex (Effect.LAUNCHING, 1, false, false);
      return false;
    });
    add_icon.clicked.connect (this.on_add_icon_clicked);
    add_icon.context_menu_popup.connect (this.on_add_icon_clicked);
    add_icon.set_info_simple (canonical_name, uid, "add");
    add_icon.set_tooltip_text ("Drop launcher here");
    add_icon.show ();

    icon_box.add (add_icon);
    icon_box.set_child_packing (add_icon, false, false, 0, Gtk.PackType.END);
    icon_box.show ();

    this.add (icon_box);
    this.size_changed.connect (this.on_size_changed);
  }

  construct
  {
    this.config_dir = Path.build_filename (Environment.get_user_config_dir (),
                                           "awn", "applets", "simple-launcher",
                                           null);
    DirUtils.create_with_parents (config_dir, 0755);

    launchers = new GenericArray<Awn.ThemedIcon> ();

    this.client = Awn.Config.get_default_for_applet (this);

    try
    {
      this.client.bind (DesktopAgnostic.Config.GROUP_DEFAULT, "launcher_list",
                        this, "launcher-list",
                        false, DesktopAgnostic.Config.BindMethod.FALLBACK);
    }
    catch (DesktopAgnostic.Config.Error err)
    {
      critical ("Config Error: %s", err.message);
    }
  }

  private void on_size_changed ()
  {
    foreach (unowned Gtk.Widget w in icon_box.get_children ())
    {
      unowned Awn.ThemedIcon i = w as Awn.ThemedIcon;
      if (i != null) i.set_size (this.size);
    }
  }

  private void on_add_icon_clicked ()
  {
    Gdk.Event? e = Gtk.get_current_event ();
    uint button = e == null || e.type != Gdk.EventType.BUTTON_PRESS ?
      0 : e.button.button;
    add_icon.popup_gtk_menu (add_icon_menu, button,
                             Gtk.get_current_event_time ());
  }

  private void add_folders ()
  {
    string[] directories = {};
    this.freeze_notify ();
    for (UserDirectory dir = UserDirectory.DESKTOP;
         dir <= UserDirectory.VIDEOS; //dir < UserDirectory.N_DIRECTORIES;
         dir = dir + 1)
    {
      var path = Environment.get_user_special_dir (dir);
      if (path in directories) continue;

      directories += path;
      var f = VFS.file_new_for_path (path);
      if (f.exists ())
      {
        process_uri (f);
      }
    }
    this.thaw_notify ();
  }

  private void uri_received (Gdk.DragContext context,
                             int x, int y,
                             Gtk.SelectionData data,
                             uint info,
                             uint time_)
  {
    if (data == null || data.get_length () == 0)
    {
      Gtk.drag_finish (context, false, false, time_);
      return;
    }

    SList<VFS.File> files = VFS.files_from_uri_list ((string) data.data);
    process_uri (files.data);

    Gtk.drag_finish (context, true, false, time_);
  }

  private string? pick_icon_name (string[] icon_names)
  {
    var it = Gtk.IconTheme.get_default ();
    foreach (unowned string icon in icon_names)
    {
      if (it.has_icon (icon)) return icon;
    }

    return null;
  }

  private void process_uri (VFS.File file)
  {
    string uri = file.uri;
    debug ("received %s", uri);

    var df = get_new_desktop_file ();
    var path = df.path;

    if (uri.has_suffix (".desktop"))
    {
      if (file.is_writable ())
      {
        // make a copy if the file is writable
        // FIXME: not exactly correct (we should check file's uid)
        FDO.DesktopEntry de = FDO.desktop_entry_new_for_file (file);
        de.save (df);
      }
      else
      {
        // but don't make copies of system desktop files
        path = file.path;
      }
    }
    else
    {
      FDO.DesktopEntry link_entry = FDO.desktop_entry_new ();
      link_entry.entry_type = FDO.DesktopEntryType.LINK;
      link_entry.name = Uri.unescape_string (Path.get_basename (uri));
      link_entry.set_string ("URL", uri);
      // try to get icon for the uri
      string icon_name = "image-missing";
      try
      {
        var f = VFS.file_new_for_uri (uri);
        if (f.is_native ())
        {
          //string? i = f.get_thumbnail_path () ?? pick_icon_name (f.get_icon_names ());
          string? i = pick_icon_name (f.get_icon_names ());
          if (i != null) icon_name = i;
        }
        else
        {
          icon_name = "unknown";
        }
      }
      catch (GLib.Error err)
      {
        warning ("%s", err.message);
      }
      link_entry.icon = icon_name;
      link_entry.save (df);
    }

    Value v = path;
    _launcher_list.append (v);
    this.notify_property ("launcher-list");
  }

  private VFS.File get_new_desktop_file ()
  {
    VFS.File? f = null;
    int counter = 1;
    do
    {
      string path = Path.build_filename (
        config_dir,
        "launcher-%d.desktop".printf (counter++),
        null);
      f = VFS.file_new_for_path (path);
    } while (f.exists ());

    return f;
  }

  private void create_new_launcher ()
  {
    var f = get_new_desktop_file ();
    var d = new UI.LauncherEditorDialog (f);
    d.show_all ();
    if (d.run () == Gtk.ResponseType.APPLY)
    {
      Value v = f.path;
      _launcher_list.append (v);
      this.notify_property ("launcher-list");
    }
    d.destroy ();
  }

  private void launchers_changed ()
  {
    int i;
    int[] invalid_launchers = {};

    // mark all icons as untouched
    for (i=0; i<launchers.length; i++)
    {
      launchers[i].set_data ("untouched", 1);
    }

    // create new launchers
    int index = 0;
    foreach (Value v in _launcher_list)
    {
      string de_path = v.get_string ();
      bool found = false;
      for (i=0; i<launchers.length; i++)
      {
        FDO.DesktopEntry? de = launchers[i].get_data (DESKTOP_ENTRY);
        if (de != null && de.file.path == de_path)
        {
          launchers[i].set_data ("untouched", 0);
          found = true;

          icon_box.reorder_child (launchers[i], index);
          break;
        }
      }

      if (!found)
      {
        var icon = create_launcher (de_path);
        if (icon != null)
        {
          icon_box.reorder_child (icon, index);
        }
        else
        {
          invalid_launchers += index;
        }
      }

      index++;
    }

    // destroy removed launchers
    List<unowned Awn.ThemedIcon> l = new List<unowned Awn.ThemedIcon> ();
    for (i=0; i<launchers.length; i++)
    {
      if (launchers[i].get_data<int> ("untouched") != 0)
      {
        l.append (launchers[i]);
      }
    }

    foreach (unowned Awn.ThemedIcon ti in l)
    {
      remove_launcher (ti, true);
    }

    if (launchers.length > 0) add_icon.hide ();
    else add_icon.show ();

    // remove invalid launchers from config backend
    bool something_removed = false;
    for (i = invalid_launchers.length-1; i >= 0; i--)
    {
      _launcher_list.remove (invalid_launchers[i]);
      something_removed = true;
    }
    if (something_removed) this.notify_property ("launcher-list");
  }

  private Awn.ThemedIcon? create_launcher (string path)
  {
    var f = VFS.file_new_for_path (path);
    if (!f.exists ())
    {
      warning ("Desktop file \"%s\" doesn't exist!", path);
      return null;
    }
    var de = FDO.desktop_entry_new_for_file (f);

    var icon = new Awn.ThemedIcon ();
    icon.drag_and_drop = false;
    icon.set_data (DESKTOP_ENTRY, de);
    icon.set_size (this.size);

    string icon_name = "image-missing";
    if (de.key_exists ("Icon"))
    {
      icon_name = de.get_string ("Icon");
      // FIXME: what if there's full path?
    }
    icon.set_info_simple (this.canonical_name,
                          this.uid,
                          icon_name);
    icon.set_tooltip_text (de.get_localestring ("Name", null));

    icon.clicked.connect (this.on_launcher_clicked);
    icon.context_menu_popup.connect (this.on_launcher_ctx_menu);
    icon_box.add (icon);
    icon.show ();

    Gtk.drag_dest_set (icon, 0, null, Gdk.DragAction.PRIVATE);
    icon.drag_motion.connect (this.on_launcher_drag_motion);

    launchers.add (icon);

    return icon;
  }

  private void remove_launcher (Awn.ThemedIcon launcher,
                                bool delete_desktop_file)
  {
    FDO.DesktopEntry de = launcher.steal_data (DESKTOP_ENTRY);
    if (delete_desktop_file && de.file.path.has_prefix (config_dir))
    {
      de.file.remove ();
    }
    this.launchers.remove (launcher);
    launcher.destroy (); // removes the launcher from IconBox
  }

  private bool on_launcher_drag_motion ()
  {
    if (timer_id != 0)
    {
      Source.remove (timer_id);
    }
    timer_id = Timeout.add (2500,
                            () => { this.add_icon.hide (); return false; });
    add_icon.show ();
    return false;
  }

  private void on_launcher_clicked (Awn.Icon launcher)
  {
    FDO.DesktopEntry de = launcher.get_data (DESKTOP_ENTRY);
    return_if_fail (de != null);

    try
    {
      de.launch (0, null);
      launcher.get_effects ().start_ex (Awn.Effect.LAUNCHING, 1, false, false);
    }
    catch (GLib.Error err)
    {
      var d = new Gtk.MessageDialog (null,
                                     Gtk.DialogFlags.MODAL,
                                     Gtk.MessageType.ERROR,
                                     Gtk.ButtonsType.CLOSE,
                                     "Unable to start launcher!\n\n" +
                                     "Error details:\n%s",
                                     err.message);
      d.run ();
      d.destroy ();
    }
  }

  private void on_launcher_ctx_menu (Awn.Icon launcher, Gdk.EventButton event)
  {
    remove_menu_item.set_data (DESKTOP_ENTRY, launcher);
    edit_menu_item.set_data (DESKTOP_ENTRY, launcher);

    launcher.popup_gtk_menu (menu, event.button, event.time);
  }

  private void remove_clicked ()
  {
    Awn.ThemedIcon ti = remove_menu_item.get_data (DESKTOP_ENTRY);
    FDO.DesktopEntry de = ti.get_data (DESKTOP_ENTRY);

    uint index = 0;
    foreach (Value v in _launcher_list)
    {
      if (v.get_string () == de.file.path) break;
      index++;
    }

    if (index < _launcher_list.n_values) _launcher_list.remove (index);

    this.notify_property ("launcher-list");
  }

  private void edit_clicked ()
  {
    Awn.ThemedIcon ti = remove_menu_item.get_data (DESKTOP_ENTRY);
    FDO.DesktopEntry de = ti.get_data (DESKTOP_ENTRY);

    var input = de.file;
    VFS.File? output = null;
    if (!input.path.has_prefix (this.config_dir))
    {
      output = get_new_desktop_file ();
    }

    var d = new UI.LauncherEditorDialog (input, output, true);
    d.show_all ();
    if (d.run () == Gtk.ResponseType.APPLY)
    {
      if (output == null)
      {
        // reload the desktop file
        remove_launcher (ti, false);
        launchers_changed ();
      }
      else
      {
        unowned Value? val;
        for (uint i=0; i<_launcher_list.n_values; i++)
        {
          val = _launcher_list.get_nth (i);
          if (val.get_string () == input.path) val.set_string (output.path);
        }
        this.notify_property ("launcher-list");
      }
    }
    d.destroy ();
  }
}

public Applet?
awn_applet_factory_initp (string canonical_name, string uid, int panel_id)
{
  return new SimpleLauncher (canonical_name, uid, panel_id);
}

