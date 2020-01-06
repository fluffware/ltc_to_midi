#include <mtc_sender.h>
#include <sys/time.h>

struct _MTCSender
{
  GSource source;
  int nfds;
  struct pollfd poll_fds[4];
  snd_seq_t *seq;
  int port_id;
  int queue_id;
  snd_seq_port_info_t *port_info;
  snd_seq_client_info_t *client_info;
  snd_seq_real_time_t last_timestamp;
};

GQuark
mtc_sender_error_quark (void)
{
  static GQuark quark = 0;
  if (!quark) {
    quark = g_quark_from_static_string("MTCSender_Error");
  }
  return quark;
}
static gboolean
io_prepare  (GSource    *source, gint       *timeout)
{
  *timeout = -1;
  return FALSE;
}

static gboolean
io_check(GSource *source)
{
  unsigned short revents;
  MTCSender *io = (MTCSender*)source;
  snd_seq_poll_descriptors_revents(io->seq, io->poll_fds,
                                   io->nfds, &revents);
  return revents & POLLIN;
}

static gboolean
io_dispatch(GSource *source, GSourceFunc callback, gpointer user_data)
{
  
  snd_seq_event_t *event;
  int res;
  MTCSender *io = (MTCSender*)source;
  while ((res = snd_seq_event_input(io->seq, &event)) >= 0) {
    if (callback) {
      ((MTCSenderEventFunc*)callback)(io, event, user_data);
    }
  }
  return TRUE;
}
static GSourceFuncs io_source_funcs =
  {
    io_prepare,
    io_check,
    io_dispatch,
    NULL,
    NULL,
    NULL
  };

/*
static void
clear_subscription(snd_seq_t *seq, snd_seq_port_subscribe_t **subscr)
{
  if (*subscr != NULL) {
    snd_seq_unsubscribe_port(seq, *subscr);
    snd_seq_port_subscribe_free(*subscr);
    *subscr = NULL;
  }
  }

*/

MTCSender *
mtc_sender_new(const gchar *client_name, const gchar *port_name,
            GError **error)
{
  int res;
  MTCSender *io = (MTCSender*)g_source_new (&io_source_funcs, sizeof(MTCSender));
  unsigned int caps = 0;
  io->seq = NULL;
  io->port_info = NULL;
  io->client_info = NULL;
  io->nfds = 0;
  io->queue_id = -1;
  io->port_id = -1;
  res = snd_seq_open (&io->seq, "default",
                      SND_SEQ_OPEN_OUTPUT,
                      0);
  if (res < 0) {
    g_set_error(error, G_IO_CHANNEL_ERROR, G_IO_CHANNEL_ERROR_NXIO,
                "Failed to open sequencer: %d", res);
    mtc_sender_destroy(io);
    return NULL;
  }

  snd_seq_client_info_malloc(&io->client_info);
  snd_seq_client_info_set_name (io->client_info, client_name);
  snd_seq_client_info_set_broadcast_filter(io->client_info, 1);
  snd_seq_client_info_set_error_bounce(io->client_info, 0);
  snd_seq_set_client_info(io->seq, io->client_info);

  snd_seq_port_info_malloc(&io->port_info);
  snd_seq_port_info_set_name(io->port_info, port_name);

  caps |= SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ;
  
  snd_seq_port_info_set_capability(io->port_info,caps);

  snd_seq_port_info_set_type(io->port_info,
                             SND_SEQ_PORT_TYPE_APPLICATION);
  snd_seq_port_info_set_midi_channels(io->port_info, 0);
  snd_seq_port_info_set_midi_voices(io->port_info, 0);
  snd_seq_port_info_set_synth_voices(io->port_info, 0);
  io->port_id=snd_seq_create_port(io->seq, io->port_info);
  
  if (io->port_id < 0) {
    g_set_error(error, MTC_SENDER_ERROR, MTC_SENDER_ERROR_PORT,
                "Failed to create port: %d", io->port_id);
    mtc_sender_destroy(io);
    return NULL;
  }
  io->queue_id = snd_seq_alloc_queue(io->seq);
  if (io->queue_id < 0) {
    g_set_error(error, MTC_SENDER_ERROR, MTC_SENDER_ERROR_QUEUE,
                "Failed to create queue: %s", strerror(-io->queue_id));
    mtc_sender_destroy(io);
    return NULL;
  }
  snd_seq_set_client_pool_output(io->seq, 25*4);
  
  int started = snd_seq_start_queue(io->seq, io->queue_id, NULL);
  if (started < 0) {
    g_set_error(error, MTC_SENDER_ERROR, MTC_SENDER_ERROR_QUEUE,
                "Failed to start queue: %s", strerror(-started));
    mtc_sender_destroy(io);
    return NULL;
  }
  
  int drained = snd_seq_drain_output(io->seq) ;
  if (drained < 0) {
    g_set_error(error, MTC_SENDER_ERROR, MTC_SENDER_ERROR_QUEUE,
                "Failed to start queue: %s", strerror(-drained));
    mtc_sender_destroy(io);
    return NULL;
  }
  
  // Synchronise queue to clock
  snd_seq_real_time_t stamp;
  struct timeval now;
  snd_seq_event_t event;
  gettimeofday(&now, NULL);
  stamp.tv_sec = now.tv_sec;
  stamp.tv_nsec = now.tv_usec * 1000;
  snd_seq_ev_clear(&event);
  snd_seq_ev_set_fixed(&event);
  snd_seq_ev_set_direct(&event);
  snd_seq_ev_set_source(&event, io->port_id);
  snd_seq_ev_set_queue_pos_real(&event, io->queue_id, &stamp);
  res = snd_seq_event_output_direct(io->seq, &event);
  if (res < 0) {
    g_set_error(error, MTC_SENDER_ERROR, MTC_SENDER_ERROR_QUEUE,
                "Failed to synchronize queue: %s", strerror(-res));
    mtc_sender_destroy(io);
    return NULL;
  }

  
  

  g_source_attach(&io->source, NULL);
  return io;
}

