/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) 2003 Arwed v. Merkatz <v.merkatz@gmx.net>
 * Copyright (C) 2006 Mark Nauwelaerts <manauw@skynet.be>
 * Copyright (C) 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
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

/*
 * This file was (probably) generated from
 * gstvideotemplate.c,v 1.12 2004/01/07 21:07:12 ds Exp 
 * and
 * make_filter,v 1.6 2004/01/07 21:33:01 ds Exp 
 */

/**
 * SECTION:element-simpleoverlay
 * @title: simpleoverlay
 *
 * Performs overlay correction on a video stream.
 *
 * ## Example launch line
 * |[
 * gst-launch-1.0 videotestsrc ! simpleoverlay ! videoconvert ! ximagesink
 * ]|
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include <gst/video/video.h>
#include "gstsimpleoverlay.h"
#include "fonts/times_new_roman_12.h"

GST_DEBUG_CATEGORY_STATIC (simpleoverlay_debug);
#define GST_CAT_DEFAULT simpleoverlay_debug

/* _GstSimpleOverlay properties */
enum
{
  PROP_0,
      /* FILL ME */
};


static guint8 num_table[SIZE_OF_NUM * NUM_WIDTH * NUM_HEIGHT] = {0};

static GstStaticPadTemplate gst_simple_overlay_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ NV12 }")));

static GstStaticPadTemplate gst_simple_overlay_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ NV12 }"))
    );

static void gst_simple_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_simple_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_simple_overlay_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info);
static GstFlowReturn gst_simple_overlay_transform_frame_ip (GstVideoFilter * vfilter,
    GstVideoFrame * frame);
static void gst_simple_overlay_before_transform (GstBaseTransform * transform,
    GstBuffer * buf);

static void gst_simple_overlay_calculate_tables (GstSimpleOverlay *overlay);

G_DEFINE_TYPE (GstSimpleOverlay, gst_simple_overlay, GST_TYPE_VIDEO_FILTER);
GST_ELEMENT_REGISTER_DEFINE (simpleoverlay, "simpleoverlay", GST_RANK_NONE, GST_TYPE_SIMPLE_OVERLAY);

static void
gst_simple_overlay_class_init (GstSimpleOverlayClass * g_class)
{
  GObjectClass *gobject_class = (GObjectClass *) g_class;
  GstElementClass *gstelement_class = (GstElementClass *) g_class;
  GstBaseTransformClass *trans_class = (GstBaseTransformClass *) g_class;
  GstVideoFilterClass *vfilter_class = (GstVideoFilterClass *) g_class;

  GST_DEBUG_CATEGORY_INIT (simpleoverlay_debug, "simpleoverlay", 0, "simpleoverlay");

  gobject_class->set_property = gst_simple_overlay_set_property;
  gobject_class->get_property = gst_simple_overlay_get_property;


  gst_element_class_set_static_metadata (gstelement_class,
      "Add clock overlay", "Filter/Effect/Video",
      "Add clock overlay on a video stream", 
      "Xu,Baoling <baoling.xu@aptiv.com>");

  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_simple_overlay_sink_template);
  gst_element_class_add_static_pad_template (gstelement_class,
      &gst_simple_overlay_src_template);

  trans_class->before_transform =
      GST_DEBUG_FUNCPTR (gst_simple_overlay_before_transform);
  trans_class->transform_ip_on_passthrough = FALSE;

  vfilter_class->set_info = GST_DEBUG_FUNCPTR (gst_simple_overlay_set_info);
  vfilter_class->transform_frame_ip =
      GST_DEBUG_FUNCPTR (gst_simple_overlay_transform_frame_ip);
}

static void
gst_simple_overlay_init (GstSimpleOverlay *overlay)
{
  /* properties */
  gst_simple_overlay_calculate_tables (overlay);
}

