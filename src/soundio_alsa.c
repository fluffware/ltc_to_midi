#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <soundio_alsa.h>
#include <soundio_common.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>
#include <alsa/asoundlib.h>

#include <msglogger/msg_logger.h>

#ifdef VALGRIND
#include <valgrind.h>
#endif

#if (SND_LIB_MAJOR >= 1)
#define ALSA_PCM_NEW_HW_PARAMS_API
#endif
     
#define STARTSTOP_DEBUG

#define MAX_POLLFD 4

typedef struct _StreamInfo
{
  snd_pcm_t *pcm;
  struct pollfd fds[MAX_POLLFD];
  unsigned int nfds;
  GSource *io_source;
  guint io_source_id;
  /* Preferred size for doing I/O */
  snd_pcm_uframes_t io_size;
} StreamInfo;

struct _SoundIOAlsa
{
  SoundIOCommon parent_instance;

  StreamInfo output;
  StreamInfo input;
};

struct _SoundIOALSAClass
{
  SoundIOCommonClass parent_class;
};



static gpointer    parent_class = NULL;

static FrameLength
source_pipe_length(AudioSource *source)
{
  SoundIOAlsa *alsa = SOUND_IO_ALSA(source);
  snd_pcm_sframes_t delay = 0;
  if (snd_pcm_state(alsa->input.pcm) == SND_PCM_STATE_RUNNING) {
    int err;
    err = snd_pcm_delay(alsa->input.pcm, &delay);
    if (err < 0) {
      g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			"MESSAGE", 
			"Failed to get ALSA input buffer delay: %s",
			snd_strerror(err));
      return 0;
    }
  }
  if (delay < 0) return 0;  
  return delay;
}

static FrameLength
sink_pipe_length(AudioSink *sink)
{
  SoundIOAlsa *alsa = SOUND_IO_ALSA(sink);
  snd_pcm_sframes_t delay = 0;
  if (snd_pcm_state(alsa->output.pcm) == SND_PCM_STATE_RUNNING) {
    int err;
    err = snd_pcm_delay(alsa->output.pcm, &delay);
    if (err < 0) {
      g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			"MESSAGE", 
			"Failed to get ALSA output buffer delay: %s",
			snd_strerror(err));
      return 0;
    }
  }
  if (delay < 0) return 0;  
  return delay;
}

static gboolean
add_file_handlers(StreamInfo *stream)
{
  unsigned int p;
  /* Assumes struct pollfd and GPollFD are the same. */
  for(p = 0; p < stream->nfds; p++) {
    g_source_add_poll(stream->io_source, (GPollFD*)&stream->fds[p]);
  }
  return TRUE;
}

static gboolean
remove_file_handlers(StreamInfo *stream)
{
  unsigned int p;
  /* Assumes struct pollfd and GPollFD are the same. */
  for(p = 0; p < stream->nfds; p++) {
    g_source_remove_poll(stream->io_source, (GPollFD*)&stream->fds[p]);
  }
  return TRUE;
}

static void
output_io_start(SoundIOCommon *io)
{
  if (!io->output_running) {
    SoundIOAlsa *alsa = SOUND_IO_ALSA(io);
    if (!alsa->output.io_source) return;
    add_file_handlers(&alsa->output);
    io->output_running = TRUE;
#ifdef STARTSTOP_DEBUG
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "MESSAGE",
		      "ALSA output started");
#endif
  }
}

static void
output_io_stop(SoundIOCommon *io)
{
  if (io->output_running) {
    SoundIOAlsa *alsa = SOUND_IO_ALSA(io);
    if (!alsa->output.io_source) return;
    remove_file_handlers(&alsa->output);
    io->output_running = FALSE;
#ifdef STARTSTOP_DEBUG
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "MESSAGE", 
		      "ALSA output stopped");
#endif
  }
}


static void
input_io_start(SoundIOCommon *io)
{
  if (!io->input_running) {
    SoundIOAlsa *alsa = SOUND_IO_ALSA(io);
    if (!alsa->input.io_source) return;
    add_file_handlers(&alsa->input);
    io->input_running = TRUE;
#ifdef STARTSTOP_DEBUG
     g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "MESSAGE", 
		       "ALSA input started");
#endif
  }
}

