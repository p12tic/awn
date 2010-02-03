/*
 * Copyright (C) 2009 Mark Lee <avant-wn@lazymalevolence.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

namespace Awn
{
  // Cannot use Awn.Cairo due to a symbol resolution conflict with the real Cairo namespace.
  [CCode (cheader_filename = "libawn/libawn.h", cprefix = "AwnCairo", lower_case_cprefix = "awn_cairo_")]
  namespace CairoUtils
  {
    public static void pattern_add_color_stop_color (Cairo.Pattern pattern, double offset, DesktopAgnostic.Color color);
    public static void pattern_add_color_stop_color_with_alpha_multiplier (Cairo.Pattern pattern, double offset, DesktopAgnostic.Color color, double multiplier);
    public static void rounded_rect (Cairo.Context cr, double x0, double y0, double width, double height, double radius, Awn.CairoRoundCorners state);
    public static void rounded_rect_shadow (Cairo.Context cr, double rx0, double ry0, double width, double height, double radius, Awn.CairoRoundCorners state, double shadow_radius, double shadow_alpha);
    public static void set_source_color (Cairo.Context cr, DesktopAgnostic.Color color);
    public static void set_source_color_with_alpha_multiplier (Cairo.Context cr, DesktopAgnostic.Color color, double multiplier);
    public static void set_source_color_with_multipliers (Cairo.Context cr, DesktopAgnostic.Color color, double color_multiplier, double alpha_multiplier);
  }

  [CCode (cheader_filename = "libawn/libawn.h")]
  namespace Config
  {
    public static void free ();
    public static unowned DesktopAgnostic.Config.Client get_default (int panel_id) throws GLib.Error;
    public static unowned DesktopAgnostic.Config.Client get_default_for_applet (Awn.Applet applet) throws GLib.Error;
    public static unowned DesktopAgnostic.Config.Client get_default_for_applet_by_info (string name, string uid) throws GLib.Error;
  }

  [CCode (cheader_filename = "libawn/awn-utils.h")]
  namespace Utils
  {
    public static void ensure_transparent_bg (Gtk.Widget widget);
    public static float get_offset_modifier_by_path_type (Awn.PathType path_type, Gtk.PositionType position, float offset_modifier, int pos_x, int pos_y, int width, int height);
    public static GLib.ValueArray gslist_to_gvaluearray (GLib.SList list);
    public static void make_transparent_bg (Gtk.Widget widget);
    public static void show_menu_images (Gtk.Menu menu);
  }
}

// vim:et:ai:cindent:ts=2 sts=2 sw=2:
