/*
 * Copyright (c) 2007 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pygobject.h>
#include <cairo/cairo.h>
#include <pycairo.h>
#include <gtk/gtk.h>
#include <libawn/awn-applet.h>
#include <libawn/awn-applet-simple.h>
#include <libawn/awn-defines.h>
#include <libawn/awn-cairo-utils.h>
#include <libawn/awn-enum-types.h>
#include <libawn/awn-effects.h>
#include <libawn/awn-tooltip.h>

/* the following symbols are declared in awn.c: */
void pyawn_add_constants (PyObject *module, const gchar *strip_prefix);
void pyawn_register_classes (PyObject *d);
extern PyMethodDef pyawn_functions[];

Pycairo_CAPI_t *Pycairo_CAPI;

void sink_awnoverlay (GObject *object)
{
  if (g_object_is_floating (object))
  {
    g_object_ref_sink (object);
  }
}

DL_EXPORT (void)
initawn (void)
{
        PyObject *m, *d;

        init_pygobject ();

        Pycairo_IMPORT;
        if (PyImport_ImportModule ("gtk") == NULL) {
                PyErr_SetString (PyExc_ImportError,
                                 "could not import gtk");
                return;
        }

        pygobject_register_sinkfunc (AWN_TYPE_OVERLAY, sink_awnoverlay);

        m = Py_InitModule ("awn", pyawn_functions);
        d = PyModule_GetDict (m);

        pyawn_register_classes (d);
        pyawn_add_constants (m, "AWN_");

        PyModule_AddIntConstant (m, "PANEL_ID_DEFAULT", AWN_PANEL_ID_DEFAULT);

        if (PyErr_Occurred ()) {
                Py_FatalError ("unable to initialise awn module");
        }
}
