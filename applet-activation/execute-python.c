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

gboolean
execute_python (const gchar* module_path, const gchar* args, GError **error);

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
    initialized = TRUE;
  }

  PyObject* main_module = PyImport_AddModule ("__main__");

  PyObject* main_dict = PyModule_GetDict (main_module);
  PyObject* main_dict_copy = PyDict_Copy (main_dict);

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

  g_debug ("PyRun_File result: %p [argv[0]=%s]", res, module_path);
  if (res == NULL)
  {
    PyErr_Print ();
  }

  g_strfreev (argv);
  g_free (python_args);

  return TRUE;
}

