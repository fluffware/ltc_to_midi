#include <LTC_reader.h>
#include <malloc.h>
#include <audio_source.h>

#define BUFFER_SIZE 5

#define TC_SAMPLE_TIME_SCALE 4	/* number of bits used for fractional
				   sample intervals */
#define TC_SAMPLE_TIME (1<< TC_SAMPLE_TIME_SCALE)

#define TC_MAX_INTERVAL (1000 * TC_SAMPLE_TIME)	/*maximum acceptable interval*/
#define TC_MIN_INTERVAL (1 *TC_SAMPLE_TIME)	/*minimum acceptable interval*/

#define MAX_UNSYNCED 10

#define MIN_FRAME_WRAPS 3

static const unsigned int sync_word = 0xbffc;

struct _LTCReader
{
  GObject parent_instance;
  AudioSource *source;
  Sample *buffer;
  FrameLength buffer_capacity;
  FrameLength buffer_length;
  Channels channels;
  Channel use_channel;
  FrameLength sample_rate;

  /* Decoder state. */
  int prev_sample;
  int bit_interval;
  int bit_interval_adjust;
  gboolean half_bit;
  guint32 sync_buffer; /* the last  bits received so for, used for syncing */
  guint16 codeword_buffer[BUFFER_SIZE];
  guint16 *codeword_pos;
  /* samples since the previous transition * SAMPLE_TIME_SCALE */
  unsigned int prev_transition;
  guint16 frame_rate;
  guint16 out_of_sync_count;
  guint16 bit_count;	/* Incremented for every bit received */
  guint8 prev_frame;	/* Frame part of previous codeword. */
  guint8 pending_frame_rate;	/* New frame rate awaiting minimum number
				   of wraps before being accepted. */
  guint8 frame_wrap;		/* Number of wraps left before accepting
				   pending frame rate. */
  TimePosition local_tc;
  TimePosition last_tc;
  GTimeVal last_start;
  
  UserBits user_bits;

  GTimeVal frame_start; /* Start of frame currentl being decoded. */

  gboolean tc_is_valid;
};

struct _LTCReaderClass
{
  GObjectClass parent_class;

  /* Signals */
  void (*frame_rate_changed)(LTCReader *, guint frame_rate);
  void (*valid)(LTCReader *, gboolean valid);
};

static gpointer    parent_class = NULL;

static gboolean
tcdecode(LTCReader *reader, const Sample **datap, FrameLength *datalenp,
	 guint16 *codeword);

enum {
  PROP_0,
  PROP_BUFFER_LEN,
  PROP_USE_CHANNEL,
  PROP_FRAME_RATE
};


static gboolean
init_buffer(LTCReader *reader)
{
  if (reader->buffer) return TRUE;
  reader->buffer =
    g_malloc(sizeof(Sample) * reader->channels * reader->buffer_capacity);
  return TRUE;
}

static void
clear_buffer(LTCReader *reader)
{
  if (reader->buffer) {
    g_free(reader->buffer);
    reader->buffer = NULL;
  }
  if (reader->use_channel >= reader->channels)
    reader->use_channel = reader->channels - 1;
}

