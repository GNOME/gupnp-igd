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

gboolean return_conflict = 0;


static void
test_gupnp_simple_igd_new (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new (NULL);
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

  if (ct == CONNECTION_IP)
    gupnp_service_action_set (action,
        "NewExternalIPAddress", G_TYPE_STRING, IP_ADDRESS_FIRST,
        NULL);
  else if (ct == CONNECTION_PPP)
    gupnp_service_action_set (action,
        "NewExternalIPAddress", G_TYPE_STRING, PPP_ADDRESS_FIRST,
        NULL);
  else
    g_assert_not_reached ();
  gupnp_service_action_return (action);

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
    gupnp_service_action_return (action);
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

  gupnp_service_action_return (action);

  g_free (remote_host);
  g_free (proto);

  g_main_loop_quit (loop);
}

static void
mapped_external_port_cb (GUPnPSimpleIgd *igd, gchar *proto,
    gchar *external_ip, gchar *replaces_external_ip, guint external_port,
    gchar *local_ip, guint local_port, gchar *description, gpointer user_data)
{
  guint requested_external_port = GPOINTER_TO_UINT (user_data);

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
    gupnp_simple_igd_remove_port (igd, "UDP", requested_external_port);
  }
  else
  {
    if (!strcmp (external_ip, IP_ADDRESS_FIRST))
      gupnp_service_notify (GUPNP_SERVICE (ipservice),
          "ExternalIPAddress", G_TYPE_STRING, IP_ADDRESS_SECOND, NULL);
    else if (!strcmp (external_ip, PPP_ADDRESS_FIRST))
      gupnp_service_notify (GUPNP_SERVICE (pppservice),
          "ExternalIPAddress", G_TYPE_STRING, PPP_ADDRESS_SECOND, NULL);
    else
      g_assert_not_reached ();
  }
}

static void
error_mapping_port_cb (GUPnPSimpleIgd *igd, GError *error, gchar *proto,
    guint external_port, gchar *local_ip, guint local_port,
    gchar *description, gpointer user_data)
{
  g_assert_not_reached ();
}

static void
run_gupnp_simple_igd_test (GMainContext *mainctx, GUPnPSimpleIgd *igd,
    guint requested_port)
{
  GUPnPContext *context;
  GUPnPRootDevice *dev;
  GUPnPDeviceInfo *subdev1;
  GUPnPDeviceInfo *subdev2;

  context = gupnp_context_new (mainctx, NULL, 0, NULL);
  g_assert (context);

  if (g_getenv ("XML_PATH"))
    gupnp_context_host_path (context, g_getenv ("XML_PATH"), "");
  else
    gupnp_context_host_path (context, ".", "");

  /*
  gupnp_context_host_path (context, "InternetGatewayDevice.xml", "/InternetGatewayDevice.xml");
  gupnp_context_host_path (context, "WANIPConnection.xml", "/WANIPConnection.xml");
  gupnp_context_host_path (context, "WANPPPConnection.xml", "/WANPPPConnection.xml");
  */

  dev = gupnp_root_device_new (context, "/InternetGatewayDevice.xml");
  g_assert (dev);

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


  g_signal_connect (igd, "mapped-external-port",
      G_CALLBACK (mapped_external_port_cb), GUINT_TO_POINTER (requested_port));
  g_signal_connect (igd, "error-mapping-port",
      G_CALLBACK (error_mapping_port_cb), NULL);

  gupnp_simple_igd_add_port (igd, "UDP", requested_port, "192.168.4.22",
      INTERNAL_PORT, 10, "GUPnP Simple IGD test");

  loop = g_main_loop_new (mainctx, FALSE);

  g_main_loop_run (loop);

  g_object_unref (context);
}

static void
test_gupnp_simple_igd_default_ctx (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new (NULL);

  run_gupnp_simple_igd_test (NULL, igd, INTERNAL_PORT);
  g_object_unref (igd);
}

static void
test_gupnp_simple_igd_custom_ctx (void)
{
  GMainContext *mainctx = g_main_context_new ();
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new (mainctx);

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
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new (NULL);

  run_gupnp_simple_igd_test (NULL, igd, 0);
  g_object_unref (igd);
}


static void
test_gupnp_simple_igd_random_conflict (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new (NULL);

  return_conflict = TRUE;
  run_gupnp_simple_igd_test (NULL, igd, 0);
  return_conflict = FALSE;
  g_object_unref (igd);
}


int main (int argc, char **argv)
{
  g_type_init ();
  g_thread_init (NULL);
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/simpleigd/new", test_gupnp_simple_igd_new);
  g_test_add_func ("/simpleigd/default_ctx", test_gupnp_simple_igd_default_ctx);
  g_test_add_func ("/simpleigd/custom_ctx", test_gupnp_simple_igd_custom_ctx);
  g_test_add_func ("/simpleigd/thread", test_gupnp_simple_igd_thread);
  g_test_add_func ("/simpleigd/random/no_conflict",
      test_gupnp_simple_igd_random_no_conflict);
  g_test_add_func ("/simpleigd/random/conflict",
      test_gupnp_simple_igd_random_conflict);

  g_test_run ();

  return 0;
}

