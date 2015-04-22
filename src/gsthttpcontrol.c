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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gst-http-controller
 *
 * Web browser based GStreamer universal pipeline controller
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * GST_DEBUG=httpcontrol:3 GST_PLUGIN_PATH=src/.libs/ gst-launch-1.0 videotestsrc ! http-control ! fakesink  silent=true -v
 * ]|
 * Previous pipeline can be used in order to test http-control basic functionality,
 * run the pipeline and open a web browser with system IP address, pipeline elements
 * should appear at web browser.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gsthttpcontrol.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>

GST_DEBUG_CATEGORY_STATIC (gst_my_element_debug_category);
#define GST_CAT_DEFAULT gst_my_element_debug_category

/* prototypes */


static void gst_my_element_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_my_element_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_my_element_dispose (GObject * object);
static void gst_my_element_finalize (GObject * object);

static GstCaps *gst_my_element_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter);
static GstCaps *gst_my_element_fixate_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * othercaps);
static gboolean gst_my_element_accept_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps);
static gboolean gst_my_element_set_caps (GstBaseTransform * trans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_my_element_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * query);
static gboolean gst_my_element_decide_allocation (GstBaseTransform * trans,
    GstQuery * query);
static gboolean gst_my_element_filter_meta (GstBaseTransform * trans,
    GstQuery * query, GType api, const GstStructure * params);
static gboolean gst_my_element_propose_allocation (GstBaseTransform * trans,
    GstQuery * decide_query, GstQuery * query);
static gboolean gst_my_element_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize);
static gboolean gst_my_element_get_unit_size (GstBaseTransform * trans,
    GstCaps * caps, gsize * size);
static gboolean gst_my_element_start (GstBaseTransform * trans);
static gboolean gst_my_element_stop (GstBaseTransform * trans);
static gboolean gst_my_element_sink_event (GstBaseTransform * trans,
    GstEvent * event);
static gboolean gst_my_element_src_event (GstBaseTransform * trans,
    GstEvent * event);
static GstFlowReturn gst_my_element_prepare_output_buffer (GstBaseTransform *
    trans, GstBuffer * input, GstBuffer ** outbuf);
static gboolean gst_my_element_copy_metadata (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer * outbuf);
static gboolean gst_my_element_transform_meta (GstBaseTransform * trans,
    GstBuffer * outbuf, GstMeta * meta, GstBuffer * inbuf);
static void gst_my_element_before_transform (GstBaseTransform * trans,
    GstBuffer * buffer);
static GstFlowReturn gst_my_element_transform (GstBaseTransform * trans,
    GstBuffer * inbuf, GstBuffer * outbuf);
static GstFlowReturn gst_my_element_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);
void gst_hls_demux_updates_loop (GstHttpControl * httpcontrol);


enum
{
  PROP_0
};

/* pad templates */

static GstStaticPadTemplate gst_my_element_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate gst_my_element_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstHttpControl, gst_my_element, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_my_element_debug_category, "httpcontrol", 0,
        "debug category for httpcontrol element"));

static void
gst_my_element_class_init (GstHttpControlClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_my_element_src_template));
  gst_element_class_add_pad_template (GST_ELEMENT_CLASS (klass),
      gst_static_pad_template_get (&gst_my_element_sink_template));

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_my_element_set_property;
  gobject_class->get_property = gst_my_element_get_property;
  gobject_class->dispose = gst_my_element_dispose;
  gobject_class->finalize = gst_my_element_finalize;
  base_transform_class->transform_caps =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_caps);
/*  base_transform_class->fixate_caps =
      GST_DEBUG_FUNCPTR (gst_my_element_fixate_caps);
  base_transform_class->accept_caps =
      GST_DEBUG_FUNCPTR (gst_my_element_accept_caps);
  base_transform_class->set_caps = GST_DEBUG_FUNCPTR (gst_my_element_set_caps);
  base_transform_class->query = GST_DEBUG_FUNCPTR (gst_my_element_query); */
/*  base_transform_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_my_element_decide_allocation);
  base_transform_class->filter_meta =
      GST_DEBUG_FUNCPTR (gst_my_element_filter_meta);
  base_transform_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_my_element_propose_allocation);
  base_transform_class->transform_size =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_size);
  base_transform_class->get_unit_size =
      GST_DEBUG_FUNCPTR (gst_my_element_get_unit_size);
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_my_element_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_my_element_stop);
  base_transform_class->sink_event =
      GST_DEBUG_FUNCPTR (gst_my_element_sink_event);
  base_transform_class->src_event =
      GST_DEBUG_FUNCPTR (gst_my_element_src_event);
  base_transform_class->prepare_output_buffer =
      GST_DEBUG_FUNCPTR (gst_my_element_prepare_output_buffer);
  base_transform_class->copy_metadata =
      GST_DEBUG_FUNCPTR (gst_my_element_copy_metadata);
  base_transform_class->transform_meta =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_meta);*/
