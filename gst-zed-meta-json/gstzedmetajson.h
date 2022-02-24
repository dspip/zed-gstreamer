#ifndef __GST_ZED_META_JSON_H__
#define __GST_ZED_META_JSON_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_ZED_META_JSON \
  (gst_zedmetajson_get_type())
#define GST_ZED_META_JSON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ZED_META_JSON,GstZedMetaJson))
#define GST_ZED_META_JSON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ZED_META_JSON,GstZedMetaJsonClass))
#define GST_IS_ZED_META_JSON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ZED_META_JSON))
#define GST_IS_ZED_META_JSON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ZED_META_JSON))

typedef struct _GstZedMetaJson      GstZedMetaJson;
typedef struct _GstZedMetaJsonClass GstZedMetaJsonClass;

struct _GstZedMetaJson
{
  GstElement element;
  GstPad *sinkpad, *srcpad;
};

struct _GstZedMetaJsonClass 
{
  GstElementClass parent_class;
};

GType gst_zedmetajson_get_type (void);

G_END_DECLS

#endif /* __GST_ZED_META_JSON_H__ */
