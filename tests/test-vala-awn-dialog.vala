/*
 * Tests the Vala bindings for Awn.
 *
 * Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
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
 * Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

using Awn;
using Gtk;

public class AwnTest : Window
{
  public AwnTest ()
  {
    Gtk.Label label;

    this.type = WindowType.TOPLEVEL;
    this.title = "Awn Dialog Test (Vala)";
    this.delete_event.connect (this.on_quit);
    label = new Gtk.Label ("One of the dialogs points to this window.");
    this.add (label);
    this.show_all ();
  }

  private bool
  on_quit (Gtk.Widget widget, Gdk.Event event)
  {
    Gtk.main_quit ();
    return true;
  }

  private static void
  on_orient_clicked (Gtk.Button button)
  {
    Awn.Dialog dialog = (Awn.Dialog)button.parent.parent.parent;
    int pos;

    pos = (int)dialog.position;

    pos = (pos + 1) % 4;

    dialog.position = (PositionType)pos;
  }

  private static void
  on_attach_clicked (Gtk.Button button)
  {
    Awn.Dialog dialog = (Awn.Dialog)button.parent.parent.parent;

    dialog.anchored = !dialog.anchored;
  }

  public static int
  main (string[] args)
  {
    Awn.Dialog dialog;
    AwnTest window;
    Awn.Dialog dialog2;
    Button orient;
    Button attach;

    Gtk.init (ref args);

    dialog = new Awn.Dialog ();
    dialog.set_title ("Hello");
    dialog.show_all ();

    window = new AwnTest ();

    dialog2 = new Awn.Dialog.for_widget (window);
    dialog2.set_title ("Another Title");
    dialog2.set_size_request (-1, 200);
    orient = new Button.with_label ("Change orientation");
    orient.clicked.connect (on_orient_clicked);
    dialog2.add (orient);
    attach = new Button.with_label ("Toggle dialog attach");
    attach.clicked.connect (on_attach_clicked);
    dialog2.add (attach);
    dialog2.show_all ();

    Gtk.main ();
    return 0;
  }
}

// vim:et:ai:cindent:ts=2 sts=2 sw=2:
