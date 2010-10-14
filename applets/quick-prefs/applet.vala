/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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

using Gdk;
using Gtk;
using Awn;
using DesktopAgnostic;
using DesktopAgnostic.Config;
using Wnck;
using DBus;

// only here so that config.h is before gi18n-lib.h
private const string not_used = Build.APPLETSDIR;

[DBus (name="net.launchpad.DockManager")]
interface DockManager: GLib.Object
{
  public async abstract string[] get_capabilities () throws DBus.Error;
  public async abstract void awn_set_visibility (string win_class, bool visible) throws DBus.Error;
}

public class PrefsApplet : AppletSimple
{
  const TargetEntry[] targets = {
    { "awn/awn-panel", Gtk.TargetFlags.SAME_WIDGET, 0 }
  };

  private unowned DesktopAgnostic.Config.Client panel_client = null;
  private Gtk.Menu? ctx_menu = null;
  private List<unowned Wnck.Window> windows;
  private uint timer_id = 0;
  private bool in_drag = false;
  private uint autohide_cookie = 0;

  private Awn.Applet? docklet;
  private List<unowned Gtk.Widget>? docklet_icons;

  public PrefsApplet (string canonical_name, string uid, int panel_id)
  {
    GLib.Object (canonical_name: canonical_name,
                 uid: uid,
                 panel_id: panel_id);
  }

  construct
  {
    unowned Wnck.Screen wnck_screen = Wnck.Screen.get_default ();
    Wnck.set_client_type (Wnck.ClientType.PAGER);

    wnck_screen.window_opened.connect (this.on_window_opened);
    wnck_screen.window_closed.connect (this.on_window_closed);
    wnck_screen.active_window_changed.connect (this.on_active_changed);

    this.windows = new List<unowned Wnck.Window>();
  }

  public override void constructed ()
  {
    base.constructed ();

    this.panel_client = Awn.Config.get_default (this.panel_id);

    this.set_icon_info ({"main-icon", "dir", "prefs", "about"},
                        {"awn-settings", "gtk-directory", "gtk-preferences", "gtk-about"});
    this.set_icon_state ("main-icon");

    unowned Awn.Icon icon = this.get_icon ();
    icon.clicked.connect (this.on_clicked);
    icon.context_menu_popup.connect (this.on_context_menu_popup);
    icon.drag_begin.connect (this.on_drag_begin);
    icon.drag_end.connect (this.on_drag_end);
    icon.drag_failed.connect (this.on_drag_failed);
    
    Gtk.drag_source_set (icon, Gdk.ModifierType.BUTTON1_MASK,
                         targets, Gdk.DragAction.LINK);

    // we don't want any icon when repositioning the panel
    Gdk.Pixbuf pixbuf = new Gdk.Pixbuf(Gdk.Colorspace.RGB, true, 8, 1, 1);
    pixbuf.fill (0);
    Gtk.drag_source_set_icon_pixbuf (icon, pixbuf);

    this.set_tooltip_text ("Avant Window Navigator");

    this.initialize_menu ();

    unowned DBusWatcher watcher = DBusWatcher.get_default ();
    watcher.name_appeared["net.launchpad.DockManager"].
        connect (this.taskmanager_appeared);

    // ask taskman to hide awn-settings
    this.update_taskmanager (false);
    // but tell it to show it again when we're removed
    this.applet_deleted.connect((w) =>
    {
      this.update_taskmanager (true);
    });
  }

  private void
  initialize_menu ()
  {
    this.ctx_menu = this.create_default_menu () as Menu;

    // quit menu item
    Gtk.ImageMenuItem quit_item = 
      new ImageMenuItem.from_stock (Gtk.STOCK_QUIT, null);
    quit_item.activate.connect (this.on_quit_click);
    quit_item.show ();
    this.ctx_menu.append (quit_item);
    
    // about menu item
    Gtk.ImageMenuItem about_item = 
      new ImageMenuItem.with_label (Gettext._ ("About Awn"));
    about_item.set_image (new Gtk.Image.from_stock (Gtk.STOCK_ABOUT,
      Gtk.IconSize.MENU));
    about_item.activate.connect ((w) => { this.run_preferences (true); });
    about_item.show ();
    this.ctx_menu.append (about_item);

    Awn.Utils.show_menu_images (this.ctx_menu);
  }

  private void
  taskmanager_appeared (string name)
  {
    this.update_taskmanager (false);
  }

