#
#  Copyright (C) 2007 Mark Lee <avant-wn@lazymalevolence.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
#
#  Author: Mark Lee <avant-wn@lazymalevolence.com>
#
#  Notes: Avant Window Navigator Launcher Editor

import os.path, re, sys
try:
    import pygtk
    pygtk.require("2.0")
except:
    pass
try:
    import gobject
    import pango
    import gtk
    import gtk.glade
    import gtk.gdk as gdk
except:
    sys.exit(1)

import awn
from xdg.DesktopEntry import DesktopEntry
import awnDefs as defs

defs.i18nize(globals())

# enumeration for the iconview liststore
ICON_COL_PIXBUF, ICON_COL_LABEL, ICON_COL_DATA = range(3)

class awnLauncherEditor:
    def __init__(self, filename, launcher = None):
        self.glade = gtk.glade.XML(os.path.join(defs.PKGDATADIR, 'awn-manager', 'launcher-editor.glade'))
        self.glade.signal_autoconnect(self)
        self.main_dialog = self.glade.get_widget('dialog_desktop_item')
        self.launcher = launcher
        self.filename = os.path.abspath(filename)
        self.desktop_entry = DesktopEntry(self.filename)
        self.client = awn.Config()
        if os.path.exists(self.filename):
            self.glade.get_widget('entry_name').set_text(self.desktop_entry.get('Name'))
            self.glade.get_widget('entry_description').set_text(self.desktop_entry.get('Comment'))
            self.glade.get_widget('entry_command').set_text(self.desktop_entry.get('Exec'))
            self.icon_path = self.desktop_entry.get('Icon')
            image = self.glade.get_widget('image_icon')
            if os.path.exists(self.icon_path):
                icon = gdk.pixbuf_new_from_file_at_size (self.icon_path, 32, 32)
                image.set_from_pixbuf(icon)
            elif self.icon_path != '':
                theme = gtk.icon_theme_get_default()
                try:
                    icon = theme.load_icon(self.icon_path, 32, 0)
                    image.set_from_pixbuf(icon)
                except gobject.GError:
                    self.icon_path = None
        else:
            self.icon_path = None
        self.command_chooser = None
        self.stock_icons = None
        self.icon_viewer = self.glade.get_widget('iconview_launcher_icons')
        self.custom_icons = gtk.ListStore(gdk.Pixbuf, str, str)
        self.icon_viewer.set_model(self.custom_icons)
        # based on Gimmie's ItemIconView code.
        if isinstance(self.icon_viewer, gtk.CellLayout):
            cr_pixbuf = gtk.CellRendererPixbuf()
            cr_pixbuf.set_property('xalign', 0.5)
            cr_pixbuf.set_property('yalign', 0.5)
            cr_pixbuf.set_property('width', 64)
            self.icon_viewer.pack_start(cr_pixbuf)
            self.icon_viewer.add_attribute(cr_pixbuf, 'pixbuf', ICON_COL_PIXBUF)
            cr_label = gtk.CellRendererText()
            cr_label.set_property('xalign', 0.5)
            cr_label.set_property('yalign', 0)
            cr_label.set_property('wrap-mode', pango.WRAP_WORD)
            cr_label.set_property('wrap-width', 48)
            self.icon_viewer.pack_start(cr_label, True)
            self.icon_viewer.add_attribute(cr_label, 'text', ICON_COL_LABEL)
        else:
            self.icon_viewer.set_text_column(ICON_COL_LABEL)
            self.icon_viewer.set_pixbuf_column(ICON_COL_PIXBUF)

    def on_button_icon_clicked(self, button):
        splitter = re.compile(r'[\-_]')
        def startswith(string, prefixes):
            for prefix in prefixes:
                if string.startswith(prefix):
                    return True
            return False
        def id_to_label(data):
            if len(data) > 1:
                return ' '.join([word[0].upper() + word[1:] for word in splitter.split(data)]).strip()
            else:
                return data
        fcb = self.glade.get_widget('filechooserbutton_icon_directory')
        self.populate_icon_viewer_custom(fcb.get_filename())
        # populate stock icon dropdown
        if self.stock_icons is None:
            self.stock_icons = gtk.ListStore(gdk.Pixbuf, str, str)
            # code based off of the code in pygtk-demo's stock item and icon browser
#            ids = gtk.stock_list_ids()
            theme = gtk.icon_theme_get_default()
            ids = [i for i in theme.list_icons() if not startswith(i, ['gnome', 'emblem', 'stock', 'mail', 'gtk-', 'document-', 'xfce', 'weather', 'multimedia-player', 'redhat', 'yast', 'messagebox', 'xfsm', 'ximian', 'novell', 'x-']) and '-x-' not in i]