static void
input_io_stop(SoundIOCommon *io)
{
  if (io->input_running) {
    SoundIOAlsa *alsa = SOUND_IO_ALSA(io);
    if (!alsa->input.io_source) return;
    remove_file_handlers(&alsa->input);
    io->input_running = FALSE;
#ifdef STARTSTOP_DEBUG
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "MESSAGE",
		      "ALSA input stopped");
#endif
  }
}

static gboolean
close_stream(StreamInfo *stream)
{
  if (stream->io_source_id) {
    g_source_remove(stream->io_source_id);
    stream->io_source_id = 0;
  }
  if (stream->io_source) {
    g_source_unref(stream->io_source);
    stream->io_source = NULL;
  }
  if (stream->pcm) {
    int err;
    snd_pcm_drain(stream->pcm);
    err = snd_pcm_close(stream->pcm);
    if (err != 0) return FALSE;
    stream->pcm = NULL;
  }
  return TRUE;
}

static void
sound_io_alsa_finalize(GObject *gobject)
{
  SoundIOAlsa *alsa = SOUND_IO_ALSA(gobject);
  if (!close_stream(&alsa->output)) {
     g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		       "Failed to close ALSA output stream");
  }
  if (!close_stream(&alsa->input)) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Failed to close ALSA input stream");
  }
  
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
init_stream(StreamInfo *stream)
{
  stream->nfds = 0;
  stream->io_source = NULL;
  stream->io_source_id = 0;
  stream->pcm = NULL;
  stream->io_size = 0;
}

static void
sound_io_alsa_init(GTypeInstance *instance, gpointer g_class)
{
  SoundIOAlsa *alsa = SOUND_IO_ALSA(instance);
  init_stream(&alsa->input);
  init_stream(&alsa->output);
}

static FrameLength
source_read(AudioSource *source, Sample *buffer,FrameLength length);

static FrameLength
source_pipe_length(AudioSource *source);

static void
sound_io_alsa_source_init(gpointer g_iface, gpointer iface_data)
{
  AudioSourceClass *source_class = g_iface;
  sound_io_common_audio_source_init(g_iface, iface_data);
  source_class->read = source_read;
  source_class->pipe_length = source_pipe_length;
}

static FrameLength
sink_write(AudioSink *sink, const Sample *buffer,FrameLength length);

static FrameLength
sink_pipe_length(AudioSink *sink);

static void
sound_io_alsa_sink_init(gpointer g_iface, gpointer iface_data)
{
  AudioSinkClass *sink_class = g_iface;
  sound_io_common_audio_sink_init(g_iface, iface_data);
  sink_class->write = sink_write;
  sink_class->pipe_length = sink_pipe_length;
}

static void
sound_io_alsa_class_init(gpointer g_class, gpointer class_data)
{
  SoundIOCommonClass *io_class = SOUND_IO_COMMON_CLASS(g_class);
  GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
  parent_class = g_type_class_ref (G_TYPE_SOUND_IO_COMMON);
  gobject_class->finalize = sound_io_alsa_finalize;

  io_class->output_io_start = output_io_start;
  io_class->output_io_stop = output_io_stop;
  io_class->input_io_start = input_io_start;
  io_class->input_io_stop = input_io_stop;

}


static void
sound_io_alsa_sound_io_init(gpointer g_iface, gpointer iface_data)
{
  SoundIOClass *sound_io_class = g_iface;
  sound_io_common_sound_io_init(g_iface, iface_data);
  sound_io_class->sample_rate = sound_io_common_sample_rate;
}



static int
xrun_recovery(snd_pcm_t *pcm, int err)
{
  if (err == -EPIPE) {    /* underrun */
    err = snd_pcm_prepare(pcm);
    if (err < 0)
      g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
			"MESSAGE",
			"Can't recover from under- or overrun, "
			"prepare failed: %s\n",
			snd_strerror(err));
    if (snd_pcm_stream(pcm) == SND_PCM_STREAM_CAPTURE) {
      /* Needs to be restarted explicitly since the read that otherwise
	 would have started it might never be called if using a passive
	 sink. */
      err = snd_pcm_start(pcm);
      if (err < 0)
	g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			    "MESSAGE",
			  "Can't recover from under- or overrun, "
			  "start failed: %s\n",
			  snd_strerror(err));
    }
    return 0;
  } else if (err == -ESTRPIPE) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "MESSAGE",
		      "Suspend");
    while ((err = snd_pcm_resume(pcm)) == -EAGAIN)
      sleep(1);       /* wait until suspend flag is released */
    if (err < 0) {
      err = snd_pcm_prepare(pcm);
      if (err < 0)
	g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			  "MESSAGE",
			  "Can't recover from suspend, prepare failed: %s\n",
			  snd_strerror(err));
    }
    return 0;
  }
  return err;
}



