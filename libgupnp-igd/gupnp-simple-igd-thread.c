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
 * SECTION:gupnp-simple-igd-thread
 * @short_description: Threaded wrapper for GUPnPSimpleIgd
 *
 * This wraps a #GUPnPSimpleIgd into a thread so that it can be used without
 * having a #GMainLoop running.
 */


#include "gupnp-simple-igd-thread.h"


struct thread_data
{
  gint refcount;

  GMutex *mutex;

  GMainContext *context;
  GMainLoop *loop;
  gboolean all_mappings_deleted;
};

struct _GUPnPSimpleIgdThreadPrivate
{
  GThread *thread;
  GMainContext *context;

  /* Protected by mutex  inside thread_data*/
  gboolean can_dispose;
  GCond *can_dispose_cond;

  struct thread_data *thread_data;
};


#define GUPNP_SIMPLE_IGD_THREAD_GET_PRIVATE(o)                        \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), GUPNP_TYPE_SIMPLE_IGD_THREAD,    \
   GUPnPSimpleIgdThreadPrivate))

#define GUPNP_SIMPLE_IGD_THREAD_LOCK(o) \
  g_mutex_lock ((o)->priv->thread_data->mutex)
#define GUPNP_SIMPLE_IGD_THREAD_UNLOCK(o) \
  g_mutex_unlock ((o)->priv->thread_data->mutex)


G_DEFINE_TYPE (GUPnPSimpleIgdThread, gupnp_simple_igd_thread,
    GUPNP_TYPE_SIMPLE_IGD);

static void gupnp_simple_igd_thread_constructed (GObject *object);
static void gupnp_simple_igd_thread_dispose (GObject *object);
static void gupnp_simple_igd_thread_finalize (GObject *object);

static void gupnp_simple_igd_thread_add_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint16 external_port,
    const gchar *local_ip,
    guint16 local_port,
    guint32 lease_duration,
    const gchar *description);
static void gupnp_simple_igd_thread_remove_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint external_port);

static void
gupnp_simple_igd_thread_class_init (GUPnPSimpleIgdThreadClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GUPnPSimpleIgdClass *simple_igd_class = GUPNP_SIMPLE_IGD_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GUPnPSimpleIgdThreadPrivate));

  gobject_class->constructed = gupnp_simple_igd_thread_constructed;
  gobject_class->dispose = gupnp_simple_igd_thread_dispose;
  gobject_class->finalize = gupnp_simple_igd_thread_finalize;

  simple_igd_class->add_port = gupnp_simple_igd_thread_add_port;
  simple_igd_class->remove_port = gupnp_simple_igd_thread_remove_port;
}


static void
gupnp_simple_igd_thread_init (GUPnPSimpleIgdThread *self)
{
  self->priv = GUPNP_SIMPLE_IGD_THREAD_GET_PRIVATE (self);

  self->priv->context = g_main_context_new ();
  self->priv->can_dispose_cond = g_cond_new ();

  g_object_set (self, "main-context", self->priv->context, NULL);
}

static gboolean
delete_all_mappings (gpointer user_data)
{
  GUPnPSimpleIgdThread *self = user_data;
  gboolean can_dispose;

  can_dispose = gupnp_simple_igd_delete_all_mappings (GUPNP_SIMPLE_IGD (self));

  GUPNP_SIMPLE_IGD_THREAD_LOCK (self);
  self->priv->can_dispose |= can_dispose;
  self->priv->thread_data->all_mappings_deleted = TRUE;
  GUPNP_SIMPLE_IGD_THREAD_UNLOCK (self);

  g_cond_broadcast (self->priv->can_dispose_cond);

  return FALSE;
}

static void
gupnp_simple_igd_thread_dispose (GObject *object)
{
  GUPnPSimpleIgdThread *self = GUPNP_SIMPLE_IGD_THREAD_CAST (object);

  GUPNP_SIMPLE_IGD_THREAD_LOCK (self);
  if (g_thread_self () == self->priv->thread)
  {
    GUPNP_SIMPLE_IGD_THREAD_UNLOCK (self);

    if (!gupnp_simple_igd_delete_all_mappings (GUPNP_SIMPLE_IGD (self)))
      return;

    GUPNP_SIMPLE_IGD_THREAD_LOCK (self);
    if (self->priv->thread_data->loop)
      g_main_loop_quit (self->priv->thread_data->loop);
    GUPNP_SIMPLE_IGD_THREAD_UNLOCK (self);
  }
  else
  {
    GSource *stop_src;
    GSource *delete_all_src;

    delete_all_src = g_idle_source_new ();
    g_source_set_priority (delete_all_src, G_PRIORITY_HIGH);
    g_source_set_callback (delete_all_src, delete_all_mappings,
        g_object_ref (self),
	g_object_unref);
    g_source_attach (delete_all_src, self->priv->context);
    g_source_unref (delete_all_src);

    while (!self->priv->thread_data->all_mappings_deleted)
      g_cond_wait (self->priv->can_dispose_cond,
          self->priv->thread_data->mutex);

    if (!self->priv->can_dispose)
    {
      GUPNP_SIMPLE_IGD_THREAD_UNLOCK (self);
      return;
    }

    if (self->priv->thread_data->loop)
    {
      g_main_loop_quit (self->priv->thread_data->loop);
    }
    GUPNP_SIMPLE_IGD_THREAD_UNLOCK (self);

    g_thread_join (self->priv->thread);
    self->priv->thread = NULL;
  }

  G_OBJECT_CLASS (gupnp_simple_igd_thread_parent_class)->dispose (object);
}

static void
thread_data_dec (struct thread_data *data)
{
  if (g_atomic_int_dec_and_test (&data->refcount))
  {
    g_mutex_free (data->mutex);
    g_main_context_unref (data->context);
    g_slice_free (struct thread_data, data);
  }
}