/*  base_transform_class->before_transform =
      GST_DEBUG_FUNCPTR (gst_my_element_before_transform);*/
/*  base_transform_class->transform =
      GST_DEBUG_FUNCPTR (gst_my_element_transform);*/
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_my_element_transform_ip);

}

void gst_http_server_loop(GstHttpControl * httpcontrol)
{
  GST_INFO_OBJECT (httpcontrol, "INIT Web Server");

  while (1) {

    int one = 1, client_fd;
    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);

    GST_INFO_OBJECT (httpcontrol, "Element Init"); 
 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      GST_ERROR_OBJECT (httpcontrol, "Can't open socket");
    }
 
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
 
    int port = 8080;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);
 
    if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
      close(sock);
      GST_ERROR_OBJECT (httpcontrol, "can't bind");
    }
 
    listen(sock, 5);
    while (1) {
      client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
      GST_INFO_OBJECT (httpcontrol, "Got Connection");
   
      if (client_fd == -1) {
        GST_ERROR_OBJECT (httpcontrol, "Can't Accept");
        continue;
      }

      GST_INFO_OBJECT (httpcontrol, "sizeof response: %i", sizeof(httpcontrol->response));
      write(client_fd, httpcontrol->response, sizeof(httpcontrol->response) - 1); 
      close(client_fd);
    }
  }
}

static void
gst_my_element_init (GstHttpControl * httpcontrol)
{

  /* http server init */
  g_rec_mutex_init (&httpcontrol->updates_lock);
  httpcontrol->updates_task =
      gst_task_new ((GstTaskFunction) gst_http_server_loop, httpcontrol, NULL);
  gst_task_set_lock (httpcontrol->updates_task, &httpcontrol->updates_lock);
  g_mutex_init (&httpcontrol->updates_timed_lock);
  gst_task_start (httpcontrol->updates_task);

  httpcontrol->parent_info = FALSE;
  httpcontrol->iterated_elements_index = 0;

}

void
gst_my_element_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (object);

  GST_DEBUG_OBJECT (httpcontrol, "set_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_my_element_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (object);

  GST_DEBUG_OBJECT (httpcontrol, "get_property");

  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_my_element_dispose (GObject * object)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (object);

  GST_DEBUG_OBJECT (httpcontrol, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_my_element_parent_class)->dispose (object);
}

void
gst_my_element_finalize (GObject * object)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (object);

  GST_DEBUG_OBJECT (httpcontrol, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_my_element_parent_class)->finalize (object);
}

static GstCaps *
gst_my_element_transform_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, GstCaps * filter)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);
  GstCaps *othercaps;

  GST_DEBUG_OBJECT (httpcontrol, "transform_caps");

  othercaps = gst_caps_copy (caps);

  /* Copy other caps and modify as appropriate */
  /* This works for the simplest cases, where the transform modifies one
   * or more fields in the caps structure.  It does not work correctly
   * if passthrough caps are preferred. */
  if (direction == GST_PAD_SRC) {
    /* transform caps going upstream */
  } else {
    /* transform caps going downstream */
  }

  if (filter) {
    GstCaps *intersect;

    intersect = gst_caps_intersect (othercaps, filter);
    gst_caps_unref (othercaps);

    return intersect;
  } else {
    return othercaps;
  }
}

static GstCaps *
gst_my_element_fixate_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps, GstCaps * othercaps)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "fixate_caps");

  return NULL;
}

static gboolean
gst_my_element_accept_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "accept_caps");

  return TRUE;
}

static gboolean
gst_my_element_set_caps (GstBaseTransform * trans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "set_caps");

  return TRUE;
}

static gboolean
gst_my_element_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "query");

  return TRUE;
}

/* decide allocation query for output buffers */
static gboolean
gst_my_element_decide_allocation (GstBaseTransform * trans, GstQuery * query)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "decide_allocation");

  return TRUE;
}

static gboolean
gst_my_element_filter_meta (GstBaseTransform * trans, GstQuery * query,
    GType api, const GstStructure * params)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "filter_meta");

  return TRUE;
}

/* propose allocation query parameters for input buffers */
static gboolean
gst_my_element_propose_allocation (GstBaseTransform * trans,
    GstQuery * decide_query, GstQuery * query)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "propose_allocation");

  return TRUE;
}

/* transform size */
static gboolean
gst_my_element_transform_size (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps, gsize size, GstCaps * othercaps,
    gsize * othersize)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "transform_size");

  return TRUE;
}

static gboolean
gst_my_element_get_unit_size (GstBaseTransform * trans, GstCaps * caps,
    gsize * size)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "get_unit_size");

  return TRUE;
}

/* states */
static gboolean
gst_my_element_start (GstBaseTransform * trans)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "start");

  return TRUE;
}

