#ifndef __AUDIO_SINK_H__
#define __AUDIO_SINK_H__

#include <as_types.h>
#include <glib-object.h>
#include <audio_link.h>

#define G_TYPE_AUDIO_SINK (audio_sink_get_type())
#define AUDIO_SINK(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_AUDIO_SINK, AudioSink))
#define AUDIO_SINK_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_AUDIO_SINK, AudioSinkClass)

#define IS_AUDIO_SINK(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_AUDIO_SINK)

#define IS_AUDIO_SINK_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_AUDIO_SINK)

#define AUDIO_SINK_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_INTERFACE((object), G_TYPE_AUDIO_SINK, AudioSinkClass)

#define AUDIO_SINK_TYPE(object) (G_TYPE_FROM_INTERFACE (object))
#define AUDIO_SINK_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))


typedef struct _AudioSinkClass AudioSinkClass;


struct _AudioSinkClass
{
  GTypeInterface base_iface;

  /* Returns true if this object can make read calls to a connected source. */
  gboolean (*has_buffer)(AudioSink *sink);

  /* Write at most length frames (i.e. (length * channels) samples)
     to the sink. Returns the number of frames actually transfered */
  FrameLength (*write)(AudioSink *sink, const Sample *buffer,
		       FrameLength length);
  
  /* Request that the read method of the connected source will be called,
     either within this method or later. Only works if the source has buffer.
  */
  void (*request_read)(AudioSink *sink, FrameLength length);
  
  void (*flush)(AudioSink *sink);
  
  /* Total number of frames buffered before they are consumed. */
  FrameLength (*pipe_length)(AudioSink *sink);
  
  /* Connect to a sink. Connect to NULL to disconnect */
  void (*connect)(AudioSink *sink, AudioSource *source);

  /* The source is no longer connected to a sink. */
  void (*disconnected)(AudioSink *sink);
  
  /* Signals */

};

GType
audio_sink_get_type();

gboolean
audio_sink_has_buffer(AudioSink *sink);

FrameLength
audio_sink_write(AudioSink *sink,const Sample *buffer, FrameLength length);

void
audio_sink_request_read(AudioSink *sink, FrameLength length);

FrameLength
audio_sink_pipe_length(AudioSink *sink);

void
audio_sink_disconnect(AudioSink *sink);

void
audio_sink_flush(AudioSink *sink);


#endif /* __AUDIO_SINK_H__ */
