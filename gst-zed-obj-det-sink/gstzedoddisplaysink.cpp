﻿#include "gstzedoddisplaysink.h"

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/gstbuffer.h>
#include <gst/gstcaps.h>
#include <gst/gstformat.h>

#include "gst-zed-meta/gstzedmeta.h"

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_ALWAYS,
                                                                    GST_STATIC_CAPS( ("video/x-raw, " // Double stream
                                                                                      "format = (string) { BGRA }, "
                                                                                      "width = (int) { 672, 1280, 1920, 2208 } , "
                                                                                      "height =  (int) { 752, 1440, 2160, 2484 } , "
                                                                                      "framerate =  (fraction) { 15, 30, 60, 100 }") ) );

GST_DEBUG_CATEGORY_STATIC (gst_zedoddisplaysink_debug);
#define GST_CAT_DEFAULT gst_zedoddisplaysink_debug

#define DEFAULT_PROP_DISPLAY_3D     TRUE

enum
{
    PROP_0,
    PROP_DISPLAY3D,
    PROP_LAST
};

static void gst_zedoddisplaysink_dispose(GObject * object);

static void gst_zedoddisplaysink_set_property (GObject * object, guint prop_id,
                                               const GValue * value, GParamSpec * pspec);
static void gst_zedoddisplaysink_get_property (GObject * object, guint prop_id,
                                               GValue * value, GParamSpec * pspec);

static gboolean gst_zedoddisplaysink_start (GstBaseSink * sink);
static gboolean gst_zedoddisplaysink_stop (GstBaseSink * sink);

static gboolean gst_zedoddisplaysink_event (GstBaseSink * sink, GstEvent * event);
static GstFlowReturn gst_zedoddisplaysink_render (GstBaseSink * sink, GstBuffer * buffer);

G_DEFINE_TYPE( GstZedOdDisplaySink, gst_zedoddisplaysink, GST_TYPE_BASE_SINK );

static void gst_zedoddisplaysink_class_init (GstZedOdDisplaySinkClass * klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);
    GstBaseSinkClass *gstbasesink_class = GST_BASE_SINK_CLASS (klass);

    gobject_class->dispose = gst_zedoddisplaysink_dispose;

    gobject_class->set_property = gst_zedoddisplaysink_set_property;
    gobject_class->get_property = gst_zedoddisplaysink_get_property;

    // TODO UNCOMMENT WHEN 3D DISPLAY SINK IS READY
//    g_object_class_install_property( gobject_class, PROP_DISPLAY3D,
//                                     g_param_spec_boolean("display-3d", "Display results in 3D  ",
//                                                          "Creates a 3D OpenGL View to display bounding boxes and skeletons",
//                                                          DEFAULT_PROP_DISPLAY_3D,
//                                                          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gst_element_class_set_static_metadata (gstelement_class,
                                           "ZED Object Detection Display Sink",
                                           "Sink/Video", "Display results of ZED Object Detection module",
                                           "Stereolabs <support@stereolabs.com>");
    gst_element_class_add_static_pad_template (gstelement_class, &sink_factory);

    gstbasesink_class->start = GST_DEBUG_FUNCPTR (gst_zedoddisplaysink_start);
    gstbasesink_class->stop = GST_DEBUG_FUNCPTR (gst_zedoddisplaysink_stop);
    gstbasesink_class->render = GST_DEBUG_FUNCPTR (gst_zedoddisplaysink_render);
    gstbasesink_class->event = GST_DEBUG_FUNCPTR (gst_zedoddisplaysink_event);
}

static void gst_zedoddisplaysink_init(GstZedOdDisplaySink * displaysink)
{
    GST_TRACE_OBJECT( displaysink, "Init" );

    displaysink->display3d = DEFAULT_PROP_DISPLAY_3D;
    displaysink->ocv_wnd_name = "Detections";

    gst_base_sink_set_sync(GST_BASE_SINK(displaysink), FALSE);
}

static void gst_zedoddisplaysink_dispose (GObject * object)
{
    GstZedOdDisplaySink* sink = GST_OD_DISPLAY_SINK(object);

    cv::destroyAllWindows();

    GST_TRACE_OBJECT( sink, "Dispose" );

    G_OBJECT_CLASS(gst_zedoddisplaysink_parent_class)->dispose(object);
}

