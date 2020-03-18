
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>

#include <glib.h>

#include <libgupnp-igd/gupnp-simple-igd.h>

GMainContext *ctx = NULL;
GMainLoop *loop = NULL;
GUPnPSimpleIgd *igd = NULL;
guint external_port, internal_port;

static gboolean
_remove_port (gpointer user_data)
{
  g_debug ("removing port");
  gupnp_simple_igd_remove_port (igd, "TCP", external_port);

  return FALSE;
}

static void
_mapped_external_port (GUPnPSimpleIgd *igd, gchar *proto,
    gchar *external_ip, gchar *replaces_external_ip, guint external_port,
    gchar *local_ip, guint local_port,
    gchar *description, gpointer user_data)
{
  GSource *src;

  g_debug ("proto:%s ex:%s oldex:%s exp:%u local:%s localp:%u desc:%s",
      proto, external_ip, replaces_external_ip, external_port, local_ip,
      local_port, description);

  src = g_timeout_source_new_seconds (10);
  g_source_set_callback (src, _remove_port, user_data, NULL);
  g_source_attach (src, ctx);
}



static void
_error_mapping_external_port (GUPnPSimpleIgd *igd, GError *error,
    gchar *proto, guint external_port,
    gchar *description, gpointer user_data)
{
  g_error ("proto:%s port:%u desc:%s error: %s:%d %s", proto, external_port,
      description, g_quark_to_string (error->domain), error->code, error->message);
}


int
main (int argc, char **argv)
{

  if (argc != 5)
  {
    g_print ("Usage: %s <external port> <local ip> <local port> <description>\n",
        argv[0]);
    return 0;
  }

  external_port = atoi (argv[1]);
  internal_port = atoi (argv[3]);
  g_return_val_if_fail (external_port && internal_port, 1);

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, FALSE);

  g_main_context_push_thread_default (ctx);
  igd = gupnp_simple_igd_new ();

  g_signal_connect (igd, "mapped-external-port",
      G_CALLBACK (_mapped_external_port),
      NULL);
  g_signal_connect (igd, "error-mapping-port",
      G_CALLBACK (_error_mapping_external_port),
      NULL);

  gupnp_simple_igd_add_port (igd, "TCP", external_port, argv[2],
      internal_port, 20, argv[4]);

  GSource *src = g_timeout_source_new_seconds (15);
  g_source_set_callback (src, (GSourceFunc) g_main_loop_quit, loop, NULL);
  g_source_attach (src, ctx);

  g_main_loop_run (loop);

  g_object_unref (igd);

  g_main_context_pop_thread_default (ctx);

  g_main_loop_unref (loop);
  g_main_context_unref (ctx);

  return 0;
}
