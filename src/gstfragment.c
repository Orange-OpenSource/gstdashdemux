/* GStreamer
 * Copyright (C) 2011 Andoni Morales Alastruey <ylatuya@gmail.com>
 *
 * gstfragment.c:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <gst/base/gsttypefindhelper.h>
#include "glibcompat.h"
#include "gstfragmented.h"
#include "gstfragment.h"

#define GST_CAT_DEFAULT fragmented_debug

#define GST_FRAGMENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_FRAGMENT, GstFragmentPrivate))

enum
{
  PROP_0,
  PROP_INDEX,
  PROP_NAME,
  PROP_DURATION,
  PROP_DISCONTINOUS,
  PROP_BUFFER_LIST,
  PROP_CAPS,
  PROP_LAST
};

struct _GstFragmentPrivate
{
  GstBufferList *buffer_list;
  guint64 size;
  GstBufferListIterator *buffer_iterator;
  GstCaps *caps;
  G_MUTEX lock;
};

G_DEFINE_TYPE (GstFragment, gst_fragment, G_TYPE_OBJECT);

static void gst_fragment_dispose (GObject * object);
static void gst_fragment_finalize (GObject * object);

static void
gst_fragment_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstFragment *fragment = GST_FRAGMENT (object);

  switch (property_id) {
    case PROP_CAPS:
      gst_fragment_set_caps (fragment, g_value_get_boxed (value));
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_fragment_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec)
{
  GstFragment *fragment = GST_FRAGMENT (object);

  switch (property_id) {
    case PROP_INDEX:
      g_value_set_uint (value, fragment->index);
      break;

    case PROP_NAME:
      g_value_set_string (value, fragment->name);
      break;

    case PROP_DURATION:
      g_value_set_uint64 (value, fragment->stop_time - fragment->start_time);
      break;

    case PROP_DISCONTINOUS:
      g_value_set_boolean (value, fragment->discontinuous);
      break;

    case PROP_BUFFER_LIST:
      g_value_set_object (value, gst_fragment_get_buffer_list (fragment));
      break;

    case PROP_CAPS:
      g_value_set_boxed (value, gst_fragment_get_caps (fragment));
      break;

    default:
      /* We don't have any other property... */
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}



static void
gst_fragment_class_init (GstFragmentClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GstFragmentPrivate));

  gobject_class->set_property = gst_fragment_set_property;
  gobject_class->get_property = gst_fragment_get_property;
  gobject_class->dispose = gst_fragment_dispose;
  gobject_class->finalize = gst_fragment_finalize;

  g_object_class_install_property (gobject_class, PROP_INDEX,
      g_param_spec_uint ("index", "Index", "Index of the fragment", 0,
          G_MAXUINT, 0, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_NAME,
      g_param_spec_string ("name", "Name",
          "Name of the fragment (eg:fragment-12.ts)", NULL, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_DISCONTINOUS,
      g_param_spec_boolean ("discontinuous", "Discontinous",
          "Whether this fragment has a discontinuity or not",
          FALSE, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_DURATION,
      g_param_spec_uint64 ("duration", "Fragment duration",
          "Duration of the fragment", 0, G_MAXUINT64, 0, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_BUFFER_LIST,
      g_param_spec_object ("buffer-list", "Buffer List",
          "A list with the fragment's buffers", GST_TYPE_FRAGMENT,
          G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_CAPS,
      g_param_spec_boxed ("caps", "Fragment caps",
          "The caps of the fragment's buffer. (NULL = detect)", GST_TYPE_CAPS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_fragment_init (GstFragment * fragment)
{
  GstFragmentPrivate *priv;

  fragment->priv = priv = GST_FRAGMENT_GET_PRIVATE (fragment);

  G_MUTEX_INIT (fragment->priv->lock);
  priv->buffer_list = gst_buffer_list_new ();
  priv->size = 0;
  priv->buffer_iterator = gst_buffer_list_iterate (priv->buffer_list);
  gst_buffer_list_iterator_add_group (priv->buffer_iterator);
  fragment->download_start_time = g_get_real_time ();
  fragment->start_time = 0;
  fragment->stop_time = 0;
  fragment->index = 0;
  fragment->name = g_strdup ("");
  fragment->completed = FALSE;
  fragment->discontinuous = FALSE;
}

GstFragment *
gst_fragment_new (void)
{
  return GST_FRAGMENT (g_object_new (GST_TYPE_FRAGMENT, NULL));
}

static void
gst_fragment_finalize (GObject * gobject)
{
  GstFragment *fragment = GST_FRAGMENT (gobject);

  g_free (fragment->name);
  G_MUTEX_CLEAR (fragment->priv->lock);

  G_OBJECT_CLASS (gst_fragment_parent_class)->finalize (gobject);
}

void
gst_fragment_dispose (GObject * object)
{
  GstFragmentPrivate *priv = GST_FRAGMENT (object)->priv;

  if (priv->buffer_list != NULL) {
    gst_buffer_list_iterator_free (priv->buffer_iterator);
    gst_buffer_list_unref (priv->buffer_list);
    priv->buffer_list = NULL;
    priv->size = 0;
  }

  if (priv->caps != NULL) {
    gst_caps_unref (priv->caps);
    priv->caps = NULL;
  }

  G_OBJECT_CLASS (gst_fragment_parent_class)->dispose (object);
}

GstBufferList *
gst_fragment_get_buffer_list (GstFragment * fragment)
{
  g_return_val_if_fail (fragment != NULL, NULL);

  if (!fragment->completed)
    return NULL;

  gst_buffer_list_ref (fragment->priv->buffer_list);
  return fragment->priv->buffer_list;
}

void
gst_fragment_set_caps (GstFragment * fragment, GstCaps * caps)
{
  g_return_if_fail (fragment != NULL);

  G_MUTEX_LOCK (fragment->priv->lock);
  gst_caps_replace (&fragment->priv->caps, caps);
  G_MUTEX_UNLOCK (fragment->priv->lock);
}

GstCaps *
gst_fragment_get_caps (GstFragment * fragment)
{
  g_return_val_if_fail (fragment != NULL, NULL);

  if (!fragment->completed)
    return NULL;

  G_MUTEX_LOCK (fragment->priv->lock);
  if (fragment->priv->caps == NULL) {
    GstBuffer *buf = gst_buffer_list_get (fragment->priv->buffer_list, 0, 0);
    fragment->priv->caps = gst_type_find_helper_for_buffer (NULL, buf, NULL);
  }
  gst_caps_ref (fragment->priv->caps);
  G_MUTEX_UNLOCK (fragment->priv->lock);

  return fragment->priv->caps;
}

guint64
gst_fragment_get_buffer_size (GstFragment * fragment)
{
  g_return_val_if_fail (fragment != NULL, 0);

  if (!fragment->completed)
    return 0;
  return fragment->priv->size;
}



gboolean
gst_fragment_add_buffer (GstFragment * fragment, GstBuffer * buffer)
{
  g_return_val_if_fail (fragment != NULL, FALSE);
  g_return_val_if_fail (buffer != NULL, FALSE);

  if (fragment->completed) {
    GST_WARNING ("Fragment is completed, could not add more buffers");
    return FALSE;
  }

  gst_buffer_list_iterator_add (fragment->priv->buffer_iterator, buffer);
  fragment->priv->size = fragment->priv->size + GST_BUFFER_SIZE (buffer);
  return TRUE;
}
