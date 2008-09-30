/* Farsigh2 unit tests for FsCodec
 *
 * Copyright (C) 2007 Collabora, Nokia
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

static GMainLoop *loop = NULL;


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
  gupnp_service_action_set (action,
      "NewExternalIPAddress", G_TYPE_STRING, "127.0.0.3",
      NULL);
  gupnp_service_action_return (action);

}

static void
add_port_mapping_cb (GUPnPService *service,
    GUPnPServiceAction *action,
    gpointer user_data)
{
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

  g_assert (remote_host && !strcmp (remote_host, ""));
  g_assert (external_port == 6543);
  g_assert (proto && (!strcmp (proto, "UDP") || !strcmp (proto, "TCP")));
  g_assert (internal_port == 6543);
  g_assert (internal_client && !strcmp (internal_client, "192.168.4.22"));
  g_assert (enabled == TRUE);
  g_assert (desc != NULL);
  g_assert (lease == 10);

  g_free (remote_host);
  g_free (proto);
  g_free (internal_client);
  g_free (desc);

  gupnp_service_action_return (action);
}


static void
delete_port_mapping_cb (GUPnPService *service,
    GUPnPServiceAction *action,
    gpointer user_data)
{
  gchar *remote_host = NULL;
  guint external_port = 0;
  gchar *proto = NULL;

  gupnp_service_action_get (action,
      "NewRemoteHost", G_TYPE_STRING, &remote_host,
      "NewExternalPort", G_TYPE_UINT, &external_port,
      "NewProtocol", G_TYPE_STRING, &proto,
      NULL);

  g_assert (remote_host != NULL);
  g_assert (external_port);
  g_assert (proto && !strcmp (proto, "UDP"));

  gupnp_service_action_return (action);

  g_free (remote_host);
  g_free (proto);

  g_main_loop_quit (loop);
}

static void
mapping_external_port_cb (GUPnPSimpleIgd *igd, gchar *proto,
    gchar *external_ip, gchar *replaces_external_ip, guint external_port,
    gchar *local_ip, guint local_port, gchar *description, gpointer user_data)
{
  GUPnPService *service = GUPNP_SERVICE (user_data);

  g_assert (external_port == 6543);
  g_assert (proto && !strcmp (proto, "UDP"));
  g_assert (local_port == 6543);
  g_assert (local_ip && !strcmp (local_ip, "192.168.4.22"));
  g_assert (description != NULL);

  if (replaces_external_ip)
  {
    g_assert (!strcmp (replaces_external_ip, "127.0.0.3"));
    g_assert (external_ip && !strcmp (external_ip, "127.0.0.2"));
    gupnp_simple_igd_remove_port (igd, "UDP", external_port);
  }
  else
  {
    g_assert (external_ip && !strcmp (external_ip, "127.0.0.3"));
    gupnp_service_notify (service,
        "ExternalIPAddress", G_TYPE_STRING, "127.0.0.2", NULL);
  }
}

static void
error_mapping_port_cb (GUPnPSimpleIgd *igd, GError *error, gchar *proto,
    guint external_port, gchar *description, gpointer user_data)
{
  g_assert_not_reached ();
}


static void
run_gupnp_simple_igd_test (GMainContext *mainctx, GUPnPSimpleIgd *igd)
{
  GUPnPContext *context;
  GUPnPRootDevice *dev;
  GUPnPServiceInfo *service;
  GUPnPDeviceInfo *subdev1;
  GUPnPDeviceInfo *subdev2;

  context = gupnp_context_new (mainctx, NULL, 0, NULL);
  g_assert (context);

  gupnp_context_host_path (context, "InternetGatewayDevice.xml", "/InternetGatewayDevice.xml");
  gupnp_context_host_path (context, "WANIPConnection.xml", "/WANIPConnection.xml");

  dev = gupnp_root_device_new (context, "/InternetGatewayDevice.xml");
  g_assert (dev);

  subdev1 = gupnp_device_info_get_device (GUPNP_DEVICE_INFO (dev),
      "urn:schemas-upnp-org:device:WANDevice:1");
  g_assert (subdev1);

  subdev2 = gupnp_device_info_get_device (subdev1,
      "urn:schemas-upnp-org:device:WANConnectionDevice:1");
  g_assert (subdev2);
  g_object_unref (subdev1);

  service = gupnp_device_info_get_service (subdev2,
      "urn:schemas-upnp-org:service:WANIPConnection:1");
  g_assert (service);
  g_object_unref (subdev2);

  g_signal_connect (service, "action-invoked::GetExternalIPAddress",
      G_CALLBACK (get_external_ip_address_cb), NULL);
  g_signal_connect (service, "action-invoked::AddPortMapping",
      G_CALLBACK (add_port_mapping_cb), NULL);
  g_signal_connect (service, "action-invoked::DeletePortMapping",
      G_CALLBACK (delete_port_mapping_cb), NULL);

  gupnp_root_device_set_available (dev, TRUE);


  g_signal_connect (igd, "mapped-external-port",
      G_CALLBACK (mapping_external_port_cb), service);
  g_signal_connect (igd, "error-mapping-port",
      G_CALLBACK (error_mapping_port_cb), NULL);

  gupnp_simple_igd_add_port (igd, "UDP", 6543, "192.168.4.22",
      6543, 10, "Farsight test");

  loop = g_main_loop_new (mainctx, FALSE);

  g_main_loop_run (loop);

  g_object_unref (context);
}

static void
test_gupnp_simple_igd_default_ctx (void)
{
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new (NULL);

  run_gupnp_simple_igd_test (NULL, igd);
  g_object_unref (igd);
}

static void
test_gupnp_simple_igd_custom_ctx (void)
{
  GMainContext *mainctx = g_main_context_new ();
  GUPnPSimpleIgd *igd = gupnp_simple_igd_new (mainctx);

  run_gupnp_simple_igd_test (mainctx, igd);
  g_object_unref (igd);
  g_main_context_unref (mainctx);
}


static void
test_gupnp_simple_igd_thread (void)
{
  GUPnPSimpleIgdThread *igd = gupnp_simple_igd_thread_new ();
  GMainContext *mainctx = g_main_context_new ();

  run_gupnp_simple_igd_test (mainctx, GUPNP_SIMPLE_IGD (igd));
  g_object_unref (igd);
  g_main_context_unref (mainctx);
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

  g_test_run ();

  return 0;
}

