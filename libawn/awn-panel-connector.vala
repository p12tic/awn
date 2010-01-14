namespace Awn
{
  public interface PanelConnector: Object
  {
    public void connect (DBus.Connection con, out DBus.Object proxy) throws DBus.Error
    {
      if (this.panel_id > 0)
      {
        dynamic DBus.Object panel =
          con.get_object ("org.awnproject.Awn",
                          "/org/awnproject/Awn/Panel%d".printf (this.panel_id),
                          "org.awnproject.Awn.Panel");

        panel.PositionChanged += this.on_position_changed;
        panel.OffsetChanged += this.on_offset_changed;
        panel.SizeChanged += this.on_size_changed;
        panel.PropertyChanged += this.on_property_changed;
        panel.DestroyApplet += this.on_applet_destroy;

        proxy = (owned)panel;
        proxy.destroy.connect (this.on_proxy_destroyed);

        // initialize properties
        dynamic DBus.Object props =
          con.get_object (proxy.get_bus_name (),
                          proxy.get_path (),
                          "org.freedesktop.DBus.Properties");

        HashTable<string, Value?> table = 
          props.GetAll ("org.awnproject.Awn.Panel");
        HashTableIter<string, Value?> iter = 
          HashTableIter<string, Value?>(table);
        unowned string key;
        unowned Value? value;
        while (iter.next (out key, out value))
        {
          this.property_changed (key, value);
        }
      }
      else
      {
        warning ("Panel-id is not set to correct value!");
      }
    }

    private void on_position_changed (dynamic DBus.Object proxy,
                                      int new_position)
    {
      if (this.position != new_position)
      {
        this.position = (Gtk.PositionType)new_position;
      }
    }

    private void on_offset_changed (dynamic DBus.Object proxy,
                                    int new_offset)
    {
      if (this.offset != new_offset)
      {
        this.offset = new_offset;
      }
    }

    private void on_size_changed (dynamic DBus.Object proxy,
                                  int new_size)
    {
      if (this.size != new_size)
      {
        this.size = new_size;
      }
    }

    public abstract void property_changed (string prop_name, Value value);

    private void on_property_changed (dynamic DBus.Object proxy,
                                      string prop_name, Value value)
    {
      this.property_changed (prop_name, value);
    }

    private void on_applet_destroy (dynamic DBus.Object proxy,
                                    string uid)
    {
      if (this.uid == uid)
      {
        this.applet_deleted ();
      }
    }

    public virtual void on_proxy_destroyed ()
    {
      Gtk.main_quit ();
    }

    public uint inhibit_autohide (string reason)
    {
      string app_name = "%s:%d".printf (Environment.get_prgname (),
                                        Posix.getpid ());
      dynamic unowned DBus.Object panel = this.panel_proxy;
      return panel.InhibitAutohide (app_name, reason);
    }

    public void uninhibit_autohide (uint cookie)
    {
      dynamic unowned DBus.Object panel = this.panel_proxy;
      panel.UninhibitAutohide (cookie);
    }

    public int64 bus_docklet_request (int min_size, bool shrink, bool expand)
    {
      int64 window_id;
      dynamic unowned DBus.Object panel = this.panel_proxy;

      try
      {
        window_id = panel.DockletRequest (min_size, shrink, expand);
      }
      catch
      {
        window_id = 0;
      }

      return window_id;
    }

    public abstract int panel_id { get; set construct; }
    public abstract DBus.Object panel_proxy { get; }
    public abstract string uid { get; set construct; }
    public abstract int64 panel_xid { get; }
    public abstract Gtk.PositionType position { get; set; }
    public abstract int offset { get; set; }
    public abstract int size { get; set; }
    public abstract int max_size { get; set; }
    public abstract PathType path_type { get; set; }
    public abstract float offset_modifier { get; set; }

    public signal void applet_deleted ();
  }
}
