#!/usr/bin/python
#
#  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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

import gtk
from desktopagnostic import config
from desktopagnostic import Color
from desktopagnostic.ui import ColorButton


def bind_to_gtk_component (client, group, key, obj, prop_name, widget,
                           read_only=True,
                           getter_transform=None, setter_transform=None):
    """Bind config key to a property and widget it represents.

    @param client: desktopagnostic.config.Client instance.
    @param group: Group of the configuration key.
    @param key: Configuration key name.
    @param obj: Object to which the configuration key will be bound.
    @param prop_name: Property name of the object.
    @param widget: Widget which is used to display the setting.
    @param read_only: Only load the value from configuration, don't change it.
    @param getter_transform: Can be used to transform the config key value
                             to a value accepted by the widget
    @param setter_transform: Can be used to transform the value displayed
                             by widget back to the config key's value type
                             (and/or value range)
    """

    def get_widget_value (widget):
        # radio group needs real special case
        if (isinstance(widget, gtk.RadioButton)):
            # we need to reverse the list, cause gtk is smart and uses prepend
            group_widgets = widget.get_group()
            group_widgets.reverse()
            i = range(len(group_widgets))
            for i in range(len(group_widgets)):
                if group_widgets[i].get_active(): return i
            else:
                return -1

        if (isinstance(widget, (gtk.CheckButton, gtk.ComboBox))):
            return widget.get_active()
        elif (isinstance(widget, (gtk.SpinButton, gtk.Range))):
            return widget.get_value()
        elif (isinstance(widget, gtk.FontButton)):
            return widget.get_font_name()
        elif (isinstance(widget, ColorButton)):
            return widget.props.da_color
        else: raise NotImplementedError()

    def set_widget_value (widget, value):
        # radio group needs real special case
        if (isinstance(widget, gtk.RadioButton)):
            # we need to reverse the list, cause gtk is smart and uses prepend
            group_widgets = widget.get_group()
            group_widgets.reverse()
            for i in range(len(group_widgets)):
                group_widgets[i].set_active(True if value == i else False)
            return

        if (isinstance(widget, (gtk.CheckButton, gtk.ComboBox))):
            widget.set_active(value)
        elif (isinstance(widget, (gtk.SpinButton, gtk.Range))):
            widget.set_value(value)
        elif (isinstance(widget, gtk.FontButton)):
            return widget.set_font_name(value)
        elif (isinstance(widget, ColorButton)):
            widget.props.da_color = value
        else: raise NotImplementedError()

    def get_widget_change_signal_name (widget):
        signal_names = {
            gtk.CheckButton: 'toggled',
            gtk.SpinButton: 'value-changed',
            gtk.ComboBox: 'changed',
            gtk.Range: 'value-changed',
            gtk.FontButton: 'font-set',
            ColorButton: 'color-set'
        }
        for cls in signal_names.keys():
            if isinstance(widget, cls): return signal_names[cls]
        else: raise NotImplementedError()

    def connect_to_widget_changes (widget, callback, data):
        # radio group needs real special case
        if (isinstance(widget, gtk.RadioButton)):

            def radio_button_changed (widget, extra):
                if widget.get_active(): callback(widget, extra)

            for radio in widget.get_group():
                radio.connect("toggled", radio_button_changed, data)
            return

        signal_name = get_widget_change_signal_name (widget)
        widget.connect (signal_name, callback, data)

    def key_changed (obj, pspec, tuple):
        widget, getter = tuple

        old_value = get_widget_value(widget)
        new_value = obj.get_property(pspec.name)

        # FIXME: does it need also the widget param?
        if getter: new_value = getter(new_value)

        if (old_value != new_value):
            set_widget_value(widget, new_value)

    def widget_changed (widget, *args):
        obj, prop_name, setter = args[-1]
        new_value = get_widget_value(widget)

        # FIXME: does it need also the widget param?
        if setter: new_value = setter(new_value)

        # we shouldn't compare color properties, cause they might point
        # to the same instance and would therefore be the same all the time
        if new_value != obj.get_property(prop_name) or isinstance(new_value, Color):
            obj.set_property(prop_name, new_value) # update config key

    # make sure the widget updates when the prop changes
    obj.connect("notify::" + prop_name, key_changed, (widget, getter_transform))

    # bind the prop to the config key
    client.bind (group, key, obj, prop_name, read_only,
                 config.BIND_METHOD_FALLBACK)

    # and last connect to widget's change signal if we're supposed to update it
    if (read_only == False):
        data = (obj, prop_name, setter_transform)

        connect_to_widget_changes (widget, widget_changed, data)
