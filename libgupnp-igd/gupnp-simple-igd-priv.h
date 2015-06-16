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

#ifndef __GUPNP_SIMPLE_IGD_PRIV_H__
#define __GUPNP_SIMPLE_IGD_PRIV_H__

#include "gupnp-simple-igd.h"

/**
 * GUPnPSimpleIgdClass:

 * @add_port: An implementation of the add_port function
 * @remove_port: An implementation of the delete_port function
 * @remove_local_port: An implementation of the remove_local_port function
 *
 * The Raw UDP component transmitter class
 */

struct _GUPnPSimpleIgdClass
{
  GObjectClass parent_class;

  /*virtual functions */

  void (*add_port) (GUPnPSimpleIgd *self,
      const gchar *protocol,
      guint16 external_port,
      const gchar *local_ip,
      guint16 local_port,
      guint32 lease_duration,
      const gchar *description);

  void (*remove_port) (GUPnPSimpleIgd *self,
      const gchar *protocol,
      guint external_port);

  void (*remove_port_local) (GUPnPSimpleIgd *self,
      const gchar *protocol,
      const gchar *local_ip,
      guint16 local_port);

  /*< private >*/
};

#endif /* __GUPNP_SIMPLE_IGD_PRIV_H__ */
