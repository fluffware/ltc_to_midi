#ifndef __SOUNDIO_COMMON_H__
#define __SOUNDIO_COMMON_H__

#include <soundio.h>
#include <glib-object.h>

typedef struct _SoundIOCommonClass SoundIOCommonClass;
typedef struct _SoundIOCommon SoundIOCommon;

#define G_TYPE_SOUND_IO_COMMON (sound_io_common_get_type())
#define SOUND_IO_COMMON(object)  \
(G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_SOUND_IO_COMMON, SoundIOCommon))
#define SOUND_IO_COMMON_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_SOUND_IO_COMMON, SoundIOCommonClass)

#define IS_SOUND_IO_COMMON(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_SOUND_IO_COMMON)

#define IS_SOUND_IO_COMMON_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_SOUND_IO_COMMON)

#define SOUND_IO_COMMON_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_CLASS((object), G_TYPE_SOUND_IO_COMMON, SoundIOCommonClass)

#define SOUND_IO_COMMON_TYPE(object) (G_TYPE_FROM_INSTANCE (object))
#define SOUND_IO_COMMON_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))


struct _SoundIOCommon
{
  GObject parent_instance;

  /* Output */
  AudioSource *source;
  FrameLength output_threshold;	/* Minimum number of frames available
				   before starting output. */
  gboolean output_running;
  
  /* Input */
  AudioSink *sink;
  FrameLength input_threshold;	/* Minimum amount of space available in sink
				   before starting input. */
  gboolean input_running;
  
  FrameLength sample_rate;
  Channels channels;
  gboolean flushing;
};

struct _SoundIOCommonClass
{
  GObjectClass parent_class;

  /* Start reading from source. */
  void (*output_io_start)(SoundIOCommon *io);
  /* Stop reading from source. */
  void (*output_io_stop)(SoundIOCommon *io);

  /* Start writing to sink. */
  void (*input_io_start)(SoundIOCommon *io);
  /* Stop writing to sink. */
  void (*input_io_stop)(SoundIOCommon *io);

  
};

GType
sound_io_common_get_type();

FrameLength
sound_io_common_sample_rate(SoundIO *io);

void
sound_io_common_stop_source(SoundIOCommon *io);

void
sound_io_common_start_source(SoundIOCommon *io);

void
sound_io_common_stop_sink(SoundIOCommon *io);

void
sound_io_common_start_sink(SoundIOCommon *io);


/* Interface initialization and finalization functions. */

/* AudioSource */
void
sound_io_common_audio_source_init(gpointer g_iface, gpointer iface_data);

void
sound_io_common_audio_source_finalize(gpointer g_iface, gpointer iface_data);

/* AudioSink */
void
sound_io_common_audio_sink_init(gpointer g_iface, gpointer iface_data);

void
sound_io_common_audio_sink_finalize(gpointer g_iface, gpointer iface_data);

/* SoundIO */

void
sound_io_common_sound_io_init(gpointer g_iface, gpointer iface_data);

void
sound_io_common_sound_io_finalize(gpointer g_iface, gpointer iface_data);

#endif /* __SOUNDIO_COMMON_H__ */