static void
gst_simple_overlay_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstSimpleOverlay *overlay = GST_SIMPLE_OVERLAY (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_simple_overlay_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSimpleOverlay *overlay = GST_SIMPLE_OVERLAY (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_simple_overlay_calculate_tables (GstSimpleOverlay *overlay)
{
  gint n;
  gdouble val;
  gdouble exp;
  gboolean passthrough = FALSE;

  gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (overlay), passthrough);

  gint size = NUM_WIDTH * NUM_HEIGHT;

  memcpy(num_table + size * 0, NUM_0, size);
  memcpy(num_table + size * 1, NUM_1, size);
  memcpy(num_table + size * 2, NUM_2, size);
  memcpy(num_table + size * 3, NUM_3, size);
  memcpy(num_table + size * 4, NUM_4, size);
  memcpy(num_table + size * 5, NUM_5, size);
  memcpy(num_table + size * 6, NUM_6, size);
  memcpy(num_table + size * 7, NUM_7, size);
  memcpy(num_table + size * 8, NUM_8, size);
  memcpy(num_table + size * 9, NUM_9, size);
  memcpy(num_table + size * 10, NUM_LINE, size);
  memcpy(num_table + size * 11, NUM_SPACE, size);
  memcpy(num_table + size * 12, NUM_COLON, size);

}

static gint 
get_number_index(guint8 number)
{
    if (number >= '0' && number <= '9')
        return number - '0';
    else if (number == '-')
        return 10;
    else if (number == ' ')
        return 11;
    else if (number == ':')
        return 12;

    return 11;
}

#define DATA_TIME_FORMAT_LENGTH 20

static void
gst_simple_overlay_planar_yuv_ip (GstSimpleOverlay *overlay, GstVideoFrame * frame)
{
  gint i, j, height;
  gint width, stride, row_wrap;
  guint8 *data;

  data = GST_VIDEO_FRAME_COMP_DATA (frame, 0);
  stride = GST_VIDEO_FRAME_COMP_STRIDE (frame, 0);
  width = GST_VIDEO_FRAME_COMP_WIDTH (frame, 0);
  height = GST_VIDEO_FRAME_COMP_HEIGHT (frame, 0);
  row_wrap = stride - width;

  guint8 datetime[DATA_TIME_FORMAT_LENGTH] = {0};

  time_t stamp = time(NULL);
  struct tm* local_time = localtime(&stamp);
  strftime(datetime, DATA_TIME_FORMAT_LENGTH, "%Y-%m-%d %H:%M:%S", local_time);

  
  gint offset_x = width/2 - (DATA_TIME_FORMAT_LENGTH/2) * NUM_WIDTH;
  gint offset_y = height - NUM_HEIGHT - 30;
  gint num_size = NUM_WIDTH * NUM_HEIGHT;
  guint8* dest = data;
  guint8* start = dest + offset_y * width;

  for (gint i = 0; i < DATA_TIME_FORMAT_LENGTH; i++) {
      gint index = get_number_index(*(datetime + i));
      guint8* num = num_table + num_size * index;
      if (!num) {
          continue;
      }

      guint8* num_offset = start + i * NUM_WIDTH;
      for (gint j = 0; j < NUM_HEIGHT; j++) {
          guint8* buff_src = num_offset + j * width + offset_x;
          guint8* num_src = num + j * NUM_WIDTH;
          for (gint k = 0; k < NUM_WIDTH; k++) {
              if (*(num_src + k) != 0) { //the background is back
                  *(buff_src + k) = -21; //-21:white，0:black
              }
          }
      }
  }


}

static gboolean
gst_simple_overlay_set_info (GstVideoFilter * vfilter, GstCaps * incaps,
    GstVideoInfo * in_info, GstCaps * outcaps, GstVideoInfo * out_info)
{
  GstSimpleOverlay *overlay = GST_SIMPLE_OVERLAY (vfilter);

  GST_DEBUG_OBJECT (overlay,
      "setting caps: in %" GST_PTR_FORMAT " out %" GST_PTR_FORMAT, incaps,
      outcaps);

  switch (GST_VIDEO_INFO_FORMAT (in_info)) {
    case GST_VIDEO_FORMAT_NV12:
      overlay->process = gst_simple_overlay_planar_yuv_ip;
      break;
    default:
      goto invalid_caps;
      break;
  }
  return TRUE;

  /* ERRORS */
invalid_caps:
  {
    GST_ERROR_OBJECT (overlay, "Invalid caps: %" GST_PTR_FORMAT, incaps);
    return FALSE;
  }
}

static void
gst_simple_overlay_before_transform (GstBaseTransform * base, GstBuffer * outbuf)
{
  GstSimpleOverlay *overlay = GST_SIMPLE_OVERLAY (base);
  GstClockTime timestamp, stream_time;

  timestamp = GST_BUFFER_TIMESTAMP (outbuf);
  stream_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, timestamp);

  GST_DEBUG_OBJECT (overlay, "sync to %" GST_TIME_FORMAT,
      GST_TIME_ARGS (timestamp));

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (overlay), stream_time);
}

static GstFlowReturn
gst_simple_overlay_transform_frame_ip (GstVideoFilter * vfilter, GstVideoFrame * frame)
{
  GstSimpleOverlay *overlay = GST_SIMPLE_OVERLAY (vfilter);

  if (!overlay->process)
    goto not_negotiated;

  GST_OBJECT_LOCK (overlay);
  overlay->process (overlay, frame);
  GST_OBJECT_UNLOCK (overlay);

  return GST_FLOW_OK;

  /* ERRORS */
not_negotiated:
  {
    GST_ERROR_OBJECT (overlay, "Not negotiated yet");
    return GST_FLOW_NOT_NEGOTIATED;
  }
}