void gst_zedoddisplaysink_set_property (GObject * object, guint prop_id,
                                        const GValue * value, GParamSpec * pspec)
{
    GstZedOdDisplaySink* sink = GST_OD_DISPLAY_SINK(object);

    GST_TRACE_OBJECT( sink, "Set property" );

    switch (prop_id) {
    case PROP_DISPLAY3D:
        sink->display3d = g_value_get_boolean (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

void gst_zedoddisplaysink_get_property (GObject * object, guint prop_id,
                                        GValue * value, GParamSpec * pspec)
{
    GstZedOdDisplaySink* sink = GST_OD_DISPLAY_SINK(object);

    GST_TRACE_OBJECT( sink, "Get property" );

    switch (prop_id) {
    case PROP_DISPLAY3D:
        g_value_set_boolean (value, sink->display3d);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

gboolean gst_zedoddisplaysink_start(GstBaseSink* sink)
{
    GstZedOdDisplaySink* displaysink = GST_OD_DISPLAY_SINK(sink);

    GST_TRACE_OBJECT( displaysink, "Start" );

    cv::namedWindow(displaysink->ocv_wnd_name, cv::WINDOW_AUTOSIZE);

    return TRUE;
}

gboolean gst_zedoddisplaysink_stop (GstBaseSink * sink)
{
    GstZedOdDisplaySink* displaysink = GST_OD_DISPLAY_SINK(sink);

    GST_TRACE_OBJECT( displaysink, "Stop" );

    return TRUE;
}

static gboolean parse_sink_caps( GstZedOdDisplaySink* sink, GstCaps* sink_caps )
{
    GstVideoInfo vinfo_in;

    GST_DEBUG_OBJECT( sink, "Sink caps %" GST_PTR_FORMAT, sink_caps);

    gst_video_info_from_caps(&vinfo_in, sink_caps);
    sink->img_left_w = vinfo_in.width;
    sink->img_left_h = vinfo_in.height/2; // The pad gets only composite stream inputs

    return TRUE;
}

gboolean gst_zedoddisplaysink_event (GstBaseSink * sink, GstEvent * event)
{
    GstEventType type;
    gboolean ret;
    GstZedOdDisplaySink* displaysink = GST_OD_DISPLAY_SINK(sink);

    type = GST_EVENT_TYPE (event);

    GST_TRACE_OBJECT( displaysink, "Event " );

    switch (type) {
    case GST_EVENT_EOS:
        break;
    case GST_EVENT_CAPS:
    {
        GST_DEBUG_OBJECT( displaysink, "Event CAPS" );
        GstCaps* caps;

        gst_event_parse_caps(event, &caps);

        ret = parse_sink_caps( displaysink, caps );

        break;
    }
    default:
        break;
    }

    return GST_BASE_SINK_CLASS(gst_zedoddisplaysink_parent_class)->event (sink, event);
}

void draw_objects( cv::Mat& image, guint8 obj_count, ZedObjectData* objs )
{
    for( int i=0; i<obj_count; i++ )
    {
        cv::Scalar color = cv::Scalar::all(125);
        if(objs[i].id>=0)
        {
            color = cv::Scalar((objs[i].id*232+232)%255, (objs[i].id*176+176)%255, (objs[i].id*59+59)%255);
        }

        cv::Rect roi_render(0, 0, image.size().width, image.size().height);

        GST_TRACE( "Object: %d", i );
        GST_TRACE( " * Id: %d", (int)objs[i].label );
        GST_TRACE( " * Pos: %g,%g,%g", objs[i].position[0],objs[i].position[1],objs[i].position[2] );

        if(objs[i].skeletons_avail==FALSE)
        {

            // ----> Bounding box
            cv::Point tl;
            tl.x = objs[i].bounding_box_2d[0][0];
            tl.y = objs[i].bounding_box_2d[0][1];
            cv::Point br;
            br.x = objs[i].bounding_box_2d[2][0];
            br.y = objs[i].bounding_box_2d[2][1];
            cv::rectangle( image, tl, br, color, 3 );
            // <---- Bounding box

            // ----> Text info
            int baseline=0;
            int font_face = cv::FONT_HERSHEY_COMPLEX_SMALL;
            double font_scale = 0.75;

            std::stringstream txt_info;
            txt_info << "Id: " << objs[i].id << " - " << ((objs[i].label==OBJECT_CLASS::PERSON)?"PERSON":"VEHICLE");

            cv::Size txt_size = cv::getTextSize( txt_info.str(), font_face, font_scale, 1, &baseline );

            int offset = 5;

            cv::Point txt_tl;
            cv::Point txt_br;
            cv::Point txt_pos;

            txt_tl.x = tl.x;
            txt_tl.y = tl.y - (txt_size.height+2*offset);

            txt_br.x = tl.x + (txt_size.width+2*offset);
            txt_br.y = tl.y;

            txt_pos.x = txt_tl.x + offset;
            txt_pos.y = txt_br.y - offset;

            if( !roi_render.contains(txt_tl) )
            {
                txt_tl.y = tl.y + (txt_size.height+2*baseline);
                txt_pos.y = txt_tl.y - offset;
            }

            cv::rectangle( image, txt_tl, txt_br, color, 1 );
            cv::putText( image, txt_info.str(), txt_pos, font_face, font_scale, color, 1, cv::LINE_AA );

            if( !std::isnan(objs[i].position[0]) && !std::isnan(objs[i].position[1]) && !std::isnan(objs[i].position[2]) )
            {
                float dist = sqrtf( objs[i].position[0]*objs[i].position[0] +
                        objs[i].position[1]*objs[i].position[1] +
                        objs[i].position[2]*objs[i].position[2]);
                char text[64];
                sprintf(text, "%.2fm", abs(dist / 1000.0f));
                putText( image, text, cv::Point2i(tl.x+(br.x-tl.x)/2 - 20, tl.y+(br.y-tl.y)/2 - 12),
                         cv::FONT_HERSHEY_COMPLEX_SMALL, 0.75, color, 1 );
            }
            // <---- Text info
        }
        else
        {
            // ----> Skeletons
            {
                // ----> Bones
                for (const auto& parts : skeleton::BODY_BONES)
                {
                    cv::Point2f kp_a;
                    kp_a.x = objs[i].keypoint_2d[skeleton::getIdx(parts.first)][0];
                    kp_a.y = objs[i].keypoint_2d[skeleton::getIdx(parts.first)][1];
                    cv::Point2f kp_b;
                    kp_b.x = objs[i].keypoint_2d[skeleton::getIdx(parts.second)][0];
                    kp_b.y = objs[i].keypoint_2d[skeleton::getIdx(parts.second)][1];
                    if (roi_render.contains(kp_a) && roi_render.contains(kp_b))
                        cv::line(image, kp_a, kp_b, color, 1, cv::LINE_AA);
                }
                // <---- Bones
                // ----> Joints
                for(int j=0; j<18; j++)
                {
                    cv::Point2f cv_kp;
                    cv_kp.x = objs[i].keypoint_2d[j][0];
                    cv_kp.y = objs[i].keypoint_2d[j][1];
                    if (roi_render.contains(cv_kp))
                    {
                        cv::circle(image, cv_kp, 3, color+cv::Scalar(50,50,50), -1, cv::LINE_AA);
                    }
                }
                // <---- Joints
            }
            // <---- Skeletons
        }

    }
}

GstFlowReturn gst_zedoddisplaysink_render( GstBaseSink * sink, GstBuffer* buf )
{
    GstZedOdDisplaySink* displaysink = GST_OD_DISPLAY_SINK(sink);

    GstMapInfo map_in;

    GST_TRACE_OBJECT( displaysink, "Render" );

    if(gst_buffer_map(buf, &map_in, GST_MAP_READ))
    {
        // Get left image (upper half memory buffer)
        cv::Mat ocv_left = cv::Mat( displaysink->img_left_h, displaysink->img_left_w, CV_8UC4, map_in.data ).clone();

        // Metadata
        GstZedSrcMeta* meta = (GstZedSrcMeta*)gst_buffer_get_meta( buf, GST_ZED_SRC_META_API_TYPE );

        GST_TRACE_OBJECT( displaysink, "Cam. Model: %d",meta->info.cam_model );
        GST_TRACE_OBJECT( displaysink, "Stream type: %d",meta->info.stream_type );

        if(meta->od_enabled)
        {
            GST_TRACE_OBJECT( displaysink, "Detected %d objects",meta->obj_count );
            // Draw 2D detections
            draw_objects( ocv_left, meta->obj_count, meta->objects );
        }

        // rendering
        cv::imshow( displaysink->ocv_wnd_name, ocv_left );

        // Release incoming buffer
        gst_buffer_unmap( buf, &map_in );
    }

    return GST_FLOW_OK;
}

static gboolean plugin_init (GstPlugin * plugin)
{
    GST_DEBUG_CATEGORY_INIT( gst_zedoddisplaysink_debug, "zedoddisplaysink", 0,
                             "debug category for zedoddisplaysink element");
    gst_element_register( plugin, "zedoddisplaysink", GST_RANK_NONE,
                          gst_zedoddisplaysink_get_type());

    return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   zedoddisplaysink,
                   "ZED Object Detection Display sink",
                   plugin_init,
                   GST_PACKAGE_VERSION,
                   GST_PACKAGE_LICENSE,
                   GST_PACKAGE_NAME,
                   GST_PACKAGE_ORIGIN)
