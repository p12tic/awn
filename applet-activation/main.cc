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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <libintl.h>

#include <libdesktop-agnostic/fdo.h>
#include <libawn/libawn.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

/* Forwards */
GtkWidget *
_awn_applet_new(const gchar *canonical_name,
                const gchar *path,
                const gchar *uid,
                gint         panel_id);
static void
launch_applet_with(const gchar *program,
                   const gchar *file,
                   const gchar *module,
                   const gchar *uid,
                   gint64 window,
                   gint panel_id);

static gint
do_dbus_call ();

static gboolean
execute_wrapper (const gchar* cmd_line,
                 GError **error);

static gint
g_execute (const gchar *file,
           gchar      **argv,
           gchar      **envp,
           gboolean     search_path);


/* Commmand line options */
static gchar    *path = NULL;
static gchar    *uid  = NULL;
static gint64    window = 0;
static gint      panel_id = 1;


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

  {  "panel-id",
     'i', 0,
     G_OPTION_ARG_INT,
     &panel_id,
     "AwnPanel ID for the DBus connection.",
     "" },

  { NULL }
};

gint
main(gint argc, gchar **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  DesktopAgnosticVFSFile *desktop_file = NULL;
  DesktopAgnosticFDODesktopEntry *entry = NULL;
  GtkWidget *applet = NULL;
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

  desktop_agnostic_vfs_init (&error);
  if (error)
  {
    g_critical ("Error initializing VFS subsystem: %s", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

  gtk_init(&argc, &argv);

  if (path == NULL || path[0] == '\0')
  {
    g_warning ("You need to provide path to desktop file");
    return 1;
  }

  if (uid == NULL && window == 0)
  {
    // the binary was run only with path to desktop file (and maybe panel-id)
    // try to connect to the panel and call its AddApplet method.

    return do_dbus_call (panel_id, path);
  }

  if (uid == NULL || uid[0] == '\0' || strcmp (uid, "None") == 0)
  {
    g_warning ("You need to provide a valid UID for this applet");
    return 1;
  }

  if (window == 0)
  {
    g_warning ("You need to specify window ID for this applet!");
    return 1;
  }
  /* Try and load the desktop file */
  desktop_file = desktop_agnostic_vfs_file_new_for_path (path, &error);

  if (error)
  {
    g_critical ("Error: %s", error->message);
    g_error_free (error);
    return 1;
  }

  if (desktop_file == NULL || !desktop_agnostic_vfs_file_exists (desktop_file))
  {
    g_warning ("The desktop file '%s' does not exist.", path);
    return 1;
  }

  entry = desktop_agnostic_fdo_desktop_entry_new_for_file (desktop_file, &error);

  if (error)
  {
    g_critical ("Error: %s", error->message);
    g_error_free (error);
    return 1;
  }

  if (entry == NULL)
  {
    g_warning ("The desktop file '%s' does not exist.", path);
    return 1;
  }

  /* Now we have the file, lets see if we can
          a) load the dynamic library it points to
          b) Find the correct function within that library */
  exec = desktop_agnostic_fdo_desktop_entry_get_string (entry, 
                                                        "X-AWN-AppletExec");

  if (exec == NULL)
  {
    g_warning ("No X-AWN-AppletExec key found in desktop file '%s', exiting.",
               path);
    return 1;
  }

  name = desktop_agnostic_fdo_desktop_entry_get_name (entry);

  /* Check if this is a Python applet */
  type = desktop_agnostic_fdo_desktop_entry_get_string (entry, "X-AWN-AppletType");

  if (!type)
  {
    g_warning ("No X-AWN-AppletType key found in desktop file '%s', exiting.", path);
    return 1;
  }

  if (strcmp(type, "Python") == 0)
  {
    launch_applet_with ("python", path, exec, uid, window, panel_id);
    return 0;
  }

  if (strcmp(type, "Mono") == 0)
  {
    launch_applet_with ("mono", path, exec, uid, window, panel_id);
    return 0;
  }

  /* Set locale stuff */
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);

  /* Extract canonical-name from exec */
  gchar *canonical_name = g_strrstr(exec, "/");
  // canonical-name is now: "/applet.ext" or NULL
  canonical_name = canonical_name ? canonical_name+1 : (gchar*)exec;
  // canonical_name is now: "applet.ext" or "applet.ext"
  gchar *dot = g_strrstr(canonical_name, ".");
  canonical_name = g_strndup(canonical_name,
                         dot ? dot - canonical_name : strlen (canonical_name));

  /* Create a GtkPlug for the applet */
  applet = _awn_applet_new (canonical_name, exec, uid, panel_id);

  g_free (canonical_name);

  if (applet == NULL)
  {
    if (desktop_agnostic_fdo_desktop_entry_key_exists (entry, "X-AWN-NoWindow"))
    {
      return desktop_agnostic_fdo_desktop_entry_get_boolean (entry, 
          "X-AWN-NoWindow") ? 0 : 1;
    }
    else
    {
      g_warning ("Could not create applet!");
      return 1;
    }
  }

  if (name != NULL)
  {
    gtk_window_set_title (GTK_WINDOW (applet), name);
  }

  if (window)
  {
    gtk_plug_construct (GTK_PLUG (applet), window);
    // AwnApplet's embedded signal handler automatically calls show_all()
  }

  gtk_main();

  desktop_agnostic_vfs_shutdown (&error);
  if (error)
  {
    g_critical ("Error shutting down VFS subsystem: %s", error->message);
    g_error_free (error);
    return EXIT_FAILURE;
  }

  return 0;
}

static gint
do_dbus_call (gint panel_id, gchar *desktop_file_path)
{
  GError *error = NULL;
  DesktopAgnosticVFSFile *desktop_file = NULL;
  DBusGConnection *connection;
  DBusGProxy *proxy;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    return 1;
  }

  if (!g_path_is_absolute (desktop_file_path))
  {
    gchar *tmp = desktop_file_path;
    desktop_file_path = g_strdup_printf ("%s/%s", g_get_current_dir (),
                                         desktop_file_path);
    g_free (tmp);
  }

  // check if the file exists
  desktop_file = desktop_agnostic_vfs_file_new_for_path (desktop_file_path,
                                                         &error);

  if (error)
  {
    g_critical ("Error: %s", error->message);
    g_error_free (error);
    return 1;
  }

  if (desktop_file == NULL ||
      !desktop_agnostic_vfs_file_exists (desktop_file))
  {
    g_warning ("The desktop file '%s' does not exist.", desktop_file_path);
    return 1;
  }

  gchar *object_path = g_strdup_printf ("/org/awnproject/Awn/Panel%d",
                                        panel_id);
  proxy = dbus_g_proxy_new_for_name (connection,
                                     "org.awnproject.Awn", object_path,
                                     "org.awnproject.Awn.Panel");

  dbus_g_proxy_call (proxy, "AddApplet", &error,
                     G_TYPE_STRING, desktop_file_path,
                     G_TYPE_INVALID,
                     G_TYPE_INVALID);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    return 1;
  }

  g_object_unref (proxy);
  dbus_g_connection_unref (connection);

  return 0;
}

