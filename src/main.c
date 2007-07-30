/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "config.h"

#include <config.h>
#include <string.h>
#include <unistd.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "awn-gconf.h"
#include "awn-bar.h"
#include "awn-window.h"
#include "awn-app.h"
#include "awn-win-manager.h"
#include "awn-task-manager.h"
#include "awn-applet-manager.h"
#include "awn-hotspot.h"
#include "awn-utils.h"
#include "awn-task.h"


#define AWN_NAMESPACE "com.google.code.Awn"
#define AWN_OBJECT_PATH "/com/google/code/Awn"
#define AWN_APPLET_NAMESPACE "com.google.code.Awn.AppletManager"
#define AWN_APPLET_OBJECT_PATH "/com/google/code/Awn/AppletManager"

static gboolean expose (GtkWidget *widget, GdkEventExpose *event, AwnSettings *settings);
static gboolean drag_motion (GtkWidget *widget, GdkDragContext *drag_context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time,
                                                     GtkWidget *win);
static gboolean drag_motion_hot (GtkWidget *widget, 
				 GdkDragContext *drag_context,
                                 gint            x,
                                 gint            y,
                                 guint           time,
            		         AwnSettings *settings);            
void drag_leave_hot (GtkWidget      *widget, GdkDragContext *drag_context,
                                             guint           time,
                                             AwnSettings *settings);           		                                                                 
static gboolean enter_notify_event (GtkWidget *window, GdkEventCrossing *event, AwnSettings *settings);
static gboolean leave_notify_event (GtkWidget *window, GdkEventCrossing *event, AwnSettings *settings);
static gboolean button_press_event (GtkWidget *window, GdkEventButton *event);

static void 
bar_height_changed (GConfClient *client, guint cid, GConfEntry *entry, AwnSettings *Settings);

static Atom
panel_atom_get (const char *atom_name)
{
	static GHashTable *atom_hash;
	Display           *xdisplay;
	Atom               retval;

	g_return_val_if_fail (atom_name != NULL, None);

	xdisplay = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());

	if (!atom_hash)
		atom_hash = g_hash_table_new_full (
				g_str_hash, g_str_equal, g_free, NULL);

	retval = GPOINTER_TO_UINT (g_hash_table_lookup (atom_hash, atom_name));
	if (!retval) {
		retval = XInternAtom (xdisplay, atom_name, FALSE);

		if (retval != None)
			g_hash_table_insert (atom_hash, g_strdup (atom_name),
					     GUINT_TO_POINTER (retval));
	}

	return retval;
}
    
