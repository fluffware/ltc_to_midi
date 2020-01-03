#include <audio_sink.h>
#include <audio_source.h>
#include <stdarg.h>

gboolean
audio_link_connect(AudioSource *source, AudioSink *sink)
{
  if (!audio_source_has_buffer(source) && !audio_sink_has_buffer(sink)) {
    g_error("Can't connect an AudioSource to a an AudioSink "
	    "if both lacks buffers");
    return FALSE;
  }
  AUDIO_SOURCE_GET_CLASS(source)->connect(source,sink);
  AUDIO_SINK_GET_CLASS(sink)->connect(sink,source);
  return TRUE;
}

gboolean
audio_link_connect_chain(gpointer source, ...)
{
  guint arg_pos = 1;
  GObject *source_obj = G_OBJECT(source);
  va_list args;
  va_start(args, source);
  while(TRUE) {
    GObject *sink_obj = va_arg(args, GObject *);
    if (!sink_obj) break;
    if (!IS_AUDIO_SOURCE(source_obj)) {
      g_error("Arg %d of %s is not an AudioSource", arg_pos,  __FUNCTION__);
      va_end(args);
      return FALSE;
    }
    arg_pos++;
    if (!IS_AUDIO_SINK(sink_obj)) {
      g_error("Arg %d of %s is not an AudioSink", arg_pos,  __FUNCTION__);
      va_end(args);
      return FALSE;
    }
    audio_link_connect(AUDIO_SOURCE(source_obj), AUDIO_SINK(sink_obj));
    source_obj = sink_obj;
  }
  va_end(args);
  return TRUE;
}