  private async void
  update_taskmanager (bool visible)
  {
    try
    {
      DBus.Connection con = DBus.Bus.get (DBus.BusType.SESSION);

      var taskman = (DockManager) 
        con.get_object ("net.launchpad.DockManager",
                        "/net/launchpad/DockManager",
                        "net.launchpad.DockManager");

      string[] caps = yield taskman.get_capabilities ();
      bool supports_visibility_setting = false;
      foreach (string cap in caps)
      {
        if (cap == "x-awn-set-visibility") supports_visibility_setting = true;
      }

      if (supports_visibility_setting)
      {
        taskman.awn_set_visibility ("awn-settings", visible);
      }
    }
    catch (DBus.Error err)
    {
      // silently ignore
    }
  }

  private void
  on_active_changed (Wnck.Window? old_active)
  {
    unowned Awn.Icon icon = this.get_icon ();
    foreach (unowned Wnck.Window window in this.windows)
    {
      if (window.is_active ())
      {
        icon.set_is_active (true);
        return;
      }
    }
    icon.set_is_active (false);
  }

  private void
  on_window_opened (Wnck.Window window)
  {
    // we only care about awn-settings window
    if (window.get_class_group ().get_name () == "awn-settings")
    {
      this.windows.append (window);
    }

    this.update_icon ();
  }

  private void
  on_window_closed (Wnck.Window window)
  {
    // we only care about awn-settings window
    if (this.windows.find (window) != null)
    {
      this.windows.remove (window);
    }

    this.update_icon ();
  }

  private void
  update_icon ()
  {
    unowned Awn.Icon icon = this.get_icon ();
    icon.set_indicator_count ((int)this.windows.length ());
  }

  private bool
  on_timer_tick ()
  {
    int mouse_x, mouse_y;
    unowned Gdk.Screen screen;
    Gdk.Rectangle rect;

    this.get_icon ().get_display ().get_pointer (out screen,
                                                 out mouse_x, out mouse_y,
                                                 null);
    int monitor_num = screen.get_monitor_at_point (mouse_x, mouse_y);
    int n_monitors = screen.get_n_monitors ();
    int default_mon = screen.get_monitor_at_point (0, 0);
    screen.get_monitor_geometry (monitor_num, out rect);

    float rel_x = (mouse_x - rect.x) / (float)rect.width;
    float rel_y = (mouse_y - rect.y) / (float)rect.height;

    Gtk.PositionType pos;
    bool is_top = rel_y <= 0.15 && rel_y >= 0;
    bool is_bottom = rel_y >= 0.85 && rel_y <= 1;
    bool is_left = rel_x <= 0.15 && rel_x >= 0;
    bool is_right = rel_x >= 0.85 && rel_x <= 1;
    bool on_edge = is_top || is_bottom || is_left || is_right;

    if (is_bottom) pos = Gtk.PositionType.BOTTOM;
    else if (is_top) pos = Gtk.PositionType.TOP;
    else if (is_left) pos = Gtk.PositionType.LEFT;
    else if (is_right) pos = Gtk.PositionType.RIGHT;
    else pos = Gtk.PositionType.BOTTOM;

    try
    {
      this.panel_client.set_bool ("panel", "monitor_force", false);
      // set monitor number if necessary
      if (n_monitors > 1)
      {
        int config_mon = this.panel_client.get_int ("panel", "monitor_num");
        if (default_mon != monitor_num || monitor_num != config_mon)
        {
          this.panel_client.set_int ("panel", "monitor_num", monitor_num);
        }
      }
      // finally set panel orientation
      if (on_edge)
      {
        this.panel_client.set_int ("panel", "orient", (int)pos);
      }
    }
    catch (GLib.Error e)
    {
      warning ("Unable to set panel properties. Error: %s", e.message);
    }
    return true;
  }

  private void
  on_drag_begin (DragContext context)
  {
    this.in_drag = true;

    this.set_tooltip_text (_ ("Drag to change panel orientation"));
    unowned Awn.Tooltip tooltip = this.get_icon ().get_tooltip ();
    tooltip.smart_behavior = false;
    tooltip.toggle_on_click = false;
    tooltip.show ();

    if (this.timer_id == 0)
    {
      this.timer_id = Timeout.add (300, this.on_timer_tick);
    }
    if (this.autohide_cookie == 0)
    {
      this.autohide_cookie = this.inhibit_autohide ("awn-settings");
    }
  }

  private void
  on_drag_end (DragContext context)
  {
    if (this.timer_id != 0)
    {
      Source.remove (this.timer_id);
      this.timer_id = 0;
    }
    if (this.autohide_cookie != 0)
    {
      this.uninhibit_autohide (this.autohide_cookie);
      this.autohide_cookie = 0;
    }

    unowned Awn.Tooltip tooltip = this.get_icon ().get_tooltip ();
    tooltip.hide ();
    this.set_tooltip_text ("Avant Window Navigator");
    tooltip.smart_behavior = true;
    tooltip.toggle_on_click = true;

    this.in_drag = false;
  }

  private bool
  on_drag_failed (DragContext context, DragResult result)
  {
    // don't show the fail animation
    return true;
  }

