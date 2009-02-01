<?xml version="1.0"?>
<api version="1.0">
	<namespace name="Awn">
		<function name="cairo_rounded_rect" symbol="awn_cairo_rounded_rect">
			<return-type type="void"/>
			<parameters>
				<parameter name="cr" type="cairo_t*"/>
				<parameter name="x0" type="int"/>
				<parameter name="y0" type="int"/>
				<parameter name="width" type="int"/>
				<parameter name="height" type="int"/>
				<parameter name="radius" type="double"/>
				<parameter name="state" type="AwnCairoRoundCorners"/>
			</parameters>
		</function>
		<function name="cairo_string_to_color" symbol="awn_cairo_string_to_color">
			<return-type type="void"/>
			<parameters>
				<parameter name="string" type="gchar*"/>
				<parameter name="color" type="AwnColor*"/>
			</parameters>
		</function>
		<function name="get_settings" symbol="awn_get_settings">
			<return-type type="AwnSettings*"/>
		</function>
		<function name="vfs_get_pathlist_from_string" symbol="awn_vfs_get_pathlist_from_string">
			<return-type type="GSList*"/>
			<parameters>
				<parameter name="paths" type="gchar*"/>
				<parameter name="err" type="GError**"/>
			</parameters>
		</function>
		<function name="vfs_init" symbol="awn_vfs_init">
			<return-type type="void"/>
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
				<parameter name="uid" type="gchar*"/>
				<parameter name="orient" type="gint"/>
				<parameter name="height" type="gint"/>
			</parameters>
		</callback>
		<callback name="AwnConfigClientNotifyFunc">
			<return-type type="void"/>
			<parameters>
				<parameter name="entry" type="AwnConfigClientNotifyEntry*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="AwnEffectsOpfn">
			<return-type type="gboolean"/>
			<parameters>
				<parameter name="fx" type="AwnEffects*"/>
				<parameter name="ds" type="DrawIconState*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="AwnEventNotify">
			<return-type type="void"/>
			<parameters>
				<parameter name="p1" type="GtkWidget*"/>
			</parameters>
		</callback>
		<callback name="AwnIconsChange">
			<return-type type="void"/>
			<parameters>
				<parameter name="fx" type="AwnIcons*"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<callback name="AwnTitleCallback">
			<return-type type="gchar*"/>
			<parameters>
				<parameter name="p1" type="GtkWidget*"/>
			</parameters>
		</callback>
		<callback name="AwnVfsMonitorFunc">
			<return-type type="void"/>
			<parameters>
				<parameter name="monitor" type="AwnVfsMonitor*"/>
				<parameter name="monitor_path" type="gchar*"/>
				<parameter name="event_path" type="gchar*"/>
				<parameter name="event" type="AwnVfsMonitorEvent"/>
				<parameter name="user_data" type="gpointer"/>
			</parameters>
		</callback>
		<struct name="AwnColor">
			<field name="red" type="gfloat"/>
			<field name="green" type="gfloat"/>
			<field name="blue" type="gfloat"/>
			<field name="alpha" type="gfloat"/>
		</struct>
		<struct name="AwnConfigClientNotifyEntry">
			<field name="client" type="AwnConfigClient*"/>
			<field name="group" type="gchar*"/>
			<field name="key" type="gchar*"/>
			<field name="value" type="AwnConfigClientValue"/>
		</struct>
		<struct name="AwnDesktopItem">
			<method name="copy" symbol="awn_desktop_item_copy">
				<return-type type="AwnDesktopItem*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
				</parameters>
			</method>
			<method name="exists" symbol="awn_desktop_item_exists">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
				</parameters>
			</method>
			<method name="free" symbol="awn_desktop_item_free">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
				</parameters>
			</method>
			<method name="get_exec" symbol="awn_desktop_item_get_exec">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
				</parameters>
			</method>
			<method name="get_filename" symbol="awn_desktop_item_get_filename">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
				</parameters>
			</method>
			<method name="get_icon" symbol="awn_desktop_item_get_icon">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="icon_theme" type="GtkIconTheme*"/>
				</parameters>
			</method>
			<method name="get_item_type" symbol="awn_desktop_item_get_item_type">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
				</parameters>
			</method>
			<method name="get_localestring" symbol="awn_desktop_item_get_localestring">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="key" type="gchar*"/>
				</parameters>
			</method>
			<method name="get_name" symbol="awn_desktop_item_get_name">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
				</parameters>
			</method>
			<method name="get_string" symbol="awn_desktop_item_get_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="key" type="gchar*"/>
				</parameters>
			</method>
			<method name="launch" symbol="awn_desktop_item_launch">
				<return-type type="gint"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="documents" type="GSList*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="new" symbol="awn_desktop_item_new">
				<return-type type="AwnDesktopItem*"/>
				<parameters>
					<parameter name="filename" type="gchar*"/>
				</parameters>
			</method>
			<method name="save" symbol="awn_desktop_item_save">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="new_filename" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="set_exec" symbol="awn_desktop_item_set_exec">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="exec" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_icon" symbol="awn_desktop_item_set_icon">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="icon" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_item_type" symbol="awn_desktop_item_set_item_type">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="item_type" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_localestring" symbol="awn_desktop_item_set_localestring">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="locale" type="gchar*"/>
					<parameter name="value" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_name" symbol="awn_desktop_item_set_name">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_string" symbol="awn_desktop_item_set_string">
				<return-type type="void"/>
				<parameters>
					<parameter name="item" type="AwnDesktopItem*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="value" type="gchar*"/>
				</parameters>
			</method>
		</struct>
		<struct name="AwnEffectsOp">
			<field name="fn" type="AwnEffectsOpfn"/>
			<field name="data" type="gpointer"/>
		</struct>
		<struct name="AwnSettings">
			<method name="new" symbol="awn_settings_new">
				<return-type type="AwnSettings*"/>
			</method>
			<field name="icon_theme" type="GtkIconTheme*"/>
			<field name="bar" type="GtkWidget*"/>
			<field name="window" type="GtkWidget*"/>
			<field name="title" type="GtkWidget*"/>
			<field name="appman" type="GtkWidget*"/>
			<field name="hot" type="GtkWidget*"/>
			<field name="task_width" type="gint"/>
			<field name="show_dialog" type="gboolean"/>
			<field name="monitor" type="GdkRectangle"/>
			<field name="force_monitor" type="gboolean"/>
			<field name="monitor_height" type="int"/>
			<field name="monitor_width" type="int"/>
			<field name="panel_mode" type="gboolean"/>
			<field name="auto_hide" type="gboolean"/>
			<field name="hidden" type="gboolean"/>
			<field name="hiding" type="gboolean"/>
			<field name="auto_hide_delay" type="gint"/>
			<field name="keep_below" type="gboolean"/>
			<field name="bar_height" type="int"/>
			<field name="bar_angle" type="int"/>
			<field name="bar_pos" type="gfloat"/>
			<field name="no_bar_resize_ani" type="gboolean"/>
			<field name="rounded_corners" type="gboolean"/>
			<field name="corner_radius" type="gfloat"/>
			<field name="render_pattern" type="gboolean"/>
			<field name="expand_bar" type="gboolean"/>
			<field name="pattern_uri" type="gchar*"/>
			<field name="pattern_alpha" type="gfloat"/>
			<field name="g_step_1" type="AwnColor"/>
			<field name="g_step_2" type="AwnColor"/>
			<field name="g_histep_1" type="AwnColor"/>
			<field name="g_histep_2" type="AwnColor"/>
			<field name="border_color" type="AwnColor"/>
			<field name="hilight_color" type="AwnColor"/>
			<field name="show_separator" type="gboolean"/>
			<field name="sep_color" type="AwnColor"/>
			<field name="show_all_windows" type="gboolean"/>
			<field name="launchers" type="GSList*"/>
			<field name="use_png" type="gboolean"/>
			<field name="active_png" type="gchar*"/>
			<field name="arrow_color" type="AwnColor"/>
			<field name="arrow_offset" type="int"/>
			<field name="tasks_have_arrows" type="gboolean"/>
			<field name="name_change_notify" type="gboolean"/>
			<field name="alpha_effect" type="gboolean"/>
			<field name="icon_effect" type="gint"/>
			<field name="icon_alpha" type="float"/>
			<field name="reflection_alpha_mult" type="float"/>
			<field name="frame_rate" type="gint"/>
			<field name="icon_depth_on" type="gboolean"/>
			<field name="icon_offset" type="int"/>
			<field name="reflection_offset" type="int"/>
			<field name="show_shadows" type="gboolean"/>
			<field name="text_color" type="AwnColor"/>
			<field name="shadow_color" type="AwnColor"/>
			<field name="background" type="AwnColor"/>
			<field name="font_face" type="gchar*"/>
			<field name="btest" type="gboolean"/>
			<field name="ftest" type="float"/>
			<field name="stest" type="char*"/>
			<field name="ctest" type="AwnColor"/>
			<field name="ltest" type="GSList*"/>
			<field name="bar_width" type="int"/>
			<field name="curviness" type="gfloat"/>
			<field name="curves_symmetry" type="gfloat"/>
		</struct>
		<struct name="AwnVfsMonitor">
			<method name="add" symbol="awn_vfs_monitor_add">
				<return-type type="AwnVfsMonitor*"/>
				<parameters>
					<parameter name="path" type="gchar*"/>
					<parameter name="monitor_type" type="AwnVfsMonitorType"/>
					<parameter name="callback" type="AwnVfsMonitorFunc"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="emit" symbol="awn_vfs_monitor_emit">
				<return-type type="void"/>
				<parameters>
					<parameter name="monitor" type="AwnVfsMonitor*"/>
					<parameter name="path" type="gchar*"/>
					<parameter name="event" type="AwnVfsMonitorEvent"/>
				</parameters>
			</method>
			<method name="remove" symbol="awn_vfs_monitor_remove">
				<return-type type="void"/>
				<parameters>
					<parameter name="monitor" type="AwnVfsMonitor*"/>
				</parameters>
			</method>
		</struct>
		<struct name="DrawIconState">
			<field name="current_height" type="gint"/>
			<field name="current_width" type="gint"/>
			<field name="x1" type="gint"/>
			<field name="y1" type="gint"/>
		</struct>
		<boxed name="AwnConfigClient" type-name="AwnConfigClient" get-type="awn_config_client_get_type">
			<method name="clear" symbol="awn_config_client_clear">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="ensure_group" symbol="awn_config_client_ensure_group">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
				</parameters>
			</method>
			<method name="entry_exists" symbol="awn_config_client_entry_exists">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
				</parameters>
			</method>
			<method name="free" symbol="awn_config_client_free">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
				</parameters>
			</method>
			<method name="get_bool" symbol="awn_config_client_get_bool">
				<return-type type="gboolean"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_float" symbol="awn_config_client_get_float">
				<return-type type="gfloat"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_int" symbol="awn_config_client_get_int">
				<return-type type="gint"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_list" symbol="awn_config_client_get_list">
				<return-type type="GSList*"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="list_type" type="AwnConfigListType"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_string" symbol="awn_config_client_get_string">
				<return-type type="gchar*"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="get_value_type" symbol="awn_config_client_get_value_type">
				<return-type type="AwnConfigValueType"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="key_lock" symbol="awn_config_client_key_lock">
				<return-type type="int"/>
				<parameters>
					<parameter name="fd" type="int"/>
					<parameter name="operation" type="int"/>
				</parameters>
			</method>
			<method name="key_lock_close" symbol="awn_config_client_key_lock_close">
				<return-type type="int"/>
				<parameters>
					<parameter name="fd" type="int"/>
				</parameters>
			</method>
			<method name="key_lock_open" symbol="awn_config_client_key_lock_open">
				<return-type type="int"/>
				<parameters>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
				</parameters>
			</method>
			<method name="load_defaults_from_schema" symbol="awn_config_client_load_defaults_from_schema">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_config_client_new">
				<return-type type="AwnConfigClient*"/>
			</constructor>
			<constructor name="new_for_applet" symbol="awn_config_client_new_for_applet">
				<return-type type="AwnConfigClient*"/>
				<parameters>
					<parameter name="name" type="gchar*"/>
					<parameter name="uid" type="gchar*"/>
				</parameters>
			</constructor>
			<method name="notify_add" symbol="awn_config_client_notify_add">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="callback" type="AwnConfigClientNotifyFunc"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="query_backend" symbol="awn_config_client_query_backend">
				<return-type type="AwnConfigBackend"/>
			</method>
			<method name="set_bool" symbol="awn_config_client_set_bool">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="value" type="gboolean"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="set_float" symbol="awn_config_client_set_float">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="value" type="gfloat"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="set_int" symbol="awn_config_client_set_int">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="value" type="gint"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="set_list" symbol="awn_config_client_set_list">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="list_type" type="AwnConfigListType"/>
					<parameter name="value" type="GSList*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
			<method name="set_string" symbol="awn_config_client_set_string">
				<return-type type="void"/>
				<parameters>
					<parameter name="client" type="AwnConfigClient*"/>
					<parameter name="group" type="gchar*"/>
					<parameter name="key" type="gchar*"/>
					<parameter name="value" type="gchar*"/>
					<parameter name="err" type="GError**"/>
				</parameters>
			</method>
		</boxed>
		<boxed name="AwnEffects" type-name="AwnEffects" get-type="awn_effects_get_type">
			<method name="draw_background" symbol="awn_effects_draw_background">
				<return-type type="void"/>
				<parameters>
					<parameter name="p1" type="AwnEffects*"/>
					<parameter name="p2" type="cairo_t*"/>
				</parameters>
			</method>
			<method name="draw_foreground" symbol="awn_effects_draw_foreground">
				<return-type type="void"/>
				<parameters>
					<parameter name="p1" type="AwnEffects*"/>
					<parameter name="p2" type="cairo_t*"/>
				</parameters>
			</method>
			<method name="draw_icons" symbol="awn_effects_draw_icons">
				<return-type type="void"/>
				<parameters>
					<parameter name="p1" type="AwnEffects*"/>
					<parameter name="p2" type="cairo_t*"/>
					<parameter name="p3" type="GdkPixbuf*"/>
					<parameter name="p4" type="GdkPixbuf*"/>
				</parameters>
			</method>
			<method name="draw_icons_cairo" symbol="awn_effects_draw_icons_cairo">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="cr" type="cairo_t*"/>
					<parameter name="p3" type="cairo_t*"/>
					<parameter name="p4" type="cairo_t*"/>
				</parameters>
			</method>
			<method name="draw_set_icon_size" symbol="awn_effects_draw_set_icon_size">
				<return-type type="void"/>
				<parameters>
					<parameter name="p1" type="AwnEffects*"/>
					<parameter name="p2" type="gint"/>
					<parameter name="p3" type="gint"/>
				</parameters>
			</method>
			<method name="draw_set_window_size" symbol="awn_effects_draw_set_window_size">
				<return-type type="void"/>
				<parameters>
					<parameter name="p1" type="AwnEffects*"/>
					<parameter name="p2" type="gint"/>
					<parameter name="p3" type="gint"/>
				</parameters>
			</method>
			<method name="finalize" symbol="awn_effects_finalize">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<method name="free" symbol="awn_effects_free">
				<return-type type="void"/>
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
			<constructor name="new" symbol="awn_effects_new">
				<return-type type="AwnEffects*"/>
			</constructor>
			<constructor name="new_for_widget" symbol="awn_effects_new_for_widget">
				<return-type type="AwnEffects*"/>
				<parameters>
					<parameter name="widget" type="GtkWidget*"/>
				</parameters>
			</constructor>
			<method name="reflection_off" symbol="awn_effects_reflection_off">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<method name="reflection_on" symbol="awn_effects_reflection_on">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<method name="register" symbol="awn_effects_register">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="obj" type="GtkWidget*"/>
				</parameters>
			</method>
			<method name="set_offset_cut" symbol="awn_effects_set_offset_cut">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="cut" type="gboolean"/>
				</parameters>
			</method>
			<method name="set_title" symbol="awn_effects_set_title">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="title" type="AwnTitle*"/>
					<parameter name="title_func" type="AwnTitleCallback"/>
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
					<parameter name="start" type="AwnEventNotify"/>
					<parameter name="stop" type="AwnEventNotify"/>
					<parameter name="max_loops" type="gint"/>
				</parameters>
			</method>
			<method name="stop" symbol="awn_effects_stop">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
					<parameter name="effect" type="AwnEffect"/>
				</parameters>
			</method>
			<method name="unregister" symbol="awn_effects_unregister">
				<return-type type="void"/>
				<parameters>
					<parameter name="fx" type="AwnEffects*"/>
				</parameters>
			</method>
			<field name="self" type="GtkWidget*"/>
			<field name="focus_window" type="GtkWidget*"/>
			<field name="settings" type="AwnSettings*"/>
			<field name="title" type="AwnTitle*"/>
			<field name="get_title" type="AwnTitleCallback"/>
			<field name="effect_queue" type="GList*"/>
			<field name="icon_width" type="gint"/>
			<field name="icon_height" type="gint"/>
			<field name="window_width" type="gint"/>
			<field name="window_height" type="gint"/>
			<field name="effect_lock" type="gboolean"/>
			<field name="current_effect" type="AwnEffect"/>
			<field name="direction" type="AwnEffectSequence"/>
			<field name="count" type="gint"/>
			<field name="x_offset" type="gdouble"/>
			<field name="y_offset" type="gdouble"/>
			<field name="curve_offset" type="gdouble"/>
			<field name="delta_width" type="gint"/>
			<field name="delta_height" type="gint"/>
			<field name="clip_region" type="GtkAllocation"/>
			<field name="rotate_degrees" type="gdouble"/>
			<field name="alpha" type="gfloat"/>
			<field name="spotlight_alpha" type="gfloat"/>
			<field name="saturation" type="gfloat"/>
			<field name="glow_amount" type="gfloat"/>
			<field name="icon_depth" type="gint"/>
			<field name="icon_depth_direction" type="gint"/>
			<field name="hover" type="gboolean"/>
			<field name="clip" type="gboolean"/>
			<field name="flip" type="gboolean"/>
			<field name="spotlight" type="gboolean"/>
			<field name="do_reflections" type="gboolean"/>
			<field name="do_offset_cut" type="gboolean"/>
			<field name="enter_notify" type="guint"/>
			<field name="leave_notify" type="guint"/>
			<field name="timer_id" type="guint"/>
			<field name="icon_ctx" type="cairo_t*"/>
			<field name="reflect_ctx" type="cairo_t*"/>
			<field name="op_list" type="AwnEffectsOp*"/>
			<field name="pad1" type="void*"/>
			<field name="pad2" type="void*"/>
			<field name="pad3" type="void*"/>
			<field name="pad4" type="void*"/>
		</boxed>
		<boxed name="EggDesktopFile" type-name="EggDesktopFile" get-type="awn_desktop_item_get_type">
		</boxed>
		<enum name="AwnCairoRoundCorners" type-name="AwnCairoRoundCorners" get-type="awn_cairo_round_corners_get_type">
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
		<enum name="AwnConfigBackend" type-name="AwnConfigBackend" get-type="awn_config_backend_get_type">
			<member name="AWN_CONFIG_CLIENT_GCONF" value="0"/>
			<member name="AWN_CONFIG_CLIENT_GKEYFILE" value="1"/>
		</enum>
		<enum name="AwnConfigListType" type-name="AwnConfigListType" get-type="awn_config_list_type_get_type">
			<member name="AWN_CONFIG_CLIENT_LIST_TYPE_BOOL" value="0"/>
			<member name="AWN_CONFIG_CLIENT_LIST_TYPE_FLOAT" value="1"/>
			<member name="AWN_CONFIG_CLIENT_LIST_TYPE_INT" value="2"/>
			<member name="AWN_CONFIG_CLIENT_LIST_TYPE_STRING" value="3"/>
		</enum>
		<enum name="AwnConfigValueType" type-name="AwnConfigValueType" get-type="awn_config_value_type_get_type">
			<member name="AWN_CONFIG_VALUE_TYPE_NULL" value="-1"/>
			<member name="AWN_CONFIG_VALUE_TYPE_BOOL" value="0"/>
			<member name="AWN_CONFIG_VALUE_TYPE_FLOAT" value="1"/>
			<member name="AWN_CONFIG_VALUE_TYPE_INT" value="2"/>
			<member name="AWN_CONFIG_VALUE_TYPE_STRING" value="3"/>
			<member name="AWN_CONFIG_VALUE_TYPE_LIST_BOOL" value="4"/>
			<member name="AWN_CONFIG_VALUE_TYPE_LIST_FLOAT" value="5"/>
			<member name="AWN_CONFIG_VALUE_TYPE_LIST_INT" value="6"/>
			<member name="AWN_CONFIG_VALUE_TYPE_LIST_STRING" value="7"/>
		</enum>
		<enum name="AwnEffect" type-name="AwnEffect" get-type="awn_effect_get_type">
			<member name="AWN_EFFECT_NONE" value="0"/>
			<member name="AWN_EFFECT_OPENING" value="1"/>
			<member name="AWN_EFFECT_LAUNCHING" value="2"/>
			<member name="AWN_EFFECT_HOVER" value="3"/>
			<member name="AWN_EFFECT_ATTENTION" value="4"/>
			<member name="AWN_EFFECT_CLOSING" value="5"/>
			<member name="AWN_EFFECT_DESATURATE" value="6"/>
		</enum>
		<enum name="AwnEffectPriority" type-name="AwnEffectPriority" get-type="awn_effect_priority_get_type">
			<member name="AWN_EFFECT_PRIORITY_HIGHEST" value="0"/>
			<member name="AWN_EFFECT_PRIORITY_HIGH" value="1"/>
			<member name="AWN_EFFECT_PRIORITY_ABOVE_NORMAL" value="2"/>
			<member name="AWN_EFFECT_PRIORITY_NORMAL" value="3"/>
			<member name="AWN_EFFECT_PRIORITY_BELOW_NORMAL" value="4"/>
			<member name="AWN_EFFECT_PRIORITY_LOW" value="5"/>
			<member name="AWN_EFFECT_PRIORITY_LOWEST" value="6"/>
		</enum>
		<enum name="AwnEffectSequence" type-name="AwnEffectSequence" get-type="awn_effect_sequence_get_type">
			<member name="AWN_EFFECT_DIR_NONE" value="0"/>
			<member name="AWN_EFFECT_DIR_STOP" value="1"/>
			<member name="AWN_EFFECT_DIR_DOWN" value="2"/>
			<member name="AWN_EFFECT_DIR_UP" value="3"/>
			<member name="AWN_EFFECT_DIR_LEFT" value="4"/>
			<member name="AWN_EFFECT_DIR_RIGHT" value="5"/>
			<member name="AWN_EFFECT_SQUISH_DOWN" value="6"/>
			<member name="AWN_EFFECT_SQUISH_DOWN2" value="7"/>
			<member name="AWN_EFFECT_SQUISH_UP" value="8"/>
			<member name="AWN_EFFECT_SQUISH_UP2" value="9"/>
			<member name="AWN_EFFECT_TURN_1" value="10"/>
			<member name="AWN_EFFECT_TURN_2" value="11"/>
			<member name="AWN_EFFECT_TURN_3" value="12"/>
			<member name="AWN_EFFECT_TURN_4" value="13"/>
			<member name="AWN_EFFECT_SPOTLIGHT_ON" value="14"/>
			<member name="AWN_EFFECT_SPOTLIGHT_TREMBLE_UP" value="15"/>
			<member name="AWN_EFFECT_SPOTLIGHT_TREMBLE_DOWN" value="16"/>
			<member name="AWN_EFFECT_SPOTLIGHT_OFF" value="17"/>
		</enum>
		<enum name="AwnOrientation" type-name="AwnOrientation" get-type="awn_orientation_get_type">
			<member name="AWN_ORIENTATION_BOTTOM" value="0"/>
			<member name="AWN_ORIENTATION_TOP" value="1"/>
			<member name="AWN_ORIENTATION_RIGHT" value="2"/>
			<member name="AWN_ORIENTATION_LEFT" value="3"/>
		</enum>
		<enum name="AwnVfsMonitorEvent" type-name="AwnVfsMonitorEvent" get-type="awn_vfs_monitor_event_get_type">
			<member name="AWN_VFS_MONITOR_EVENT_CHANGED" value="0"/>
			<member name="AWN_VFS_MONITOR_EVENT_CREATED" value="1"/>
			<member name="AWN_VFS_MONITOR_EVENT_DELETED" value="2"/>
		</enum>
		<enum name="AwnVfsMonitorType" type-name="AwnVfsMonitorType" get-type="awn_vfs_monitor_type_get_type">
			<member name="AWN_VFS_MONITOR_FILE" value="0"/>
			<member name="AWN_VFS_MONITOR_DIRECTORY" value="1"/>
		</enum>
		<object name="AwnApplet" parent="GtkEventBox" type-name="AwnApplet" get-type="awn_applet_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="AtkImplementor"/>
			</implements>
			<method name="create_default_menu" symbol="awn_applet_create_default_menu">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="create_pref_item" symbol="awn_applet_create_pref_item">
				<return-type type="GtkWidget*"/>
			</method>
			<method name="get_height" symbol="awn_applet_get_height">
				<return-type type="guint"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</method>
			<method name="get_orientation" symbol="awn_applet_get_orientation">
				<return-type type="AwnOrientation"/>
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
			<constructor name="new" symbol="awn_applet_new">
				<return-type type="AwnApplet*"/>
				<parameters>
					<parameter name="uid" type="gchar*"/>
					<parameter name="orient" type="gint"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</constructor>
			<property name="height" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="orient" type="gint" readable="1" writable="1" construct="1" construct-only="0"/>
			<property name="uid" type="char*" readable="1" writable="1" construct="1" construct-only="0"/>
			<signal name="applet-deleted" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="object" type="AwnApplet*"/>
					<parameter name="p0" type="char*"/>
				</parameters>
			</signal>
			<signal name="height-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</signal>
			<signal name="orientation-changed" when="FIRST">
				<return-type type="void"/>
				<parameters>
					<parameter name="object" type="AwnApplet*"/>
					<parameter name="p0" type="gint"/>
				</parameters>
			</signal>
			<vfunc name="deleted">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="uid" type="gchar*"/>
				</parameters>
			</vfunc>
			<vfunc name="orient_changed">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="oreint" type="AwnOrientation"/>
				</parameters>
			</vfunc>
			<vfunc name="plug_embedded">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</vfunc>
			<vfunc name="size_changed">
				<return-type type="void"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
					<parameter name="x" type="gint"/>
				</parameters>
			</vfunc>
		</object>
		<object name="AwnAppletDialog" parent="GtkWindow" type-name="AwnAppletDialog" get-type="awn_applet_dialog_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="AtkImplementor"/>
			</implements>
			<constructor name="new" symbol="awn_applet_dialog_new">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</constructor>
			<method name="position_reset" symbol="awn_applet_dialog_position_reset">
				<return-type type="void"/>
				<parameters>
					<parameter name="dialog" type="AwnAppletDialog*"/>
				</parameters>
			</method>
		</object>
		<object name="AwnAppletSimple" parent="AwnApplet" type-name="AwnAppletSimple" get-type="awn_applet_simple_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="AtkImplementor"/>
			</implements>
			<method name="effects_off" symbol="awn_applet_simple_effects_off">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<method name="effects_on" symbol="awn_applet_simple_effects_on">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<method name="get_awn_icons" symbol="awn_applet_simple_get_awn_icons">
				<return-type type="AwnIcons*"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<method name="get_effects" symbol="awn_applet_simple_get_effects">
				<return-type type="AwnEffects*"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_applet_simple_new">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="uid" type="gchar*"/>
					<parameter name="orient" type="gint"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</constructor>
			<method name="set_awn_icon" symbol="awn_applet_simple_set_awn_icon">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="applet_name" type="gchar*"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_awn_icon_state" symbol="awn_applet_simple_set_awn_icon_state">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="state" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_awn_icons" symbol="awn_applet_simple_set_awn_icons">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="applet_name" type="gchar*"/>
					<parameter name="states" type="GStrv"/>
					<parameter name="icon_names" type="GStrv"/>
				</parameters>
			</method>
			<method name="set_icon" symbol="awn_applet_simple_set_icon">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="pixbuf" type="GdkPixbuf*"/>
				</parameters>
			</method>
			<method name="set_icon_context" symbol="awn_applet_simple_set_icon_context">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="cr" type="cairo_t*"/>
				</parameters>
			</method>
			<method name="set_icon_context_scaled" symbol="awn_applet_simple_set_icon_context_scaled">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="cr" type="cairo_t*"/>
				</parameters>
			</method>
			<method name="set_temp_icon" symbol="awn_applet_simple_set_temp_icon">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="pixbuf" type="GdkPixbuf*"/>
				</parameters>
			</method>
			<method name="set_title" symbol="awn_applet_simple_set_title">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="title_string" type="char*"/>
				</parameters>
			</method>
			<method name="set_title_visibility" symbol="awn_applet_simple_set_title_visibility">
				<return-type type="void"/>
				<parameters>
					<parameter name="simple" type="AwnAppletSimple*"/>
					<parameter name="state" type="gboolean"/>
				</parameters>
			</method>
		</object>
		<object name="AwnIcons" parent="GObject" type-name="AwnIcons" get-type="awn_icons_get_type">
			<method name="get_icon" symbol="awn_icons_get_icon">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="state" type="gchar*"/>
				</parameters>
			</method>
			<method name="get_icon_at_height" symbol="awn_icons_get_icon_at_height">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="state" type="gchar*"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</method>
			<method name="get_icon_simple" symbol="awn_icons_get_icon_simple">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
				</parameters>
			</method>
			<method name="get_icon_simple_at_height" symbol="awn_icons_get_icon_simple_at_height">
				<return-type type="GdkPixbuf*"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_icons_new">
				<return-type type="AwnIcons*"/>
				<parameters>
					<parameter name="applet_name" type="gchar*"/>
				</parameters>
			</constructor>
			<method name="override_gtk_theme" symbol="awn_icons_override_gtk_theme">
				<return-type type="void"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="theme_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_changed_cb" symbol="awn_icons_set_changed_cb">
				<return-type type="void"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="fn" type="AwnIconsChange"/>
					<parameter name="user_data" type="gpointer"/>
				</parameters>
			</method>
			<method name="set_height" symbol="awn_icons_set_height">
				<return-type type="void"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="height" type="gint"/>
				</parameters>
			</method>
			<method name="set_icon_info" symbol="awn_icons_set_icon_info">
				<return-type type="void"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="applet" type="GtkWidget*"/>
					<parameter name="uid" type="gchar*"/>
					<parameter name="height" type="gint"/>
					<parameter name="icon_name" type="gchar*"/>
				</parameters>
			</method>
			<method name="set_icons_info" symbol="awn_icons_set_icons_info">
				<return-type type="void"/>
				<parameters>
					<parameter name="icons" type="AwnIcons*"/>
					<parameter name="applet" type="GtkWidget*"/>
					<parameter name="uid" type="gchar*"/>
					<parameter name="height" type="gint"/>
					<parameter name="states" type="GStrv"/>
					<parameter name="icon_names" type="GStrv"/>
				</parameters>
			</method>
		</object>
		<object name="AwnPlug" parent="GtkPlug" type-name="AwnPlug" get-type="awn_plug_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="AtkImplementor"/>
			</implements>
			<method name="construct" symbol="awn_plug_construct">
				<return-type type="void"/>
				<parameters>
					<parameter name="plug" type="AwnPlug*"/>
					<parameter name="socket_id" type="GdkNativeWindow"/>
				</parameters>
			</method>
			<constructor name="new" symbol="awn_plug_new">
				<return-type type="GtkWidget*"/>
				<parameters>
					<parameter name="applet" type="AwnApplet*"/>
				</parameters>
			</constructor>
			<signal name="applet-deleted" when="LAST">
				<return-type type="void"/>
				<parameters>
					<parameter name="plug" type="AwnPlug*"/>
					<parameter name="uid" type="char*"/>
				</parameters>
			</signal>
		</object>
		<object name="AwnTitle" parent="GtkWindow" type-name="AwnTitle" get-type="awn_title_get_type">
			<implements>
				<interface name="GtkBuildable"/>
				<interface name="AtkImplementor"/>
			</implements>
			<method name="get_default" symbol="awn_title_get_default">
				<return-type type="GtkWidget*"/>
			</method>
			<method name="hide" symbol="awn_title_hide">
				<return-type type="void"/>
				<parameters>
					<parameter name="title" type="AwnTitle*"/>
					<parameter name="focus" type="GtkWidget*"/>
				</parameters>
			</method>
			<method name="show" symbol="awn_title_show">
				<return-type type="void"/>
				<parameters>
					<parameter name="title" type="AwnTitle*"/>
					<parameter name="focus" type="GtkWidget*"/>
					<parameter name="text" type="gchar*"/>
				</parameters>
			</method>
		</object>
		<constant name="AWN_APPLET_GCONF_PATH" type="char*" value="/apps/avant-window-navigator/applets"/>
		<constant name="AWN_CONFIG_CLIENT_DEFAULT_GROUP" type="char*" value="DEFAULT"/>
		<constant name="AWN_GCONF_PATH" type="char*" value="/apps/avant-window-navigator"/>
		<constant name="AWN_MAX_HEIGHT" type="int" value="100"/>
		<constant name="AWN_MIN_BAR_HEIGHT" type="int" value="10"/>
		<constant name="AWN_MIN_HEIGHT" type="int" value="12"/>
		<union name="AwnConfigClientValue">
			<field name="bool_val" type="gboolean"/>
			<field name="float_val" type="gfloat"/>
			<field name="int_val" type="gint"/>
			<field name="str_val" type="gchar*"/>
			<field name="list_val" type="GSList*"/>
		</union>
	</namespace>
</api>
