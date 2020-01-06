#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <glib.h>
#include <soundio_alsa.h>
#include <LTC_reader.h>
#include <mtc_sender.h>

static GMainLoop *main_loop = NULL;

RETSIGTYPE
sig_handler(int s)
{
  if (main_loop) {
    g_main_loop_quit(main_loop);
  }
}

typedef struct _AppContext
{
  SoundIO *audio_source;
  LTCReader *ltc_reader;
  MTCSender *mtc_sender;
} AppContext;

static void
app_init(AppContext *app)
{
  app->audio_source = NULL;
  app->ltc_reader = NULL;
  app->mtc_sender = NULL;
}

static void 
app_clean(AppContext *app)
{
  if (app->audio_source) {
    g_object_unref(app->audio_source);
  }
  if (app->ltc_reader) {
    g_object_unref(app->ltc_reader);
  }
  if (app->mtc_sender) {
    mtc_sender_destroy(app->mtc_sender);
  }
}

static struct Options
{
  const char *device;
} options = {
	     "default"
};
  
static GOptionEntry entries[] =
{
  { "device", 'd', 0, G_OPTION_ARG_STRING, &options.device,
    "ALSA input device", "DEV" },
  { NULL }
};

static void
timecode_changed(TimecodeSource *source,
		 TimePosition tc, GTimeVal *when, gpointer user_data)
{
  AppContext *app = (AppContext*)user_data;
  g_print("TC: %d\n", tc);
  guint16 frame_rate = timecode_source_frame_rate(TIMECODE_SOURCE(source));

  // Move one second ahead to make sure the timestamp is in the future
  when->tv_sec += 1;
  tc += 600;
  
  mtc_sender_send_mtc(app->mtc_sender, tc, frame_rate, when);
}

int
main(int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  AppContext app;
  app_init(&app);
  context = g_option_context_new ("- read LTC and output MIDI timecode");
  g_option_context_add_main_entries (context, entries, 0);
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_print ("option parsing failed: %s\n", error->message);
    app_clean(&app);
    exit (1);
  }

  
  main_loop = g_main_loop_new(NULL, FALSE);
  
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  app.audio_source = sound_io_alsa_new(options.device, 1, 44100, TRUE, FALSE);
  g_assert(G_IS_OBJECT(app.audio_source));
  app.ltc_reader = LTC_reader_new();
  app.mtc_sender = mtc_sender_new("LTCtoMTC", "MTC out", &error);
  if (!app.mtc_sender) {
    g_print("Failed to create MIDI output: %s\n", error->message);
    g_clear_error(&error);
    app_clean(&app);
    return 1;
  }
  g_signal_connect(app.ltc_reader, "timecode-changed", G_CALLBACK(timecode_changed), &app);
  audio_link_connect(AUDIO_SOURCE(app.audio_source),
		     AUDIO_SINK(app.ltc_reader));
  g_main_loop_run(main_loop);
  g_debug("Stopped");
  g_main_loop_unref(main_loop);
  app_clean(&app);
}
