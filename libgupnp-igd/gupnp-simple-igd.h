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

#ifndef __GUPNP_SIMPLE_IGD_H__
#define __GUPNP_SIMPLE_IGD_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* TYPE MACROS */
#define GUPNP_TYPE_SIMPLE_IGD       \
  (gupnp_simple_igd_get_type ())
#define GUPNP_SIMPLE_IGD(obj)                               \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GUPNP_TYPE_SIMPLE_IGD, \
      GUPnPSimpleIgd))
#define GUPNP_SIMPLE_IGD_CLASS(klass)                       \
  (G_TYPE_CHECK_CLASS_CAST((klass), GUPNP_TYPE_SIMPLE_IGD,  \
      GUPnPSimpleIgdClass))
#define GUPNP_IS_SIMPLE_IGD(obj)                            \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GUPNP_TYPE_SIMPLE_IGD))
#define GUPNP_IS_SIMPLE_IGD_CLASS(klass)                    \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GUPNP_TYPE_SIMPLE_IGD))
#define GUPNP_SIMPLE_IGD_GET_CLASS(obj)                     \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GUPNP_TYPE_SIMPLE_IGD, \
      GUPnPSimpleIgdClass))
#define GUPNP_SIMPLE_IGD_CAST(obj)                          \
  ((GUPnPSimpleIgd *) (obj))

typedef struct _GUPnPSimpleIgd GUPnPSimpleIgd;
typedef struct _GUPnPSimpleIgdClass GUPnPSimpleIgdClass;
typedef struct _GUPnPSimpleIgdPrivate GUPnPSimpleIgdPrivate;


/**
 * GUPnPSimpleIgd:
 *
 * All members are private, access them using methods and properties
 */
struct _GUPnPSimpleIgd
{
  GObject parent;

  /*< private >*/
  GUPnPSimpleIgdPrivate *priv;
};

/**
 * GUPNP_SIMPLE_IGD_ERROR:
 *
 * The error domain for GUPnP Simple IGD
 */

#define GUPNP_SIMPLE_IGD_ERROR (gupnp_simple_igd_error_quark ())

/**
 * GUPnPSimpleIgdError:
 * @GUPNP_SIMPLE_IGD_ERROR_EXTERNAL_ADDRESS: Error getting the external
 * address of the router
 *
 * Errors coming out of the GUPnPSimpleIGD object.
 */

typedef enum {
  GUPNP_SIMPLE_IGD_ERROR_EXTERNAL_ADDRESS,
} GUPnPSimpleIgdError;

GQuark gupnp_simple_igd_error_quark (void);

GType gupnp_simple_igd_get_type (void);

GUPnPSimpleIgd *
gupnp_simple_igd_new (void);

void
gupnp_simple_igd_add_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint16 external_port,
    const gchar *local_ip,
    guint16 local_port,
    guint32 lease_duration,
    const gchar *description);

void
gupnp_simple_igd_remove_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint external_port);

void
gupnp_simple_igd_remove_port_local (GUPnPSimpleIgd *self,
    const gchar *protocol,
    const gchar *local_ip,
    guint16 local_port);


gboolean
gupnp_simple_igd_delete_all_mappings (GUPnPSimpleIgd *self);


G_END_DECLS

#endif /* __GUPNP_SIMPLE_IGD_H__ */
