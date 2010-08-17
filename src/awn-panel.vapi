/* awn-panel.vapi stub (manually generated) */

[CCode (cprefix = "Awn", lower_case_cprefix = "awn_")]
namespace Awn {
	[CCode (cheader_filename = "awn-panel.h")]
	public class Panel : Gtk.Window, Atk.Implementor, Gtk.Buildable {
		[CCode (type = "GtkWidget*", has_construct_function = false)]
		public Panel.with_panel_id (int panel_id);
    public bool add_applet (string desktop_file) throws GLib.Error;
    public bool delete_applet (string uid) throws GLib.Error;
    public bool set_applet_flags (string uid, int flags) throws GLib.Error;
    public bool set_glow (string sender, bool activate);
    public uint inhibit_autohide (string sender, string app_name, string reason); 
    public bool uninhibit_autohide (uint cookie);

    [CCode (array_length = false, array_null_terminated = true)]
    public string[] get_inhibitors ();

    public bool get_snapshot (out int width, out int height, out int rowstride, out bool has_alpha, out int bits_per_sample, out int num_channels, out char[] pixel_data) throws GLib.Error;

    public int64 docklet_request (int min_size, bool shrink, bool expand) throws GLib.Error;

		[NoAccessorMethod]
    public int panel_id { get; construct; }
		[NoAccessorMethod]
    public int64 panel_xid { get; }
		[NoAccessorMethod]
    public int position { get; set construct; }
		[NoAccessorMethod]
    public int size { get; set construct; }
		[NoAccessorMethod]
    public int offset { get; set construct; }
		[NoAccessorMethod]
    public int max_size { get; }
		[NoAccessorMethod]
    public int path_type { get; set construct; }
		[NoAccessorMethod]
    public float offset_modifier { get; set construct; }

    public virtual signal void size_changed (int size);
    public virtual signal void position_changed (int position);
    public virtual signal void offset_changed (int offset);
    public virtual signal void property_changed (string prop_name, GLib.Value val);

    public virtual signal void destroy_applet (string uid);
    public virtual signal void destroy_notify ();
	}
}
