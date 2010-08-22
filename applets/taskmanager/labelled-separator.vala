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

    private override Type child_type ()
    {
      return typeof (Widget);
    }

    private override bool expose_event (Gdk.EventExpose event)
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

      this.propagate_expose (this.get_child (), event);

      return true;
    }
  }
}