static void
gupnp_simple_igd_thread_finalize (GObject *object)
{
  GUPnPSimpleIgdThread *self = GUPNP_SIMPLE_IGD_THREAD_CAST (object);

  g_main_context_unref (self->priv->context);
  g_cond_free (self->priv->can_dispose_cond);

  thread_data_dec (self->priv->thread_data);

  G_OBJECT_CLASS (gupnp_simple_igd_thread_parent_class)->finalize (object);
}

static gpointer
thread_func (gpointer dat)
{
  struct thread_data *data = dat;
  GMainLoop *loop = g_main_loop_new (data->context, FALSE);

  g_mutex_lock (data->mutex);
  data->loop = loop;
  g_mutex_unlock (data->mutex);

  g_main_loop_run (loop);

  g_mutex_lock (data->mutex);
  data->loop = NULL;
  data->all_mappings_deleted = TRUE;
  g_mutex_unlock (data->mutex);

  g_main_loop_unref (loop);

  thread_data_dec (data);

  return NULL;
}

static void
gupnp_simple_igd_thread_constructed (GObject *object)
{
  GUPnPSimpleIgdThread *self = GUPNP_SIMPLE_IGD_THREAD_CAST (object);
  struct thread_data *data = g_slice_new0 (struct thread_data);

  if (G_OBJECT_CLASS (gupnp_simple_igd_thread_parent_class)->constructed)
    G_OBJECT_CLASS (gupnp_simple_igd_thread_parent_class)->constructed (object);

  g_atomic_int_set (&data->refcount, 2);

  self->priv->thread_data = data;

  data->mutex = g_mutex_new ();
  g_main_context_ref (self->priv->context);
  data->context = self->priv->context;

  self->priv->thread = g_thread_create (thread_func, data, TRUE, NULL);
  g_return_if_fail (self->priv->thread);
}

struct AddRemovePortData {
  GUPnPSimpleIgd *self  G_GNUC_MAY_ALIAS;
  gchar *protocol;
  guint16 external_port;
  gchar *local_ip;
  guint16 local_port;
  guint32 lease_duration;
  gchar *description;
};

static gboolean
add_port_idle_func (gpointer user_data)
{
  struct AddRemovePortData *data = user_data;
  GUPnPSimpleIgdClass *klass =
      GUPNP_SIMPLE_IGD_CLASS (gupnp_simple_igd_thread_parent_class);

  if (!data->self)
    return FALSE;

  if (klass->add_port)
    klass->add_port (data->self, data->protocol, data->external_port,
        data->local_ip, data->local_port, data->lease_duration,
        data->description);

  return FALSE;
}


static gboolean
remove_port_idle_func (gpointer user_data)
{
  struct AddRemovePortData *data = user_data;
  GUPnPSimpleIgdClass *klass =
      GUPNP_SIMPLE_IGD_CLASS (gupnp_simple_igd_thread_parent_class);

  if (!data->self)
    return FALSE;

  if (klass->remove_port)
    klass->remove_port (data->self, data->protocol, data->external_port);

  return FALSE;
}

static void
free_add_remove_port_data (gpointer user_data)
{
  struct AddRemovePortData *data = user_data;

  if (data->self)
    g_object_remove_weak_pointer (G_OBJECT (data->self),
        (gpointer*) &data->self);
  g_free (data->protocol);
  g_free (data->local_ip);
  g_free (data->description);

  g_slice_free (struct AddRemovePortData, data);
}

static void
gupnp_simple_igd_thread_add_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint16 external_port,
    const gchar *local_ip,
    guint16 local_port,
    guint32 lease_duration,
    const gchar *description)
{
  GUPnPSimpleIgdThread *realself = GUPNP_SIMPLE_IGD_THREAD (self);
  struct AddRemovePortData *data = g_slice_new0 (struct AddRemovePortData);
  GSource *source;

  data->self = self;
  data->protocol = g_strdup (protocol);
  data->external_port = external_port;
  data->local_ip = g_strdup (local_ip);
  data->local_port = local_port;
  data->lease_duration = lease_duration;
  data->description = g_strdup (description);

  source = g_idle_source_new ();
  g_object_add_weak_pointer (G_OBJECT (self), (gpointer*) &data->self);
  g_source_set_callback (source, add_port_idle_func, data,
      free_add_remove_port_data);
  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_attach (source, realself->priv->context);
  g_source_unref (source);
  g_main_context_wakeup (realself->priv->context);
}

static void
gupnp_simple_igd_thread_remove_port (GUPnPSimpleIgd *self,
    const gchar *protocol,
    guint external_port)
{
  GUPnPSimpleIgdThread *realself = GUPNP_SIMPLE_IGD_THREAD (self);
  struct AddRemovePortData *data = g_slice_new0 (struct AddRemovePortData);
  GSource *source;

  data->self = self;
  data->protocol = g_strdup (protocol);
  data->external_port = external_port;

  source = g_idle_source_new ();
  g_object_add_weak_pointer (G_OBJECT (self), (gpointer*) &data->self);
  g_source_set_callback (source, remove_port_idle_func, data,
      free_add_remove_port_data);
  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_attach (source, realself->priv->context);
  g_source_unref (source);
  g_main_context_wakeup (realself->priv->context);
}

/**
 * gupnp_simple_igd_thread_new:
 *
 * Creates a new #GUPnPSimpleIgdThread
 *
 * Returns: the new #GUPnPSimpleIgdThread
 */

GUPnPSimpleIgdThread *
gupnp_simple_igd_thread_new ()
{
  return g_object_new (GUPNP_TYPE_SIMPLE_IGD_THREAD, NULL);
}
