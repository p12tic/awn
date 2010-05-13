/*
 *  Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
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
 *  Author : Michal Hruby <michal.mhr@gmail.com>
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

gboolean
execute_wrapper (const gchar* cmd_line,
                 GError **error);

static gint
g_execute (const gchar *file,
           gchar      **argv,
           gchar      **envp,
           gboolean     search_path);


gboolean
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