#            ids = [i for i in theme.list_icons() if startswith(i, ['applications-', 'preferences-'])]
            ids.sort()
            for data in ids:
                info = theme.lookup_icon(data, 48, gtk.ICON_LOOKUP_USE_BUILTIN)
                if info is not None:
                    try:
                        pixbuf = info.get_builtin_pixbuf()
                        if pixbuf is None:
                            pixbuf = info.load_icon()
                    except gobject.GError:
                        continue
                    if (pixbuf.get_height(), pixbuf.get_width()) > (48, 48):
                        continue
                    label = info.get_display_name()
                    if label is None or len(label) > 0:
                        if data.endswith('-icon'):
                            label = id_to_label(data[:-5])
                        else:
                            label = id_to_label(data[data.find('-') + 1:])
                    self.stock_icons.append((pixbuf, label, data))
        self.glade.get_widget('dialog_select_icon').show_all()

    def on_button_open_command_clicked(self, button):
        if self.command_chooser is None:
            self.command_chooser = gtk.FileChooserDialog(_('Select an Executable File'), self.main_dialog, buttons=(gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        self.command_chooser.show_all()
        if self.command_chooser.run() == gtk.RESPONSE_OK:
            self.glade.get_widget('entry_command').set_text(self.command_chooser.get_filename())
        self.command_chooser.hide_all()

    def on_dialog_desktop_item_response(self, dialog, response):
        def err_dialog(msg):
            dialog = gtk.MessageDialog(self.main_dialog, gtk.DIALOG_MODAL, gtk.MESSAGE_ERROR, gtk.BUTTONS_OK, msg)
            dialog.set_property('use-markup', True)
            dialog.run()
            dialog.hide_all()
        if response == gtk.RESPONSE_OK:
            # check that all required fields are filled
            fields = {}
            for key in ['name', 'description', 'command']:
                fields[key] = self.glade.get_widget('entry_%s' % key).get_text()
            if fields['name'] is None or len(fields['name']) == 0:
                err_dialog(_('Please provide a name for the launcher in the "%s" field.') % _('<b>Name:</b>'))
                return
            elif fields['command'] is None or len(fields['command']) == 0:
                err_dialog(_('Please provide a program to launch in the "%s" field.') % _('<b>Command:</b>'))
                return
            self.desktop_entry.set('Name', fields['name'])
            self.desktop_entry.set('Exec', fields['command'])
            if fields['description'] is not None and len(fields['description']) > 0:
                self.desktop_entry.set('Comment', fields['description'])
            if self.icon_path is None:
                self.icon_path = "application-x-executable"
            self.desktop_entry.set('Icon', self.icon_path)
            try:
                self.desktop_entry.write()
            except IOError, e:
                err_dialog(_('Couldn\'t save the launcher. Error: %s') % e)
                self.main_dialog.hide_all()
                return
            try:
                uris = self.client.get_list(defs.WINMAN, defs.LAUNCHERS, awn.CONFIG_LIST_STRING)
            except gobject.GError:
                uris = []
            index = len(uris)
            if self.filename in uris:
                index = uris.index(self.filename)
                uris.remove(self.filename)
            if os.path.exists(self.filename):
                uris.insert(index, self.filename)
            self.client.set_list(defs.WINMAN, defs.LAUNCHERS, awn.CONFIG_LIST_STRING, uris)
            if self.launcher is not None:
                self.launcher.refresh_tree(uris)
        self.main_dialog.hide_all()
        if self.standalone:
            gtk.main_quit()

    def on_dialog_desktop_item_close(self, dialog):
        self.on_dialog_desktop_item_response(dialog, gtk.RESPONSE_CANCEL)

    def on_filechooserbutton_icon_directory_current_folder_changed(self, filechooser):
        self.populate_icon_viewer_custom(filechooser.get_filename())

    def on_radiobutton_icon_toggled(self, button_stock):
        fcb = self.glade.get_widget('filechooserbutton_icon_directory')
        if button_stock.get_active():
            self.icon_viewer.set_model(self.stock_icons)
            fcb.set_sensitive(False)
        else:
            self.icon_viewer.set_model(self.custom_icons)
            fcb.set_sensitive(True)

    def populate_icon_viewer_custom(self, folder):
        image_exts = ['.png', '.jpg', '.svg', '.xpm']
        model = self.icon_viewer.get_model()
        model.clear()
        files = os.listdir(folder)
        files.sort()
        for filename in files:
            if os.path.splitext(filename)[1] in image_exts:
                try:
                    full_path = os.path.join(folder, filename)
                    model.append((gdk.pixbuf_new_from_file_at_size(full_path, 48, 48), filename, full_path))
                except gobject.GError:
                    pass

    def on_dialog_select_icon_close(self, dialog):
        self.on_dialog_select_icon_response(dialog, gtk.RESPONSE_CANCEL)

    def on_dialog_select_icon_response(self, dialog, response):
        if response == gtk.RESPONSE_OK:
            selected_icon = self.icon_viewer.get_selected_items()
            if len(selected_icon) == 0:
                return
            model = self.icon_viewer.get_model()
            iterator = model.get_iter(selected_icon[0])
            self.icon_path = model.get_value(iterator, ICON_COL_DATA)
            image = self.glade.get_widget('image_icon')
            image.set_from_pixbuf(model.get_value(iterator, ICON_COL_PIXBUF))
        else:
            self.glade.get_widget('radiobutton_custom').set_active(True)
        dialog.hide_all()

    def run(self, standalone = False):
        self.standalone = standalone
        if self.standalone:
            self.main_dialog.set_property('skip-taskbar-hint', False)
            try:
                theme = gtk.icon_theme_get_default()
                self.main_dialog.set_icon(theme.load_icon('launcher-program', 48, 0))
            except gobject.GError:
                pass
        self.main_dialog.show_all()

if __name__ == '__main__':
    ale = awnLauncherEditor(sys.argv[1])
    ale.run(True)
    gtk.main()
