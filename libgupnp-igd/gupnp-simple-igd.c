/*
 * GUPnP Simple IGD abstraction
 *
 * Copyright 2008 Collabora Ltd.
 *  @author: Olivier Crete <olivier.crete@collabora.co.uk>
 * Copyright 2008 Nokia Corp.
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


/**
 * SECTION:gupnp-simple-igd
 * @short_description: A simple class to map ports on UPnP routers
 *
 * This simple class allows applications to map ports on UPnP routers.
 * It implements the basic functionalities to map ports to external ports.
 * It also allows implementations to know the external port from the router's
 * perspective.
 *
 * Every call to this object, including its creation, MUST always be done
 * using the same thread local #GMainContext pushed via
 * g_main_context_push_thread_default();
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "gupnp-simple-igd.h"
#include "gupnp-simple-igd-priv.h"
#include "gupnp-simple-igd-marshal.h"

#include <string.h>

#include <libgupnp/gupnp.h>

#define SOUP_REQUEST_TIMEOUT 5

struct _GUPnPSimpleIgdPrivate
{
  GMainContext *main_context;

  GUPnPContextManager *gupnp_context_manager;

  GPtrArray *service_proxies;
  GPtrArray *mappings;

  gboolean no_new_mappings;

  guint deleting_count;
};

struct Proxy {
  GUPnPSimpleIgd *parent;
  GUPnPControlPoint *cp;
  GUPnPServiceProxy *proxy;

  gchar *external_ip;
  GCancellable *external_ip_cancellable;
  gboolean external_ip_failed;

  GPtrArray *proxymappings;
};

struct Mapping {
  gchar *protocol;
  guint requested_external_port;
  gchar *local_ip;
  guint16 local_port;
  guint32 lease_duration;
  gchar *description;
};

struct ProxyMapping {
  struct Proxy *proxy;
  struct Mapping *mapping;

  GCancellable *cancellable;

  gboolean mapped;
  guint actual_external_port;

  GSource *renew_src;
};

/* signals */
enum
{
  SIGNAL_MAPPED_EXTERNAL_PORT,
  SIGNAL_ERROR_MAPPING_PORT,
  SIGNAL_CONTEXT_AVAILABLE,
  LAST_SIGNAL
};

/* props */
enum
{
  PROP_0,
  PROP_MAIN_CONTEXT
};

guint signals[LAST_SIGNAL] = { 0 };


#define GUPNP_SIMPLE_IGD_GET_PRIVATE(o)                                 \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GUPNP_TYPE_SIMPLE_IGD,             \
   GUPnPSimpleIgdPrivate))


G_DEFINE_TYPE_WITH_CODE (GUPnPSimpleIgd, gupnp_simple_igd, G_TYPE_OBJECT,
    G_ADD_PRIVATE (GUPnPSimpleIgd));


static void gupnp_simple_igd_constructed (GObject *object);
static void gupnp_simple_igd_dispose (GObject *object);
static void gupnp_simple_igd_finalize (GObject *object);
static void gupnp_simple_igd_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec);

static void gupnp_simple_igd_gather (GUPnPSimpleIgd *self,
    struct Proxy *prox);
static void gupnp_simple_igd_add_proxy_mapping (GUPnPSimpleIgd *self,
    struct Proxy *prox,
    struct Mapping *mapping);

static void free_proxy (struct Proxy *prox);
static void free_mapping (GUPnPSimpleIgd *self, struct Mapping *mapping);

static void stop_proxymapping (struct ProxyMapping *pm, gboolean stop_renew);

static void gupnp_simple_igd_add_port_real (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint16 external_port,
    const gchar *local_ip,
    guint16 local_port,
    guint32 lease_duration,
    const gchar *description);
static void gupnp_simple_igd_remove_port_real (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint external_port);
static void gupnp_simple_igd_remove_port_local_real (GUPnPSimpleIgd *self,
    const gchar *protocol,
    const gchar *local_ip,
    guint16 local_port);

GQuark
gupnp_simple_igd_error_quark (void)
{
  return g_quark_from_static_string ("gupnp-simple-igd-error");
}


