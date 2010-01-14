namespace Awn
{
  public interface PanelConnector: Object
  {
    public void connect (DBus.Connection con, out DBus.Object proxy) throws DBus.Error
    {
      if (this.panel_id > 0)
      {
        string object_path = 
          "/org/awnproject/Awn/Panel%d".printf (this.panel_id);
        
        dynamic DBus.Object panel = con.get_object ("org.awnproject.Awn",
                                                    object_path,
                                                    "org.awnproject.Awn.Panel");

        panel.PositionChanged += this.on_position_changed;
        panel.OffsetChanged += this.on_offset_changed;
        panel.SizeChanged += this.on_size_changed;
        panel.DestroyApplet += this.on_applet_destroy;

        //proxy.destroy.connect (this.on_proxy_destroyed);
        proxy = panel;
      }
      else
      {
        warning ("Panel-id is not set to correct value!");
      }
    }

    protected void on_position_changed (int new_position)
    {
      if (this.position != new_position)
      {
        this.position = (Gtk.PositionType)new_position;
        this.position_changed ((Gtk.PositionType)new_position);
      }
    }

    protected void on_offset_changed (int new_offset)
    {
      if (this.offset != new_offset)
      {
        this.offset = new_offset;
        this.offset_changed (new_offset);
      }
    }

    protected void on_size_changed (int new_size)
    {
      if (this.size != new_size)
      {
        this.size = new_size;
        this.size_changed (new_size);
      }
    }

    protected void on_applet_destroy (string uid)
    {
      if (this.uid == uid)
      {
        this.applet_deleted ();
      }
    }

    public virtual uint inhibit_autohide (string reason)
    {
      string app_name = "%s:%d".printf (Environment.get_prgname (),
                                        Posix.getpid ());
      dynamic DBus.Object panel = this.panel_proxy;
      return panel.InhibitAutohide (app_name, reason);
    }

    public abstract int panel_id { get;  construct; }
    public abstract DBus.Object panel_proxy { get; construct; }
    public abstract string uid { get; set; construct; }
    public abstract int64 panel_xid { get; }
    public abstract Gtk.PositionType position { get; set; }
    public abstract int offset { get; set; }
    public abstract int size { get; set; }
    public abstract int max_size { get; set; }
    public abstract int path_type { get; set; }
    public abstract float offset_modifier { get; set; }

    public signal void position_changed (Gtk.PositionType pos_type);
    public signal void offset_changed (int offset);
    public signal void size_changed (int size);
    public signal void applet_deleted ();
  }
}
