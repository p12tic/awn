import gtk
from desktopagnostic import config


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

    def key_changed (obj, pspec, tuple):
        widget, getter = tuple

        old_value = None
        if (isinstance(widget, gtk.CheckButton)):
            old_value = widget.get_active()
        elif (isinstance(widget, gtk.SpinButton)):
            old_value = widget.get_value()
        else: raise NotImplementedError()

        new_value = obj.get_property(pspec.name)
        # FIXME: does it need also the widget param?
        if getter: new_value = getter(new_value)

        if (old_value != new_value):
            if (isinstance(widget, gtk.CheckButton)):
                widget.set_active(new_value)
            elif (isinstance(widget, gtk.SpinButton)):
                widget.set_value(new_value)

    def widget_changed (widget, *args):
        new_value = None
        obj, prop_name, setter = args[-1]
        if (isinstance(widget, gtk.CheckButton)):
            new_value = widget.get_active()
        elif (isinstance(widget, gtk.SpinButton)):
            new_value = widget.get_value()
        else: raise NotImplementedError()

        # FIXME: does it need also the widget param?
        if setter: new_value = setter(new_value)

        if new_value != obj.get_property(prop_name):
            obj.set_property(prop_name, new_value) # update config key

    # make sure the widget updates when the prop changes
    obj.connect("notify::" + prop_name, key_changed, (widget, getter_transform))

    # bind the prop to the config key
    client.bind (group, key, obj, prop_name, read_only,
                 config.BIND_METHOD_FALLBACK)

    # and last connect to widget's change signal if we're supposed to update it
    if (read_only == False):
        data = (obj, prop_name, setter_transform)
        if (isinstance(widget, gtk.CheckButton)):
            widget.connect("toggled", widget_changed, data)
        elif (isinstance(widget, gtk.SpinButton)):
            widget.connect("value-changed", widget_changed, data)
        else: raise NotImplementedError()
