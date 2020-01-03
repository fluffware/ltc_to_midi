#ifndef __AUDIO_SOURCE_H__
#define __AUDIO_SOURCE_H__

#include <as_types.h>
#include <glib-object.h>
#include <audio_link.h>

#define G_TYPE_AUDIO_SOURCE (audio_source_get_type())
#define AUDIO_SOURCE(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_AUDIO_SOURCE, AudioSource))
#define AUDIO_SOURCE_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_AUDIO_SOURCE, AudioSourceClass)

#define IS_AUDIO_SOURCE(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_AUDIO_SOURCE)

#define IS_AUDIO_SOURCE_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_AUDIO_SOURCE)

#define AUDIO_SOURCE_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_INTERFACE((object), G_TYPE_AUDIO_SOURCE, AudioSourceClass)

#define AUDIO_SOURCE_TYPE(object) (G_TYPE_FROM_INTERFACE (object))
#define AUDIO_SOURCE_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))


typedef struct _AudioSourceClass AudioSourceClass;



struct _AudioSourceClass
{
  GTypeInterface base_iface;

  /* Number of samples in each frame. May not change while connected*/
  Channels (*channels)(AudioSource *source);

  /* Number of frames per second. Zero if not known.
     May not change while connected */
  FrameLength (*sample_rate)(AudioSource *source);
  
  /* Returns true if this object can make write calls to a connected sink. */
  gboolean (*has_buffer)(AudioSource *source);

  /* Read at most length frames (i.e. (length * channels) samples)
     from the source. Returns the number of frames actually transfered */
  FrameLength (*read)(AudioSource *source, Sample *buffer,FrameLength length);
  
  /* Request that the write method of the connected sink will be called,
     either within this method or later. Only works if the sink has buffer.
   */
  void (*request_write)(AudioSource *source, FrameLength length);
  
  /* Total number of frames buffered since they were created. */
  FrameLength (*pipe_length)(AudioSource *source);
  
  /* Connect to a sink. Connect to NULL to disconnect */
  void (*connect)(AudioSource *source, AudioSink *sink);

  /* The source is no longer connected to a sink. */
  void (*disconnected)(AudioSource *source);
  /* Signals */

};

GType
audio_source_get_type();

Channels
audio_source_channels(AudioSource *source);

FrameLength
audio_source_sample_rate(AudioSource *source);

gboolean
audio_source_has_buffer(AudioSource *source);

FrameLength
audio_source_read(AudioSource *source,Sample *buffer, FrameLength length);

void
audio_source_request_write(AudioSource *source, FrameLength length);

FrameLength
audio_source_pipe_length(AudioSource *source);

void
audio_source_disconnect(AudioSource *source);

#endif /* __AUDIO_SOURCE_H__ */
