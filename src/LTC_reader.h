#ifndef __LTC_READER_H__
#define __LTC_READER_H__

#include <glib-object.h>
#include <as_types.h>
#include <timecode_source.h>
#include <userbits_source.h>
#include <audio_sink.h>

typedef struct _LTCReaderClass LTCReaderClass;
typedef struct _LTCReader LTCReader;

#define G_TYPE_LTC_READER (LTC_reader_get_type())
#define LTC_READER(object)  \
(G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_LTC_READER, LTCReader))
#define LTC_READER_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_LTC_READER, LTCReaderClass)

#define IS_LTC_READER(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_LTC_READER)

#define IS_LTC_READER_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_LTC_READER)

#define LTC_READER_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_CLASS((object), G_TYPE_LTC_READER, LTCReaderClass)

#define LTC_READER_TYPE(object) (G_TYPE_FROM_INSTANCE (object))
#define LTC_READER_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))

LTCReader *
LTC_reader_new();

GType
LTC_reader_get_type();

#endif /* __LTC_READER_H__ */
