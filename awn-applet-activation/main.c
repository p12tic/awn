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
#include <errno.h>

#include <libawn/awn-defines.h>
#include <libawn/awn-applet.h>
#include <libdesktop-agnostic/desktop-agnostic.h>

/* Forwards */
GtkWidget *
_awn_applet_new(const gchar *path,
                const gchar *uid,
                gint         orient,
                gint         offset,
                gint         size);
static void
launch_python(const gchar *file,
              const gchar *module,
              const gchar *uid,
              gint64 window,
              gint64 panel_win_id,
              gint orient,
              gint offset,
              gint size);

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
static gint64    awn_id = 0;
static gint      orient = AWN_ORIENTATION_BOTTOM;
static gint      offset = 0;
static gint      size   = 50;


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

  {  "panel-window-id",
     'i', 0,
     G_OPTION_ARG_INT64,
     &awn_id,
     "The window XID of the toplevel of the window we're embedding into.",
     "0" },

  {  "orient",
     'o', 0,
     G_OPTION_ARG_INT,
     &orient,
     "Awns current orientation.",
     "0" },

  {  "offset",
     'f', 0,
     G_OPTION_ARG_INT,
     &offset,
     "Current icon offset of the panel.",
     "0" },

  {  "size",
     's', 0,
     G_OPTION_ARG_INT,
     &size,
     "Current size of the panel.",
     "50" },

  { NULL }
};

gint
main(gint argc, gchar **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  DesktopAgnosticVFSImplementation *vfs;
  DesktopAgnosticDesktopEntryBackend *entry = NULL;
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

  vfs = desktop_agnostic_vfs_get_default (&error);
  if (error)
  {
    g_critical ("An error occurred when trying to create the VFS object: %s",
                error->message);
    g_error_free (error);
    return 1;
  }
  else if (!vfs)
  {
    g_critical ("Could not create the VFS object.");
    return 1;
  }
  desktop_agnostic_vfs_implementation_init (vfs);

  gtk_init(&argc, &argv);

  if (uid == NULL)
  {
    g_warning("You need to provide a uid for this applet\n");
    return 1;
  }

  /* Try and load the desktop file */
  entry = desktop_agnostic_desktop_entry_new_for_filename (path, &error);

  if (error)
  {
    g_critical ("Error: %s", error->message);
    g_error_free (error);
    return 1;
  }

  if (entry == NULL)
  {
    g_warning ("This desktop file '%s' does not exist.", path);
    return 1;
  }

  /* Now we have the file, lets see if we can
          a) load the dynamic library it points to
          b) Find the correct function within that library */
  exec = desktop_agnostic_desktop_entry_backend_get_string (entry, "Exec");

  if (exec == NULL)
  {
    g_warning ("No Exec key found in desktop file '%s', exiting.", path);
    return 1;
  }

  name = desktop_agnostic_desktop_entry_backend_get_name (entry);

  /* Check if this is a Python applet */
  type = desktop_agnostic_desktop_entry_backend_get_string (entry, "X-AWN-AppletType");

  if (!type)
  {
    g_warning ("No X-AWN-AppletType key found in desktop file '%s', exiting.", path);
    return 1;
  }

  if (strcmp(type, "Python") == 0)
  {
    launch_python(path, exec, uid, window, awn_id, orient, offset, size);
    return 0;
  }

  /* Process (re)naming */
  /* FIXME: Actually make this work */
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
  applet = _awn_applet_new (exec, uid, orient, offset, size);

  if (applet == NULL)
  {
    g_warning ("Could not create applet\n");
    return 1;
  }

  if (name != NULL)
  {
    gtk_window_set_title (GTK_WINDOW (applet), name);
  }

  if (window)
  {
    gtk_plug_construct (GTK_PLUG (applet), window);
  }
  else
  {
    gtk_plug_construct (GTK_PLUG (applet), -1);
    gtk_widget_show_all (applet);
  }

  if (awn_id && AWN_IS_APPLET (applet))
  {
    awn_applet_set_panel_window_id (AWN_APPLET (applet), awn_id);
  }

  gtk_main();

  desktop_agnostic_vfs_implementation_shutdown (vfs);

  return 0;
}

GtkWidget *
_awn_applet_new(const gchar *path,
                const gchar *uid,
                gint         orient,
                gint         offset,
                gint         size)
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
    applet = AWN_APPLET(awn_applet_new(uid, orient, offset, size));
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
      applet = AWN_APPLET(initp_func(uid, orient, offset, size));

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

  return GTK_WIDGET (applet);
}


static void
launch_python(const gchar *file,
              const gchar *module,
              const gchar *uid,
              gint64 window,
              gint64 panel_win_id,
              gint orient,
              gint offset,
              gint size)
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


  cmd = g_strdup_printf("python %s --uid=%s "
                        "--window=%" G_GINT64_FORMAT " "
                        "--panel-window-id=%" G_GINT64_FORMAT " "
                        "--orient=%d --offset=%d --size=%d",
                        exec, uid, window, panel_win_id, orient, offset, size);

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

