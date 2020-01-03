#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <soundio_common.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/* #define STARTSTOP_DEBUG */


static gpointer    parent_class = NULL;


/* Source methods. */

static Channels
source_channels(AudioSource *source)
{
  SoundIOCommon *io = SOUND_IO_COMMON(source);
  return io->channels;
}

static FrameLength
source_sample_rate(AudioSource *source)
{
  SoundIOCommon *io = SOUND_IO_COMMON(source);
  return io->sample_rate;
}

static gboolean
source_has_buffer(AudioSource *source)
{
  return FALSE;
}

static FrameLength
source_read(AudioSource *source, Sample *buffer,FrameLength length)
{
  g_error("read method of SoundIOCommon should be overridden by subclass");
  return 0;
}

static void
source_request_write(AudioSource *source, FrameLength length)
{
  /* Not supported */
}

static FrameLength
source_pipe_length(AudioSource *source)
{
  g_error("pipe_length method (in AudioSource interface) of SoundIOCommon "
	  "should be overridden by subclass");
  return 0;
}

static void
source_connect(AudioSource *source, AudioSink *sink)
{
  SoundIOCommon *io = SOUND_IO_COMMON(source);
  if (io->sink) {
    AUDIO_SINK_GET_CLASS(io->sink)->disconnected(io->sink);
  }
  io->sink = sink;
}

static void
source_disconnected(AudioSource *source)
{
  SoundIOCommon *io = SOUND_IO_COMMON(source);
  io->sink = NULL;
}

void
sound_io_common_audio_source_init(gpointer g_iface, gpointer iface_data)
{
  AudioSourceClass *source_class = g_iface;
  source_class->channels = source_channels;
  source_class->sample_rate = source_sample_rate;
  source_class->has_buffer = source_has_buffer;
  source_class->read = source_read;
  source_class->request_write = source_request_write;
  source_class->pipe_length = source_pipe_length;
  source_class->connect = source_connect;
  source_class->disconnected = source_disconnected;
}

void
sound_io_common_audio_source_finalize(gpointer g_iface, gpointer iface_data)
{
}

/* Sink methods. */

static gboolean
sink_has_buffer(AudioSink *sink)
{
  return FALSE;
}

static FrameLength
sink_write(AudioSink *sink, const Sample *buffer,FrameLength length)
{
  g_error("write method of SoundIOCommon should be overridden by subclass");
  return 0;
}
  
static void
sink_request_read(AudioSink *sink, FrameLength length)
{
  /* Not supported. */
}
  
static void
sink_flush(AudioSink *sink)
{
}
  
static FrameLength
sink_pipe_length(AudioSink *sink)
{
  g_error("pipe_length method (in AudioSink interface) of SoundIOCommon "
	  "should be overridden by subclass");
  return 0;
}
  

static void
sink_connect(AudioSink *sink, AudioSource *source)
{
  SoundIOCommon *io = SOUND_IO_COMMON(sink);
  g_assert(audio_source_channels(source)
	   == io->channels);
  if (io->source) {
    AUDIO_SOURCE_GET_CLASS(io->source)->disconnected(io->source);
  }
  io->source = source;
}

static void
sink_disconnected(AudioSink *sink)
{
  SoundIOCommon *io = SOUND_IO_COMMON(sink);
  io->source = NULL;
}

void
sound_io_common_audio_sink_init(gpointer g_iface, gpointer iface_data)
{
  AudioSinkClass *sink_class = g_iface;
  sink_class->has_buffer = sink_has_buffer;
  sink_class->write = sink_write;
  sink_class->request_read = sink_request_read;
  sink_class->flush = sink_flush;
  sink_class->pipe_length = sink_pipe_length;
  sink_class->connect = sink_connect;
  sink_class->disconnected = sink_disconnected;
}

void
sound_io_common_audio_sink_finalize(gpointer g_iface, gpointer iface_data)
{
}

/* SoundIO methods. */

static FrameLength
sound_io_common_sound_io_sample_rate(SoundIO *sio)
{
  SoundIOCommon *io = SOUND_IO_COMMON(sio);
  return io->sample_rate;
}