static void
set_property(GObject *object, guint property_id,
	     const GValue *value, GParamSpec *pspec)
{
  LTCReader *reader = LTC_READER(object);
  switch(property_id) {
   case PROP_BUFFER_LEN:
    reader->buffer_capacity = g_value_get_uint(value);
    clear_buffer(reader);
    break;
  case PROP_USE_CHANNEL:
    reader->use_channel = g_value_get_uint(value);
    clear_buffer(reader);
    break;
  case PROP_FRAME_RATE:
    reader->frame_rate = g_value_get_uint(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
get_property(GObject *object, guint property_id,
	     GValue *value, GParamSpec *pspec)
{
  LTCReader *reader = LTC_READER(object);
  switch(property_id) {
  case PROP_BUFFER_LEN:
    g_value_set_uint(value, reader->buffer_capacity);
    break;
  case PROP_USE_CHANNEL:
    g_value_set_uint(value, reader->use_channel);
    break;
  case PROP_FRAME_RATE:
    g_value_set_uint(value, reader->frame_rate);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
LTC_reader_finalize (GObject *gobject)
{
  LTCReader *reader = LTC_READER(gobject);
  if (reader->source) {
    AUDIO_SOURCE_GET_CLASS(reader->source)->disconnected(reader->source);
    reader->source = NULL;
  }
  clear_buffer(reader);
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
LTC_reader_init(GTypeInstance *instance, gpointer g_class)
{
  LTCReader *reader = LTC_READER(instance);
  reader->source = NULL;
  
  reader->buffer_capacity = 16384;
  reader->buffer = NULL;
  reader->channels = 2;
  reader->use_channel = 0;
  reader->sample_rate = 44100;

  reader->prev_sample = 0;
  reader->prev_transition = ~0;
  reader->bit_interval = TC_MAX_INTERVAL;
  reader->bit_interval_adjust = 0;
  reader->half_bit = FALSE;
  reader->sync_buffer = 0x10000;
  reader->codeword_pos = reader->codeword_buffer;
  reader->frame_rate = 25;
  reader->out_of_sync_count = 0;
  reader->local_tc = 0L;
  reader->bit_count = 0;
  reader->prev_frame = 0;
  reader->prev_frame = 0;
  reader->pending_frame_rate = 0;
  reader->frame_wrap = MIN_FRAME_WRAPS;

  reader->last_tc = 0L;
  reader->user_bits = 0L;
  g_get_current_time(&reader->frame_start);
  reader->last_start = reader->frame_start;
  
  reader->tc_is_valid = FALSE;
}

static void
LTC_reader_class_init(gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  gobject_class->finalize = LTC_reader_finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_signal_new ("frame-rate-changed",
                G_OBJECT_CLASS_TYPE (g_class),
                0,
                G_STRUCT_OFFSET (LTCReaderClass, frame_rate_changed),
                NULL, NULL,
                g_cclosure_marshal_VOID__UINT,
                G_TYPE_NONE, 1, G_TYPE_UINT);

  g_signal_new ("valid",
                G_OBJECT_CLASS_TYPE (g_class),
                0,
                G_STRUCT_OFFSET (LTCReaderClass, valid),
                NULL, NULL,
                g_cclosure_marshal_VOID__BOOLEAN,
                G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

  

  g_object_class_install_property(gobject_class, PROP_BUFFER_LEN,
				  g_param_spec_uint("buffer-len",
						    "Buffer Length",
						    "Length of decoding"
						    " buffer",
						    0,65536, 16384,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_USE_CHANNEL,
				  g_param_spec_uint("use-channel",
						    "Use Channel",
						    "Channel used for "
						    "decoding",
						    0,1,0,
						    G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_FRAME_RATE,
				  g_param_spec_uint("frame-rate",
						    "Frame Rate",
						    "LTC frame rate "
						    "(i.e. 24, 25 or 30)",
						    0,30,25,
						    G_PARAM_READWRITE));

}

/* TimecodeSource */
static guint16
tc_source_frame_rate(TimecodeSource *source)
{
  LTCReader *reader = LTC_READER(source);
  return reader->frame_rate;
}

static void
tc_source_timecode(TimecodeSource *source, TimePosition *tc, GTimeVal *when)
{
  LTCReader *reader = LTC_READER(source);
  *tc = reader->last_tc;
  *when = reader->frame_start;
}

static void
tc_source_init(gpointer g_iface, gpointer iface_data)
{
  TimecodeSourceClass *tc_source_class = g_iface;
  tc_source_class->frame_rate = tc_source_frame_rate;
  tc_source_class->timecode = tc_source_timecode;
}

/* UserbitsSource */

static UserBits
tc_source_userbits(UserbitsSource *source)
{
  LTCReader *reader = LTC_READER(source);
  return reader->user_bits;
}

static void
ub_source_init(gpointer g_iface, gpointer iface_data)
{
  UserbitsSourceClass *ub_source_class = g_iface;
  ub_source_class->userbits = tc_source_userbits;
}

/* Audio sink */
static gboolean
sink_has_buffer(AudioSink *sink)
{
  return TRUE;
}

static void
tc_invalid(LTCReader *reader)
{
  if (reader->tc_is_valid) {
    g_signal_emit_by_name(reader,"valid", FALSE);
    reader->tc_is_valid = FALSE;
  }
}

static void
tc_valid(LTCReader *reader)
{
  if (!reader->tc_is_valid) {
    g_signal_emit_by_name(reader,"valid", TRUE);
    reader->tc_is_valid = TRUE;
  }
}

/* delay is how many samples that are buffer between the start of the buffer
   and the input. */
static void
decode_frames(LTCReader *reader, const Sample *buffer, FrameLength frames,
	      FrameLength delay)
{
  GTimeVal now;
  const Sample *pos = buffer;
  /* g_message("Processing %d frames", length);*/
  g_get_current_time(&now);
  while(frames > 0) {
    guint16 codeword[BUFFER_SIZE];
    if (tcdecode(reader, &pos, &frames, codeword)) {
      FrameLength offset;
      GTimeVal when;

      TimePosition tc;
      unsigned int frame,sec,min,hour;

      /* Make sure the codewords are back to back */
      if (reader->bit_count != 80) {
	tc_invalid(reader);
	reader->bit_count = 0;
	continue;
      } else {
	tc_valid(reader);
      }
      
      reader->bit_count = 0;
      frame =(codeword[0] & 0x0f) + (codeword[0]>>8 & 0x03) * 10;
      sec = (codeword[1] & 0x0f) + (codeword[1]>>8 & 0x07) * 10;
      min = (codeword[2] & 0x0f) + (codeword[2]>>8 & 0x07) * 10;
      hour = (codeword[3] & 0x0f) + (codeword[3]>>8 & 0x03) * 10;
      /* g_message("%02d:%02d:%02d:%02d", hour, min, sec, frame);  */
      /* g_message("last: %d", reader->last_sync); */
      /* g_message("%04x %04x %04x %04x %04x", codeword[0], codeword[1],
	 codeword[2], codeword[3], codeword[4]); */

      if (frame == 0 && reader->prev_frame + 1 != reader->frame_rate
	  && reader->prev_frame != 0) {
	if (reader->prev_frame + 1 == reader->pending_frame_rate) {
	  if (--reader->frame_wrap == 0) {
	    reader->frame_rate = reader->pending_frame_rate;
	    /* g_message("Frame rate %d", reader->frame_rate);*/
	    g_signal_emit_by_name(reader,"frame-rate-changed",
				  reader->frame_rate);
	  }
	} else {
	  reader->pending_frame_rate = reader->prev_frame + 1;
	  reader->frame_wrap = MIN_FRAME_WRAPS;
	}
      }
      reader->prev_frame = frame;
      tc = ((((hour *60 + min) * 60 +sec) * reader->frame_rate + frame )
	    * TIME_POSITION_FRACTIONS / reader->frame_rate);
      
      
      if (reader->out_of_sync_count > MAX_UNSYNCED) {
	reader->local_tc = tc;
	reader->out_of_sync_count = 0;
      } else if (tc - reader->local_tc > TIME_POSITION_FRACTIONS) {
	reader->local_tc += TIME_POSITION_FRACTIONS;
	reader->out_of_sync_count++;
      } else if (reader->local_tc - tc > TIME_POSITION_FRACTIONS) {
	reader->local_tc =- TIME_POSITION_FRACTIONS;
	reader->out_of_sync_count++;
      } else {
	guint g;
	UserBits user_bits = 0;
	  
	reader->local_tc = tc;
	reader->last_tc = tc;
	reader->last_start = reader->frame_start;
	reader->out_of_sync_count = 0;

	/* Decode user bits */
	for (g = 0; g < 4; g++) {
	  user_bits = ((user_bits >> 8)
		       | (((guint32)codeword[g] & 0x00f0) << 20)
		       | (((guint32)codeword[g] & 0xf000) << 16));
	}
	
	if (user_bits != reader->user_bits) {
	  reader->user_bits = user_bits;
	  g_signal_emit_by_name(reader, "userbits-changed", user_bits, NULL);
	}
	
	g_signal_emit_by_name(reader, "timecode-changed", tc, &reader->frame_start,NULL);

	
      }
      /* Calculate when the sample at pos was sampled. */
      offset = delay - (pos - buffer) / reader->channels;
      when = now;
      /*g_message("Offset %d", offset);*/
      when.tv_sec -= offset / reader->sample_rate;
      when.tv_usec -=
	(offset % reader->sample_rate) * 1e6 / (gdouble)reader->sample_rate;
      if (when.tv_usec < 0) {
	when.tv_sec--;
	when.tv_usec += 1000000;
      }
      reader->frame_start = when;
    }
    if (reader->prev_transition >= TC_MAX_INTERVAL) {
      /* No transitions for a long while */
      tc_invalid(reader);
    }
  }
}

static FrameLength
sink_write(AudioSink *sink, const Sample *buffer, FrameLength length)
{
  LTCReader *reader = LTC_READER(sink);
  g_assert(reader->source != NULL);
  if (length == 0) return 0;
/*   g_message("sink_write: %d",length); */
  decode_frames(reader, buffer, length,
		audio_source_pipe_length(reader->source));
  return length;
}

static void
sink_request_read(AudioSink *sink, FrameLength length)
{
  FrameLength frames_read;
  LTCReader *reader = LTC_READER(sink);
  g_assert(reader->source != NULL);
  if (!init_buffer(reader)) {
    return;
  }
  if (length > reader->buffer_capacity) length = reader->buffer_capacity;
  frames_read =audio_source_read(reader->source,
				 reader->buffer, reader->buffer_capacity);
  if (frames_read > 0) {
    decode_frames(reader, reader->buffer,  frames_read,
		  audio_source_pipe_length(reader->source) + frames_read);
  }
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
  LTCReader *reader = LTC_READER(sink);
  if (reader->source) {
    AUDIO_SOURCE_GET_CLASS(reader->source)->disconnected(reader->source);
  }
  reader->source = source;
  if (source) {
    reader->channels = audio_source_channels(source);
    clear_buffer(reader);
    reader->sample_rate = audio_source_sample_rate(source);
  }
}

static void
sink_disconnected(AudioSink *sink)
{
  LTCReader *reader = LTC_READER(sink);
  reader->source = NULL;
}
  
static void
sink_init(gpointer g_iface, gpointer iface_data)
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

GType
LTC_reader_get_type()
{
  static GType object_type = 0;

  if (!object_type) {
    static const GTypeInfo object_info =
      {
	sizeof(struct _LTCReaderClass),
	NULL, /* base_init */
	NULL, /* base_finalize */
	
	LTC_reader_class_init, /* class_init */
	NULL, /* class_finalize */
	NULL, /* class_data */
	
	sizeof(struct _LTCReader),
	0, /* n_preallocs */
	LTC_reader_init, /* instance_init */
	
	NULL, /* value_table */
      };
    static const GInterfaceInfo tc_source_info =
      {
	tc_source_init,
	NULL,
	NULL
      };
    static const GInterfaceInfo ub_source_info =
      {
	ub_source_init,
	NULL,
	NULL
      };
    static const GInterfaceInfo sink_info =
      {
	sink_init,
	NULL,
	NULL
      };

    object_type = g_type_register_static (G_TYPE_OBJECT, "LTCReader",
					  &object_info, 0);
    g_type_add_interface_static (object_type, G_TYPE_TIMECODE_SOURCE,
				 &tc_source_info);
    g_type_add_interface_static (object_type, G_TYPE_USERBITS_SOURCE,
				 &ub_source_info);

    g_type_add_interface_static (object_type, G_TYPE_AUDIO_SINK,
				 &sink_info);
  }
  return object_type;
}

/**
 * LTC_reader_new:
 * @Returns: LTCReader
 *
 * Creates a new #LTCReader.
 **/
 
LTCReader *
LTC_reader_new()
{
  LTCReader *reader = g_object_new(G_TYPE_LTC_READER, NULL);
  return reader;
}

UserBits
LTC_reader_user_bits(LTCReader *reader)
{
  return reader->user_bits;
}

static gboolean
tcdecode(LTCReader *reader, const Sample **datap, FrameLength *datalenp,
	 guint16 *codeword)
{
  unsigned int offset;
  unsigned int step = reader->channels;
  const Sample *data = *datap += reader->use_channel;
  const Sample *dataend = data + *datalenp * reader->channels;
  while(TRUE) {
    const Sample *datastart = data;
    if (reader->prev_sample >= 0) {
      /* search for a transition from positive to negative values */
      while(data != dataend && *data >= 0) data += step;
    } else {
      /* search for a transition from negative to positive values */
      while(data != dataend && *data < 0) data += step;
    }
    reader->prev_transition += TC_SAMPLE_TIME * (data - datastart) / step;
    if (data == dataend) {
      reader->prev_sample = data[-(int)step];
      *datap = data;
      *datalenp = 0;
      return FALSE;
    }
    if (data != datastart) reader->prev_sample = data[-(int)step];
    
    
    offset = (TC_SAMPLE_TIME * ABS(reader->prev_sample)) / ABS(reader->prev_sample - data[0]);
    reader->prev_transition += offset;
    reader->prev_sample = *data;
    data += step;

    /*fprintf(stderr,"%d %d\n",reader->prev_transition,reader->bit_interval);*/
    if ((4 * reader->prev_transition) < 3 * reader->bit_interval) {
      /* It's a half interval */
      
      /* Adjust interval if too long */
      if ((reader->prev_transition * 2 + 1)  < reader->bit_interval) {
	if (reader->bit_interval_adjust > 0)
	  reader->bit_interval_adjust = - 1;
	else
	  reader->bit_interval_adjust--;
	reader->bit_interval += reader->bit_interval_adjust;
	if (reader->bit_interval < TC_MIN_INTERVAL)
	  reader->bit_interval = TC_MIN_INTERVAL;
      } else
	reader->bit_interval_adjust = 0;
      
      if (reader->half_bit) {
	reader->half_bit = FALSE;
	if (reader->sync_buffer & 0x0001) {
	  reader->sync_buffer >>= 1;
	  *(reader->codeword_pos)++ = reader->sync_buffer;
	  if (reader->codeword_pos - reader->codeword_buffer == BUFFER_SIZE)
	    reader->codeword_pos = reader->codeword_buffer;
	  reader->sync_buffer =
	    (reader->sync_buffer & 0xffff0000) | 0x80008000;
	} else {
	  reader->sync_buffer = (reader->sync_buffer>>1)  | 0x80000000;
	}
	reader->bit_count++;
      } else
	reader->half_bit = TRUE;
    } else {
      /* It's a full interval */
      
      /* Adjust interval if too short */
      if (reader->prev_transition > reader->bit_interval + 1) {
	if (reader->bit_interval_adjust < 0)
	  reader->bit_interval_adjust = 1;
	else
	  reader->bit_interval_adjust++;
	reader->bit_interval += reader->bit_interval_adjust;
      if (reader->bit_interval > TC_MAX_INTERVAL)
	reader->bit_interval = TC_MAX_INTERVAL;
      }
      else
	reader->bit_interval_adjust = 0;
      
      reader->half_bit = FALSE;
      if (reader->sync_buffer & 0x0001) {
	reader->sync_buffer >>= 1;
	*(reader->codeword_pos)++ = reader->sync_buffer;
	if (reader->codeword_pos - reader->codeword_buffer == BUFFER_SIZE)
	  reader->codeword_pos = reader->codeword_buffer;
	reader->sync_buffer = (reader->sync_buffer & 0xffff0000) | 0x8000;
      } else {
	reader->sync_buffer >>= 1;
      }
      reader->bit_count++;
    }
    
    reader->prev_transition = TC_SAMPLE_TIME - offset;

    if ((reader->sync_buffer >> 16) == sync_word) {
      /* Found a syncword */
      int i;
      
      unsigned int shift =  1;
      guint32 mask;
      unsigned int carry;
      codeword[BUFFER_SIZE - 1] = reader->sync_buffer >> 16;
      carry = reader->sync_buffer;
      mask = 0x0001;
      while(!(carry & mask)) {
	mask <<= 1;
	shift++;
      }
      mask <<= 1;
      mask--;
      carry &= ~mask;
      for (i= BUFFER_SIZE - 2; i >=0 ; i--) {
	if (reader->codeword_pos == reader->codeword_buffer) {
	  reader->codeword_pos = reader->codeword_buffer + BUFFER_SIZE - 1;
	} else {
	  --(reader->codeword_pos);
	}
	codeword[i] = carry | ((*reader->codeword_pos) >> (16 - shift));
	carry = *reader->codeword_pos << shift;
      }
      /* Clear sync word. */
      reader->sync_buffer &= 0xffff;
      *datap = data - reader->use_channel; /* Align with frame. */
      *datalenp = (dataend - data) / reader->channels;
      return TRUE;
    }
  }
}
