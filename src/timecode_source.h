#ifndef __TIMECODE_SOURCE_H__
#define __TIMECODE_SOURCE_H__

#include <glib-object.h>
#include <timeposition.h>

#define G_TYPE_TIMECODE_SOURCE (timecode_source_get_type())
#define TIMECODE_SOURCE(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_TIMECODE_SOURCE, TimecodeSource))
#define TIMECODE_SOURCE_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_TIMECODE_SOURCE, TimecodeSourceClass)

#define IS_TIMECODE_SOURCE(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_TIMECODE_SOURCE)

#define IS_TIMECODE_SOURCE_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_TIMECODE_SOURCE)

#define TIMECODE_SOURCE_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_INTERFACE((object), G_TYPE_TIMECODE_SOURCE, TimecodeSourceClass)

#define TIMECODE_SOURCE_TYPE(object) (G_TYPE_FROM_INTERFACE (object))
#define TIMECODE_SOURCE_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))

typedef struct _TimecodeSource TimecodeSource;
typedef struct _TimecodeSourceClass TimecodeSourceClass;

struct _TimecodeSourceClass
{
  GTypeInterface base_iface;
  /* Last read timecode. */
  void (*timecode)(TimecodeSource *source,TimePosition *tc, GTimeVal *when);
  /* Frame rate of timecode or 0 if unknown. */
  guint16 (*frame_rate)(TimecodeSource *source);

  /* Signals */

  /* Sent whenever the timecode of the source changes. when indicates what
     system time the timecode corresponds to. */
  void (*timecode_changed)(TimecodeSource *source,
			   TimePosition tc, const GTimeVal *when);
};

GType
timecode_source_get_type();

void
timecode_source_timecode(TimecodeSource *source,
			 TimePosition *tc, GTimeVal *when);
guint16
timecode_source_frame_rate(TimecodeSource *source);

#endif /* __TIMECODE_SOURCE_H__ */