void
sound_io_common_sound_io_init(gpointer g_iface, gpointer iface_data)
{
  SoundIOClass *sound_io_class = g_iface;
  sound_io_class->sample_rate = sound_io_common_sound_io_sample_rate;
}

void
sound_io_common_sound_io_finalize(gpointer g_iface, gpointer iface_data)
{
}

/* SoundIOCommon methods. */
FrameLength
sound_io_common_sample_rate(SoundIO *io)
{
  SoundIOCommon *common = SOUND_IO_COMMON(io);
  return common->sample_rate;
}


static void
sound_io_common_finalize(GObject *gobject)
{
  SoundIOCommon *common = SOUND_IO_COMMON(gobject);

  if (common->source) {
    AUDIO_SOURCE_GET_CLASS(common->source)
      ->disconnected(AUDIO_SOURCE(common->source));
    common->source = NULL;
  }
  
  if (common->sink) {
    AUDIO_SINK_GET_CLASS(common->sink)
      ->disconnected(AUDIO_SINK(common->sink));
    common->sink = NULL;
  }

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
sound_io_common_init(GTypeInstance *instance, gpointer g_class)
{
  SoundIOCommon *common = SOUND_IO_COMMON(instance);
  
  common->source = NULL;
  common->output_threshold = 1;
  
  common->sink = NULL;
  common->input_threshold = 1;
  
  common->sample_rate = 0;
  common->channels = 2;
  common->flushing = FALSE;
}


/* Dummy methods. */
static void
output_io_start(SoundIOCommon *io)
{
  g_warning("output_io_start method not implemented");
}

static void
output_io_stop(SoundIOCommon *io)
{
  g_warning("output_io_stop method not implemented");
}

static void
input_io_start(SoundIOCommon *io)
{
  g_warning("input_io_start method not implemented");
}

static void
input_io_stop(SoundIOCommon *io)
{
  g_warning("input_io_stop method not implemented");
}

enum {
  PROP_0
};


static void
set_property(GObject *object, guint property_id,
	     const GValue *value, GParamSpec *pspec)
{
  SoundIOCommon *io = SOUND_IO_COMMON(object);
  switch(property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
get_property(GObject *object, guint property_id,
	     GValue *value, GParamSpec *pspec)
{
  SoundIOCommon *io = SOUND_IO_COMMON(object);
  switch(property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
sound_io_common_class_init(gpointer g_class, gpointer class_data)
{
  SoundIOCommonClass *io_class = SOUND_IO_COMMON_CLASS(g_class);
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  gobject_class->finalize = sound_io_common_finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  
  io_class->output_io_start = output_io_start;
  io_class->output_io_stop = output_io_stop;
  io_class->input_io_start = input_io_start;
  io_class->input_io_stop = input_io_stop;
  
}

GType
sound_io_common_get_type()
{
  static GType object_type = 0;

  if (!object_type) {
    static const GTypeInfo object_info =
      {
	sizeof(struct _SoundIOCommonClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	
	sound_io_common_class_init, /* class_init */
	NULL, /* class_finalize */
	NULL, /* class_data */
	
	sizeof(struct _SoundIOCommon),
	0, /* n_preallocs */
	sound_io_common_init, /* instance_init */
	
	NULL, /* value_table */
      };

    
    object_type = g_type_register_static (G_TYPE_OBJECT, "SoundIOCommon",
					  &object_info, 0);
    
  }
  return object_type;
}

/* Source handling */

void
sound_io_common_stop_source(SoundIOCommon *io)
{
#ifdef STARTSTOP_DEBUG
    LogF(io->logger, LOG_DEBUG, "Source stopped");
#endif
}

void
sound_io_common_start_source(SoundIOCommon *io)
{
#ifdef STARTSTOP_DEBUG
    LogF(io->logger, LOG_DEBUG, "Source started");
#endif
}




/* Sink handling */
void
sound_io_common_stop_sink(SoundIOCommon *io)
{
#ifdef STARTSTOP_DEBUG
  LogF(io->logger, LOG_DEBUG, "Sink stopped");
#endif
}

void
sound_io_common_start_sink(SoundIOCommon *io)
{
#ifdef STARTSTOP_DEBUG
    LogF(io->logger, LOG_DEBUG, "Sink started");
#endif
}



