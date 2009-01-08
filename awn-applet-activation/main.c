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
#include <stdio.h>
#include <string.h>

#include <libawn/awn-defines.h>
#include <libawn/awn-desktop-item.h>
#include <libawn/awn-plug.h>
#include <libawn/awn-applet.h>
#include <libawn/awn-vfs.h>

/* Forwards */
GtkWidget *
_awn_plug_new(const gchar *path,
              const gchar *uid,
              gint         orient,
              gint         height);
static void
launch_python(const gchar *file,
              const gchar *module,
              const gchar *uid,
              gint64 window,
              gint orient,
              gint height);

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
main(gint argc, gchar **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  AwnDesktopItem *item = NULL;
  GtkWidget *plug = NULL;
  const gchar *exec;
  const gchar *name;
  const gchar *type;

  /* Load options */
  context = g_option_context_new(" - Awn Applet Activation Options");
  g_option_context_add_main_entries(context, entries, NULL);
  g_option_context_parse(context, &argc, &argv, &error);

  if (error)
  {
    g_print("%s\n", error->message);
    g_error_free(error);
    return 1;
  }

  g_type_init();

  if (!g_thread_supported()) g_thread_init(NULL);

  awn_vfs_init();

  gtk_init(&argc, &argv);

  if (uid == NULL)
  {
    g_warning("You need to provide a uid for this applet\n");
    return 1;
  }

  /* Try and load the desktop file */
  item = awn_desktop_item_new(path);

  if (item == NULL)
  {
    g_warning("This desktop file does not exist %s\n", path);
    return 1;
  }

  /* Now we have the file, lets see if we can
          a) load the dynamic library it points to
          b) Find the correct function within that library */
  exec = awn_desktop_item_get_exec(item);

  if (exec == NULL)
  {
    g_warning("No exec path found in desktop file %s\n", path);
    return 1;
  }

  name = awn_desktop_item_get_name(item);

  /* Check if this is a Python applet */
  type = awn_desktop_item_get_string(item, "X-AWN-AppletType");

  if (!type)
  {
    /* FIXME we'll maintain this for a bit until the .desktop files are fixed */
    type = awn_desktop_item_get_item_type(item);

    if (type)
    {
      g_warning("Please inform the developer(s) of the applet '%s' that the .desktop file associated with their applet need to be updated per the Applet Development Guidelines on the AWN Wiki.", name);
    }
  }

  if (type)
  {
    if (strcmp(type, "Python") == 0)
    {
      launch_python(path,
                    exec,
                    uid,
                    window,
                    orient,
                    height);
      return 0;
    }
  }

  /* Process (re)naming */
  /*FIXME: Actually make this work */
  if (name != NULL)
  {
    gint len = strlen(argv[0]);
    gint nlen = strlen(name);

    if (len < nlen)
      strncpy(argv[0], name, len);
    else
      strncpy(argv[0], name, nlen);

    argv[0][nlen] = '\0';
  }

  /* Create a GtkPlug for the applet */
  plug = _awn_plug_new(exec, uid, orient, height);

  if (plug == NULL)
  {
    g_warning("Could not create plug\n");
    return 1;
  }

  name = awn_desktop_item_get_name(item);

  if (name != NULL)
  {
    gtk_window_set_title(GTK_WINDOW(plug), name);
  }

  if (window)
  {
    gtk_plug_construct(GTK_PLUG(plug), window);
  }
  else
  {
    gtk_plug_construct(GTK_PLUG(plug), -1);
    gtk_widget_show_all(plug);
  }

  gtk_main();

  return 0;
}

GtkWidget *
_awn_plug_new(const gchar *path,
              const gchar *uid,
              gint         orient,
              gint         height)
{
  GModule *module;
  AwnApplet *applet;
  GtkWidget *plug;
  AwnAppletInitFunc init_func;
  AwnAppletInitPFunc initp_func;

  module = g_module_open(path,
                         G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

  if (module == NULL)
  {
    g_warning("Unable to load module %s\n", path);
    g_warning("%s", g_module_error());
    return NULL;
  }

  /* Try and load the factory symbol */
  if (g_module_symbol(module, "awn_applet_factory_init",
                      (gpointer *)&init_func))
  {
    // create new applet
    applet = AWN_APPLET(awn_applet_new(uid, orient, height));
    // send applet to factory method

    if (!init_func(applet))
    {
      g_warning("Unable to create applet from factory\n");
      gtk_object_destroy(GTK_OBJECT(applet));
      return NULL;
    }

  }
  else
  {
    /* Try and load the factory_init symbol */
    if (g_module_symbol(module, "awn_applet_factory_initp",
                        (gpointer *)&initp_func))
    {
      /* Create the applet */
      applet = AWN_APPLET(initp_func(uid, orient, height));

      if (applet == NULL)
      {
        g_warning("awn_applet_factory_initp method returned NULL");
        return NULL;
      }
    }
    else
    {
      g_warning("awn_applet_factory_init method not found in applet '%s'", path);
      g_warning("%s: %s", path, g_module_error());

      if (!g_module_close(module))
      {
        g_warning("%s: %s", path, g_module_error());
      }

      return NULL;
    }
  }

  plug = awn_plug_new(applet);

  gtk_container_add(GTK_CONTAINER(plug), GTK_WIDGET(applet));

  return plug;
}


static void
launch_python(const gchar *file,
              const gchar *module,
              const gchar *uid,
              gint64 window,
              gint orient,
              gint height)
{
  gchar *cmd = NULL;
  gchar *exec = NULL;
  GError *err = NULL;


  if (g_path_is_absolute(module))
  {
    exec = g_strdup(module);
  }
  else
  {
    gchar *dir = g_path_get_dirname(file);
    exec = g_build_filename(dir, module, NULL);
    g_free(dir);
  }


  cmd = g_strdup_printf("python %s --uid=%s --window=%" G_GINT64_FORMAT " "
                        " --orient=%d --height=%d",
                        exec, uid, window, orient, height);
  g_spawn_command_line_async(cmd, &err);

  if (err)
  {
    g_warning("%s", err->message);
    g_error_free(err);
  }

  g_free(cmd);

  g_free(exec);
}