static void
output_io_handler(SoundIOAlsa *alsa)
{
  int count;
  unsigned short revents;
  g_assert(alsa->parent_instance.source != NULL);
  count = snd_pcm_poll_descriptors_revents(alsa->output.pcm,
					   alsa->output.fds,
					   alsa->output.nfds, &revents);
  if (count < 0) {
    
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "MESSAGE", 
		      "Receiving (output-)event failed: %s",
		      snd_strerror(count));
    output_io_stop(&alsa->parent_instance);
    return;
  }
  
  if (revents & POLLOUT) {
    if (alsa->parent_instance.source) {
      snd_pcm_sframes_t available;
      available = snd_pcm_avail_update(alsa->output.pcm);
      if (available > 0) {
	audio_source_request_write(alsa->parent_instance.source, available);
      } else if (available < 0) {
	if (available == -EPIPE) {
	    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
			      "MESSAGE", "Underrun");
	    g_signal_emit_by_name(alsa, "underrun",NULL);
	}
	if (xrun_recovery(alsa->output.pcm,
			  available)) {
	  g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
			    "MESSAGE",
			    "Failed to get available frames to write: %s",
			    snd_strerror(available));
	}
      }
    }
  }
}

static FrameLength
sink_write(AudioSink *sink, const Sample *buffer,FrameLength buffer_len)
{
  SoundIOAlsa *alsa = SOUND_IO_ALSA(sink);
  int count;
  Byte *pos = (Byte*)buffer;
  g_assert(alsa->output.pcm);
  if (buffer_len == 0) {
    output_io_stop(&alsa->parent_instance);
    sound_io_common_start_source(&alsa->parent_instance);
    return 0;
  }
  /* LogF(alsa->parent_instance.logger, LOG_DEBUG,
     "%d in buffer before writing", sink_pipe_length(sink));*/
  output_io_start(SOUND_IO_COMMON(alsa));


  count = snd_pcm_writei(alsa->output.pcm, pos , buffer_len);

  if (count == -EAGAIN) return 0;
  if (count < 0) {
    if (count == -EPIPE) {
      g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
			"MESSAGE",
			"Underrun");
      g_signal_emit_by_name(alsa, "underrun",NULL);
    }
    if (xrun_recovery(alsa->output.pcm, count) < 0) {
        g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
			  "MESSAGE",
			  "Failed to write samples: %s",
			  snd_strerror(count));
    }
    return 0;
  }
#if 0
  {
    GTimeVal now;
    g_get_current_time(&now);
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "MESSAGE",
		      "%lu.%lu Wrote %d frames (%d in buffer)",
		      now.tv_sec, now.tv_usec, count, sink_pipe_length(sink));
  }
#endif
  return count;
}

static void
input_io_handler(SoundIOAlsa *alsa)
{
  int count;
  unsigned short revents;
  g_assert(alsa->parent_instance.sink != NULL);
  count = snd_pcm_poll_descriptors_revents(alsa->input.pcm,
					   alsa->input.fds,
					   alsa->input.nfds, &revents);
  if (count < 0) {
     g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		       "MESSAGE",
		       "Receiving (input-)event failed: %s",
		       snd_strerror(count));
     input_io_stop(&alsa->parent_instance);
     return;
  }
  if (revents & POLLIN) {
    if (alsa->parent_instance.sink) {
      snd_pcm_sframes_t available;
      available = snd_pcm_avail_update(alsa->input.pcm);
      if (available > 0) {
	audio_sink_request_read(alsa->parent_instance.sink,
				available);
      } else if (available < 0) {
	if (available == -EPIPE) {
	  g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			    "MESSAGE",
			    "Overrun");
	  g_signal_emit_by_name(alsa, "overrun",NULL);
	}
	if (xrun_recovery( alsa->input.pcm,
			  available)) {
	   g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
			     "MESSAGE",
			     "Failed to get available frames to read: %s",
			     snd_strerror(available));
	}
	
      } else {
	snd_pcm_state_t state = snd_pcm_state(alsa->input.pcm);
	if (state == SND_PCM_STATE_PREPARED) {
	  snd_pcm_start(alsa->input.pcm);
	}
      }
    }
  }
}

