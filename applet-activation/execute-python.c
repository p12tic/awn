/*
 *  Copyright (C) 2010 Michal Hruby <michal.mhr@gmail.com>
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <Python.h>
#include <pygobject.h>

gboolean
execute_python (const gchar* module_path, const gchar* args, GError **error);

void
gtk_main_wrapper (void);

static gboolean
pygtk_main_watch_prepare(GSource *source,
                         int     *timeout)
{
    /* Python only invokes signal handlers from the main thread,
     * so if a thread other than the main thread receives the signal
     * from the kernel, PyErr_CheckSignals() from that thread will
     * do nothing. So, we need to time out and check for signals
     * regularily too.
     * Also, on Windows g_poll() won't be interrupted by a signal
     * (AFAIK), so we need the timeout there too.
     */
#ifndef PLATFORM_WIN32
    if (pyg_threads_enabled)
#endif
        *timeout = 100;

    return FALSE;
}

static gboolean
pygtk_main_watch_check(GSource *source)
{
    PyGILState_STATE state;

    state = pyg_gil_state_ensure();

    if (PyErr_CheckSignals() == -1 && gtk_main_level() > 0) {
        PyErr_SetNone(PyExc_KeyboardInterrupt);
        gtk_main_quit();
    }

    pyg_gil_state_release(state);

    return FALSE;
}

static gboolean
pygtk_main_watch_dispatch(GSource    *source,
                          GSourceFunc callback,
                          gpointer    user_data)
{
    /* We should never be dispatched */
    g_assert_not_reached();
    return TRUE;
}

static GSourceFuncs pygtk_main_watch_funcs =
{
    pygtk_main_watch_prepare,
    pygtk_main_watch_check,
    pygtk_main_watch_dispatch,
    NULL
};

static GSource *
pygtk_main_watch_new(void)
{
    return g_source_new(&pygtk_main_watch_funcs, sizeof(GSource));
}


void
gtk_main_wrapper (void)
{
    GSource *main_watch;
    // Call enable_threads again to ensure that the thread state is recorded
    if (pyg_threads_enabled)
        pyg_enable_threads ();

    main_watch = pygtk_main_watch_new();
    pyg_begin_allow_threads;
    g_source_attach(main_watch, NULL);

    gtk_main();
    g_source_destroy(main_watch);
    pyg_end_allow_threads;
}


gboolean
execute_python (const gchar* module_path, const gchar* args, GError **error)
{
  static gboolean initialized = FALSE;
  char *python_args = NULL;
  char **argv = NULL;
  gint argc = 0;

  if (!initialized)
  {
    Py_Initialize ();
    pygobject_init (-1, -1, -1);
    initialized = TRUE;
  }

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure ();

  PyObject* main_module = PyImport_AddModule ("__main__");

  PyObject* main_dict = PyModule_GetDict (main_module);
  //PyObject* main_dict_copy = PyDict_Copy (main_dict);

  gchar* module_name = g_path_get_basename (module_path);

  // set sys.argv
  python_args = g_strdup_printf ("%s %s", module_path, args);
  g_shell_parse_argv (python_args, &argc, &argv, error);
  PySys_SetArgv (argc, argv);

  // set __file__
  PyObject *fn = PyString_FromString (module_path);
  if (fn == NULL) return FALSE;
  PyDict_SetItemString (main_dict, "__file__", fn);
  Py_DECREF (fn);

  FILE* f = fopen (module_path, "r");
  PyObject* res = PyRun_FileEx (f, module_name, Py_file_input, main_dict, main_dict, TRUE);

  g_strfreev (argv);
  g_free (python_args);

  g_debug ("PyRun_File result: %p [argv[0]=%s]", res, module_path);
  if (res == NULL)
  {
    PyErr_Print ();
    PyGILState_Release (gstate);
    return FALSE;
  }

  PyGILState_Release (gstate);
  return TRUE;
}