GtkWidget *
_awn_applet_new(const gchar *canonical_name,
                const gchar *path,
                const gchar *uid,
                gint         panel_id)
{
  GModule *module;
  AwnApplet *applet;
  AwnAppletInitFunc init_func;
  AwnAppletInitPFunc initp_func;

  module = g_module_open(path,
                         G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

  if (module == NULL)
  {
    g_warning ("Unable to load module %s\n", path);
    g_warning ("%s", g_module_error());
    return NULL;
  }

  /* Try and load the factory symbol */
  if (g_module_symbol(module, "awn_applet_factory_init",
                      (gpointer *)&init_func))
  {
    /* create new applet */
    applet = AWN_APPLET(awn_applet_new(canonical_name, uid, panel_id));
    /* send applet to factory method */

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
      applet = initp_func(canonical_name, uid, panel_id);
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

  return (GtkWidget*)applet;
}


static void
launch_applet_with(const gchar *program,
                   const gchar *file,
                   const gchar *module,
                   const gchar *uid,
                   gint64 window,
                   gint panel_id)
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


  cmd = g_strdup_printf("%s %s --uid=%s "
                        "--window=%" G_GINT64_FORMAT " --panel-id=%d",
                        program, exec, uid, window, panel_id);

  // this wraps execv
  execute_wrapper (cmd, &err);

  if (err)
  {
    g_warning ("%s", err->message);
    g_error_free (err);
  }

  g_free(cmd);

  g_free(exec);
}

static gboolean
execute_wrapper (const gchar* cmd_line,
                 GError **error)
{
  gboolean retval;
  gchar **argv = NULL;

  g_return_val_if_fail (cmd_line != NULL, FALSE);

  if (!g_shell_parse_argv (cmd_line,
                           NULL, &argv,
                           error))
    return FALSE;

  // FIXME: perhaps we should do some fd closing etc as glib is?

  // doesn't return if it doesn't fail
  retval = g_execute (argv[0], argv, NULL, TRUE);

  g_strfreev (argv);

  return retval;
}

/* borrowed from glib */
static gchar*
my_strchrnul (const gchar *str, gchar c)
{
  gchar *p = (gchar*) str;
  while (*p && (*p != c))
    ++p;

  return p;
}

static gint
g_execute (const gchar *file,
           gchar      **argv,
           gchar      **envp,
           gboolean     search_path)
{
  if (*file == '\0')
    {
      /* We check the simple case first. */
      errno = ENOENT;
      return -1;
    }

  if (!search_path || strchr (file, '/') != NULL)
    {
      /* Don't search when it contains a slash. */
      if (envp)
        execve (file, argv, envp);
      else
        execv (file, argv);
      /*
      if (errno == ENOEXEC)
        script_execute (file, argv, envp, FALSE);
      */
    }
  else
    {
      gboolean got_eacces = 0;
      const gchar *path, *p;
      gchar *name, *freeme;
      size_t len;
      size_t pathlen;

      path = g_getenv ("PATH");
      if (path == NULL)
        {
          /* There is no `PATH' in the environment.  The default
           * search path in libc is the current directory followed by
           * the path `confstr' returns for `_CS_PATH'.
           */

          /* In GLib we put . last, for security, and don't use the
           * unportable confstr(); UNIX98 does not actually specify
           * what to search if PATH is unset. POSIX may, dunno.
           */

          path = "/bin:/usr/bin:.";
        }

      len = strlen (file) + 1;
      pathlen = strlen (path);
      freeme = name = g_malloc (pathlen + len + 1);

      /* Copy the file name at the top, including '\0'  */
      memcpy (name + pathlen + 1, file, len);
      name = name + pathlen;
      /* And add the slash before the filename  */
      *name = '/';

      p = path;
      do
        {
          char *startp;

          path = p;
          p = my_strchrnul (path, ':');

          if (p == path)
            /* Two adjacent colons, or a colon at the beginning or the end
             * of `PATH' means to search the current directory.
             */
            startp = name + 1;
          else
            startp = memcpy (name - (p - path), path, p - path);

          /* Try to execute this name.  If it works, execv will not return.  */
          if (envp)
            execve (startp, argv, envp);
          else
            execv (startp, argv);

          /*
          if (errno == ENOEXEC)
            script_execute (startp, argv, envp, search_path);
          */

          switch (errno)
            {
            case EACCES:
              /* Record the we got a `Permission denied' error.  If we end
               * up finding no executable we can use, we want to diagnose
               * that we did find one but were denied access.
               */
              got_eacces = TRUE;

              /* FALL THRU */

            case ENOENT:
#ifdef ESTALE
            case ESTALE:
#endif
#ifdef ENOTDIR
            case ENOTDIR:
#endif
              /* Those errors indicate the file is missing or not executable
               * by us, in which case we want to just try the next path
               * directory.
               */
              break;

            default:
              /* Some other error means we found an executable file, but
               * something went wrong executing it; return the error to our
               * caller.
               */
              g_free (freeme);
              return -1;
            }
        }
      while (*p++ != '\0');

      /* We tried every element and none of them worked.  */
      if (got_eacces)
        /* At least one failure was due to permissions, so report that
         * error.
         */
        errno = EACCES;

      g_free (freeme);
    }

  /* Return the error from the last attempt (probably ENOENT).  */
  return -1;
}