  private void
  on_clicked ()
  {
    if (this.in_drag) return;

    uint32 cur_time = Gtk.get_current_event_time ();

    if (this.windows.length () >= 1)
    {
      unowned Wnck.Window window = this.windows.data; // returns first
      if (window.is_active ())
      {
        foreach (var w in this.windows)
        {
          w.minimize ();
        }
      }
      else
      {
        window.activate (cur_time);
      }
    }
    else
    {
      Gdk.NativeWindow window_id = this.docklet_request (450, false, true);
      if ((long)window_id != 0)
      {
        this.setup_docklet (window_id);
      }
      else
      {
        this.ctx_menu.popup (null, null, null, 0, cur_time);
      }
    }
  }

  public static void
  setup_label_for_docklet (Awn.Label label, Awn.Applet docklet)
  {
    Gtk.PositionType pos_type = docklet.get_pos_type ();
    if (pos_type == Gtk.PositionType.TOP || 
        pos_type == Gtk.PositionType.BOTTOM)
    {
      label.set_size_request (-1, docklet.get_size ());
      label.set_alignment (1.0f, 0.5f);
      label.set_angle (0);
    }
    else
    {
      label.set_size_request (docklet.get_size (), -1);
      label.set_alignment (0.5f, 1.0f);
      label.set_angle (pos_type == Gtk.PositionType.LEFT ? 90 : 270);
    }
  }

  private static Awn.Icon
  new_unbound_icon ()
  {
    // vala is buggy when constructing InitiallyUnowned objects, so this
    // code from hell fixes its ref counting
    Awn.Icon icon;

    icon = GLib.Object.@new (typeof (Awn.Icon), "bind-effects", false)
      .ref_sink ().@ref () as Awn.Icon;

    return icon;
  }

  protected void
  setup_docklet (Gdk.NativeWindow window_id)
  {
    Awn.Icon icon;
    Awn.Label label;
    Awn.Alignment align;
    int icon_size = this.size;

    this.docklet_icons = new List<unowned Gtk.Widget> ();
    this.docklet = new Applet ("quick-prefs", "docklet", this.panel_id);
    this.docklet.quit_on_delete = false;
    this.set_behavior (AppletFlags.DOCKLET_HANDLES_POSITION_CHANGE);

    this.docklet.destroy.connect ((w) =>
    {
      this.docklet = null;
      this.docklet_icons = null;
    });
    this.docklet.size_changed.connect ((w, s) =>
    {
      unowned string icon_name;
      unowned Awn.ThemedIcon loader = this.get_icon () as Awn.ThemedIcon;
      foreach (unowned Gtk.Widget widget in this.docklet_icons)
      {
        void *ptr = widget.get_data ("icon-name");
        icon_name = (string) ptr;
        if (icon_name != null)
        {
          unowned Awn.Icon it_icon = widget as Awn.Icon;
          it_icon.set_from_pixbuf (loader.get_icon_at_size (s, icon_name));
        }
        else
        {
          PrefsApplet.setup_label_for_docklet (widget as Awn.Label, w);
        }
      }
    });
    this.docklet.position_changed.connect ((w, p) =>
    {
      unowned Awn.Box box = w.get_child () as Awn.Box;
      box.set_orientation_from_pos_type (p);

      foreach (unowned Gtk.Widget widget in this.docklet_icons)
      {
        if (widget.get_type () == typeof (Awn.Label))
          PrefsApplet.setup_label_for_docklet (widget as Awn.Label, w);
      }
    });

    Awn.Box main_box = new Awn.Box (Gtk.Orientation.HORIZONTAL);
    main_box.set_orientation_from_pos_type (this.docklet.get_pos_type ());
    this.docklet.add (main_box);

    unowned Awn.ThemedIcon icon_loader = this.get_icon () as Awn.ThemedIcon;

    // awn-settings icon, which is used to change orientation of the panel
    IconBox box = new IconBox.for_applet (this.docklet);
    main_box.add (box);

    icon = PrefsApplet.new_unbound_icon ();
    icon.set_data ("icon-name", (void*) "main-icon");
    icon.set_from_pixbuf (icon_loader.get_icon_at_size (icon_size,
                                                        "main-icon"));
    icon.set_tooltip_text (_ ("Drag to change panel orientation"));
    icon.clicked.connect ((w) => { this.docklet.destroy (); });
    icon.drag_begin.connect (this.on_drag_begin);
    icon.drag_end.connect (this.on_drag_end);
    icon.drag_failed.connect (this.on_drag_failed);
    Gtk.drag_source_set (icon, Gdk.ModifierType.BUTTON1_MASK,
                         targets, Gdk.DragAction.LINK);
    Gdk.Pixbuf pixbuf = new Gdk.Pixbuf(Gdk.Colorspace.RGB, true, 8, 1, 1);
    pixbuf.fill (0);
    Gtk.drag_source_set_icon_pixbuf (icon, pixbuf);
    this.docklet_icons.append (icon);
    box.add (icon);

    // separator
    align = new Awn.Alignment.for_applet (this.docklet);
    label = new Awn.Label ();
    label.set_text (_ ("Icon size:"));
    PrefsApplet.setup_label_for_docklet (label, this.docklet);
    this.docklet_icons.append (label);
    align.add (label);
    main_box.pack_start (align, true, true, 6);

    // icons which are used to change panel's size
    box = new Awn.IconBox.for_applet (this.docklet);
    main_box.pack_start (box, false, false, 0);

    icon = PrefsApplet.new_unbound_icon ();
    icon.set_from_pixbuf (icon_loader.get_icon_at_size (32, "dir"));
    icon.clicked.connect ((w) => { this.change_icon_size (32); });
    box.add (icon);

    icon = PrefsApplet.new_unbound_icon ();
    icon.set_from_pixbuf (icon_loader.get_icon_at_size (40, "dir"));
    icon.clicked.connect ((w) => { this.change_icon_size (40); });
    box.add (icon);

    icon = PrefsApplet.new_unbound_icon ();
    icon.set_from_pixbuf (icon_loader.get_icon_at_size (48, "dir"));
    icon.clicked.connect ((w) => { this.change_icon_size (48); });
    box.add (icon);

    // separator
    label = new Awn.Label ();
    main_box.pack_start (label, true, false, 0);

    // icons to run awn-settings
    box = new Awn.IconBox.for_applet (this.docklet);
    main_box.add (box);

    icon = PrefsApplet.new_unbound_icon ();
    icon.set_data ("icon-name", (void*) "prefs");
    icon.set_from_pixbuf (icon_loader.get_icon_at_size (icon_size, "prefs"));
    icon.set_tooltip_text (_ ("Dock Preferences"));
    icon.clicked.connect ((w) =>
    {
      this.run_preferences (false);
      this.docklet.destroy ();
    });
    this.docklet_icons.append (icon);
    box.add (icon);

    // icon to show about dialog
    icon = PrefsApplet.new_unbound_icon ();
    icon.set_data ("icon-name", (void*) "about");
    icon.set_from_pixbuf (icon_loader.get_icon_at_size (icon_size, "about"));
    icon.set_tooltip_text (_ ("About Awn"));
    icon.clicked.connect ((w) =>
    {
      this.run_preferences (true);
      this.docklet.destroy ();
    });
    this.docklet_icons.append (icon);
    box.add (icon);

    // we're done initializing, show the docklet
    (this.docklet as Gtk.Plug).@construct (window_id);
  }