static gboolean
gst_my_element_stop (GstBaseTransform * trans)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "stop");

  return TRUE;
}

/* sink and src pad event handlers */
static gboolean
gst_my_element_sink_event (GstBaseTransform * trans, GstEvent * event)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "sink_event");

  return GST_BASE_TRANSFORM_CLASS (gst_my_element_parent_class)->
      sink_event (trans, event);
}

static gboolean
gst_my_element_src_event (GstBaseTransform * trans, GstEvent * event)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "src_event");

  return GST_BASE_TRANSFORM_CLASS (gst_my_element_parent_class)->
      src_event (trans, event);
}

static GstFlowReturn
gst_my_element_prepare_output_buffer (GstBaseTransform * trans,
    GstBuffer * input, GstBuffer ** outbuf)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "prepare_output_buffer");

  return GST_FLOW_OK;
}

/* metadata */
static gboolean
gst_my_element_copy_metadata (GstBaseTransform * trans, GstBuffer * input,
    GstBuffer * outbuf)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "copy_metadata");

  return TRUE;
}

static gboolean
gst_my_element_transform_meta (GstBaseTransform * trans, GstBuffer * outbuf,
    GstMeta * meta, GstBuffer * inbuf)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "transform_meta");

  return TRUE;
}

static void
gst_my_element_before_transform (GstBaseTransform * trans, GstBuffer * buffer)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "before_transform");

}

/* transform */
static GstFlowReturn
gst_my_element_transform (GstBaseTransform * trans, GstBuffer * inbuf,
    GstBuffer * outbuf)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);

  GST_DEBUG_OBJECT (httpcontrol, "transform");

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_my_element_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstHttpControl *httpcontrol = GST_HTTPCONTROL (trans);


  if (!httpcontrol->parent_info)
  {
    GST_DEBUG_OBJECT (httpcontrol, "transform_ip");

    char a[2048] = "HTTP/1.1 200 OK\r\n \
Content-Type: text/html; charset=UTF-8\r\n\r\n \
<!DOCTYPE html><html><head><title>Pipeline Structure</title> \
<style>body { background-color: #111 } \
h1 { font-size:4cm; text-align: center; color: black; \
 text-shadow: 0 0 2mm red}</style></head> \
<body><h1>";
    char b[1024] = "Pipeline Elements: ";
    char c[1024] = "</h1></body></html>\r\n";

    GstObject *parent = gst_element_get_parent(httpcontrol);
    GST_INFO_OBJECT (httpcontrol, "Element parent pointer: 0x%p", parent);
    httpcontrol->parent_info = TRUE;

    httpcontrol->it = gst_bin_iterate_elements (GST_BIN(parent));

    /* iterator size is not returning the amount of iterated elements */
    while (gst_iterator_next (httpcontrol->it, &httpcontrol->elem) == GST_ITERATOR_OK) 
    {
//      GST_INFO_OBJECT(httpcontrol, "Iterator size: %i", httpcontrol->it->size);
//      GST_INFO_OBJECT(httpcontrol, "Object Name: %s", gst_element_get_name (g_value_get_object(&httpcontrol->elem)));
//      strcat(b, gst_element_get_name(g_value_get_object(&httpcontrol->elem)));
      httpcontrol->iterated_elements[httpcontrol->iterated_elements_index] = g_value_get_object(&httpcontrol->elem);
      httpcontrol->iterated_elements_index++;
    }

    /*
     * First iterated elements are the ones at the "right" of the pipeline 
     * this is why we are doing index--
     */
    httpcontrol->iterated_elements_index--;
    for (httpcontrol->iterated_elements_index; httpcontrol->iterated_elements_index >= 0; httpcontrol->iterated_elements_index--)
    {
      GST_INFO_OBJECT(httpcontrol, "Element Named %s, Index: %i", gst_element_get_name(httpcontrol->iterated_elements[httpcontrol->iterated_elements_index]), httpcontrol->iterated_elements_index);

      /* Concatenate element name into web server */
      strcat(b, gst_element_get_name(httpcontrol->iterated_elements[httpcontrol->iterated_elements_index]));
      strcat(b, " ");
    }

    g_value_unset(&httpcontrol->elem);

    strcat(a,b);
    strcat(a,c);
    strcpy(httpcontrol->response, a);
    
/*    strcpy(httpcontrol->response, "HTTP/1.1 200 OK\r\n \
Content-Type: text/html; charset=UTF-8\r\n\r\n \
<!DOCTYPE html><html><head><title>Pipeline Structure</title> \
<style>body { background-color: #111 } \
h1 { font-size:4cm; text-align: center; color: black; \
 text-shadow: 0 0 2mm red}</style></head> \
<body><h1>TRANSFORM_IP</h1></body></html>\r\n");*/

  }

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "httpcontrol", GST_RANK_NONE,
      GST_TYPE_HTTPCONTROL);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    httpcontrol,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
