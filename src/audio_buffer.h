#ifndef __AUDIO_BUFFER_H__
#define __AUDIO_BUFFER_H__

#include <audio_source.h>
#include <audio_sink.h>
#include <glib-object.h>
#include <as_types.h>

typedef struct _AudioBufferClass AudioBufferClass;
typedef struct _AudioBuffer AudioBuffer;

#define G_TYPE_AUDIO_BUFFER (audio_buffer_get_type())
#define AUDIO_BUFFER(object)  \
(G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_AUDIO_BUFFER, AudioBuffer))
#define AUDIO_BUFFER_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_AUDIO_BUFFER, AudioBufferClass)

#define IS_AUDIO_BUFFER(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_AUDIO_BUFFER)

#define IS_AUDIO_BUFFER_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_AUDIO_BUFFER)

#define AUDIO_BUFFER_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_CLASS((object), G_TYPE_AUDIO_BUFFER, AudioBufferClass)

#define AUDIO_BUFFER_TYPE(object) (G_TYPE_FROM_INSTANCE (object))
#define AUDIO_BUFFER_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))

AudioBuffer *
audio_buffer_new(SampleLength size);

gboolean
audio_buffer_set_length(AudioBuffer *buffer, FrameLength length);

void
audio_buffer_flush(AudioBuffer *buffer);

FrameLength
audio_buffer_length(AudioBuffer *buffer);

FrameLength
audio_buffer_capacity(AudioBuffer *buffer);

GType
audio_buffer_get_type();

#endif /* __AUDIO_BUFFER_H__ */
