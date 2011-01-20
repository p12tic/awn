/*
 * Copyright (C) 2010 Michal Hruby <michal.mhr@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
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

using Gtk;

namespace TaskManager
{
  public class LabelledSeparator : SeparatorMenuItem
  {
    private unowned Label label_widget;
    public LabelledSeparator (string text)
    {
      GLib.Object (label: text);
      label_widget = get_child () as Label;
      label_widget.set_alignment (0.5f, 0.5f);
    }

    protected override Type child_type ()
    {
      return typeof (Widget);
    }

    protected override bool expose_event (Gdk.EventExpose event)
    {
      bool wide_separators;
      int separator_height;
      int horizontal_padding;
      Gtk.Allocation alloc;
      this.get_allocation (out alloc);

      this.style_get ("wide-separators", out wide_separators,
                      "separator-height", out separator_height,
                      "horizontal-padding", out horizontal_padding,
                      null);

      int xthickness = this.get_style ().xthickness;
      int ythickness = this.get_style ().ythickness;
      if (wide_separators)
      {
        Gtk.paint_box (this.get_style (), this.get_window (), StateType.NORMAL,
                       ShadowType.ETCHED_OUT, event.area, this, "hseparator",
                       alloc.x + horizontal_padding + xthickness,
                       alloc.y + (alloc.height - separator_height - ythickness)/2,
                       alloc.width - 2 * (horizontal_padding + xthickness),
                       separator_height);
      }
      else
      {
        Gtk.paint_hline (this.get_style (), this.get_window (), StateType.NORMAL,
                         event.area, this, "menuitem",
                         alloc.x + horizontal_padding + xthickness,
                         alloc.x + alloc.width - horizontal_padding - xthickness - 1,
                         alloc.y + (alloc.height - ythickness) / 2);
      }

      Allocation child_alloc;
      this.get_child ().get_allocation (out child_alloc);
      Pango.Layout layout = (this.get_child () as Label).get_layout ();
      Pango.Rectangle rect;
      layout.get_pixel_extents (null, out rect);
      unowned Widget parent = this.get_parent ();
      int x, y;
      x = (child_alloc.width - rect.width) / 2;
      y = (child_alloc.height - rect.height) / 2;

      Gtk.paint_flat_box (parent.get_style (), this.get_window (),
                          StateType.NORMAL, ShadowType.NONE,
                          null, this, null,
                          child_alloc.x + x, child_alloc.y + y,
                          rect.width, rect.height);

      this.propagate_expose (this.get_child (), event);

      return true;
    }
  }
}