static FrameLength
source_read(AudioSource *source, Sample *buffer,FrameLength buffer_len)
{
  SoundIOAlsa *alsa = SOUND_IO_ALSA(source);
  int count;
  Byte *pos = (Byte*)buffer;
  snd_pcm_uframes_t read_length;
  g_assert(alsa->input.pcm);
  if (buffer_len == 0) {
    input_io_stop(&alsa->parent_instance);
    sound_io_common_start_sink(&alsa->parent_instance);
    return 0;
  }
  input_io_start(&alsa->parent_instance);
  read_length = buffer_len;
  count = snd_pcm_readi(alsa->input.pcm, pos , read_length);
#ifdef VALGRIND
  VALGRIND_MAKE_READABLE(pos,( read_length * alsa->parent_instance.channels
			       * sizeof(Sample)));
#endif
  if (count < 0) {
    if (count == -EAGAIN) return 0;
    if (count == -EPIPE) {
      g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
			"MESSAGE","Overrun");
      g_signal_emit_by_name(alsa, "overrun",NULL);
    }
    if (xrun_recovery(alsa->input.pcm, count) < 0) {
      g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
			"MESSAGE",
			"Failed to read samples: %s",
			snd_strerror(count));
    }
    return 0;
  }
  /*LogF(alsa->parent_instance.logger, LOG_DEBUG, "Read %d frames", count);*/
  return count;
}


#ifdef WORDS_BIGENDIAN
#define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_BE
#else
#define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_LE
#endif

#define PERIOD_TIME 100000
#define BUFFER_TIME 1000000
static int
setup_hw_params(snd_pcm_t *pcm,
		unsigned int sample_rate, unsigned int channels,
		snd_pcm_uframes_t *period_size)
{
  snd_pcm_hw_params_t *params;
  int dir;
  int err;
  unsigned int buffer_time = BUFFER_TIME;
  unsigned int period_time = PERIOD_TIME;
  unsigned int requested_sample_rate = sample_rate;
  
  snd_pcm_hw_params_alloca(&params);
  err = snd_pcm_hw_params_any(pcm, params);
  if (err < 0) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Failed to get hardware parameters: %s",
		      snd_strerror(err));
    return err;
  }

  err = snd_pcm_hw_params_set_access(pcm, params,
				     SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Failed to set interleaved access: %s",
		      snd_strerror(err));
    return err;
  }
  
 err = snd_pcm_hw_params_set_format(pcm, params,
				    SND_PCM_FORMAT_S16_NE);
 if (err < 0) {
   g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		     "MESSAGE",
		     "Failed to set format (16bit signed): %s",
		     snd_strerror(err));
    return err;
 }


  err = snd_pcm_hw_params_set_channels(pcm, params, channels);

  if (err < 0) {
     g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		       "Failed to set number of channels (%d): %s",
		       channels, snd_strerror(err));
    return err;
  }

  dir = 0;
#ifdef ALSA_PCM_NEW_HW_PARAMS_API
  err = snd_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, &dir);
#else
  err = snd_pcm_hw_params_set_rate_near(pcm, params, sample_rate, &dir);
  sample_rate = err;
#endif
  if (err < 0) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Failed to set sample rate (%d): %s",
		      sample_rate, snd_strerror(err));
    return err;
  }
  if (requested_sample_rate != sample_rate) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Didn't get requested sample rate (%d, got %d)",
		      sample_rate, err);
    return -EINVAL;
  }

  dir = -1;
#ifdef ALSA_PCM_NEW_HW_PARAMS_API
  err = snd_pcm_hw_params_set_buffer_time_near(pcm, params, &buffer_time,&dir);
