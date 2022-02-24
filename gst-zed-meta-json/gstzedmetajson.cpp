/**
 * SECTION:element-zedmetajson
 *
 * FIXME:Describe zedmetajson here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! zedmetajson ! fakesink silent=TRUE
 * ]|
 * 
 * gst-launch-1.0 filesrc location="/home/pavel/Desktop/PCAPS/stanag4609-239.10.12.2.pcap" ! pcapparse dst-ip=239.10.12.2 ! tsdemux ! meta/x-klv  ! identity eos-after=2 ! zedmetajson ! fakesink dump=true
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst-zed-meta-json/gstzedmetajson.h>
#include <inttypes.h>
#include "gst-zed-meta/gstzedmeta.h"
#include <nlohmann/json.hpp>

namespace nl = nlohmann;

GST_DEBUG_CATEGORY_STATIC (gst_zedmetajson_debug);
#define GST_CAT_DEFAULT gst_zedmetajson_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/data")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/json, encoding=(string)UTF-8")
    );

#define gst_zedmetajson_parent_class parent_class
G_DEFINE_TYPE (GstZedMetaJson, gst_zedmetajson, GST_TYPE_ELEMENT);

static void gst_zedmetajson_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_zedmetajson_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_zedmetajson_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_zedmetajson_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);
static void gst_zedmetajson_finalize(GObject * object);
static void gst_zedmetajson_dispose(GObject * object);

/* GObject vmethod implementations */

/* initialize the zedmetajson's class */
static void
gst_zedmetajson_class_init (GstZedMetaJsonClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_zedmetajson_set_property;
  gobject_class->get_property = gst_zedmetajson_get_property;

  gobject_class->finalize = gst_zedmetajson_finalize;
  gobject_class->dispose = gst_zedmetajson_dispose;

  // g_object_class_install_property (gobject_class, PROP_SILENT,
  //     g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
  //         FALSE, G_PARAM_READWRITE));

  gst_element_class_set_details_simple(gstelement_class,
    "zedmetajson",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "shvarpa <<user@hostname.org>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_zedmetajson_init (GstZedMetaJson * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_zedmetajson_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_zedmetajson_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  // GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  // filter->silent = FALSE;
  // filter->sent_sei = FALSE;
  // filter->fps_num = 0;
  // filter->fps_den = 1;

}

static void
gst_zedmetajson_finalize(GObject * object) {
    GstZedMetaJson *filter = GST_ZED_META_JSON (object);
}

// called multiple times
static void gst_zedmetajson_dispose(GObject * object) {}

static void
gst_zedmetajson_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstZedMetaJson *filter = GST_ZED_META_JSON (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_zedmetajson_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstZedMetaJson *filter = GST_ZED_META_JSON (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean
gst_zedmetajson_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  GstZedMetaJson *filter;
  gboolean ret;

  filter = GST_ZED_META_JSON (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
      ret = gst_pad_event_default (pad, parent, event);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

// static gboolean gst_buffer_copy_timestamps(GstBuffer *src, GstBuffer *dest) {
//   GST_BUFFER_DTS(dest) = GST_BUFFER_DTS(src);
//   GST_BUFFER_DTS(dest) = GST_BUFFER_PTS(src);
//   GST_BUFFER_OFFSET(dest) = GST_BUFFER_OFFSET(src);
//   GST_BUFFER_OFFSET_END(dest) = GST_BUFFER_OFFSET_END(src);
//   return true;
// }

static nl::json zed_meta_to_json(GstZedSrcMeta *meta) {
  return {
    {"info",{
      {"stream_type", meta->info.stream_type},
      {"cam_model", meta->info.cam_model},
      {"grab_single_frame_width", meta->info.grab_single_frame_width},
      {"grab_single_frame_height", meta->info.grab_single_frame_height},
    }},
    {"pose", {
      {"pose_avail", meta->pose.pose_avail},
      {"pos_tracking_state", meta->pose.pos_tracking_state},
      {"pos", meta->pose.pos},
      {"orient", meta->pose.orient},
    }},
    {"imu", {
      {"imu_avail", meta->pose.pose_avail},
      {"pos_tracking_state", meta->pose.pos_tracking_state},
      {"pos", meta->pose.pos},
      {"orient", meta->pose.orient},
    }},
    {"sens", {
      {"imu", {
        {"imu_avail", meta->sens.imu.imu_avail},
        {"acc", meta->sens.imu.acc},
        {"gyro", meta->sens.imu.gyro},
      }},
      {"mag", {
        {"mag_avail", meta->sens.mag.mag},
        {"mag", meta->sens.mag.mag},
      }},
      {"env", {
        {"env_avail", meta->sens.env.env_avail},
        {"temp", meta->sens.env.temp},
        {"press", meta->sens.env.press},
      }},
      {"temp", {
        {"temp_avail", meta->sens.temp.temp_avail},
        {"temp_cam_left", meta->sens.temp.temp_cam_left},
        {"temp_cam_right", meta->sens.temp.temp_cam_right},
      }},
    }},
  };
  // // endline
  // *csvsink->out_file_ptr << std::endl;
  // return {};
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_zedmetajson_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstZedMetaJson *filter = GST_ZED_META_JSON (parent);

    GstMapInfo info;
    if(!gst_buffer_map(buf, &info, GST_MAP_READ)) {
      return GST_FLOW_ERROR;
    }

    GstFlowReturn ret = GST_FLOW_OK;
    nl::json parsed = zed_meta_to_json((GstZedSrcMeta *)info.data);
    // std::cout << j.dump(2) << std::endl;
    std::string parsed_str = parsed.dump(-1, (char)32, false, nl::json::error_handler_t::replace);
    // std::string j_str = j.dump();
    GstBuffer *parsed_buf = gst_buffer_new_and_alloc(parsed_str.size());
    gst_buffer_copy_into(parsed_buf, buf, (GstBufferCopyFlags)(GST_BUFFER_COPY_TIMESTAMPS | GST_BUFFER_COPY_META | GST_BUFFER_COPY_METADATA), 0, 0);
    GstMapInfo parsed_info;
    if(gst_buffer_map(parsed_buf, &parsed_info, GST_MAP_READWRITE)) {
      memcpy(parsed_info.data, parsed_str.c_str(), parsed_str.size());
      gst_buffer_unmap(parsed_buf, &parsed_info);
      // GstBuffer *parsed_buf = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, (gpointer) parsed_str.c_str(), parsed_str.size(), 0, parsed_str.size(), NULL, NULL);
      ret = gst_pad_push(filter->srcpad, parsed_buf);
    }

    gst_buffer_unmap(buf, &info);
    gst_buffer_unref(buf);
    
    return ret;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
zedmetajson_init (GstPlugin * zedmetajson)
{
  /* debug category for filtering log messages
   *
   * exchange the string 'Template zedmetajson' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_zedmetajson_debug, "zedmetajson",
      0, "Template zedmetajson");

  return gst_element_register (zedmetajson, "zedmetajson", GST_RANK_NONE,
      GST_TYPE_ZED_META_JSON);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstzedmetajson"
#endif

/* gstreamer looks for this structure to register zedmetajsons
 *
 * exchange the string 'Template zedmetajson' with your zedmetajson description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    zedmetajson,
    "Template zedmetajson",
    zedmetajson_init,
    GST_PACKAGE_VERSION, 
    GST_PACKAGE_LICENSE, 
    GST_PACKAGE_NAME, 
    GST_PACKAGE_ORIGIN
)