int 
main (int argc, char* argv[])
{
	
	AwnSettings* settings;
	GConfClient *client;
	GtkWidget *box = NULL;
	GtkWidget *applet_manager = NULL;
	
	DBusGConnection *connection;
	DBusGProxy *proxy;
	GError *error = NULL;
	guint32 ret;
	
  	if (!g_thread_supported ()) g_thread_init (NULL);
	dbus_g_thread_init ();
  
  	g_type_init ();

  	gtk_init (&argc, &argv);
	gnome_vfs_init ();
	
	settings = awn_gconf_new();
	settings->bar = awn_bar_new(settings);
	client = gconf_client_get_default();
	
	gconf_client_notify_add (client, "/apps/avant-window-navigator/bar/bar_height", 
				(GConfClientNotifyFunc)bar_height_changed, settings, 
				NULL, NULL);
	
	settings->window = awn_window_new (settings);
	
	gtk_window_set_policy (GTK_WINDOW (settings->window), FALSE, FALSE, TRUE);
	
	gtk_widget_add_events (GTK_WIDGET (settings->window),GDK_ALL_EVENTS_MASK);
	g_signal_connect(G_OBJECT(settings->window), "expose-event",
			 G_CALLBACK(expose), (gpointer)settings);
	

	box = gtk_hbox_new(FALSE, 2);
	gtk_container_add(GTK_CONTAINER(settings->window), box);
	
	applet_manager = awn_applet_manager_new (settings);
	settings->appman = applet_manager;
	
	gtk_box_pack_start(GTK_BOX(box), gtk_label_new("  "), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), applet_manager, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), gtk_label_new("  "), FALSE, FALSE, 0);
        
	gtk_window_set_transient_for(GTK_WINDOW(settings->window), 
                                     GTK_WINDOW(settings->bar));
        gtk_widget_show_all(settings->bar);
	gtk_widget_show_all(settings->window);
	
	Atom atoms [2] = { None, None };
        
	atoms [0] = panel_atom_get ("_NET_WM_WINDOW_TYPE_DOCK");
        
        XChangeProperty (GDK_WINDOW_XDISPLAY (
                                        GTK_WIDGET (settings->window)->window),
                         GDK_WINDOW_XWINDOW (GTK_WIDGET (settings->window)->window),
			 panel_atom_get ("_NET_WM_WINDOW_TYPE"),
                         XA_ATOM, 32, PropModeReplace,
                         (unsigned char *) atoms, 
			 1);   
	
	GtkWidget *hot = awn_hotspot_new (settings);
	gtk_widget_show (hot);
	
	g_signal_connect (G_OBJECT(settings->window), "drag-motion",
	                  G_CALLBACK(drag_motion), (gpointer)settings->window);
	g_signal_connect (G_OBJECT(hot), "drag-motion",
	                  G_CALLBACK(drag_motion_hot), (gpointer)settings);	      
	g_signal_connect (G_OBJECT(hot), "drag-leave",
	                  G_CALLBACK(drag_leave_hot), (gpointer)settings);
	                  		                              
	                  
	
	g_signal_connect(G_OBJECT(hot), "enter-notify-event",
			 G_CALLBACK(enter_notify_event), (gpointer)settings);	                  
	g_signal_connect(G_OBJECT(settings->window), "leave-notify-event",
			 G_CALLBACK(leave_notify_event), (gpointer)settings);
	
	g_signal_connect(G_OBJECT(settings->window), "button-press-event",
			 G_CALLBACK(button_press_event), (gpointer)settings);
	
	
	/* Get the connection and ensure the name is not used yet */
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (connection == NULL) {
		g_warning ("Failed to make connection to session bus: %s",
			   error->message);
		g_error_free (error);
		//exit(1);
	}
		
	proxy = dbus_g_proxy_new_for_name (connection, DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	/* Now the applet manager */
	if (!org_freedesktop_DBus_request_name (proxy, AWN_APPLET_NAMESPACE,
						0, &ret, &error)) {
		g_warning ("There was an error requesting the name: %s",
			   error->message);
		g_error_free (error);
		//exit(1);
	}
	if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		/* Someone else registered the name before us */
		//exit(1);
	}
	/* Register the applet manager on the bus */
	dbus_g_connection_register_g_object (connection,
					     AWN_APPLET_OBJECT_PATH,
					     G_OBJECT (applet_manager));
	
	//g_timeout_add (1000, (GSourceFunc)load_applets, applet_manager);
	awn_applet_manager_load_applets (AWN_APPLET_MANAGER (applet_manager));
	
	gtk_main ();
	
	return 0;
}

static gboolean
expose (GtkWidget *widget, GdkEventExpose *event, AwnSettings *settings)
{
        gint width, height;
        
        gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
        
        awn_bar_resize(settings->bar, width);
        //awn_window_resize(window, width);
        return FALSE;
}


static gboolean mouse_over_window = FALSE;

static gboolean 
drag_motion (GtkWidget *widget, GdkDragContext *drag_context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time,
                                                     GtkWidget       *window)
{
	mouse_over_window = TRUE;
	return FALSE;
}

static gboolean 
drag_motion_hot (GtkWidget *widget, GdkDragContext *drag_context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time,
                                                     AwnSettings     *settings)
{
	awn_show (settings);	
	return FALSE;
}

void 
drag_leave_hot (GtkWidget *widget, GdkDragContext *drag_context,
                                             guint           time,
                                             AwnSettings *settings) 
{
	gint width, height;
	gint x, y;
	gint x_root, y_root;
	
	GdkDisplay *display = gdk_display_get_default ();
	
	gdk_display_get_pointer (display,
                                 NULL,
                                 &x_root,
                                 &y_root,
                                 0);
                                                         
	if (settings->auto_hide == FALSE) {
		if (settings->hidden == TRUE)
			awn_show (settings);
		return;
	}
	gtk_window_get_position (GTK_WINDOW (settings->window), &x, &y);
	gtk_window_get_size (GTK_WINDOW (settings->window), &width, &height);
	
		
	if ( (x < x_root) && (x_root < x+width) && ( ( settings->monitor.height - (settings->bar_height + 2)) < y_root)) {
		
		//g_print ("Do nothing\n", event->y_root);
	} else {
		awn_hide (settings);
	}
	//g_print ("%d < %f < %d", x, event->x_root, x+width);
	return;
}

