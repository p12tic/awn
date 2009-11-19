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

  public PrefsApplet (string canonical_name, string uid, int panel_id)
  {
    this.canonical_name = canonical_name;
    this.uid = uid;
    this.panel_id = panel_id;
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

    this.set_icon_name ("awn-settings");

    unowned Awn.Icon icon = this.get_icon ();
    icon.clicked.connect (this.on_clicked);
    icon.context_menu_popup.connect (this.on_context_menu_popup);
    icon.drag_begin.connect (this.on_drag_begin);
    icon.drag_end.connect (this.on_drag_end);
    icon.drag_failed.connect (this.on_drag_failed);
    
    Gtk.drag_source_set (icon, Gdk.ModifierType.BUTTON1_MASK,
                         targets, Gdk.DragAction.LINK);
    Gtk.drag_source_set_icon_name (icon, "avant-window-navigator");
    this.set_tooltip_text ("Avant Window Navigator");

    this.initialize_menu ();

    unowned DBusWatcher watcher = DBusWatcher.get_default ();
    watcher.name_appeared["org.awnproject.Applet.Taskmanager"].
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
    about_item.activate.connect (this.on_show_about);
    about_item.show ();
    this.ctx_menu.append (about_item);

    Awn.Utils.show_menu_images (this.ctx_menu);
  }

  private void
  taskmanager_appeared (string name)
  {
    this.update_taskmanager (false);
  }

  private void
  update_taskmanager (bool visible)
  {
    try
    {
      DBus.Connection con = DBus.Bus.get (DBus.BusType.SESSION);

      dynamic DBus.Object taskman;
      taskman = con.get_object ("org.awnproject.Applet.Taskmanager",
                                "/org/awnproject/Applet/Taskmanager",
                                "org.awnproject.Applet.Taskmanager");

      Value window = Value (typeof (string));
      window.set_string ("awn-settings");

      HashTable<string, Value?> hints;
      hints = new HashTable<string, Value?> (str_hash, str_equal);
      Value val = Value (typeof (bool));
      val.set_boolean (visible);
      hints.insert ("visible", val);

      taskman.Update(window, hints);
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
    unowned Awn.Icon icon = this.get_icon () as Awn.Icon;
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

    // divide monitors into 4 parts (shaped like X)
    Gdk.Point center = Gdk.Point();
    center.x = rect.x + rect.width / 2;
    center.y = rect.y + rect.height / 2;

    Gdk.Point[] top_points = new Gdk.Point[3];
    // top left
    top_points[0].x = rect.x;
    top_points[0].y = rect.y;
    // top right
    top_points[1].x = rect.x + rect.width;
    top_points[1].y = rect.y;
    top_points[2] = center;
    Gdk.Region top = Gdk.Region.polygon (top_points,
                                         Gdk.FillRule.EVEN_ODD_RULE);

    Gdk.Point[] bottom_points = new Gdk.Point[3];
    // bottom left
    bottom_points[0].x = rect.x;
    bottom_points[0].y = rect.y + rect.height;
    // bottom right
    bottom_points[1].x = rect.x + rect.width;
    bottom_points[1].y = rect.y + rect.height;
    bottom_points[2] = center;
    Gdk.Region bot = Gdk.Region.polygon (bottom_points,
                                         Gdk.FillRule.EVEN_ODD_RULE);

    Gtk.PositionType pos;
    bool is_top = top.point_in (mouse_x, mouse_y);

    if (!is_top && !bot.point_in (mouse_x, mouse_y))
    {
      // not in top part, nor bottom
      pos = mouse_x < center.x ? 
        Gtk.PositionType.LEFT : Gtk.PositionType.RIGHT;
    }
    else
    {
      pos = is_top ? Gtk.PositionType.TOP : Gtk.PositionType.BOTTOM;
    }

    try
    {
      this.panel_client.set_bool ("panel", "monitor_force", false);
      // set monitor number if necessary
      if (default_mon != monitor_num && n_monitors > 1)
      {
        this.panel_client.set_int ("panel", "monitor_num", monitor_num);
      }
      // finally set panel orientation
      this.panel_client.set_int ("panel", "orient", (int)pos);
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

    this.set_tooltip_text (_ ("Drag the icon to change panel orientation..."));
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
      this.ctx_menu.popup (null, null, null, 0, cur_time);
    }
  }

  private void
  on_show_about ()
  {
    try
    {
      string[] argv = new string[] { "awn-settings", "--about" };
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
    this.ctx_menu.popup (null, null, null, evt.button, evt.time);
  }
}

public Applet
awn_applet_factory_initp (string canonical_name, string uid, int panel_id)
{
  Intl.setlocale (LocaleCategory.ALL, "");
  Gettext.bindtextdomain (Build.GETTEXT_PACKAGE, Build.LOCALEDIR);
  Gettext.textdomain (Build.GETTEXT_PACKAGE);

  Applet applet = new PrefsApplet (canonical_name, uid, panel_id);
  Wnck.Screen.get_default ().force_update ();
  return applet;
}

// vim: set ft=vala et ts=2 sts=2 sw=2 ai :
