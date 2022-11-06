/* GUPNP Simple IGD unit tests
 *
 * Copyright (C) 2008 Collabora, Nokia
 * @author: Olivier Crete <olivier.crete@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#include <string.h>

#include "libgupnp-igd/gupnp-simple-igd.h"
#include "libgupnp-igd/gupnp-simple-igd-thread.h"

#include <libgupnp/gupnp.h>

#define IP_ADDRESS_FIRST   "127.0.0.2"
#define IP_ADDRESS_SECOND  "127.0.0.3"
#define PPP_ADDRESS_FIRST  "127.0.0.4"
#define PPP_ADDRESS_SECOND "127.0.0.5"

#define INTERNAL_PORT    6543

typedef enum {
  CONNECTION_IP,
  CONNECTION_PPP
} ConnectionType;

static GMainLoop *loop = NULL;

static GUPnPServiceInfo *ipservice = NULL;
static GUPnPServiceInfo *pppservice = NULL;

gboolean return_conflict = FALSE;
gboolean dispose_removes = FALSE;
gboolean local_remove = FALSE;
gchar *invalid_ip = NULL;

static void
test_gupnp_simple_igd_new (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();
  GUPnPSimpleIgdThread *igdthread = gupnp_simple_igd_thread_new ();
  GUPnPSimpleIgdThread *igdthread1 = gupnp_simple_igd_thread_new ();

  g_object_unref (igd);
  g_object_unref (igdthread);
  g_object_unref (igdthread1);
}


static void
get_external_ip_address_cb (GUPnPService *service,
    GUPnPServiceAction *action,
    gpointer user_data)
{
  ConnectionType ct = GPOINTER_TO_INT (user_data);

  if (invalid_ip)
    gupnp_service_action_set (action,
        "NewExternalIPAddress", G_TYPE_STRING, invalid_ip,
        NULL);
  else if (ct == CONNECTION_IP)
    gupnp_service_action_set (action,
        "NewExternalIPAddress", G_TYPE_STRING, IP_ADDRESS_FIRST,
        NULL);
  else if (ct == CONNECTION_PPP)
    gupnp_service_action_set (action,
        "NewExternalIPAddress", G_TYPE_STRING, PPP_ADDRESS_FIRST,
        NULL);
  else
    g_assert_not_reached ();

  gupnp_service_action_return_success (action);

}

static void
add_port_mapping_cb (GUPnPService *service,
    GUPnPServiceAction *action,
    gpointer user_data)
{
  guint requested_external_port = GPOINTER_TO_UINT (user_data);
  gchar *remote_host = NULL;
  guint external_port = 0;
  gchar *proto = NULL;
  guint internal_port = 0;
  gchar *internal_client = NULL;
  gboolean enabled = -1;
  gchar *desc = NULL;
  guint lease = 0;

  gupnp_service_action_get (action,
      "NewRemoteHost", G_TYPE_STRING, &remote_host,
      "NewExternalPort", G_TYPE_UINT, &external_port,
      "NewProtocol", G_TYPE_STRING, &proto,
      "NewInternalPort", G_TYPE_UINT, &internal_port,
      "NewInternalClient", G_TYPE_STRING, &internal_client,
      "NewEnabled", G_TYPE_BOOLEAN, &enabled,
      "NewPortMappingDescription", G_TYPE_STRING, &desc,
      "NewLeaseDuration", G_TYPE_UINT, &lease,
      NULL);

  g_assert (external_port);
  g_assert (remote_host && !strcmp (remote_host, ""));
  g_assert (proto && (!strcmp (proto, "UDP") || !strcmp (proto, "TCP")));
  g_assert (internal_port == INTERNAL_PORT);
  g_assert (internal_client && !strcmp (internal_client, "192.168.4.22"));
  g_assert (enabled == TRUE);
  g_assert (desc != NULL);
  g_assert (lease == 10);

  g_free (remote_host);
  g_free (proto);
  g_free (internal_client);
  g_free (desc);

  if (requested_external_port)
    g_assert (external_port == requested_external_port);


  if (return_conflict && external_port == INTERNAL_PORT)
    gupnp_service_action_return_error (action, 718, "ConflictInMappingEntry");
  else
    gupnp_service_action_return_success (action);
}

static gboolean
loop_quit (gpointer user_data) {
    g_main_loop_quit (loop);

    return FALSE;
}

static void
delete_port_mapping_cb (GUPnPService *service,
    GUPnPServiceAction *action,
    gpointer user_data)
{
  guint requested_external_port = GPOINTER_TO_UINT (user_data);
  gchar *remote_host = NULL;
  guint external_port = 0;
  gchar *proto = NULL;

  gupnp_service_action_get (action,
      "NewRemoteHost", G_TYPE_STRING, &remote_host,
      "NewExternalPort", G_TYPE_UINT, &external_port,
      "NewProtocol", G_TYPE_STRING, &proto,
      NULL);

  g_assert (remote_host != NULL);
  if (requested_external_port || !return_conflict)
    g_assert (external_port == INTERNAL_PORT);
  else
    g_assert (external_port != INTERNAL_PORT);
  g_assert (proto && !strcmp (proto, "UDP"));

  gupnp_service_action_return_success (action);

  g_free (remote_host);
  g_free (proto);

  GSource* src = g_idle_source_new ();
  g_source_set_callback (src, loop_quit, NULL, NULL);
  g_source_attach (src, g_main_context_get_thread_default ());
}

typedef struct _MappedData {
    GMainContext *context;
    const char *ip_address;
    guint port;
} MappedData;

gboolean service_notify (gpointer user_data) {
  MappedData *d = (MappedData *) user_data;
  gupnp_service_notify (GUPNP_SERVICE (ipservice),
                        "ExternalIPAddress", G_TYPE_STRING, d->ip_address, NULL);

  return G_SOURCE_REMOVE;
}

static void
mapped_external_port_cb (GUPnPSimpleIgd *igd, gchar *proto,
    gchar *external_ip, gchar *replaces_external_ip, guint external_port,
    gchar *local_ip, guint local_port, gchar *description, gpointer user_data)
{

  MappedData *d = (MappedData *) user_data;
  guint requested_external_port = d->port;

  g_assert (invalid_ip == NULL);

  if (requested_external_port)
    g_assert (external_port == requested_external_port);
  else if (return_conflict)
    g_assert (external_port != INTERNAL_PORT);
  else
    g_assert (external_port == INTERNAL_PORT);
  g_assert (proto && !strcmp (proto, "UDP"));
  g_assert (local_port == INTERNAL_PORT);
  g_assert (local_ip && !strcmp (local_ip, "192.168.4.22"));
  g_assert (description != NULL);
  g_assert (external_ip);

  if (replaces_external_ip)
  {
    g_assert ((!strcmp (replaces_external_ip, IP_ADDRESS_FIRST) &&
            !strcmp (external_ip, IP_ADDRESS_SECOND)) ||
        (!strcmp (replaces_external_ip, PPP_ADDRESS_FIRST) &&
            !strcmp (external_ip, PPP_ADDRESS_SECOND)));
    if (dispose_removes)
      g_object_unref (igd);
    else if (local_remove)
      gupnp_simple_igd_remove_port_local (igd, proto, local_ip, local_port);
    else
      gupnp_simple_igd_remove_port (igd, proto, requested_external_port);
  }
  else
  {
    if (!strcmp (external_ip, IP_ADDRESS_FIRST)) {
      d->ip_address = IP_ADDRESS_SECOND;
      g_main_context_invoke(d->context, service_notify, d);
    } else if (!strcmp (external_ip, PPP_ADDRESS_FIRST)) {
      d->ip_address = PPP_ADDRESS_SECOND;
      g_main_context_invoke(d->context, service_notify, d);
    } else
      g_assert_not_reached ();
  }
}

static void
error_mapping_port_cb (GUPnPSimpleIgd *igd, GError *error, gchar *proto,
    guint external_port, gchar *local_ip, guint local_port,
    gchar *description, gpointer user_data)
{
  g_assert (proto && !strcmp (proto, "UDP"));
  g_assert (local_ip && !strcmp (local_ip, "192.168.4.22"));
  g_assert (description != NULL);
  g_assert (local_port == INTERNAL_PORT);

  if (invalid_ip && error->domain != GUPNP_CONTROL_ERROR)
  {
    g_assert (error);
    g_assert (error->domain == GUPNP_SIMPLE_IGD_ERROR);
    g_assert (error->code == GUPNP_SIMPLE_IGD_ERROR_EXTERNAL_ADDRESS);
    g_assert (error->message);
    g_main_loop_quit (loop);
  }
#if 0
  else
    g_assert_not_reached ();
#endif
}

static gboolean
ignore_non_localhost (GUPnPSimpleIgd *igd, GUPnPContext *gupnp_context,
    gpointer user_data)
{
  if (!g_strcmp0 (gssdp_client_get_interface (GSSDP_CLIENT (gupnp_context)),
          "lo"))
    return FALSE;
  else
    return TRUE;
}

static void
run_gupnp_simple_igd_test (GMainContext *mainctx, GUPnPSimpleIgd *igd,
    guint requested_port)
{
  GUPnPContext *context;
  GUPnPRootDevice *dev;
  GUPnPDeviceInfo *subdev1;
  GUPnPDeviceInfo *subdev2;
  const gchar *xml_path = ".";
  GError *error = NULL;
  GInetAddress *loopback = NULL;

  g_signal_connect (igd, "context-available",
        G_CALLBACK (ignore_non_localhost), NULL);

  if (mainctx)
    g_main_context_push_thread_default (mainctx);
  loopback = g_inet_address_new_loopback (G_SOCKET_FAMILY_IPV4);
  context = gupnp_context_new_for_address (loopback, 0, GSSDP_UDA_VERSION_1_0, NULL);
  g_object_unref (loopback);
  g_assert (context);

  if (g_getenv ("XML_PATH"))
    xml_path = g_getenv ("XML_PATH");


  dev = gupnp_root_device_new (context, "InternetGatewayDevice.xml", xml_path, &error);
  g_assert (dev);
  g_assert (error == NULL);

  subdev1 = gupnp_device_info_get_device (GUPNP_DEVICE_INFO (dev),
      "urn:schemas-upnp-org:device:WANDevice:1");
  g_assert (subdev1);

  subdev2 = gupnp_device_info_get_device (subdev1,
      "urn:schemas-upnp-org:device:WANConnectionDevice:1");
  g_assert (subdev2);
  g_object_unref (subdev1);

  ipservice = gupnp_device_info_get_service (subdev2,
      "urn:schemas-upnp-org:service:WANIPConnection:1");
  g_assert (ipservice);
  pppservice = gupnp_device_info_get_service (subdev2,
      "urn:schemas-upnp-org:service:WANPPPConnection:1");
  g_assert (pppservice);
  g_object_unref (subdev2);

  g_signal_connect (ipservice, "action-invoked::GetExternalIPAddress",
      G_CALLBACK (get_external_ip_address_cb), GINT_TO_POINTER (CONNECTION_IP));
  g_signal_connect (ipservice, "action-invoked::AddPortMapping",
      G_CALLBACK (add_port_mapping_cb), GUINT_TO_POINTER (requested_port));;
  g_signal_connect (ipservice, "action-invoked::DeletePortMapping",
      G_CALLBACK (delete_port_mapping_cb), GUINT_TO_POINTER (requested_port));

  g_signal_connect (pppservice, "action-invoked::GetExternalIPAddress",
      G_CALLBACK (get_external_ip_address_cb),
      GINT_TO_POINTER (CONNECTION_PPP));
  g_signal_connect (pppservice, "action-invoked::AddPortMapping",
      G_CALLBACK (add_port_mapping_cb), GUINT_TO_POINTER (requested_port));
  g_signal_connect (pppservice, "action-invoked::DeletePortMapping",
      G_CALLBACK (delete_port_mapping_cb), GUINT_TO_POINTER (requested_port));


  gupnp_root_device_set_available (dev, TRUE);

  MappedData d;
  d.context = mainctx;
  d.port = requested_port;

  g_signal_connect (igd, "mapped-external-port",
      G_CALLBACK (mapped_external_port_cb), &d);
  g_signal_connect (igd, "error-mapping-port",
      G_CALLBACK (error_mapping_port_cb), NULL);

  gupnp_simple_igd_add_port (igd, "UDP", requested_port, "192.168.4.22",
      INTERNAL_PORT, 10, "GUPnP Simple IGD test");

  loop = g_main_loop_new (mainctx, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  gupnp_root_device_set_available (dev, FALSE);
  g_object_unref (dev);

  if (mainctx)
    g_main_context_pop_thread_default (mainctx);
  g_object_unref (context);

}

static void
test_gupnp_simple_igd_default_ctx (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();

  run_gupnp_simple_igd_test (NULL, igd, INTERNAL_PORT);
  g_object_unref (igd);
}

static void
test_gupnp_simple_igd_default_ctx_local (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();

  local_remove = TRUE;

  run_gupnp_simple_igd_test (NULL, igd, INTERNAL_PORT);
  g_object_unref (igd);

  local_remove = FALSE;
}

static void
test_gupnp_simple_igd_custom_ctx (void)
{
  GMainContext *mainctx = g_main_context_new ();
  GUPnPSimpleIgd *igd;

  g_main_context_push_thread_default (mainctx);
  igd = gupnp_simple_igd_new ();
  g_main_context_pop_thread_default (mainctx);

  run_gupnp_simple_igd_test (mainctx, igd, INTERNAL_PORT);
  g_object_unref (igd);
  g_main_context_unref (mainctx);
}


static void
test_gupnp_simple_igd_thread (void)
{
  GUPnPSimpleIgdThread *igd = gupnp_simple_igd_thread_new ();
  GMainContext *mainctx = g_main_context_new ();

  run_gupnp_simple_igd_test (mainctx, GUPNP_SIMPLE_IGD (igd), INTERNAL_PORT);
  g_object_unref (igd);
  g_main_context_unref (mainctx);
}


static void
test_gupnp_simple_igd_random_no_conflict (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();

  run_gupnp_simple_igd_test (NULL, igd, 0);
  g_object_unref (igd);
}


static void
test_gupnp_simple_igd_random_conflict (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();

  return_conflict = TRUE;
  run_gupnp_simple_igd_test (NULL, igd, 0);
  return_conflict = FALSE;
  g_object_unref (igd);
}


static void
test_gupnp_simple_igd_dispose_removes (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();

  dispose_removes = TRUE;
  run_gupnp_simple_igd_test (NULL, igd, INTERNAL_PORT);
  dispose_removes = FALSE;
}


static void
test_gupnp_simple_igd_dispose_removes_thread (void)
{
  GUPnPSimpleIgdThread *igd = gupnp_simple_igd_thread_new ();
  GMainContext *mainctx = g_main_context_new ();

  dispose_removes = TRUE;
  run_gupnp_simple_igd_test (mainctx, GUPNP_SIMPLE_IGD (igd), INTERNAL_PORT);
  dispose_removes = FALSE;
  g_main_context_unref (mainctx);
}


static void
test_gupnp_simple_igd_invalid_ip(void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();

  invalid_ip = "asdas";
  run_gupnp_simple_igd_test (NULL, igd, INTERNAL_PORT);
  invalid_ip = NULL;
  g_object_unref (igd);
}
static void
test_gupnp_simple_igd_empty_ip(void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new ();

  invalid_ip = "";
  run_gupnp_simple_igd_test (NULL, igd, INTERNAL_PORT);
  invalid_ip = NULL;
  g_object_unref (igd);
}


int main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/simpleigd/new", test_gupnp_simple_igd_new);
  g_test_add_func ("/simpleigd/default_ctx", test_gupnp_simple_igd_default_ctx);
  g_test_add_func ("/simpleigd/default_ctx/remove_local",
      test_gupnp_simple_igd_default_ctx_local);
  g_test_add_func ("/simpleigd/custom_ctx", test_gupnp_simple_igd_custom_ctx);
  g_test_add_func ("/simpleigd/thread", test_gupnp_simple_igd_thread);
  g_test_add_func ("/simpleigd/random/no_conflict",
      test_gupnp_simple_igd_random_no_conflict);
  g_test_add_func ("/simpleigd/random/conflict",
      test_gupnp_simple_igd_random_conflict);
  g_test_add_func ("/simpleigd/dispose_removes/regular",
      test_gupnp_simple_igd_dispose_removes);
  g_test_add_func ("/simpleigd/dispose_removes/thread",
      test_gupnp_simple_igd_dispose_removes_thread);
  g_test_add_func ("/simpleigd/invalid_ip",
      test_gupnp_simple_igd_invalid_ip);
  g_test_add_func ("/simpleigd/empty_ip",
      test_gupnp_simple_igd_empty_ip);

  g_test_run ();

  return 0;
}