  private void
  change_icon_size (int new_size)
  {
    try
    {
      this.panel_client.set_int ("panel", "size", new_size);
    }
    catch (GLib.Error e)
    {
      warning ("Unable to set panel properties. Error: %s", e.message);
    }
  }

  private void
  run_preferences (bool about_only)
  {
    try
    {
      string[] argv;
      if (about_only)
      {
        argv = new string[] { "awn-settings", "--about" };
      }
      else
      {
        argv = new string[] { "awn-settings",
                              "--panel-id=%d".printf (this.panel_id) };
      }
      spawn_on_screen (this.get_screen (),
                       null,
                       argv,
                       null,
                       SpawnFlags.SEARCH_PATH,
                       null,
                       null);
    }
    catch (GLib.Error err)
    {
      string msg;
      msg = Gettext._ ("Could not open Awn settings: %s")
            .printf (err.message);
      warning ("%s", msg);
    }
  }

  private void
  on_quit_click ()
  {
    try
    {
      DBus.Connection con = DBus.Bus.get (DBus.BusType.SESSION);

      dynamic DBus.Object awn;
      awn = con.get_object ("org.awnproject.Awn",
                            "/org/awnproject/Awn",
                            "org.awnproject.Awn.App");

      awn.RemovePanel (this.panel_id);
    }
    catch (DBus.Error err)
    {
      error ("%s", err.message);
    }
  }

  private void
  on_context_menu_popup (EventButton evt)
  {
    this.get_icon().popup_gtk_menu (this.ctx_menu, evt.button, evt.time);
  }
}

public Applet
awn_applet_factory_initp (string canonical_name, string uid, int panel_id)
{
  Gettext.textdomain (Build.GETTEXT_PACKAGE);

  Applet applet = new PrefsApplet (canonical_name, uid, panel_id);
  Wnck.Screen.get_default ().force_update ();
  return applet;
}

// vim: set ft=vala et ts=2 sts=2 sw=2 ai :
