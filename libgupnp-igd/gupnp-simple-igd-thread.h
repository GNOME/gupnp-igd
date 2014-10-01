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

#ifndef __GUPNP_SIMPLE_IGD_THREAD_H__
#define __GUPNP_SIMPLE_IGD_THREAD_H__

#include <libgupnp-igd/gupnp-simple-igd.h>

G_BEGIN_DECLS

/* TYPE MACROS */
#define GUPNP_TYPE_SIMPLE_IGD_THREAD       \
  (gupnp_simple_igd_thread_get_type ())
#define GUPNP_SIMPLE_IGD_THREAD(obj)                               \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GUPNP_TYPE_SIMPLE_IGD_THREAD, \
      GUPnPSimpleIgdThread))
#define GUPNP_SIMPLE_IGD_THREAD_CLASS(klass)                       \
  (G_TYPE_CHECK_CLASS_CAST((klass), GUPNP_TYPE_SIMPLE_IGD_THREAD,  \
      GUPnPSimpleIgdThreadClass))
#define GUPNP_IS_SIMPLE_IGD_THREAD(obj)                            \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GUPNP_TYPE_SIMPLE_IGD_THREAD))
#define GUPNP_IS_SIMPLE_IGD_THREAD_CLASS(klass)                    \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GUPNP_TYPE_SIMPLE_IGD_THREAD))
#define GUPNP_SIMPLE_IGD_THREAD_GET_CLASS(obj)                     \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GUPNP_TYPE_SIMPLE_IGD_THREAD, \
      GUPnPSimpleIgdThreadClass))
#define GUPNP_SIMPLE_IGD_THREAD_CAST(obj)                          \
  ((GUPnPSimpleIgdThread *) (obj))

typedef struct _GUPnPSimpleIgdThread GUPnPSimpleIgdThread;
typedef struct _GUPnPSimpleIgdThreadClass GUPnPSimpleIgdThreadClass;
typedef struct _GUPnPSimpleIgdThreadPrivate GUPnPSimpleIgdThreadPrivate;


/**
 * GUPnPSimpleIgdThread:
 *
 * All members are private, access them using methods and properties
 */
struct _GUPnPSimpleIgdThread
{
  GUPnPSimpleIgd parent;

  /*< private >*/
  GUPnPSimpleIgdThreadPrivate *priv;
};

GType gupnp_simple_igd_thread_get_type (void);

GUPnPSimpleIgdThread *
gupnp_simple_igd_thread_new (void);

G_END_DECLS

#endif /* __GUPNP_SIMPLE_IGD_THREAD_H__ */
