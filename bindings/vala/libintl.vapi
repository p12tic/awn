[CCode (cheader_filename = "glib.h,glib/gi18n.h", cprefix = "", lower_case_cprefix = "")]
namespace GLib.Gettext {
	public static unowned string _ (string msgid);
	public static unowned string ngettext (string msgid, string msgid_plural, ulong n);
	public static unowned string? bindtextdomain (string domainname, string? dirname);
	public static unowned string? textdomain (string? domainname);
}

// vim: set noet ts=4 sts=4 sw=4 :
