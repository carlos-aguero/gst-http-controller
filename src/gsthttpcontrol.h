/* GStreamer
 * Copyright (C) 2015 FIXME <fixme@example.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_HTTPCONTROL_H_
#define _GST_HTTPCONTROL_H_

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_HTTPCONTROL   (gst_my_element_get_type())
#define GST_HTTPCONTROL(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_HTTPCONTROL,GstHttpControl))
#define GST_HTTPCONTROL_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_HTTPCONTROL,GstHttpControlClass))
#define GST_IS_HTTPCONTROL(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_HTTPCONTROL))
#define GST_IS_HTTPCONTROL_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_HTTPCONTROL))

typedef struct _GstHttpControl GstHttpControl;
typedef struct _GstHttpControlClass GstHttpControlClass;

struct _GstHttpControl
{
  GstBaseTransform base_myelement;

   /* Updates task */
  GstTask *updates_task;
  GRecMutex updates_lock;
  GMutex updates_timed_lock;
  GTimeVal next_update;         /* Time of the next update */
  gboolean cancelled;
  GstElement *iterated_elements[1024];
  gint iterated_elements_index;

  gboolean parent_info;
  GstIterator *it, *it_pads;
  GValue elem;
  gint amount_of_elements;
  char response[1024];
};

struct _GstHttpControlClass
{
  GstBaseTransformClass base_myelement_class;
};

GType gst_my_element_get_type (void);

G_END_DECLS

#endif
