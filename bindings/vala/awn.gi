<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Awn">
		<function name="cairo_pattern_add_color_stop_color" symbol="awn_cairo_pattern_add_color_stop_color">
			<return-type type="void"/>
			<parameters>
				<parameter name="pattern" type="cairo_pattern_t*"/>
				<parameter name="offset" type="double"/>
				<parameter name="color" type="DesktopAgnosticColor*"/>
			</parameters>
		</function>
		<function name="cairo_pattern_add_color_stop_color_with_alpha_multiplier" symbol="awn_cairo_pattern_add_color_stop_color_with_alpha_multiplier">
			<return-type type="void"/>
			<parameters>
				<parameter name="pattern" type="cairo_pattern_t*"/>
				<parameter name="offset" type="double"/>
				<parameter name="color" type="DesktopAgnosticColor*"/>
				<parameter name="multiplier" type="gdouble"/>
			</parameters>
		</function>
		<function name="cairo_rounded_rect" symbol="awn_cairo_rounded_rect">
			<return-type type="void"/>
			<parameters>
				<parameter name="cr" type="cairo_t*"/>
				<parameter name="x0" type="double"/>
				<parameter name="y0" type="double"/>
				<parameter name="width" type="double"/>
				<parameter name="height" type="double"/>
				<parameter name="radius" type="double"/>
				<parameter name="state" type="AwnCairoRoundCorners"/>
			</parameters>
		</function>
		<function name="cairo_rounded_rect_shadow" symbol="awn_cairo_rounded_rect_shadow">
			<return-type type="void"/>
			<parameters>
				<parameter name="cr" type="cairo_t*"/>
				<parameter name="rx0" type="double"/>
				<parameter name="ry0" type="double"/>
				<parameter name="width" type="double"/>
				<parameter name="height" type="double"/>
				<parameter name="radius" type="double"/>
				<parameter name="state" type="AwnCairoRoundCorners"/>
				<parameter name="shadow_radius" type="double"/>
				<parameter name="shadow_alpha" type="double"/>
			</parameters>
		</function>
		<function name="cairo_set_source_color" symbol="awn_cairo_set_source_color">
			<return-type type="void"/>
			<parameters>
				<parameter name="cr" type="cairo_t*"/>
				<parameter name="color" type="DesktopAgnosticColor*"/>
			</parameters>
		</function>
		<function name="cairo_set_source_color_with_alpha_multiplier" symbol="awn_cairo_set_source_color_with_alpha_multiplier">
			<return-type type="void"/>
			<parameters>
				<parameter name="cr" type="cairo_t*"/>
				<parameter name="color" type="DesktopAgnosticColor*"/>
				<parameter name="multiplier" type="gdouble"/>
			</parameters>
		</function>
		<function name="cairo_set_source_color_with_multipliers" symbol="awn_cairo_set_source_color_with_multipliers">
			<return-type type="void"/>
			<parameters>
				<parameter name="cr" type="cairo_t*"/>
				<parameter name="color" type="DesktopAgnosticColor*"/>
				<parameter name="color_multiplier" type="gdouble"/>
				<parameter name="alpha_multiplier" type="gdouble"/>
			</parameters>
		</function>
		<function name="config_free" symbol="awn_config_free">
			<return-type type="void"/>
		</function>
		<function name="config_get_default" symbol="awn_config_get_default">
			<return-type type="DesktopAgnosticConfigClient*"/>
			<parameters>
				<parameter name="panel_id" type="gint"/>
				<parameter name="error" type="GError**"/>
			</parameters>
		</function>
		<function name="config_get_default_for_applet" symbol="awn_config_get_default_for_applet">
			<return-type type="DesktopAgnosticConfigClient*"/>
			<parameters>
				<parameter name="applet" type="AwnApplet*"/>
				<parameter name="error" type="GError**"/>
			</parameters>
		</function>
		<function name="config_get_default_for_applet_by_info" symbol="awn_config_get_default_for_applet_by_info">
			<return-type type="DesktopAgnosticConfigClient*"/>
			<parameters>
				<parameter name="name" type="gchar*"/>
				<parameter name="uid" type="gchar*"/>
				<parameter name="error" type="GError**"/>
			</parameters>
		</function>
		<function name="utils_ensure_transparent_bg" symbol="awn_utils_ensure_transparent_bg">
			<return-type type="void"/>
			<parameters>
				<parameter name="widget" type="GtkWidget*"/>
			</parameters>
		</function>
		<function name="utils_get_gtk_icon_theme_name" symbol="awn_utils_get_gtk_icon_theme_name">
			<return-type type="gchar*"/>
			<parameters>
				<parameter name="theme" type="GtkIconTheme*"/>
			</parameters>
		</function>
		<function name="utils_get_offset_modifier_by_path_type" symbol="awn_utils_get_offset_modifier_by_path_type">
			<return-type type="gfloat"/>
			<parameters>
				<parameter name="path_type" type="AwnPathType"/>
				<parameter name="position" type="GtkPositionType"/>
				<parameter name="offset" type="gint"/>
				<parameter name="offset_modifier" type="gfloat"/>
				<parameter name="pos_x" type="gint"/>
				<parameter name="pos_y" type="gint"/>
				<parameter name="width" type="gint"/>
				<parameter name="height" type="gint"/>
			</parameters>
		</function>
		<function name="utils_gslist_to_gvaluearray" symbol="awn_utils_gslist_to_gvaluearray">
			<return-type type="GValueArray*"/>
			<parameters>
				<parameter name="list" type="GSList*"/>
			</parameters>
		</function>
		<function name="utils_make_transparent_bg" symbol="awn_utils_make_transparent_bg">
			<return-type type="void"/>
			<parameters>
				<parameter name="widget" type="GtkWidget*"/>
			</parameters>
		</function>
		<function name="utils_menu_set_position_widget_relative" symbol="awn_utils_menu_set_position_widget_relative">
			<return-type type="void"/>
			<parameters>
				<parameter name="menu" type="GtkMenu*"/>
				<parameter name="px" type="gint*"/>
				<parameter name="py" type="gint*"/>
				<parameter name="push_in" type="gboolean*"/>
				<parameter name="data" type="gpointer"/>
			</parameters>
		</function>
		<function name="utils_show_menu_images" symbol="awn_utils_show_menu_images">
			<return-type type="void"/>
			<parameters>
				<parameter name="menu" type="GtkMenu*"/>
			</parameters>
		</function>
		<callback name="AwnAppletInitFunc">
			<return-type type="gboolean"/>
			<parameters>
				<parameter name="applet" type="AwnApplet*"/>
			</parameters>
		</callback>
		<callback name="AwnAppletInitPFunc">
			<return-type type="AwnApplet*"/>
			<parameters>
				<parameter name="canonical_name" type="gchar*"/>
				<parameter name="uid" type="gchar*"/>
				<parameter name="panel_id" type="gint"/>
			</parameters>
		</callback>
		<callback name="AwnEffectsOpfn">
			<return-type type="gboolean"/>
			<parameters>
				<parameter name="fx" type="AwnEffects*"/>
				<parameter name="alloc" type="GtkAllocation*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<struct name="AwnEffectsOp">
			<field name="fn" type="AwnEffectsOpfn"/>
			<field name="data" type="gpointer"/>
		</struct>
		<struct name="AwnOverlayCoord">
			<field name="x" type="gdouble"/>
			<field name="y" type="gdouble"/>
		</struct>
		<enum name="AwnAppletFlags">
			<member name="AWN_APPLET_FLAGS_NONE" value="0"/>
			<member name="AWN_APPLET_EXPAND_MINOR" value="1"/>
			<member name="AWN_APPLET_EXPAND_MAJOR" value="2"/>
			<member name="AWN_APPLET_IS_EXPANDER" value="4"/>
			<member name="AWN_APPLET_IS_SEPARATOR" value="8"/>
			<member name="AWN_APPLET_HAS_SHAPE_MASK" value="16"/>
			<member name="AWN_APPLET_DOCKLET_HANDLES_POSITION_CHANGE" value="1024"/>
			<member name="AWN_APPLET_DOCKLET_CLOSE_ON_MOUSE_OUT" value="2048"/>
		</enum>
		<enum name="AwnAppletLicense">
			<member name="AWN_APPLET_LICENSE_GPLV2" value="10"/>
			<member name="AWN_APPLET_LICENSE_GPLV3" value="11"/>
			<member name="AWN_APPLET_LICENSE_LGPLV2_1" value="12"/>
			<member name="AWN_APPLET_LICENSE_LGPLV3" value="13"/>
		</enum>
		<enum name="AwnCairoRoundCorners">
			<member name="ROUND_NONE" value="0"/>
			<member name="ROUND_TOP_LEFT" value="1"/>
			<member name="ROUND_TOP_RIGHT" value="2"/>
			<member name="ROUND_BOTTOM_RIGHT" value="4"/>
			<member name="ROUND_BOTTOM_LEFT" value="8"/>
			<member name="ROUND_TOP" value="3"/>
			<member name="ROUND_BOTTOM" value="12"/>
			<member name="ROUND_LEFT" value="9"/>
			<member name="ROUND_RIGHT" value="6"/>
			<member name="ROUND_ALL" value="15"/>
		</enum>
		<enum name="AwnEffect">
			<member name="AWN_EFFECT_NONE" value="0"/>
			<member name="AWN_EFFECT_OPENING" value="1"/>
			<member name="AWN_EFFECT_CLOSING" value="2"/>
			<member name="AWN_EFFECT_HOVER" value="3"/>
			<member name="AWN_EFFECT_LAUNCHING" value="4"/>
			<member name="AWN_EFFECT_ATTENTION" value="5"/>
			<member name="AWN_EFFECT_DESATURATE" value="6"/>
		</enum>
		<enum name="AwnOverlayAlign">
			<member name="AWN_OVERLAY_ALIGN_CENTRE" value="0"/>
			<member name="AWN_OVERLAY_ALIGN_LEFT" value="1"/>
			<member name="AWN_OVERLAY_ALIGN_RIGHT" value="2"/>
		</enum>
		<enum name="AwnPathType">
			<member name="AWN_PATH_LINEAR" value="0"/>
			<member name="AWN_PATH_ELLIPSE" value="1"/>
			<member name="AWN_PATH_LAST" value="2"/>
		</enum>
		<object name="AwnAlignment" parent="GtkAlignment" type-name="AwnAlignment" get-type="awn_alignment_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
			</implements>
			<method name="get_offset_modifier" symbol="awn_alignment_get_offset_modifier">
				<return-type type="gint"/>
				<parameters>
					<parameter name="alignment" type="AwnAlignment*"/>
				</parameters>
			</method>
			<constructor name="new_for_applet" symbol="awn_alignment_new_for_applet">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</constructor>
			<method name="set_offset_modifier" symbol="awn_alignment_set_offset_modifier">
				<return-type type="void"/>
				<parameters>
					<parameter name="alignment" type="AwnAlignment*"/>
					<parameter name="modifier" type="gint"/>
				</parameters>
			</method>
			<property name="applet" type="AwnApplet*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="offset-modifier" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="offset-multiplier" type="gfloat" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="scale" type="gfloat" readable="1" writable="1" construct="0" construct-only="0"/>
		</object>
		<object name="AwnApplet" parent="GtkPlug" type-name="AwnApplet" get-type="awn_applet_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
			</implements>
			<method name="create_about_item" symbol="awn_applet_create_about_item">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="copyright" type="gchar*"/>
					<parameter name="license" type="AwnAppletLicense"/>
					<parameter name="version" type="gchar*"/>
					<parameter name="comments" type="gchar*"/>
					<parameter name="website" type="gchar*"/>
					<parameter name="website_label" type="gchar*"/>
					<parameter name="icon_name" type="gchar*"/>
					<parameter name="translator_credits" type="gchar*"/>
					<parameter name="authors" type="gchar**"/>
					<parameter name="artists" type="gchar**"/>
					<parameter name="documenters" type="gchar**"/>
				</parameters>
			</method>
			<method name="create_about_item_simple" symbol="awn_applet_create_about_item_simple">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="copyright" type="gchar*"/>
					<parameter name="license" type="AwnAppletLicense"/>
					<parameter name="version" type="gchar*"/>
				</parameters>
			</method>
			<method name="create_default_menu" symbol="awn_applet_create_default_menu">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="create_pref_item" symbol="awn_applet_create_pref_item">
				<return-type type="GtkWidget*"/>
			</method>
			<method name="docklet_request" symbol="awn_applet_docklet_request">
				<return-type type="GdkNativeWindow"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="min_size" type="gint"/>
					<parameter name="shrink" type="gboolean"/>
					<parameter name="expand" type="gboolean"/>
				</parameters>
			</method>
			<method name="get_behavior" symbol="awn_applet_get_behavior">
				<return-type type="AwnAppletFlags"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="get_canonical_name" symbol="awn_applet_get_canonical_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="get_offset" symbol="awn_applet_get_offset">
				<return-type type="gint"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="get_offset_at" symbol="awn_applet_get_offset_at">
				<return-type type="gint"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="x" type="gint"/>
					<parameter name="y" type="gint"/>
				</parameters>
			</method>
			<method name="get_path_type" symbol="awn_applet_get_path_type">
				<return-type type="AwnPathType"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="get_pos_type" symbol="awn_applet_get_pos_type">
				<return-type type="GtkPositionType"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="get_size" symbol="awn_applet_get_size">
				<return-type type="gint"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="get_uid" symbol="awn_applet_get_uid">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="inhibit_autohide" symbol="awn_applet_inhibit_autohide">
				<return-type type="guint"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="reason" type="gchar*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_applet_new">
				<return-type type="AwnApplet*"/>
				<parameters>
					<parameter name="canonical_name" type="gchar*"/>
					<parameter name="uid" type="gchar*"/>
					<parameter name="panel_id" type="gint"/>
				</parameters>
			</constructor>
			<method name="popup_gtk_menu" symbol="awn_applet_popup_gtk_menu">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="menu" type="GtkWidget*"/>
					<parameter name="button" type="guint"/>
					<parameter name="activate_time" type="guint32"/>
				</parameters>
			</method>
			<method name="set_behavior" symbol="awn_applet_set_behavior">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="flags" type="AwnAppletFlags"/>
				</parameters>
			</method>
			<method name="set_offset" symbol="awn_applet_set_offset">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="offset" type="gint"/>
				</parameters>
			</method>
			<method name="set_path_type" symbol="awn_applet_set_path_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="path" type="AwnPathType"/>
				</parameters>
			</method>
			<method name="set_pos_type" symbol="awn_applet_set_pos_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="position" type="GtkPositionType"/>
				</parameters>
			</method>
			<method name="set_size" symbol="awn_applet_set_size">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="size" type="gint"/>
				</parameters>
			</method>
			<method name="set_uid" symbol="awn_applet_set_uid">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="uid" type="gchar*"/>
				</parameters>
			</method>
			<method name="uninhibit_autohide" symbol="awn_applet_uninhibit_autohide">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="cookie" type="guint"/>
				</parameters>
			</method>
			<property name="canonical-name" type="char*" readable="1" writable="1" construct="0" construct-only="1"/>
			<property name="display-name" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="max-size" type="gint" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="offset" type="gint" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="offset-modifier" type="gfloat" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="panel-id" type="gint" readable="1" writable="1" construct="0" construct-only="1"/>
			<property name="panel-xid" type="gint64" readable="1" writable="0" construct="0" construct-only="0"/>
			<property name="path-type" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="position" type="GtkPositionType" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="quit-on-delete" type="gboolean" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="show-all-on-embed" type="gboolean" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="size" type="gint" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="uid" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<signal name="applet-deleted" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="object" type="AwnApplet*"/>
				</parameters>
			</signal>
			<signal name="flags-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="flags" type="gint"/>
				</parameters>
			</signal>
			<signal name="menu-creation" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="menu" type="GtkMenu*"/>
				</parameters>
			</signal>
			<signal name="offset-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="offset" type="gint"/>
				</parameters>
			</signal>
			<signal name="origin-changed" when="LAST">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="rect" type="GdkRectangle*"/>
				</parameters>
			</signal>
			<signal name="panel-configure-event" when="LAST">
				<return-type type="void"/>
				<parameters>
					<parameter name="object" type="AwnApplet*"/>
					<parameter name="p0" type="GdkEvent*"/>
				</parameters>
			</signal>
			<signal name="position-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="position" type="GtkPositionType"/>
				</parameters>
			</signal>
			<signal name="size-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="size" type="gint"/>
				</parameters>
			</signal>
			<vfunc name="deleted">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</vfunc>
			<vfunc name="panel_configure">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="event" type="GdkEventConfigure*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="AwnAppletSimple" parent="AwnApplet" type-name="AwnAppletSimple" get-type="awn_applet_simple_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="AwnOverlayable"/>
			</implements>
			<method name="get_icon" symbol="awn_applet_simple_get_icon">
				<return-type type="AwnIcon*"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<method name="get_message" symbol="awn_applet_simple_get_message">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<method name="get_progress" symbol="awn_applet_simple_get_progress">
				<return-type type="gfloat"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<method name="get_tooltip_text" symbol="awn_applet_simple_get_tooltip_text">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_applet_simple_new">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="canonical_name" type="gchar*"/>
					<parameter name="uid" type="gchar*"/>
					<parameter name="panel_id" type="gint"/>
				</parameters>
			</constructor>
			<method name="set_effect" symbol="awn_applet_simple_set_effect">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</method>
			<method name="set_icon_context" symbol="awn_applet_simple_set_icon_context">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="cr" type="cairo_t*"/>
				</parameters>
			</method>
			<method name="set_icon_info" symbol="awn_applet_simple_set_icon_info">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="states" type="GStrv"/>
					<parameter name="icon_names" type="GStrv"/>
				</parameters>
			</method>
			<method name="set_icon_name" symbol="awn_applet_simple_set_icon_name">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_icon_pixbuf" symbol="awn_applet_simple_set_icon_pixbuf">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="pixbuf" type="GdkPixbuf*"/>
				</parameters>
			</method>
			<method name="set_icon_state" symbol="awn_applet_simple_set_icon_state">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="state" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_icon_surface" symbol="awn_applet_simple_set_icon_surface">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="surface" type="cairo_surface_t*"/>
				</parameters>
			</method>
			<method name="set_message" symbol="awn_applet_simple_set_message">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="message" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_progress" symbol="awn_applet_simple_set_progress">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="progress" type="gfloat"/>
				</parameters>
			</method>
			<method name="set_tooltip_text" symbol="awn_applet_simple_set_tooltip_text">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnAppletSimple*"/>
					<parameter name="text" type="gchar*"/>
				</parameters>
			</method>
			<signal name="clicked" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
				</parameters>
			</signal>
			<signal name="context-menu-popup" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="event" type="GdkEvent*"/>
				</parameters>
			</signal>
			<signal name="middle-clicked" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
				</parameters>
			</signal>
		</object>
		<object name="AwnBox" parent="GtkBox" type-name="AwnBox" get-type="awn_box_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="GtkOrientable"/>
			</implements>
			<constructor name="new" symbol="awn_box_new">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="orient" type="GtkOrientation"/>
				</parameters>
			</constructor>
			<method name="set_orientation" symbol="awn_box_set_orientation">
				<return-type type="void"/>
				<parameters>
					<parameter name="box" type="AwnBox*"/>
					<parameter name="orient" type="GtkOrientation"/>
				</parameters>
			</method>
			<method name="set_orientation_from_pos_type" symbol="awn_box_set_orientation_from_pos_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="box" type="AwnBox*"/>
					<parameter name="pos_type" type="GtkPositionType"/>
				</parameters>
			</method>
		</object>
		<object name="AwnDBusWatcher" parent="GObject" type-name="AwnDBusWatcher" get-type="awn_dbus_watcher_get_type">
			<method name="get_default" symbol="awn_dbus_watcher_get_default">
				<return-type type="AwnDBusWatcher*"/>
			</method>
			<method name="has_name" symbol="awn_dbus_watcher_has_name">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="self" type="AwnDBusWatcher*"/>
					<parameter name="name" type="gchar*"/>
				</parameters>
			</method>
			<signal name="name-appeared" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="watcher" type="AwnDBusWatcher*"/>
					<parameter name="name" type="char*"/>
				</parameters>
			</signal>
			<signal name="name-disappeared" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="watcher" type="AwnDBusWatcher*"/>
					<parameter name="name" type="char*"/>
				</parameters>
			</signal>
		</object>
		<object name="AwnDesktopLookupClient" parent="GObject" type-name="AwnDesktopLookupClient" get-type="awn_desktop_lookup_client_get_type">
			<constructor name="new" symbol="awn_desktop_lookup_client_new">
				<return-type type="AwnDesktopLookupClient*"/>
			</constructor>
		</object>
		<object name="AwnDialog" parent="GtkWindow" type-name="AwnDialog" get-type="awn_dialog_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
			</implements>
			<method name="get_content_area" symbol="awn_dialog_get_content_area">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="dialog" type="AwnDialog*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_dialog_new">
				<return-type type="GtkWidget*"/>
			</constructor>
			<constructor name="new_for_widget" symbol="awn_dialog_new_for_widget">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="widget" type="GtkWidget*"/>
				</parameters>
			</constructor>
			<constructor name="new_for_widget_with_applet" symbol="awn_dialog_new_for_widget_with_applet">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="widget" type="GtkWidget*"/>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</constructor>
			<method name="set_padding" symbol="awn_dialog_set_padding">
				<return-type type="void"/>
				<parameters>
					<parameter name="dialog" type="AwnDialog*"/>
					<parameter name="padding" type="gint"/>
				</parameters>
			</method>
			<property name="anchor" type="GtkWidget*" readable="0" writable="1" construct="0" construct-only="0"/>
			<property name="anchor-applet" type="AwnApplet*" readable="0" writable="1" construct="0" construct-only="0"/>
			<property name="anchored" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="border" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="dialog-bg" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="effects-hilight" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="hide-on-esc" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="hide-on-unfocus" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="hilight" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="position" type="GtkPositionType" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="title-bg" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="window-offset" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="window-padding" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnEffects" parent="GObject" type-name="AwnEffects" get-type="awn_effects_get_type">
			<method name="add_overlay" symbol="awn_effects_add_overlay">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="overlay" type="AwnOverlay*"/>
				</parameters>
			</method>
			<method name="cairo_create" symbol="awn_effects_cairo_create">
				<return-type type="cairo_t*"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<method name="cairo_create_clipped" symbol="awn_effects_cairo_create_clipped">
				<return-type type="cairo_t*"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="event" type="GdkEventExpose*"/>
				</parameters>
			</method>
			<method name="cairo_destroy" symbol="awn_effects_cairo_destroy">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<method name="emit_anim_end" symbol="awn_effects_emit_anim_end">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</method>
			<method name="emit_anim_start" symbol="awn_effects_emit_anim_start">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</method>
			<method name="get_overlays" symbol="awn_effects_get_overlays">
				<return-type type="GList*"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<method name="main_effect_loop" symbol="awn_effects_main_effect_loop">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<constructor name="new_for_widget" symbol="awn_effects_new_for_widget">
				<return-type type="AwnEffects*"/>
				<parameters>
					<parameter name="widget" type="GtkWidget*"/>
				</parameters>
			</constructor>
			<method name="redraw" symbol="awn_effects_redraw">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<method name="remove_overlay" symbol="awn_effects_remove_overlay">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="overlay" type="AwnOverlay*"/>
				</parameters>
			</method>
			<method name="set_icon_size" symbol="awn_effects_set_icon_size">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="width" type="gint"/>
					<parameter name="height" type="gint"/>
					<parameter name="requestSize" type="gboolean"/>
				</parameters>
			</method>
			<method name="start" symbol="awn_effects_start">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</method>
			<method name="start_ex" symbol="awn_effects_start_ex">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
					<parameter name="max_loops" type="gint"/>
					<parameter name="signal_start" type="gboolean"/>
					<parameter name="signal_end" type="gboolean"/>
				</parameters>
			</method>
			<method name="stop" symbol="awn_effects_stop">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</method>
			<property name="active" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="active-rect-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="active-rect-outline" type="DesktopAgnosticColor*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="arrow-png" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="arrows-count" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="border-clip" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="custom-active-png" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="depressed" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="dot-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="effects" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="icon-alpha" type="gfloat" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="icon-offset" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="indirect-paint" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="make-shadow" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="no-clear" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="position" type="GtkPositionType" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="progress" type="gfloat" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="reflection-alpha" type="gfloat" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="reflection-offset" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="reflection-visible" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="spotlight-png" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="widget" type="GtkWidget*" readable="1" writable="1" construct="0" construct-only="0"/>
			<signal name="animation-end" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</signal>
			<signal name="animation-start" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</signal>
			<field name="widget" type="GtkWidget*"/>
			<field name="no_clear" type="gboolean"/>
			<field name="indirect_paint" type="gboolean"/>
			<field name="position" type="gint"/>
			<field name="set_effects" type="guint"/>
			<field name="icon_offset" type="gint"/>
			<field name="refl_offset" type="gint"/>
			<field name="icon_alpha" type="gfloat"/>
			<field name="refl_alpha" type="gfloat"/>
			<field name="do_reflection" type="gboolean"/>
			<field name="make_shadow" type="gboolean"/>
			<field name="is_active" type="gboolean"/>
			<field name="depressed" type="gboolean"/>
			<field name="arrows_count" type="gint"/>
			<field name="label" type="gchar*"/>
			<field name="progress" type="gfloat"/>
			<field name="border_clip" type="gint"/>
			<field name="spotlight_icon" type="GQuark"/>
			<field name="arrow_icon" type="GQuark"/>
			<field name="custom_active_icon" type="GQuark"/>
			<field name="window_ctx" type="cairo_t*"/>
			<field name="virtual_ctx" type="cairo_t*"/>
		</object>
		<object name="AwnIcon" parent="GtkDrawingArea" type-name="AwnIcon" get-type="awn_icon_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="AwnOverlayable"/>
			</implements>
			<method name="clicked" symbol="awn_icon_clicked">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_hover_effects" symbol="awn_icon_get_hover_effects">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_indicator_count" symbol="awn_icon_get_indicator_count">
				<return-type type="gint"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_input_mask" symbol="awn_icon_get_input_mask">
				<return-type type="GdkRegion*"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_is_active" symbol="awn_icon_get_is_active">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_offset" symbol="awn_icon_get_offset">
				<return-type type="gint"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_pos_type" symbol="awn_icon_get_pos_type">
				<return-type type="GtkPositionType"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_tooltip" symbol="awn_icon_get_tooltip">
				<return-type type="AwnTooltip*"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="get_tooltip_text" symbol="awn_icon_get_tooltip_text">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<method name="middle_clicked" symbol="awn_icon_middle_clicked">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_icon_new">
				<return-type type="GtkWidget*"/>
			</constructor>
			<method name="popup_gtk_menu" symbol="awn_icon_popup_gtk_menu">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="menu" type="GtkWidget*"/>
					<parameter name="button" type="guint"/>
					<parameter name="activate_time" type="guint32"/>
				</parameters>
			</method>
			<method name="set_custom_paint" symbol="awn_icon_set_custom_paint">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="width" type="gint"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</method>
			<method name="set_effect" symbol="awn_icon_set_effect">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</method>
			<method name="set_from_context" symbol="awn_icon_set_from_context">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="ctx" type="cairo_t*"/>
				</parameters>
			</method>
			<method name="set_from_pixbuf" symbol="awn_icon_set_from_pixbuf">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="pixbuf" type="GdkPixbuf*"/>
				</parameters>
			</method>
			<method name="set_from_surface" symbol="awn_icon_set_from_surface">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="surface" type="cairo_surface_t*"/>
				</parameters>
			</method>
			<method name="set_hover_effects" symbol="awn_icon_set_hover_effects">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="enable" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_indicator_count" symbol="awn_icon_set_indicator_count">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="count" type="gint"/>
				</parameters>
			</method>
			<method name="set_is_active" symbol="awn_icon_set_is_active">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="is_active" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_offset" symbol="awn_icon_set_offset">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="offset" type="gint"/>
				</parameters>
			</method>
			<method name="set_pos_type" symbol="awn_icon_set_pos_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="position" type="GtkPositionType"/>
				</parameters>
			</method>
			<method name="set_tooltip_text" symbol="awn_icon_set_tooltip_text">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="text" type="gchar*"/>
				</parameters>
			</method>
			<property name="bind-effects" type="gboolean" readable="1" writable="1" construct="0" construct-only="1"/>
			<property name="icon-height" type="gint" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="icon-width" type="gint" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="long-press-timeout" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<signal name="clicked" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</signal>
			<signal name="context-menu-popup" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
					<parameter name="event" type="GdkEvent*"/>
				</parameters>
			</signal>
			<signal name="long-press" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</signal>
			<signal name="middle-clicked" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</signal>
			<signal name="size-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</signal>
			<vfunc name="icon_padding0">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</vfunc>
			<vfunc name="icon_padding1">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</vfunc>
			<vfunc name="icon_padding2">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</vfunc>
			<vfunc name="icon_padding3">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnIcon*"/>
				</parameters>
			</vfunc>
		</object>
		<object name="AwnIconBox" parent="AwnBox" type-name="AwnIconBox" get-type="awn_icon_box_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="GtkOrientable"/>
			</implements>
			<constructor name="new" symbol="awn_icon_box_new">
				<return-type type="GtkWidget*"/>
			</constructor>
			<constructor name="new_for_applet" symbol="awn_icon_box_new_for_applet">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</constructor>
			<method name="set_offset" symbol="awn_icon_box_set_offset">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon_box" type="AwnIconBox*"/>
					<parameter name="offset" type="gint"/>
				</parameters>
			</method>
			<method name="set_pos_type" symbol="awn_icon_box_set_pos_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon_box" type="AwnIconBox*"/>
					<parameter name="position" type="GtkPositionType"/>
				</parameters>
			</method>
			<property name="applet" type="AwnApplet*" readable="0" writable="1" construct="0" construct-only="1"/>
		</object>
		<object name="AwnImage" parent="GtkImage" type-name="AwnImage" get-type="awn_image_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="AwnOverlayable"/>
			</implements>
			<constructor name="new" symbol="awn_image_new">
				<return-type type="AwnImage*"/>
			</constructor>
		</object>
		<object name="AwnLabel" parent="GtkLabel" type-name="AwnLabel" get-type="awn_label_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
			</implements>
			<constructor name="new" symbol="awn_label_new">
				<return-type type="AwnLabel*"/>
			</constructor>
			<property name="font-mode" type="gint" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="text-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="text-outline-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="text-outline-width" type="gdouble" readable="1" writable="1" construct="0" construct-only="0"/>
		</object>
		<object name="AwnOverlay" parent="GInitiallyUnowned" type-name="AwnOverlay" get-type="awn_overlay_get_type">
			<method name="get_apply_effects" symbol="awn_overlay_get_apply_effects">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlay*"/>
				</parameters>
			</method>
			<method name="get_use_source_op" symbol="awn_overlay_get_use_source_op">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlay*"/>
				</parameters>
			</method>
			<method name="move_to" symbol="awn_overlay_move_to">
				<return-type type="void"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlay*"/>
					<parameter name="cr" type="cairo_t*"/>
					<parameter name="icon_width" type="gint"/>
					<parameter name="icon_height" type="gint"/>
					<parameter name="overlay_width" type="gint"/>
					<parameter name="overlay_height" type="gint"/>
					<parameter name="coord_req" type="AwnOverlayCoord*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_overlay_new">
				<return-type type="AwnOverlay*"/>
			</constructor>
			<method name="render" symbol="awn_overlay_render">
				<return-type type="void"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlay*"/>
					<parameter name="widget" type="GtkWidget*"/>
					<parameter name="cr" type="cairo_t*"/>
					<parameter name="width" type="gint"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</method>
			<method name="set_apply_effects" symbol="awn_overlay_set_apply_effects">
				<return-type type="void"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlay*"/>
					<parameter name="value" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_use_source_op" symbol="awn_overlay_set_use_source_op">
				<return-type type="void"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlay*"/>
					<parameter name="value" type="gboolean"/>
				</parameters>
			</method>
			<property name="active" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="align" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="apply-effects" type="gboolean" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="gravity" type="GdkGravity" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="use-source-op" type="gboolean" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="x-adj" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="x-override" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="y-adj" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="y-override" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<vfunc name="render">
				<return-type type="void"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlay*"/>
					<parameter name="widget" type="GtkWidget*"/>
					<parameter name="cr" type="cairo_t*"/>
					<parameter name="width" type="gint"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</vfunc>
		</object>
		<object name="AwnOverlayPixbuf" parent="AwnOverlay" type-name="AwnOverlayPixbuf" get-type="awn_overlay_pixbuf_get_type">
			<constructor name="new" symbol="awn_overlay_pixbuf_new">
				<return-type type="AwnOverlayPixbuf*"/>
			</constructor>
			<constructor name="new_with_pixbuf" symbol="awn_overlay_pixbuf_new_with_pixbuf">
				<return-type type="AwnOverlayPixbuf*"/>
				<parameters>
					<parameter name="pixbuf" type="GdkPixbuf*"/>
				</parameters>
			</constructor>
			<property name="alpha" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="pixbuf" type="GdkPixbuf*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="scale" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnOverlayPixbufFile" parent="AwnOverlayPixbuf" type-name="AwnOverlayPixbufFile" get-type="awn_overlay_pixbuf_file_get_type">
			<constructor name="new" symbol="awn_overlay_pixbuf_file_new">
				<return-type type="AwnOverlayPixbufFile*"/>
				<parameters>
					<parameter name="file_name" type="gchar*"/>
				</parameters>
			</constructor>
			<property name="file-name" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnOverlayProgress" parent="AwnOverlay" type-name="AwnOverlayProgress" get-type="awn_overlay_progress_get_type">
			<constructor name="new" symbol="awn_overlay_progress_new">
				<return-type type="AwnOverlayProgress*"/>
			</constructor>
			<property name="percent-complete" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnOverlayProgressCircle" parent="AwnOverlayProgress" type-name="AwnOverlayProgressCircle" get-type="awn_overlay_progress_circle_get_type">
			<constructor name="new" symbol="awn_overlay_progress_circle_new">
				<return-type type="AwnOverlayProgressCircle*"/>
			</constructor>
			<property name="background-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="foreground-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="outline-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="scale" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnOverlayText" parent="AwnOverlay" type-name="AwnOverlayText" get-type="awn_overlay_text_get_type">
			<method name="get_size" symbol="awn_overlay_text_get_size">
				<return-type type="void"/>
				<parameters>
					<parameter name="overlay" type="AwnOverlayText*"/>
					<parameter name="widget" type="GtkWidget*"/>
					<parameter name="text" type="gchar*"/>
					<parameter name="size" type="gint"/>
					<parameter name="width" type="gint*"/>
					<parameter name="height" type="gint*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_overlay_text_new">
				<return-type type="AwnOverlayText*"/>
			</constructor>
			<property name="font-mode" type="gint" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="font-sizing" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="text" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="text-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="text-color-astr" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="text-outline-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="text-outline-color-astr" type="char*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="text-outline-width" type="gdouble" readable="1" writable="1" construct="0" construct-only="0"/>
		</object>
		<object name="AwnOverlayThemedIcon" parent="AwnOverlay" type-name="AwnOverlayThemedIcon" get-type="awn_overlay_themed_icon_get_type">
			<constructor name="new" symbol="awn_overlay_themed_icon_new">
				<return-type type="AwnOverlayThemedIcon*"/>
				<parameters>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</constructor>
			<property name="alpha" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="icon-name" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="scale" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnOverlayThrobber" parent="AwnOverlay" type-name="AwnOverlayThrobber" get-type="awn_overlay_throbber_get_type">
			<constructor name="new" symbol="awn_overlay_throbber_new">
				<return-type type="GtkWidget*"/>
			</constructor>
			<property name="scale" type="gdouble" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="timeout" type="guint" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnPixbufCache" parent="GObject" type-name="AwnPixbufCache" get-type="awn_pixbuf_cache_get_type">
			<method name="get_default" symbol="awn_pixbuf_cache_get_default">
				<return-type type="AwnPixbufCache*"/>
			</method>
			<method name="insert_null_result" symbol="awn_pixbuf_cache_insert_null_result">
				<return-type type="void"/>
				<parameters>
					<parameter name="pixbuf_cache" type="AwnPixbufCache*"/>
					<parameter name="scope" type="gchar*"/>
					<parameter name="theme_name" type="gchar*"/>
					<parameter name="icon_name" type="gchar*"/>
					<parameter name="width" type="gint"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</method>
			<method name="insert_pixbuf" symbol="awn_pixbuf_cache_insert_pixbuf">
				<return-type type="void"/>
				<parameters>
					<parameter name="pixbuf_cache" type="AwnPixbufCache*"/>
					<parameter name="pbuf" type="GdkPixbuf*"/>
					<parameter name="scope" type="gchar*"/>
					<parameter name="theme_name" type="gchar*"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="insert_pixbuf_simple_key" symbol="awn_pixbuf_cache_insert_pixbuf_simple_key">
				<return-type type="void"/>
				<parameters>
					<parameter name="pixbuf_cache" type="AwnPixbufCache*"/>
					<parameter name="pbuf" type="GdkPixbuf*"/>
					<parameter name="simple_key" type="gchar*"/>
				</parameters>
			</method>
			<method name="invalidate" symbol="awn_pixbuf_cache_invalidate">
				<return-type type="void"/>
				<parameters>
					<parameter name="pixbuf_cache" type="AwnPixbufCache*"/>
				</parameters>
			</method>
			<method name="lookup" symbol="awn_pixbuf_cache_lookup">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="pixbuf_cache" type="AwnPixbufCache*"/>
					<parameter name="scope" type="gchar*"/>
					<parameter name="theme_name" type="gchar*"/>
					<parameter name="icon_name" type="gchar*"/>
					<parameter name="width" type="gint"/>
					<parameter name="height" type="gint"/>
					<parameter name="null_result" type="gboolean*"/>
				</parameters>
			</method>
			<method name="lookup_simple_key" symbol="awn_pixbuf_cache_lookup_simple_key">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="pixbuf_cache" type="AwnPixbufCache*"/>
					<parameter name="simple_key" type="gchar*"/>
					<parameter name="width" type="gint"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_pixbuf_cache_new">
				<return-type type="AwnPixbufCache*"/>
			</constructor>
			<property name="max-cache-size" type="guint" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnThemedIcon" parent="AwnIcon" type-name="AwnThemedIcon" get-type="awn_themed_icon_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
				<interface name="AwnOverlayable"/>
			</implements>
			<method name="clear_icons" symbol="awn_themed_icon_clear_icons">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="scope" type="gint"/>
				</parameters>
			</method>
			<method name="clear_info" symbol="awn_themed_icon_clear_info">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
				</parameters>
			</method>
			<method name="create_custom_icon_item" symbol="awn_themed_icon_create_custom_icon_item">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="create_remove_custom_icon_item" symbol="awn_themed_icon_create_remove_custom_icon_item">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="drag_data_received" symbol="awn_themed_icon_drag_data_received">
				<return-type type="void"/>
				<parameters>
					<parameter name="widget" type="GtkWidget*"/>
					<parameter name="context" type="GdkDragContext*"/>
					<parameter name="x" type="gint"/>
					<parameter name="y" type="gint"/>
					<parameter name="selection_data" type="GtkSelectionData*"/>
					<parameter name="info" type="guint"/>
					<parameter name="evt_time" type="guint"/>
				</parameters>
			</method>
			<method name="get_awn_theme" symbol="awn_themed_icon_get_awn_theme">
				<return-type type="GtkIconTheme*"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
				</parameters>
			</method>
			<method name="get_default_theme_name" symbol="awn_themed_icon_get_default_theme_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
				</parameters>
			</method>
			<method name="get_icon_at_size" symbol="awn_themed_icon_get_icon_at_size">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="size" type="gint"/>
					<parameter name="state" type="gchar*"/>
				</parameters>
			</method>
			<method name="get_size" symbol="awn_themed_icon_get_size">
				<return-type type="gint"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
				</parameters>
			</method>
			<method name="get_state" symbol="awn_themed_icon_get_state">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_themed_icon_new">
				<return-type type="GtkWidget*"/>
			</constructor>
			<method name="override_gtk_theme" symbol="awn_themed_icon_override_gtk_theme">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="theme_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="preload_icon" symbol="awn_themed_icon_preload_icon">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="state" type="gchar*"/>
					<parameter name="size" type="gint"/>
				</parameters>
			</method>
			<method name="set_applet_info" symbol="awn_themed_icon_set_applet_info">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="applet_name" type="gchar*"/>
					<parameter name="uid" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_info" symbol="awn_themed_icon_set_info">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="applet_name" type="gchar*"/>
					<parameter name="uid" type="gchar*"/>
					<parameter name="states" type="GStrv"/>
					<parameter name="icon_names" type="GStrv"/>
				</parameters>
			</method>
			<method name="set_info_append" symbol="awn_themed_icon_set_info_append">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="state" type="gchar*"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_info_simple" symbol="awn_themed_icon_set_info_simple">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="applet_name" type="gchar*"/>
					<parameter name="uid" type="gchar*"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_size" symbol="awn_themed_icon_set_size">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="size" type="gint"/>
				</parameters>
			</method>
			<method name="set_state" symbol="awn_themed_icon_set_state">
				<return-type type="void"/>
				<parameters>
					<parameter name="icon" type="AwnThemedIcon*"/>
					<parameter name="state" type="gchar*"/>
				</parameters>
			</method>
			<property name="applet-name" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="drag-and-drop" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="rotate" type="GdkPixbufRotation" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<object name="AwnTooltip" parent="GtkWindow" type-name="AwnTooltip" get-type="awn_tooltip_get_type">
			<implements>
				<interface name="AtkImplementor"/>
				<interface name="GtkBuildable"/>
			</implements>
			<method name="get_delay" symbol="awn_tooltip_get_delay">
				<return-type type="gint"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
				</parameters>
			</method>
			<method name="get_text" symbol="awn_tooltip_get_text">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
				</parameters>
			</method>
			<constructor name="new_for_widget" symbol="awn_tooltip_new_for_widget">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="widget" type="GtkWidget*"/>
				</parameters>
			</constructor>
			<method name="set_background_color" symbol="awn_tooltip_set_background_color">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="bg_color" type="DesktopAgnosticColor*"/>
				</parameters>
			</method>
			<method name="set_delay" symbol="awn_tooltip_set_delay">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="msecs" type="gint"/>
				</parameters>
			</method>
			<method name="set_focus_widget" symbol="awn_tooltip_set_focus_widget">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="widget" type="GtkWidget*"/>
				</parameters>
			</method>
			<method name="set_font_color" symbol="awn_tooltip_set_font_color">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="font_color" type="DesktopAgnosticColor*"/>
				</parameters>
			</method>
			<method name="set_font_name" symbol="awn_tooltip_set_font_name">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="font_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_outline_color" symbol="awn_tooltip_set_outline_color">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="outline" type="DesktopAgnosticColor*"/>
				</parameters>
			</method>
			<method name="set_position_hint" symbol="awn_tooltip_set_position_hint">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="position" type="GtkPositionType"/>
					<parameter name="size" type="gint"/>
				</parameters>
			</method>
			<method name="set_text" symbol="awn_tooltip_set_text">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
					<parameter name="text" type="gchar*"/>
				</parameters>
			</method>
			<method name="update_position" symbol="awn_tooltip_update_position">
				<return-type type="void"/>
				<parameters>
					<parameter name="tooltip" type="AwnTooltip*"/>
				</parameters>
			</method>
			<property name="delay" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="focus-widget" type="GtkWidget*" readable="1" writable="1" construct="0" construct-only="0"/>
			<property name="offset" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="smart-behavior" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="toggle-on-click" type="gboolean" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="tooltip-bg-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="tooltip-font-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="tooltip-font-name" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="tooltip-outline-color" type="DesktopAgnosticColor*" readable="1" writable="1" construct="1" construct-only="0"/>
		</object>
		<interface name="AwnOverlayable" type-name="AwnOverlayable" get-type="awn_overlayable_get_type">
			<method name="add_overlay" symbol="awn_overlayable_add_overlay">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="AwnOverlayable*"/>
					<parameter name="overlay" type="AwnOverlay*"/>
				</parameters>
			</method>
			<method name="get_effects" symbol="awn_overlayable_get_effects">
				<return-type type="AwnEffects*"/>
				<parameters>
					<parameter name="self" type="AwnOverlayable*"/>
				</parameters>
			</method>
			<method name="get_overlays" symbol="awn_overlayable_get_overlays">
				<return-type type="GList*"/>
				<parameters>
					<parameter name="self" type="AwnOverlayable*"/>
				</parameters>
			</method>
			<method name="remove_overlay" symbol="awn_overlayable_remove_overlay">
				<return-type type="void"/>
				<parameters>
					<parameter name="self" type="AwnOverlayable*"/>
					<parameter name="overlay" type="AwnOverlay*"/>
				</parameters>
			</method>
			<vfunc name="get_effects">
				<return-type type="AwnEffects*"/>
				<parameters>
					<parameter name="self" type="AwnOverlayable*"/>
				</parameters>
			</vfunc>
		</interface>
		<constant name="AWN_EFFECTS_ACTIVE_RECT_PADDING" type="int" value="3"/>
		<constant name="AWN_MAX_HEIGHT" type="int" value="100"/>
		<constant name="AWN_MIN_HEIGHT" type="int" value="12"/>
		<constant name="AWN_PANEL_ID_DEFAULT" type="int" value="1"/>
	</namespace>
</api>
