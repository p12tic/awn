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
 *
 *  Notes : Contains functions allowing Awn to get a icon from XOrg using the
 *          xid. Please note that all icon reading code  has been lifted from
 *	    libwnck (XUtils.c), so track bugfixes in libwnck.
*/


#include "config.h" 
#include "awn-x.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <libgnome/libgnome.h>

#include "xutils.h"
#include "inlinepixbufs.h"

#include "awn-gconf.h"

/*	TODO:
	This is a cut-and-paste job at the moment, I still need to bring over 
	the error checking from wnck. However, I have been using it, and haven't
	yet had a problem.
*/
GdkPixbuf * 
awn_x_get_icon (WnckWindow *window, gint width, gint height)
{
	GdkPixbuf *icon;
  	GdkPixbuf *mini_icon;

  	icon = NULL;
  	mini_icon = NULL;
  	
  	
  	if ( _wnck_read_icons_ (wnck_window_get_xid(window),
                        NULL,
                        &icon,
                        width, width,
                        &mini_icon,
                        24,
                        24) ) {
	  	if (mini_icon)
  		gdk_pixbuf_unref (mini_icon);                        
        	if (icon)
        		return icon;
  	}
  	if (mini_icon)
  		gdk_pixbuf_unref (mini_icon);
        
        return gdk_pixbuf_new_from_inline (-1,default_icon_data, TRUE, NULL);
        return wnck_window_get_icon (window);

}
int num = 0;
void 
awn_x_set_strut (GtkWindow *window)
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
  AwnSettings *settings = awn_gconf_new ();
	
	gtk_window_get_size (window, &width, &height);
	gtk_window_get_position (window, &x, &y);
 
	xutils_set_strut ((GTK_WIDGET(window)->window), 
                    (height / 2)+settings->icon_offset, x, x+width);
	num++;
	if (num == 20) {
		num = 0;
	}
}

void
awn_x_set_icon_geometry  (Window xwindow,
			  int    x,
			  int    y,
			  int    width,
			  int    height)
{
  gulong data[4];

  data[0] = x;
  data[1] = y;
  data[2] = width;
  data[3] = height;
  
  _wnck_error_trap_push ();

  XChangeProperty (gdk_display,
		   xwindow,
		   _wnck_atom_get ("_NET_WM_ICON_GEOMETRY"),
		   XA_CARDINAL, 32, PropModeReplace,
		   (guchar *)&data, 4);

  _wnck_error_trap_pop ();
}

GdkPixbuf * 
awn_x_get_icon_for_window (WnckWindow *window, gint width, gint height)
{
	//return awn_x_get_icon (window, width, height);
	WnckApplication *app;
	GString *name = NULL;
	gchar *uri = NULL;
	GdkPixbuf *icon = NULL;
		
	g_return_val_if_fail (WNCK_IS_WINDOW (window), NULL);
	
	app = wnck_window_get_application (window);
	g_return_val_if_fail (WNCK_IS_APPLICATION (app), NULL);
	
	name = g_string_new (wnck_application_get_name (app));
	name = g_string_prepend (name, ".awn/custom-icons/");
	int i = 0;
	for (i = 0; i < name->len; i++) {
		if (name->str[i] == ' ')
			name->str[i] = '-';
	}
			
	uri = gnome_util_prepend_user_home(name->str);
	
	icon = gdk_pixbuf_new_from_file_at_scale (uri, width, height, TRUE, NULL);
	 
	g_string_free (name, TRUE);
	g_free (uri);
	
	if (icon)
		return icon;
	else
		return awn_x_get_icon (window, width, height);
}

static GdkPixbuf * 
icon_loader_get_icon_spec( const char *name, int width, int height )
{
        GdkPixbuf *icon = NULL;
        GdkScreen *screen = gdk_screen_get_default();
        GtkIconTheme *theme = NULL;
        theme = gtk_icon_theme_get_for_screen (screen);
        
        if (!name)
                return NULL;
        
        GError *error = NULL;
        
        GtkIconInfo *icon_info = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (),
                                name, width, 0);
	if (icon_info != NULL) {
		icon = gdk_pixbuf_new_from_file_at_size (
				      gtk_icon_info_get_filename (icon_info),
                                      width, -1, &error);
		gtk_icon_info_free(icon_info);
	}
	  
        /* first we try gtkicontheme */
        if (icon == NULL)
        	icon = gtk_icon_theme_load_icon( theme, name, width, GTK_ICON_LOOKUP_FORCE_SVG, &error);
        else {
        	g_print("Icon theme could not be loaded");
        	error = (GError *) 1;  
        }
        if (icon == NULL) {
                /* lets try and load directly from file */
                error = NULL;
                GString *str;
                
                if ( strstr(name, "/") != NULL )
                        str = g_string_new(name);
                else {
                        str = g_string_new("/usr/share/pixmaps/");
                        g_string_append(str, name);
                }
                
                icon = gdk_pixbuf_new_from_file_at_scale(str->str, 
                                                         width,
                                                         height,
                                                         TRUE, &error);
                g_string_free(str, TRUE);
        }
        
        if (icon == NULL) {
                /* lets try and load directly from file */
                error = NULL;
                GString *str;
                
                if ( strstr(name, "/") != NULL )
                        str = g_string_new(name);
                else {
                        str = g_string_new("/usr/local/share/pixmaps/");
                        g_string_append(str, name);
                }
                
                icon = gdk_pixbuf_new_from_file_at_scale(str->str, 
                                                         width,
                                                         -1,
                                                         TRUE, &error);
                g_string_free(str, TRUE);
        }
        
        if (icon == NULL) {
                error = NULL;
                GString *str;
                
                str = g_string_new("/usr/share/");
                g_string_append(str, name);
                g_string_erase(str, (str->len - 4), -1 );
                g_string_append(str, "/");
		g_string_append(str, "pixmaps/");
		g_string_append(str, name);
		
		icon = gdk_pixbuf_new_from_file_at_scale
       		                             (str->str,
                                             width,
                                             -1,
                                             TRUE,
                                             &error);
                g_string_free(str, TRUE);
        }
        
        return icon;
}

GdkPixbuf * 
awn_x_get_icon_for_launcher (GnomeDesktopItem *item, gint width, gint height)
{
	GString *name = NULL;
	gchar *uri = NULL;
	GdkPixbuf *icon = NULL;
		
	name = g_string_new ( gnome_desktop_item_get_string (item, GNOME_DESKTOP_ITEM_EXEC));
	name = g_string_prepend (name, ".awn/custom-icons/");
	int i = 0;
	for (i = 0; i < name->len; i++) {
		if (name->str[i] == ' ')
			name->str[i] = '-';
	}	
	uri = gnome_util_prepend_user_home(name->str);
	
	//g_print ("%s\n", uri);
	
	icon = gdk_pixbuf_new_from_file_at_scale (uri, width, height, TRUE, NULL);
	
	g_string_free (name, TRUE);
	g_free (uri);
	
	if (icon)
		return icon;
	else {
		char *icon_name;
		icon_name = gnome_desktop_item_get_icon (item, gtk_icon_theme_get_default());
		icon = icon_loader_get_icon_spec (icon_name, width, height) ;
		g_free (icon_name);
		return icon;
	}
}