#else
  err = snd_pcm_hw_params_set_buffer_time_near(pcm, params, buffer_time,&dir);
#endif
  if (err < 0) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Failed to set buffer time (%d): %s", BUFFER_TIME,
		      snd_strerror(err));
    return err;
  }

  dir = 1;
#ifdef ALSA_PCM_NEW_HW_PARAMS_API
  err = snd_pcm_hw_params_set_period_time_near(pcm, params, &period_time,&dir);
#else  
  err = snd_pcm_hw_params_set_period_time_near(pcm, params, period_time,&dir);
#endif
  if (err < 0) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Failed to set period time (%d): %s", PERIOD_TIME,
		      snd_strerror(err));
    return err;
  }


  dir = 0;
  
#ifdef ALSA_PCM_NEW_HW_PARAMS_API
  snd_pcm_hw_params_get_period_size(params, period_size, &dir);
#else
  *period_size = snd_pcm_hw_params_get_period_size(params, &dir);
#endif
  err = snd_pcm_hw_params(pcm, params);
  if (err < 0) {
     g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		       "Failed to set hardware parameters: %s",
	 snd_strerror(err));
    return err;
  }
  
  
  return 0;
}

static int
setup_sw_params(snd_pcm_t *pcm, unsigned int avail_min)
{
  snd_pcm_sw_params_t *params;
  int err;

  snd_pcm_sw_params_alloca(&params);
  err = snd_pcm_sw_params_current(pcm, params);
  if (err < 0) {
     g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		       "Failed to get software parameters: %s",
		       snd_strerror(err));
    return err;
  }

  /* Start playing as soon as there is a full period available. */
  err = snd_pcm_sw_params_set_avail_min(pcm, params, avail_min);
  if (err < 0) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "Failed to set min transfer (%d): %s",
		      avail_min, snd_strerror(err));
    return err;
  }
  
  err = snd_pcm_sw_params(pcm, params);
  if (err < 0) {
     g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		       "Failed to set software parameters: %s",
		       snd_strerror(err));
    return err;
  }
  return 0;
}


typedef struct _ALSAIOSource ALSAIOSource;
struct _ALSAIOSource
{
  GSource source;
  SoundIOAlsa *alsa;
};

static gboolean
io_prepare(GSource *source, gint *timeout)
{
  *timeout = -1;
  return FALSE;
}