static gboolean 
enter_notify_event (GtkWidget *window, GdkEventCrossing *event, AwnSettings *settings)
{
	awn_show (settings);	
	return FALSE;
}

static gboolean 
leave_notify_event (GtkWidget *window, GdkEventCrossing *event, AwnSettings *settings)
{
	gint width, height;
	gint x, y;
	
	if (settings->auto_hide == FALSE) {
		if (settings->hidden == TRUE)
			awn_show (settings);
		return FALSE;
	}
	gtk_window_get_position (GTK_WINDOW (settings->window), &x, &y);
	gtk_window_get_size (GTK_WINDOW (settings->window), &width, &height);
	
	gint x_root = (int)event->x_root;
	
	if ( (x < x_root) && (x_root < x+width) && ( ( settings->monitor.height - (settings->bar_height + 2)) < event->y_root)) {
		
		//g_print ("Do nothing\n", event->y_root);
	} else {
		awn_hide (settings);
	}
	//g_print ("%d < %f < %d", x, event->x_root, x+width);
	return FALSE;
}

static void
prefs_function (GtkMenuItem *menuitem, gpointer null)
{
	GError *err = NULL;
	
	gdk_spawn_command_line_on_screen (gdk_screen_get_default(),
					  "avant-preferences", &err);
	
	if (err)
		g_print("%s\n", err->message);
}

static void
launcher_function (GtkMenuItem *menuitem, gpointer null)
{
	GError *err = NULL;
	
	gdk_spawn_command_line_on_screen (gdk_screen_get_default(),
					  "avant-launchers", &err);
	
	if (err)
		g_print("%s\n", err->message);
}

static void
applets_function (GtkMenuItem *menuitem, gpointer null)
{
	GError *err = NULL;
	
	gdk_spawn_command_line_on_screen (gdk_screen_get_default(),
					  "avant-applets", &err);
	
	if (err)
		g_print("%s\n", err->message);
}

static void
close_function (GtkMenuItem *menuitem, gpointer null)
{
	AwnSettings *s = awn_gconf_new ();
	
	awn_applet_manager_quit (AWN_APPLET_MANAGER (s->appman));
	
	gtk_main_quit ();
}

static GtkWidget *
create_menu (void)
{
	GtkWidget *menu;
	GtkWidget *item;
	GtkWidget *image;
	
	menu = gtk_menu_new ();
	
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
	g_signal_connect (G_OBJECT(item), "activate", 
	                  G_CALLBACK(prefs_function), NULL);
	
	item = gtk_image_menu_item_new_with_label ("Configure launchers");
	image = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, 
	                                  GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
	g_signal_connect (G_OBJECT(item), "activate", 
	                  G_CALLBACK(launcher_function), NULL);	
	                  
	item = gtk_image_menu_item_new_with_label ("Configure applets");
	image = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, 
	                                  GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
	g_signal_connect (G_OBJECT(item), "activate", 
	                  G_CALLBACK(applets_function), NULL);	

	
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL(menu), item);
	g_signal_connect (G_OBJECT(item), "activate", G_CALLBACK(close_function), NULL);
	
	gtk_widget_show_all(menu);
	return menu;
}

static gboolean
button_press_event (GtkWidget *window, GdkEventButton *event)
{
	GtkWidget *menu = NULL;
	
	switch (event->button) {
		case 3:
			menu = create_menu ();
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, 
					       NULL, 3, event->time);
			break;
		case 2: // 3v1n0 run preferences on middleclick
			prefs_function (NULL, NULL);
			break;
		default:
			return FALSE;
	}
 
	return FALSE;
}

static void
resize (AwnSettings *settings, gint height)
{
	settings->bar_height = height;
	awn_applet_manager_height_changed (AWN_APPLET_MANAGER (settings->appman));
	gtk_widget_set_size_request (settings->window, -1, (height+2)*2);
	_position_window (settings->window);
	gtk_window_resize(GTK_WINDOW(settings->bar), 
				  settings->monitor.width, 
				  ((settings->bar_height+2) *2));
	gtk_window_move (GTK_WINDOW (settings->bar),
			0, 
			settings->monitor.height - ((settings->bar_height + 2) * 2));
}

static void 
bar_height_changed (GConfClient *client, guint cid, GConfEntry *entry, AwnSettings *settings)
{
	GConfValue *value = NULL;
	gint height;
	
	value = gconf_entry_get_value(entry);
	height = gconf_value_get_int(value);
	resize (settings, height);	
}
