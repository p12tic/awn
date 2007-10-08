#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pygobject.h>
#include <cairo/cairo.h>
#include <pycairo.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include <gtk/gtk.h>
#include <libawn/awn-applet.h>
#include <libawn/awn-applet-dialog.h>
#include <libawn/awn-applet-gconf.h>
#include <libawn/awn-applet-simple.h>
#include <libawn/awn-defines.h>
#include <libawn/awn-cairo-utils.h>
#include <libawn/awn-enum-types.h>
#include <libawn/awn-effects.h>
#include <libawn/awn-plug.h>
#include <libawn/awn-title.h>

void pyawn_register_classes (PyObject *d);
extern PyMethodDef pyawn_functions[];

Pycairo_CAPI_t *Pycairo_CAPI;

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

        m = Py_InitModule ("awn", pyawn_functions);
        d = PyModule_GetDict (m);

        pyawn_register_classes (d);

        if (PyErr_Occurred ()) {
                Py_FatalError ("unable to initialise awn module");
        }
}
