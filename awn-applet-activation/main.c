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

#include "config.h"

#include <gtk/gtk.h>
#ifdef USE_GNOME
#include <libgnomevfs/gnome-vfs.h>
#elif defined(USE_XFCE)
#include <thunar-vfs/thunar-vfs.h>
#endif
#include <stdio.h>
#include <string.h>

#include <libawn/awn-defines.h>
#include <libawn/awn-desktop-file.h>

#include "awn-plug.h"

/* Commmand line options */
static gchar    *path = NULL;
static gchar    *uid  = NULL;
static gint64    window = 0;
static gint      orient = AWN_ORIENTATION_BOTTOM;
static gint      height = 50;


static GOptionEntry entries[] = 
{
	{  "path", 
	  'p', 0, 
	  G_OPTION_ARG_STRING, 
	  &path, 
	  "Path to the Awn applets desktop file.", 
	  "" },
	  
	{  "uid", 
	  'u', 0, 
	  G_OPTION_ARG_STRING, 
	  &uid, 
	  "The UID of this applet.", 
	  "" },
	  
	{  "window", 
	  'w', 0, 
	  G_OPTION_ARG_INT64, 
	  &window, 
	  "The window to embed in.", 
	  "" },

	{  "orient", 
	  'o', 0, 
	  G_OPTION_ARG_INT, 
	  &orient, 
	  "Awns current orientation.", 
	  "0" },
	  
	{  "height", 
	  'h', 0, 
	  G_OPTION_ARG_INT, 
	  &height, 
	  "Awns current height.", 
	  "50" },	  
	
  	{ NULL }
};

gint
main (gint argc, gchar **argv)
{
	GError *error = NULL;
	GOptionContext *context;
	AwnDesktopItem *item = NULL;	GtkWidget *plug = NULL;
	const gchar *exec;
	const gchar *name;

	/* Load options */
	context = g_option_context_new (" - Awn Applet Activation Options");
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, &error);	
	
	if (error) {
	        g_print ("%s\n", error->message);	
                return 1;
        }
	
	g_type_init();	
	if (!g_thread_supported ()) g_thread_init (NULL);
#ifdef USE_GNOME
	gnome_vfs_init ();
#elif defined(USE_XFCE)
	thunar_vfs_init ();
#endif
	gtk_init (&argc, &argv);
        
        if (uid == NULL) {
                g_warning ("You need to provide a uid for this applet\n");
                return 1;
        }
        
        /* Try and load the desktop file */
        item = awn_desktop_file_new (path);
        if (item == NULL) {
                g_warning ("This desktop file does not exist %s\n", path);
                return 1;
        }
        
        /* Now we have the file, lets see if we can     
                a) load the dynamic library it points to
                b) Find the correct function within that library */
        exec = awn_desktop_file_get_exec (item);
        
        if (exec == NULL) {
                g_warning ("No exec path found in desktop file %s\n", path);
                return 1;
        }
        
        /* Process (re)naming */
        /*FIXME: Actually make this work */
        name = awn_desktop_file_get_name (item);
        if (name != NULL) {
                gint len = strlen (argv[0]);
                gint nlen = strlen (name);
                if (len < nlen)
                        strncpy (argv[0], name, len);
                else
                        strncpy (argv[0], name, nlen);
                argv[0][nlen] = '\0';
        }

        /* Create a GtkPlug for the applet */
        plug = awn_plug_new (exec, uid, orient, height);
        
        if (plug == NULL) {
                g_warning ("Could not create plug\n");
                return 1;
        }
        name = awn_desktop_file_get_name (item);
        
        if (name != NULL) {
                gtk_window_set_title (GTK_WINDOW (plug), name);
        }
                
        if (window) {
                gtk_plug_construct (GTK_PLUG (plug), window);
        } else {
                gtk_plug_construct (GTK_PLUG (plug), -1);
                gtk_widget_show_all (plug); 
        }
        
        gtk_main ();
        
        return 0;
}

