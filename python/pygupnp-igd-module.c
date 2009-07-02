#include <pygobject.h>
#include <libgupnp-igd/gupnp-simple-igd.h>
#include <libgupnp-igd/gupnp-simple-igd-thread.h>

void igd_register_classes (PyObject *d);
void igd_add_constants(PyObject *module, const gchar *strip_prefix);

DL_EXPORT(void) initigd(void);
extern PyMethodDef igd_functions[];

DL_EXPORT(void)
initigd(void)
{
  PyObject *m, *d;

  init_pygobject ();

  m = Py_InitModule ("igd", igd_functions);
  d = PyModule_GetDict (m);

  //PyModule_AddIntConstant (m, "CODEC_ID_ANY", FS_CODEC_ID_ANY);
  //PyModule_AddIntConstant (m, "CODEC_ID_ANY", FS_CODEC_ID_ANY);

  igd_register_classes (d);
  igd_add_constants(m, "GUPNP_");

  if (PyErr_Occurred ()) {
    PyErr_Print();
    Py_FatalError ("can't initialise module gupnp.igd");
  }
}
