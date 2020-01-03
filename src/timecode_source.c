#include <timecode_source.h>
#include <gmarshal.h>

static guint timecode_source_base_init_count = 0;

static void
timecode_source_base_init (TimecodeSourceClass *source_class)
{
  timecode_source_base_init_count++;
  if (timecode_source_base_init_count == 1) {
    g_signal_new("timecode-changed",
		 G_OBJECT_CLASS_TYPE(source_class),
		 0,
		 G_STRUCT_OFFSET (TimecodeSourceClass, timecode_changed),
		 NULL, NULL,
		 sync_VOID__INT_POINTER,
		 G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_POINTER);
  }
}
static void
timecode_source_base_finalize (TimecodeSourceClass *source_class)
{
  timecode_source_base_init_count--;
  if (timecode_source_base_init_count == 0) {
  }
}

GType
timecode_source_get_type()
{
  static GType timecode_source_type = 0;

  if (!timecode_source_type) {
    static const GTypeInfo timecode_source_info =
      {
        sizeof (TimecodeSourceClass),
        (GBaseInitFunc) timecode_source_base_init,		/* base_init */
        (GBaseFinalizeFunc) timecode_source_base_finalize, /* base_finalize */
      };
    
    timecode_source_type = g_type_register_static(G_TYPE_INTERFACE,
						  "TimecodeSource",
						  &timecode_source_info, 0);
    g_type_interface_add_prerequisite (timecode_source_type, G_TYPE_OBJECT);
    }

  return timecode_source_type;

}

/**
 * timecode_source_timecode:
 * @source: a #TimecodeSource
 * @tc: last read timecode
 * @when: when the timecode was read
 *
 * Get last read timecode.
 **/
void
timecode_source_timecode(TimecodeSource *source,
			 TimePosition *tc, GTimeVal *when)
{
  TIMECODE_SOURCE_GET_CLASS(source)->timecode(source,tc, when);
}

/**
 * timecode_source_frame_rate:
 * @source: a #TimecodeSource
 * @Returns: current framerate or 0 if unknown
 *
 * Get current framerate.
 **/

guint16
timecode_source_frame_rate(TimecodeSource *source)
{
  return TIMECODE_SOURCE_GET_CLASS(source)->frame_rate(source);
}
