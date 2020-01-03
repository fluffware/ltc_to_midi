#include <audio_sink.h>

static guint audio_sink_base_init_count = 0;

static gboolean
sink_has_buffer(AudioSink *sink)
{
  return FALSE;
}

static FrameLength
sink_write(AudioSink *sink,const Sample *buffer, FrameLength length)
{
  return 0;
}
     

static void
sink_request_read(AudioSink *sink, FrameLength length)
{
  g_error("AudioSink::request_read should be overridden by "
	  "subclasses if they have a buffer");
}

static void
sink_flush(AudioSink *sink)
{
}

static FrameLength
sink_pipe_length(AudioSink *sink)
{
  return 0;
}

static void
sink_connect(AudioSink *sink, AudioSource *source)
{
}

static void
sink_disconnected(AudioSink *sink)
{
}

static void
audio_sink_base_init (AudioSinkClass *sink_class)
{
  audio_sink_base_init_count++;
  if (audio_sink_base_init_count == 1) {
   
  }
  /* g_message("audio_sink_base_init: %p", sink_class);*/
  sink_class->has_buffer = sink_has_buffer;
  sink_class->write = sink_write;
  sink_class->request_read = sink_request_read;
  sink_class->flush = sink_flush;
  sink_class->pipe_length = sink_pipe_length;
  sink_class->connect = sink_connect;
  sink_class->disconnected = sink_disconnected;
}

static void
audio_sink_base_finalize (AudioSinkClass *sink_class)
{
  audio_sink_base_init_count--;
  if (audio_sink_base_init_count == 0) {
  }
}

GType
audio_sink_get_type()
{
  static GType audio_sink_type = 0;

  if (!audio_sink_type) {
    static const GTypeInfo audio_sink_info =
      {
        sizeof (AudioSinkClass),
        (GBaseInitFunc) audio_sink_base_init,		/* base_init */
        (GBaseFinalizeFunc) audio_sink_base_finalize, /* base_finalize */
      };
    
    audio_sink_type = g_type_register_static(G_TYPE_INTERFACE, "AudioSink",
					       &audio_sink_info, 0);
    g_type_interface_add_prerequisite (audio_sink_type, G_TYPE_OBJECT);
    }

  return audio_sink_type;

}

/**
 * audio_sink_has_buffer:
 * @sink: an #AudioSink
 * @Returns: #TRUE if object has buffer for #AudioSource::read method calls
 *
 * Calls the #AudioSink::has_buffer method.
 **/

gboolean
audio_sink_has_buffer(AudioSink *sink)
{
  return AUDIO_SINK_GET_CLASS(sink)->has_buffer(sink);
}

/**
 * audio_sink_write:
 * @sink: an #AudioSink
 * @buffer: samples to write
 * @length: number of frames in @buffer
 * @Returns: frames written
 *
 * Calls the #AudioSink::write method.
 **/

FrameLength
audio_sink_write(AudioSink *sink,
		  const Sample *buffer, FrameLength length)
{
  return AUDIO_SINK_GET_CLASS(sink)->write(sink, buffer, length);
}

/**
 * audio_sink_request_read:
 * @sink: an #AudioSink
 * @length: number of frames requested. This is just a hint, the following
 * read method call may have a different length
 *
 * Calls the #AudioSink::request_read method.
 **/
void
audio_sink_request_read(AudioSink *sink, FrameLength length)
{
  AUDIO_SINK_GET_CLASS(sink)->request_read(sink, length);
}

/**
 * audio_sink_pipe_length:
 * @sink: a #AudioSink
 * @Returns: total length of downstream pipe
 *
 * Calls the #AudioSink::pipe_length method.
 **/

FrameLength
audio_sink_pipe_length(AudioSink *sink)
{
  return AUDIO_SINK_GET_CLASS(sink)->pipe_length(sink);
}

/**
 * audio_sink_disconnect:
 * @sink: a #AudioSink
 *
 * Calls the #AudioSink::connect method with a NULL source.
 **/
void
audio_sink_disconnect(AudioSink *sink)
{
  AUDIO_SINK_GET_CLASS(sink)->connect(sink, NULL);
}

/**
 * audio_sink_flush:
 * @sink: a #AudioSink
 *
 * Calls #Audiosink::flush method.
 **/
void
audio_sink_flush(AudioSink *sink)
{
  AUDIO_SINK_GET_CLASS(sink)->flush(sink);
}