static gboolean
output_io_check(GSource *source)
{
  SoundIOAlsa *alsa = ((ALSAIOSource*)source)->alsa;
  guint p;
  for(p = 0; p < alsa->output.nfds; p++) {
    if (alsa->output.fds[p].events & alsa->output.fds[p].revents) {
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
input_io_check(GSource *source)
{
  SoundIOAlsa *alsa = ((ALSAIOSource*)source)->alsa;
  guint p;
  for(p = 0; p < alsa->input.nfds; p++) {
    if (alsa->input.fds[p].events & alsa->input.fds[p].revents) {
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
output_io_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  unsigned int p;
  SoundIOAlsa *alsa = ((ALSAIOSource*)source)->alsa;
  output_io_handler(alsa);
  for(p = 0; p < alsa->output.nfds; p++) {
    alsa->output.fds[p].revents = 0;
  }
  return TRUE;
}

static gboolean
input_io_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  unsigned int p;
  SoundIOAlsa *alsa = ((ALSAIOSource*)source)->alsa;
  input_io_handler(alsa);
  for(p = 0; p < alsa->input.nfds; p++) {
    alsa->input.fds[p].revents = 0;
  }
  return TRUE;
}

GType
sound_io_alsa_get_type()
{
  static GType object_type = 0;

  if (!object_type) {
    static const GTypeInfo object_info =
      {
	sizeof(struct _SoundIOALSAClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	
	sound_io_alsa_class_init, /* class_init */
	NULL, /* class_finalize */
	NULL, /* class_data */
	
	sizeof(struct _SoundIOAlsa),
	0, /* n_preallocs */
	sound_io_alsa_init, /* instance_init */
	
	NULL, /* value_table */
      };

    static const GInterfaceInfo sound_io_info =
      {
	sound_io_alsa_sound_io_init,
	sound_io_common_sound_io_finalize,
	NULL
      };
    
    static const GInterfaceInfo sink_info =
      {
	sound_io_alsa_sink_init,
	sound_io_common_audio_sink_finalize,
	NULL
      };
    
    static const GInterfaceInfo source_info =
      {
	sound_io_alsa_source_init,
	sound_io_common_audio_source_finalize,
	NULL
      };

    object_type = g_type_register_static (G_TYPE_SOUND_IO_COMMON,
					  "SoundIOAlsa",
					  &object_info, 0);

    g_type_add_interface_static(object_type, G_TYPE_SOUND_IO, &sound_io_info);
    g_type_add_interface_static(object_type, G_TYPE_AUDIO_SOURCE,&source_info);
    g_type_add_interface_static(object_type, G_TYPE_AUDIO_SINK, &sink_info);
  }
  return object_type;
}


static GSourceFuncs output_io_source_funcs =
  {
    io_prepare,
    output_io_check,
    output_io_dispatch,
    NULL
  };

static GSourceFuncs input_io_source_funcs =
  {
    io_prepare,
    input_io_check,
    input_io_dispatch,
    NULL
  };

static gboolean
alsa_setup(SoundIOAlsa *alsa, StreamInfo *stream_info,
	   const char *device, snd_pcm_stream_t stream, unsigned int channels)
{
  int err;
  
  err = snd_pcm_open(&stream_info->pcm, device, stream,
		     SND_PCM_NONBLOCK);
  if (err != 0) {
    g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_ERROR,
		      "MESSAGE",
		      "open (ALSA): %s", snd_strerror(err));
    return FALSE;
  }

  err = setup_hw_params(stream_info->pcm,
			alsa->parent_instance.sample_rate,
			channels, &stream_info->io_size);
  if (err != 0) {
    return FALSE;
  }


  /*
  {
    snd_output_t *output;
    snd_output_stdio_attach(&output, stderr, 0); 

    snd_pcm_dump_hw_setup (stream_info->pcm, output);
    

    snd_output_close(output);
    }*/
  
  err = setup_sw_params(stream_info->pcm,
			stream_info->io_size);
  if (err != 0) {
    return FALSE;
  }

  stream_info->nfds =
    snd_pcm_poll_descriptors(stream_info->pcm, stream_info->fds, MAX_POLLFD);
  
 


  
  return TRUE;
}

SoundIO *
sound_io_alsa_new(const char *device,
		  Channels channels, FrameLength sample_rate,
		  gboolean use_input, gboolean use_output )
{
  SoundIOAlsa *alsa = g_object_new(G_TYPE_SOUND_IO_ALSA,
				   NULL);

 
  
  alsa->parent_instance.sample_rate = sample_rate;

  alsa->parent_instance.channels = channels;

  if (use_output) {
    if (!alsa_setup(alsa, &alsa->output, device, SND_PCM_STREAM_PLAYBACK,
		    channels)){
      g_object_unref(alsa);
      return NULL;
    }
    alsa->output.io_source =
      g_source_new(&output_io_source_funcs, sizeof(ALSAIOSource));
    ((ALSAIOSource*)alsa->output.io_source)->alsa = alsa;
  
    alsa->output.io_source_id = g_source_attach(alsa->output.io_source, NULL);
    output_io_start(SOUND_IO_COMMON(alsa));
    sound_io_common_stop_source(SOUND_IO_COMMON(alsa));
  }
  if (use_input) {
    if (!alsa_setup(alsa, &alsa->input, device, SND_PCM_STREAM_CAPTURE,
		    channels)){
      g_object_unref(alsa);
      return NULL;
    }
    alsa->input.io_source =
      g_source_new(&input_io_source_funcs, sizeof(ALSAIOSource));
    ((ALSAIOSource*)alsa->input.io_source)->alsa = alsa;
    alsa->input.io_source_id = g_source_attach(alsa->input.io_source, NULL);
    input_io_start(SOUND_IO_COMMON(alsa));
    sound_io_common_stop_sink(SOUND_IO_COMMON(alsa));
    snd_pcm_start(alsa->input.pcm);		  
  }
  alsa->parent_instance.input_threshold = alsa->input.io_size;
  alsa->parent_instance.output_threshold = alsa->output.io_size;

  return SOUND_IO(alsa);
}
