#ifndef __MTC_SENDER_H__7QP4R77342__
#define __MTC_SENDER_H__7QP4R77342__


#include <glib.h>
#include <alsa/asoundlib.h>
#include <timeposition.h>

enum {
  MTC_SENDER_ERROR_NONE = 0,
  MTC_SENDER_ERROR_PARSE,
  MTC_SENDER_ERROR_SUBSCRIPTION,
  MTC_SENDER_ERROR_QUEUE,
  MTC_SENDER_ERROR_PORT,
  MTC_SENDER_ERROR_SEND
};

#define MTC_SENDER_ERROR mtc_sender_error_quark()
GQuark
mtc_sender_error_quark (void);

typedef struct _MTCSender MTCSender;
typedef void (MTCSenderEventFunc)(MTCSender *io, const snd_seq_event_t *event, gpointer data);


MTCSender *
mtc_sender_new(const gchar *client_name, const gchar *port_name,
	    GError **error);

const gchar *
mtc_sender_get_port_name(MTCSender *io);

const gchar *
mtc_sender_get_client_name(MTCSender *io);

void
mtc_sender_destroy(MTCSender *io);

void
mtc_sender_send_mtc(MTCSender *io, TimePosition tc, GTimeVal *when);

#endif /* __MTC_SENDER_H__7QP4R77342__ */
