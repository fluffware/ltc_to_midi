#include <audio_source.h>

static guint audio_source_base_init_count = 0;

static Channels
source_channels(AudioSource *source)
{
  return 0;
}

static FrameLength
source_sample_rate(AudioSource *source)
{
  return 0;
}

static gboolean
source_has_buffer(AudioSource *source)
{
  return FALSE;
}

static FrameLength
source_read(AudioSource *source,Sample *buffer, FrameLength length)
{
  return 0;
}
     

static void
source_request_write(AudioSource *source, FrameLength length)
{
  g_error("AudioSource::request_write should be overridden by "
	  "subclasses if they have a buffer");
}

static FrameLength
source_pipe_length(AudioSource *source)
{
  return 0;
}

static void
source_connect(AudioSource *source, AudioSink *sink)
{
}

static void
source_disconnected(AudioSource *source)
{
}

static void
audio_source_base_init (AudioSourceClass *source_class)
{
  audio_source_base_init_count++;
  if (audio_source_base_init_count == 1) {
  }
  source_class->channels = source_channels;
  source_class->sample_rate = source_sample_rate;
  source_class->has_buffer = source_has_buffer;
  source_class->read = source_read;
  source_class->request_write = source_request_write;
  source_class->pipe_length = source_pipe_length;
  source_class->connect = source_connect;
  source_class->disconnected = source_disconnected;
 }

static void
audio_source_base_finalize (AudioSourceClass *source_class)
{
  audio_source_base_init_count--;
  if (audio_source_base_init_count == 0) {
  }
}

GType
audio_source_get_type()
{
  static GType audio_source_type = 0;

  if (!audio_source_type) {
    static const GTypeInfo audio_source_info =
      {
        sizeof (AudioSourceClass),
        (GBaseInitFunc) audio_source_base_init,		/* base_init */
        (GBaseFinalizeFunc) audio_source_base_finalize, /* base_finalize */
      };
    
    audio_source_type = g_type_register_static(G_TYPE_INTERFACE, "AudioSource",
					       &audio_source_info, 0);
    g_type_interface_add_prerequisite (audio_source_type, G_TYPE_OBJECT);
    }

  return audio_source_type;

}

/**
 * audio_source_channels:
 * @source: an #AudioSource
 * @Returns: number of audio channels
 *
 * Calls #AudioSource::channels method.
 **/

Channels
audio_source_channels(AudioSource *source)
{
  return AUDIO_SOURCE_GET_CLASS(source)->channels(source);
}

/**
 * audio_source_sample_rate:
 * @source: an #AudioSource
 * @Returns: sample rate
 *
 * Calls AudioSource::sample_rate method.
 **/

FrameLength
audio_source_sample_rate(AudioSource *source)
{
   return AUDIO_SOURCE_GET_CLASS(source)->sample_rate(source);
}

/**
 * audio_source_has_buffer:
 * @source: an #AudioSource
 * @Returns: #TRUE if object has buffer for #AudioSink::write method calls
 *
 * Calls AudioSource::sample_rate method.
 **/

gboolean
audio_source_has_buffer(AudioSource *source)
{
  return AUDIO_SOURCE_GET_CLASS(source)->has_buffer(source);
}

/**
 * audio_source_read:
 * @source: an #AudioSource
 * @buffer: write samples to this buffer
 * @length: number of frames that can be written in @buffer. A zero length
 * indicates that the calling sink currently isn't interested in any more
 * data.
 * @Returns: frames read
 *
 * Calls the #AudioSource::read method.
 **/
FrameLength
audio_source_read(AudioSource *source,
		  Sample *buffer, FrameLength length)
{
  return AUDIO_SOURCE_GET_CLASS(source)->read(source, buffer, length);
}

/**
 * audio_source_request_write:
 * @source: an #AudioSource
 * @length: number of frames requested. This is just a hint, the following
 * write method call may have a different length
 *
 * Calls the #AudioSource::request_write method.
 **/

void
audio_source_request_write(AudioSource *source, FrameLength length)
{
  AUDIO_SOURCE_GET_CLASS(source)->request_write(source, length);
}

/**
 * audio_source_pipe_length:
 * @source: an #AudioSource
 * @Returns: total length of upstream pipe
 *
 * Calls the #AudioSource::pipe_length method.
 **/
FrameLength
audio_source_pipe_length(AudioSource *source)
{
  return AUDIO_SOURCE_GET_CLASS(source)->pipe_length(source);
}

/**
 * audio_source_disconnect:
 * @source: an #AudioSource
 *
 * Calls the #AudioSource::connect method with a NULL sink.
 */
void
audio_source_disconnect(AudioSource *source)
{
  AUDIO_SOURCE_GET_CLASS(source)->connect(source, NULL);
}