static void
gupnp_simple_igd_class_init (GUPnPSimpleIgdClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = gupnp_simple_igd_constructed;
  gobject_class->dispose = gupnp_simple_igd_dispose;
  gobject_class->finalize = gupnp_simple_igd_finalize;
  gobject_class->get_property = gupnp_simple_igd_get_property;

  klass->add_port = gupnp_simple_igd_add_port_real;
  klass->remove_port = gupnp_simple_igd_remove_port_real;
  klass->remove_port_local = gupnp_simple_igd_remove_port_local_real;

  g_object_class_install_property (gobject_class,
      PROP_MAIN_CONTEXT,
      g_param_spec_pointer ("main-context",
          "The GMainContext to use",
          "This GMainContext will be used for all async activities",
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * GUPnPSimpleIgd::mapped-external-port:
   * @self: #GUPnPSimpleIgd that emitted the signal
   * @proto: the requested protocol ("UDP" or "TCP")
   * @external_ip: the external IP
   * @replaces_external_ip: if this mapping replaces another mapping,
   *  this is the old external IP
   * @external_port: the external port that was allocated
   * @local_ip: IP address that the router should forward the packets to. It
   *  could be the address of another machine on the local network
   * @local_port: the local port
   * @description: the user's selected description
   *
   * This signal means that an IGD has been found that that adding a port
   * mapping has succeeded.
   */
  signals[SIGNAL_MAPPED_EXTERNAL_PORT] = g_signal_new ("mapped-external-port",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL,
      NULL,
      _gupnp_simple_igd_marshal_VOID__STRING_STRING_STRING_UINT_STRING_UINT_STRING,
      G_TYPE_NONE, 7, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT,
      G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);

  /**
   * GUPnPSimpleIgd::error-mapping-port:
   * @self: #GUPnPSimpleIgd that emitted the signal
   * @error: a #GError
   * @proto: The requested protocol
   * @external_port: the external port requested in gupnp_simple_igd_add_port()
   * @local_ip: internal ip this is forwarded to
   * @local_port: the local port
   * @description: the passed description
   *
   * This means that mapping a port on a specific IGD has failed (it may still
   * succeed on other IGDs on the network).
   */
  signals[SIGNAL_ERROR_MAPPING_PORT] = g_signal_new ("error-mapping-port",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
      0,
      NULL,
      NULL,
      _gupnp_simple_igd_marshal_VOID__BOXED_STRING_UINT_STRING_UINT_STRING,
      G_TYPE_NONE, 6, G_TYPE_ERROR, G_TYPE_STRING, G_TYPE_UINT,
      G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING);

  /**
   * GUPnPSimpleIgd::context-available:
   * @self: #GUPnPSimpleIgd that emitted the signal
   * @context: a #GUPnPContext
   *
   * This is to allow the application to control which #GUPnPContext this
   * client should use. If the application connects to this signal, it controls
   * if a context will be used by changing the return value of the signal
   * handler.
   *
   * Returns: FALSE if the context should be used or TRUE if it should
   * be ignored
   */
  signals[SIGNAL_CONTEXT_AVAILABLE] = g_signal_new ("context-available",
      G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST,
      0,
      NULL,
      NULL,
      _gupnp_simple_igd_marshal_BOOLEAN__OBJECT,
      G_TYPE_BOOLEAN, 1, G_TYPE_OBJECT);
}

static void
gupnp_simple_igd_init (GUPnPSimpleIgd *self)
{
  self->priv = gupnp_simple_igd_get_instance_private (self);

  self->priv->service_proxies = g_ptr_array_new ();
  self->priv->mappings = g_ptr_array_new ();
}

/**
 * gupnp_simple_igd_delete_all_mappings:
 * @self: a #GUPnPSimpleIgd
 *
 * Removes all mappings and prevents other from being formed
 * Should only be called by the dispose function of subclasses
 *
 * Returns: %TRUE if the object can be disposed, %FALSE otherwise
 */

gboolean
gupnp_simple_igd_delete_all_mappings (GUPnPSimpleIgd *self)
{
  self->priv->no_new_mappings = TRUE;

  while (self->priv->mappings->len)
  {
    free_mapping (self, g_ptr_array_index (self->priv->mappings, 0));
    g_ptr_array_remove_index_fast (self->priv->mappings, 0);
  }

  return (self->priv->deleting_count == 0);
}

static void
gupnp_simple_igd_dispose (GObject *object)
{
  GUPnPSimpleIgd *self = GUPNP_SIMPLE_IGD_CAST (object);

  if (!gupnp_simple_igd_delete_all_mappings (self))
    return;

  if (self->priv->gupnp_context_manager)
    g_object_unref (self->priv->gupnp_context_manager);
  self->priv->gupnp_context_manager = NULL;

  if (self->priv->service_proxies) {
    g_ptr_array_foreach (self->priv->service_proxies, (GFunc) free_proxy, NULL);
    g_ptr_array_free (self->priv->service_proxies, TRUE);
  }

  G_OBJECT_CLASS (gupnp_simple_igd_parent_class)->dispose (object);
}

static void
_external_ip_address_changed (GUPnPServiceProxy *proxy, const gchar *variable,
    GValue *value, gpointer user_data)
{
  struct Proxy *prox = user_data;
  gchar *new_ip;
  guint i;

  g_return_if_fail (G_VALUE_HOLDS_STRING(value));

  /* It hasn't really changed, ignore it */
  if (prox->external_ip &&
      !strcmp (g_value_get_string (value), prox->external_ip))
    return;

  /* Ignore invalid external IP address */
  if (!g_hostname_is_ip_address (g_value_get_string (value)))
    return;

  new_ip = g_value_dup_string (value);

  for (i=0; i < prox->proxymappings->len; i++)
  {
    struct ProxyMapping *pm = g_ptr_array_index (prox->proxymappings, i);

    if (pm->mapped)
      g_signal_emit (prox->parent, signals[SIGNAL_MAPPED_EXTERNAL_PORT], 0,
          pm->mapping->protocol, new_ip, prox->external_ip,
          pm->actual_external_port, pm->mapping->local_ip,
          pm->mapping->local_port, pm->mapping->description);
  }

  g_free (prox->external_ip);
  prox->external_ip = new_ip;
}

static void
_service_proxy_delete_port_mapping (GObject *source_object, GAsyncResult *res,
    gpointer user_data)
{
  GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (source_object);
  GUPnPServiceProxyAction *action;
  GError *error = NULL;
  GUPnPSimpleIgd *self = user_data;

  action = gupnp_service_proxy_call_action_finish (proxy, res, &error);

  if (action == NULL ||
      !gupnp_service_proxy_action_get_result (action, &error, NULL)) {
    g_return_if_fail (error);
    g_warning ("Error deleting port mapping: %s", error->message);
  }
  g_clear_error (&error);

  if (action)
    gupnp_service_proxy_action_unref (action);

  self->priv->deleting_count--;
  g_object_unref (self);
}

static void
free_proxymapping (struct ProxyMapping *pm, GUPnPSimpleIgd *self)
{
  stop_proxymapping (pm, TRUE);

  if (pm->mapped && self)
  {
    GUPnPServiceProxyAction *action;

    self->priv->deleting_count++;
    g_object_ref (self);

    action = gupnp_service_proxy_action_new ("DeletePortMapping",
        "NewRemoteHost", G_TYPE_STRING, "",
        "NewExternalPort", G_TYPE_UINT, pm->actual_external_port,
        "NewProtocol", G_TYPE_STRING, pm->mapping->protocol,
        NULL);

    gupnp_service_proxy_call_action_async (pm->proxy->proxy, action, NULL,
        _service_proxy_delete_port_mapping, self);
  }

  g_slice_free (struct ProxyMapping, pm);
}

static void
free_proxy (struct Proxy *prox)
{
  g_cancellable_cancel (prox->external_ip_cancellable);
  g_clear_object (&prox->external_ip_cancellable);

  gupnp_service_proxy_remove_notify (prox->proxy, "ExternalIPAddress",
      _external_ip_address_changed, prox);

  g_ptr_array_foreach (prox->proxymappings, (GFunc) free_proxymapping, NULL);
  g_ptr_array_free (prox->proxymappings, TRUE);
  g_free (prox->external_ip);
  g_slice_free (struct Proxy, prox);
}

static void
free_mapping (GUPnPSimpleIgd *self, struct Mapping *mapping)
{
  guint i, j;

  for (i=0; i < self->priv->service_proxies->len; i++)
  {
    struct Proxy *prox = g_ptr_array_index (self->priv->service_proxies, i);

    for (j=0; j < prox->proxymappings->len; j++)
    {
      struct ProxyMapping *pm = g_ptr_array_index (prox->proxymappings, j);
      if (pm->mapping == mapping)
      {
        free_proxymapping (pm, self);
        g_ptr_array_remove_index_fast (prox->proxymappings, j);
        j--;
      }
    }
  }

  g_free (mapping->protocol);
  g_free (mapping->local_ip);
  g_free (mapping->description);
  g_slice_free (struct Mapping, mapping);
}

static void
gupnp_simple_igd_finalize (GObject *object)
{
  GUPnPSimpleIgd *self = GUPNP_SIMPLE_IGD_CAST (object);

  g_main_context_unref (self->priv->main_context);

  g_warn_if_fail (self->priv->mappings->len == 0);
  g_ptr_array_free (self->priv->mappings, TRUE);

  G_OBJECT_CLASS (gupnp_simple_igd_parent_class)->finalize (object);
}

static void
gupnp_simple_igd_get_property (GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec)
{
  GUPnPSimpleIgd *self = GUPNP_SIMPLE_IGD_CAST (object);

  switch (prop_id) {
    case PROP_MAIN_CONTEXT:
      g_value_set_pointer (value, self->priv->main_context);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }

}


static void
_cp_service_avail (GUPnPControlPoint *cp,
    GUPnPServiceProxy *proxy,
    GUPnPSimpleIgd *self)
{
  struct Proxy *prox;
  guint i;

  if (self->priv->no_new_mappings)
    return;

  prox = g_slice_new0 (struct Proxy);

  prox->parent = self;
  prox->cp = cp;
  prox->proxy = proxy;
  prox->proxymappings = g_ptr_array_new ();

  gupnp_simple_igd_gather (self, prox);

  for (i = 0; i < self->priv->mappings->len; i++)
    gupnp_simple_igd_add_proxy_mapping (self, prox,
        g_ptr_array_index (self->priv->mappings, i));

  g_ptr_array_add(self->priv->service_proxies, prox);
}


static void
_cp_service_unavail (GUPnPControlPoint *cp,
    GUPnPServiceProxy *proxy,
    GUPnPSimpleIgd *self)
{
  guint i;

  for (i=0; i < self->priv->service_proxies->len; i++)
  {
    struct Proxy *prox = g_ptr_array_index (self->priv->service_proxies, i);

    if (prox->cp == cp &&
        !strcmp (gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (proxy)),
            gupnp_service_info_get_udn (GUPNP_SERVICE_INFO (prox->proxy))))
    {
      free_proxy (prox);
      g_ptr_array_remove_index_fast (self->priv->service_proxies, i);
      break;
    }
  }
}

static void
gupnp_simple_igd_add_control_point (GUPnPSimpleIgd *self,
    GUPnPContext *gupnp_context, const char *target)
{
  GUPnPControlPoint *cp;

  cp = gupnp_control_point_new (gupnp_context, target);
  g_return_if_fail (cp);
  g_assert (GUPNP_IS_CONTROL_POINT (cp));
  g_assert (G_IS_OBJECT (self));

  g_signal_connect_object (cp, "service-proxy-available",
      G_CALLBACK (_cp_service_avail), self, 0);
  g_signal_connect_object (cp, "service-proxy-unavailable",
      G_CALLBACK (_cp_service_unavail), self, 0);

  gssdp_resource_browser_set_active (GSSDP_RESOURCE_BROWSER (cp), TRUE);

  gupnp_context_manager_manage_control_point (
      self->priv->gupnp_context_manager, cp);
  g_object_unref (cp);
}

static void
_context_available (GUPnPContextManager *manager, GUPnPContext *gupnp_context,
    GUPnPSimpleIgd *self)
{
  SoupSession *session;
  gboolean ignore_context = FALSE;

  g_signal_emit (self, signals[SIGNAL_CONTEXT_AVAILABLE], 0, gupnp_context,
      &ignore_context);

  if (ignore_context)
    return;

  session = gupnp_context_get_session (gupnp_context);
  g_object_set (session, "timeout", SOUP_REQUEST_TIMEOUT, NULL);

  gupnp_simple_igd_add_control_point (self, gupnp_context,
      "urn:schemas-upnp-org:service:WANIPConnection:1");
  gupnp_simple_igd_add_control_point (self, gupnp_context,
      "urn:schemas-upnp-org:service:WANPPPConnection:1");
}


static void
gupnp_simple_igd_constructed (GObject *object)
{
  GUPnPSimpleIgd *self = GUPNP_SIMPLE_IGD_CAST (object);

  self->priv->main_context = g_main_context_get_thread_default ();
  if (!self->priv->main_context)
    self->priv->main_context = g_main_context_default ();
  g_main_context_ref (self->priv->main_context);

  self->priv->gupnp_context_manager = gupnp_context_manager_create (0);

  g_signal_connect_object (self->priv->gupnp_context_manager,
      "context-available", G_CALLBACK (_context_available), self, 0);

  if (G_OBJECT_CLASS (gupnp_simple_igd_parent_class)->constructed)
    G_OBJECT_CLASS (gupnp_simple_igd_parent_class)->constructed (object);
}

/**
 * gupnp_simple_igd_new:
 * main context)
 *
 * This creates a new #GUPnpSimpleIgd object using the special GMainContext
 *
 * Returns: a new #GUPnPSimpleIgd
 */

GUPnPSimpleIgd *
gupnp_simple_igd_new (void)
{
  return g_object_new (GUPNP_TYPE_SIMPLE_IGD, NULL);
}


static void
_service_proxy_got_external_ip_address (GObject *source_object,
    GAsyncResult *res, gpointer user_data)
{
  GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (source_object);
  struct Proxy *prox = user_data;
  GUPnPSimpleIgd *self = prox->parent;
  GUPnPServiceProxyAction *action;
  GError *error = NULL;
  gchar *ip = NULL;
  guint i;

  action = gupnp_service_proxy_call_action_finish (proxy, res, &error);

  if (action == NULL &&
      g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;

  g_clear_object (&prox->external_ip_cancellable);

  if (action == NULL)
    goto error;

  if (!gupnp_service_proxy_action_get_result (action, &error,
          "NewExternalIPAddress", G_TYPE_STRING, &ip, NULL)) {
    gupnp_service_proxy_action_unref (action);
    goto error;
  }
  gupnp_service_proxy_action_unref (action);

  if (!g_hostname_is_ip_address (ip))
  {
    prox->external_ip_failed = TRUE;

    for (i=0; i < prox->proxymappings->len; i++)
    {
      struct ProxyMapping *pm = g_ptr_array_index (prox->proxymappings, i);
      GError gerror = {GUPNP_SIMPLE_IGD_ERROR,
                       GUPNP_SIMPLE_IGD_ERROR_EXTERNAL_ADDRESS,
                       "Invalid IP address returned by router"};

      g_signal_emit (self, signals[SIGNAL_ERROR_MAPPING_PORT],
          GUPNP_SIMPLE_IGD_ERROR, &gerror, pm->mapping->protocol,
          pm->mapping->requested_external_port, pm->mapping->local_ip,
          pm->mapping->local_port, pm->mapping->description);
    }
    return;
  }

  /* Only emit the new signal if the IP changes */
  if (prox->external_ip &&
      strcmp (ip, prox->external_ip))
  {
    for (i=0; i < prox->proxymappings->len; i++)
    {
      struct ProxyMapping *pm = g_ptr_array_index (prox->proxymappings, i);

      if (pm->mapped)
        g_signal_emit (self, signals[SIGNAL_MAPPED_EXTERNAL_PORT], 0,
            pm->mapping->protocol, ip, prox->external_ip,
            pm->actual_external_port, pm->mapping->local_ip,
            pm->mapping->local_port, pm->mapping->description);
    }
  }

  g_free (prox->external_ip);
  prox->external_ip = ip;

  return;

error:
{
    guint i;

    prox->external_ip_failed = TRUE;
    g_return_if_fail (error);

    for (i=0; i < prox->proxymappings->len; i++)
    {
      struct ProxyMapping *pm = g_ptr_array_index (prox->proxymappings, i);

      g_signal_emit (self, signals[SIGNAL_ERROR_MAPPING_PORT], error->domain,
          error, pm->mapping->protocol, pm->mapping->requested_external_port,
          pm->mapping->local_ip, pm->mapping->local_port,
          pm->mapping->description);
    }
  }
  g_clear_error (&error);
}

static void
gupnp_simple_igd_gather (GUPnPSimpleIgd *self,
    struct Proxy *prox)
{
  GUPnPServiceProxyAction *action;

  g_assert (prox->external_ip_cancellable == NULL);
  prox->external_ip_cancellable = g_cancellable_new ();

  action = gupnp_service_proxy_action_new (
      "GetExternalIPAddress", NULL);

  gupnp_service_proxy_call_action_async (prox->proxy, action,
      prox->external_ip_cancellable,
      _service_proxy_got_external_ip_address, prox);

  gupnp_service_proxy_add_notify (prox->proxy, "ExternalIPAddress",
      G_TYPE_STRING, _external_ip_address_changed, prox);

  gupnp_service_proxy_set_subscribed (prox->proxy, TRUE);
}

static void
_service_proxy_renewed_port_mapping (GObject *source_object, GAsyncResult *res,
    gpointer user_data)
{
  GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (source_object);
  GUPnPServiceProxyAction *action;
  struct ProxyMapping *pm = user_data;
  GUPnPSimpleIgd *self = pm->proxy->parent;
  GError *error = NULL;

  action = gupnp_service_proxy_call_action_finish (proxy, res, &error);

  if (action == NULL &&
      g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;

  g_clear_object (&pm->cancellable);

  if (action) {
    if (gupnp_service_proxy_action_get_result (action, &error, NULL)) {
      gupnp_service_proxy_action_unref (action);
      return;
    }
    gupnp_service_proxy_action_unref (action);
  }

  g_return_if_fail (error);
  g_signal_emit (self, signals[SIGNAL_ERROR_MAPPING_PORT], error->domain,
      error, pm->mapping->protocol, pm->mapping->requested_external_port,
      pm->mapping->local_ip, pm->mapping->local_port,
      pm->mapping->description);
  g_clear_error (&error);
}

static void
gupnp_simple_igd_call_add_port_mapping (struct ProxyMapping *pm,
    GAsyncReadyCallback callback)
{
  GUPnPServiceProxyAction *action;
  g_assert (pm);
  g_return_if_fail (pm->cancellable == NULL);
  g_assert (pm->proxy);
  g_assert (pm->mapping);

  pm->cancellable = g_cancellable_new ();

  action = gupnp_service_proxy_action_new ("AddPortMapping",
      "NewRemoteHost", G_TYPE_STRING, "",
      "NewExternalPort", G_TYPE_UINT, pm->actual_external_port,
      "NewProtocol", G_TYPE_STRING, pm->mapping->protocol,
      "NewInternalPort", G_TYPE_UINT, pm->mapping->local_port,
      "NewInternalClient", G_TYPE_STRING, pm->mapping->local_ip,
      "NewEnabled", G_TYPE_BOOLEAN, TRUE,
      "NewPortMappingDescription", G_TYPE_STRING, pm->mapping->description,
      "NewLeaseDuration", G_TYPE_UINT, pm->mapping->lease_duration,
      NULL);

  gupnp_service_proxy_call_action_async (pm->proxy->proxy, action,
      pm->cancellable, callback, pm);
}

static gboolean
_renew_mapping_timeout (gpointer user_data)
{
  struct ProxyMapping *pm = user_data;

  stop_proxymapping (pm, FALSE);

  gupnp_simple_igd_call_add_port_mapping (pm,
      _service_proxy_renewed_port_mapping);

  return TRUE;
}

static void
_service_proxy_added_port_mapping (GObject *source_object, GAsyncResult *res,
    gpointer user_data)
{
  GUPnPServiceProxy *proxy = GUPNP_SERVICE_PROXY (source_object);
  GUPnPServiceProxyAction *action;
  struct ProxyMapping *pm = user_data;
  GUPnPSimpleIgd *self;
  GError *error = NULL;

  /* This is a hack, but wek now that "res" is actually implemented as a GTask,
   * we're just too lazy to carry our own reference counted structure
   */
  if (g_cancellable_is_cancelled (g_task_get_cancellable (G_TASK (res))))
    return;

  action = gupnp_service_proxy_call_action_finish (proxy, res, &error);

  if (action == NULL &&
      g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return;

  self = pm->proxy->parent;
  g_clear_object (&pm->cancellable);

  if (action == NULL)
    goto error;

  if (!gupnp_service_proxy_action_get_result (action, &error, NULL)) {
    gupnp_service_proxy_action_unref (action);
    goto error;
  }

  gupnp_service_proxy_action_unref (action);

  pm->mapped = TRUE;

  if (pm->proxy->external_ip)
    g_signal_emit (self, signals[SIGNAL_MAPPED_EXTERNAL_PORT], 0,
        pm->mapping->protocol, pm->proxy->external_ip, NULL,
        pm->actual_external_port, pm->mapping->local_ip,
        pm->mapping->local_port, pm->mapping->description);

  if (pm->mapping->lease_duration > 0)
  {
    pm->renew_src =
      g_timeout_source_new_seconds (pm->mapping->lease_duration / 2);
    g_source_set_callback (pm->renew_src,
        _renew_mapping_timeout, pm, NULL);
    g_source_attach (pm->renew_src, self->priv->main_context);
  }

  return;

 error:
  {
    g_return_if_fail (error);

    /* 718 == ConflictInMappingEntry */
    if (pm->mapping->requested_external_port == 0 &&
        error->domain == GUPNP_CONTROL_ERROR && error->code == 718)
    {
      /* The previous port was already used, lets pick another random port */
      pm->actual_external_port = g_random_int_range (1025, 65535);

      gupnp_simple_igd_call_add_port_mapping (pm,
          _service_proxy_added_port_mapping);
    }
    else
    {
      g_signal_emit (self, signals[SIGNAL_ERROR_MAPPING_PORT], error->domain,
          error, pm->mapping->protocol, pm->mapping->requested_external_port,
          pm->mapping->local_ip, pm->mapping->local_port,
          pm->mapping->description);
    }
  }
  g_clear_error (&error);
}

static void
gupnp_simple_igd_add_proxy_mapping (GUPnPSimpleIgd *self, struct Proxy *prox,
    struct Mapping *mapping)
{
  struct ProxyMapping *pm = g_slice_new0 (struct ProxyMapping);

  pm->proxy = prox;
  pm->mapping = mapping;

  if (mapping->requested_external_port)
    pm->actual_external_port = mapping->requested_external_port;
  else
    pm->actual_external_port = mapping->local_port;

  gupnp_simple_igd_call_add_port_mapping (pm,
      _service_proxy_added_port_mapping);

  g_ptr_array_add (prox->proxymappings, pm);
}

static void
gupnp_simple_igd_add_port_real (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint16 external_port,
    const gchar *local_ip,
    guint16 local_port,
    guint32 lease_duration,
    const gchar *description)
{
  struct Mapping *mapping = g_slice_new0 (struct Mapping);
  guint i;

  mapping->protocol = g_strdup (protocol);
  mapping->requested_external_port = external_port;
  mapping->local_ip = g_strdup (local_ip);
  mapping->local_port = local_port;
  mapping->lease_duration = lease_duration;
  mapping->description = g_strdup (description);

  if (!mapping->description)
    mapping->description = g_strdup ("");

  g_ptr_array_add (self->priv->mappings, mapping);

  for (i=0; i < self->priv->service_proxies->len; i++)
  {
    struct Proxy *prox = g_ptr_array_index (self->priv->service_proxies, i);

    if (prox->external_ip_failed)
    {
      GError error = {GUPNP_SIMPLE_IGD_ERROR,
                      GUPNP_SIMPLE_IGD_ERROR_EXTERNAL_ADDRESS,
                      "Could not get external address"};
      g_signal_emit (self, signals[SIGNAL_ERROR_MAPPING_PORT],
          GUPNP_SIMPLE_IGD_ERROR,
          &error, mapping->protocol, mapping->requested_external_port,
          mapping->local_ip, mapping->local_port,
          mapping->description);
    }
    else
    {
      gupnp_simple_igd_add_proxy_mapping (self, prox, mapping);
    }
  }
}

/**
 * gupnp_simple_igd_add_port:
 * @self: The #GUPnPSimpleIgd object
 * @protocol: the protocol "UDP" or "TCP"
 * @external_port: The port to try to open on the external device,
 *   0 means to try a random port if the same port as the local port is already
 *   taken
 * @local_ip: The IP address to forward packets to (most likely the local ip address)
 * @local_port: The local port to forward packets to
 * @lease_duration: The duration of the lease (it will be auto-renewed before it expires). This is in seconds.
 * @description: The description that will appear in the router's table
 *
 * This adds a port to the router's forwarding table. The mapping will
 * be automatically refreshed by this object until it is either
 * removed with gupnp_simple_igd_remove_port(),
 * gupnp_simple_igd_remove_port_local() or the object disapears.
 *
 * If there is a problem, the #GUPnPSimpleIgd::error-mapping-port signal will
 * be emitted. If a router is found and a port is mapped correctly,
 * #GUPnPSimpleIgd::mapped-external-port will be emitted. These signals may
 * be emitted multiple times if there are multiple routers present.
 */

void
gupnp_simple_igd_add_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint16 external_port,
    const gchar *local_ip,
    guint16 local_port,
    guint32 lease_duration,
    const gchar *description)
{
  GUPnPSimpleIgdClass *klass = GUPNP_SIMPLE_IGD_GET_CLASS (self);

  g_return_if_fail (klass->add_port);
  g_return_if_fail (protocol && local_ip);
  g_return_if_fail (local_port > 0);
  g_return_if_fail (!strcmp (protocol, "UDP") || !strcmp (protocol, "TCP"));

  klass->add_port (self, protocol, external_port, local_ip, local_port,
      lease_duration, description);
}

static void
gupnp_simple_igd_remove_port_real (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint external_port)
{
  struct Mapping *mapping = NULL;
  guint i;

  for (i = 0; i < self->priv->mappings->len; i++)
  {
    struct Mapping *tmpmapping = g_ptr_array_index (self->priv->mappings, i);
    if (tmpmapping->requested_external_port == external_port &&
        !strcmp (tmpmapping->protocol, protocol))
    {
      mapping = tmpmapping;
      break;
    }
  }
  if (!mapping)
    return;

  g_ptr_array_remove_index_fast (self->priv->mappings, i);

  free_mapping (self, mapping);
}

/**
 * gupnp_simple_igd_remove_port:
 * @self: The #GUPnPSimpleIgd object
 * @protocol: the protocol "UDP" or "TCP" as given to
 *  gupnp_simple_igd_add_port()
 * @external_port: The port to try to open on the external device as given to
 *  gupnp_simple_igd_add_port()
 *
 * This tries to remove a port entry from the routers that was previously added
 * with gupnp_simple_igd_add_port(). There is no indicated of success or failure
 * it is a best effort mechanism. If it fails, the bindings will disapears after
 * the lease duration set when the port where added.
 */
void
gupnp_simple_igd_remove_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint external_port)
{
  GUPnPSimpleIgdClass *klass = GUPNP_SIMPLE_IGD_GET_CLASS (self);

  g_return_if_fail (protocol);
  g_return_if_fail (external_port <= 65535);

  g_return_if_fail (klass->remove_port);

  klass->remove_port (self, protocol, external_port);
}


static void
gupnp_simple_igd_remove_port_local_real (GUPnPSimpleIgd *self,
    const gchar *protocol,
    const gchar *local_ip,
    guint16 local_port)
{
  struct Mapping *mapping = NULL;
  guint i;

  for (i = 0; i < self->priv->mappings->len; i++)
  {
    struct Mapping *tmpmapping = g_ptr_array_index (self->priv->mappings, i);
    if (tmpmapping->local_port == local_port &&
        !strcmp (tmpmapping->local_ip, local_ip) &&
        !strcmp (tmpmapping->protocol, protocol))
    {
      mapping = tmpmapping;
      break;
    }
  }
  if (!mapping)
    return;

  g_ptr_array_remove_index_fast (self->priv->mappings, i);

  free_mapping (self, mapping);
}

/**
 * gupnp_simple_igd_remove_port_local:
 * @self: The #GUPnPSimpleIgd object
 * @protocol: the protocol "UDP" or "TCP" as given to
 *  gupnp_simple_igd_add_port()
 * @local_ip: The local ip on the internal device as was to
 *  gupnp_simple_igd_add_port()
 * @local_port: The port to try to open on the internal device as given to
 *  gupnp_simple_igd_add_port()
 *
 * This tries to remove a port entry from the routers that was previously added
 * with gupnp_simple_igd_add_port(). There is no indicated of success or failure
 * it is a best effort mechanism. If it fails, the bindings will disapears after
 * the lease duration set when the port where added.
 */
void
gupnp_simple_igd_remove_port_local (GUPnPSimpleIgd *self,
    const gchar *protocol,
    const gchar *local_ip,
    guint16 local_port)
{
  GUPnPSimpleIgdClass *klass = GUPNP_SIMPLE_IGD_GET_CLASS (self);

  g_return_if_fail (protocol != NULL);
  g_return_if_fail (local_ip != NULL);
  g_return_if_fail (local_port != 0);

  g_return_if_fail (klass->remove_port_local);

  klass->remove_port_local (self, protocol, local_ip, local_port);
}

static void
stop_proxymapping (struct ProxyMapping *pm, gboolean stop_renew)
{
  g_cancellable_cancel (pm->cancellable);
  g_clear_object (&pm->cancellable);

  if (stop_renew && pm->renew_src)
  {
    g_source_destroy (pm->renew_src);
    g_source_unref (pm->renew_src);
    pm->renew_src = NULL;
  }
}
