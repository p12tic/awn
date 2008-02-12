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
 *
 *  Portions Copyright (C) 2001 Havoc Pennington
 *
*/


#include "config.h" 
#include "awn-x.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <string.h>

#ifdef LIBAWN_USE_GNOME
#include <libgnome/libgnome.h>
#endif

#include "inlinepixbufs.h"

#include <libawn/awn-settings.h>




//pulled out of xutils.c

enum {
	STRUT_LEFT = 0,
	STRUT_RIGHT = 1,
	STRUT_TOP = 2,
	STRUT_BOTTOM = 3,
	STRUT_LEFT_START = 4,
	STRUT_LEFT_END = 5,
	STRUT_RIGHT_START = 6,
	STRUT_RIGHT_END = 7,
	STRUT_TOP_START = 8,
	STRUT_TOP_END = 9,
	STRUT_BOTTOM_START = 10,
	STRUT_BOTTOM_END = 11
};

//pulled out of xutils.c
static Atom net_wm_strut              = 0;
static Atom net_wm_strut_partial      = 0;

//pulled out of xutils.c
void
xutils_set_strut (GdkWindow        *gdk_window,
			guint32           strut,
			guint32           strut_start,
			guint32           strut_end)
 {
	Display *display;
	Window   window;
	gulong   struts [12] = { 0, };

	if (!GDK_IS_DRAWABLE (gdk_window))
		return;
	if (!GDK_IS_WINDOW (gdk_window))
		return;

	display = GDK_WINDOW_XDISPLAY (gdk_window);
	window  = GDK_WINDOW_XWINDOW (gdk_window);

	if (net_wm_strut == 0)
		net_wm_strut = XInternAtom (display, "_NET_WM_STRUT", False);
	if (net_wm_strut_partial == 0)
		net_wm_strut_partial = XInternAtom (display, "_NET_WM_STRUT_PARTIAL", False);

	struts [STRUT_BOTTOM] = strut;
	struts [STRUT_BOTTOM_START] = strut_start;
	struts [STRUT_BOTTOM_END] = strut_end;
	
	gdk_error_trap_push ();
	XChangeProperty (display, window, net_wm_strut,
			 XA_CARDINAL, 32, PropModeReplace,
			 (guchar *) &struts, 4);
	XChangeProperty (display, window, net_wm_strut_partial,
			 XA_CARDINAL, 32, PropModeReplace,
			 (guchar *) &struts, 12);
	gdk_error_trap_pop ();
}



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

        icon=wnck_window_get_icon(window);
        icon = gdk_pixbuf_scale_simple( icon, width, height, GDK_INTERP_BILINEAR);    
        return icon;
}
int num = 0;
void 
awn_x_set_strut (GtkWindow *window)
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
  AwnSettings *settings = awn_settings_new ();
	
	gtk_window_get_size (window, &width, &height);
	gtk_window_get_position (window, &x, &y);

	xutils_set_strut ((GTK_WIDGET(window)->window), 
                    (height-settings->icon_offset)/2+settings->icon_offset, x, x+width);
	num++;
	if (num == 20) {
		num = 0;
	}
}

/*
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
*/

GString *
awn_x_get_application_name (WnckWindow *window, WnckApplication *app)
{
	GString *windowName = NULL;
	GString *name = NULL;
	gchar *pos;
	gchar *text;

	windowName = g_string_new(wnck_window_get_name(window));
	pos = g_strrstr((const gchar *)windowName->str," - ");

	if (pos == NULL) {
		name = g_string_new((const gchar *)windowName->str);
    //name = g_string_new(wnck_application_get_name(app));
	} else {
		text = g_strdup(pos + 3);
		name = g_string_new((const gchar *)text);
		g_free(text);
	}

	//g_print("get_name_app: %s\n", wnck_application_get_name(app));
	//g_print("get_name_window: %s\n", windowName->str);
	//g_print("name: %s\n\n", name->str);
	g_string_free(windowName, TRUE);

	return name;
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
	
	name = awn_x_get_application_name(window, app);
    
	name = g_string_prepend (name, ".config/awn/custom-icons/");

	int i = 0;
	for (i = 0; i < name->len; i++) {
		if (name->str[i] == ' ')
			name->str[i] = '-';
	}
			
#ifdef LIBAWN_USE_GNOME
	uri = gnome_util_prepend_user_home(name->str);
#elif defined(LIBAWN_USE_XFCE)
	uri = g_string_free (g_string_prepend (name, g_get_home_dir ()), FALSE);
#endif
	
	if (uri) {
		icon = gdk_pixbuf_new_from_file_at_scale (uri, width, height, TRUE, NULL);
	}
	 
#ifdef LIBAWN_USE_GNOME
	/* free error under Xfce */
	g_string_free (name, TRUE);
#endif
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
		if (error) {
			g_error_free (error);
			error = NULL;
		}
	}
	  
        /* first we try gtkicontheme */
        if (icon == NULL) {
        	icon = gtk_icon_theme_load_icon( theme, name, width, GTK_ICON_LOOKUP_FORCE_SVG, &error);
		if (error) {
			g_error_free (error);
			error = NULL;
		}
	}
        else {
        	g_print("Icon theme could not be loaded");
        }
        if (icon == NULL) {
                /* lets try and load directly from file */
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
		if (error) {
			g_error_free (error);
			error = NULL;
		}
                g_string_free(str, TRUE);
        }
        
        if (icon == NULL) {
                /* lets try and load directly from file */
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
		if (error) {
			g_error_free (error);
			error = NULL;
		}
                g_string_free(str, TRUE);
        }
        
        if (icon == NULL) {
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
		if (error) {
			g_error_free (error);
			error = NULL;
		}
                g_string_free(str, TRUE);
        }
        
        return icon;
}

GdkPixbuf * 
awn_x_get_icon_for_launcher (AwnDesktopItem *item, gint width, gint height)
{
	GString *name = NULL;
	gchar *uri = NULL;
	GdkPixbuf *icon = NULL;
		
	name = g_string_new ( awn_desktop_item_get_exec (item));
	name = g_string_prepend (name, ".config/awn/custom-icons/");
	int i = 0;
	for (i = 0; i < name->len; i++) {
		if (name->str[i] == ' ')
			name->str[i] = '-';
	}	
#ifdef LIBAWN_USE_GNOME
	uri = gnome_util_prepend_user_home(name->str);
#elif defined(LIBAWN_USE_XFCE)
	uri = g_string_free (g_string_prepend (name, g_get_home_dir ()), FALSE);
#endif
	
	//g_print ("%s\n", uri);

	if (uri) {
		icon = gdk_pixbuf_new_from_file_at_scale (uri, width, height, TRUE, NULL);
	}
	
#ifndef LIBAWN_USE_XFCE
	g_string_free (name, TRUE);
#endif
	g_free (uri);
	
	if (icon)
		return icon;
	else {
		char *icon_name;
		icon_name = awn_desktop_item_get_icon (item, gtk_icon_theme_get_default());
		if (icon_name) {
			icon = icon_loader_get_icon_spec (icon_name, width, height);
			g_free (icon_name);
			return icon;
		} else {
			return NULL;
		}
	}
}