void
mtc_sender_destroy(MTCSender *io)
{
  if (io->queue_id >= 0) {
    snd_seq_free_queue(io->seq, io->queue_id);
  }
  if (io->seq) {
    snd_seq_close(io->seq);
  }
  if (io->port_info) snd_seq_port_info_free(io->port_info);
  if (io->client_info) snd_seq_client_info_free(io->client_info);
  
  g_source_destroy(&io->source);
  g_source_unref(&io->source);
}
/* Writes two consecutive SMPTE-frames to the queue. */
static int
output_smpte(snd_seq_t *seq, int queue, int out_port,
             snd_seq_real_time_t *stamp, unsigned int frame_length,
             TimePosition tc, unsigned int frame_rate)
{
  int res;
  unsigned int qf;
  unsigned int time_step = frame_length / 4;
  snd_seq_event_t event;
  unsigned char digits[8];
  unsigned int type = 3<<1;
  unsigned int hours = tc / (TIME_POSITION_FRACTIONS * 3600);
  unsigned int minutes = tc / (TIME_POSITION_FRACTIONS * 60);
  unsigned int seconds = tc / TIME_POSITION_FRACTIONS;
  unsigned int frames = ((tc - seconds * TIME_POSITION_FRACTIONS)
                         * frame_rate / TIME_POSITION_FRACTIONS);
  seconds -= minutes * 60;
  minutes -= hours *60;
  hours %= 24;

  if (frame_rate == 24) type = 0;
  else if (frame_rate == 25) type = 1<<1;
  digits[0] = frames & 0x0f;
  digits[1] = frames >>4;
  digits[2] = seconds & 0x0f;
  digits[3] = seconds >>4;
  digits[4] = minutes & 0x0f;
  digits[5] = minutes >>4;
  digits[6] = hours & 0x0f;
  digits[7] = hours >>4  | type;
  
  snd_seq_ev_clear(&event);
  snd_seq_ev_set_subs(&event);
  snd_seq_ev_set_fixed(&event);
  snd_seq_ev_set_source(&event, out_port);

  event.type = SND_SEQ_EVENT_QFRAME;
  for (qf = 0; qf < 8; qf++) {
    event.data.control.value = qf<<4 | digits[qf];
    (void)snd_seq_ev_schedule_real(&event, queue, 0, stamp);
    //snd_seq_ev_set_direct(&event);
    {
      unsigned char buffer[10];
      snd_midi_event_t *dev;
      snd_midi_event_new (10, &dev);
      int decoded = snd_midi_event_decode(dev, buffer, sizeof(buffer), &event);
      g_debug("decoded: %d, [%02x, %02x]", decoded, buffer[0], buffer[1]);
      snd_midi_event_free (dev);

      g_debug("event length: %ld", snd_seq_event_length(&event));
    }
    res = snd_seq_event_output(seq, &event);
    if (res < 0) return res;
    stamp->tv_nsec += time_step;
    if (stamp->tv_nsec > 1000000000U) {
      stamp->tv_nsec -= 1000000000U;
      stamp->tv_sec++;
    }
  }
  //return 0;
  return snd_seq_drain_output(seq);
}

void
mtc_sender_send_mtc(MTCSender *io, TimePosition tc, guint16 frame_rate, GTimeVal *when)
{
  struct snd_seq_real_time stamp;
  stamp.tv_sec = when->tv_sec;
  stamp.tv_nsec = when->tv_usec * 1000;
  
  // Only send a new timestamp until the previuos one has been sent
  if (io->last_timestamp.tv_sec < stamp.tv_sec 
      || (io->last_timestamp.tv_sec == stamp.tv_sec
	  && io->last_timestamp.tv_nsec < stamp.tv_nsec)) {
    int res = output_smpte(io->seq, io->queue_id, io->port_id,
			   &stamp, 1000000000/frame_rate, tc, frame_rate);
    if (res < 0) {
      g_warning("Failed sending MIDI event: %s",
		strerror(-res));
    }
  }
  io->last_timestamp = stamp;

}
