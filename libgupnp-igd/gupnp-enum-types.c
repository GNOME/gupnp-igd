#include "gupnp-enum-types.h"
#include "gupnp-simple-igd.h"

/* enumerations from "gupnp-simple-igd.h" */
GType
gupnp_simple_igd_error_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { GUPNP_SIMPLE_IGD_ERROR_EXTERNAL_ADDRESS, "GUPNP_SIMPLE_IGD_ERROR_EXTERNAL_ADDRESS", "address" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("GUPnPSimpleIgdError", values);
  }
  return etype;
}
