#include <soundio.h>

static guint sound_io_base_init_count = 0;

static void
sound_io_base_init (SoundIOClass *sound_io_class)
{
  sound_io_base_init_count++;
  if (sound_io_base_init_count == 1) {
    g_signal_new("underrun",
		 G_OBJECT_CLASS_TYPE(sound_io_class),
		 0,
		 G_STRUCT_OFFSET (SoundIOClass, underrun),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__VOID,
		 G_TYPE_NONE, 0);
    g_signal_new("overrun",
		 G_OBJECT_CLASS_TYPE(sound_io_class),
		 0,
		 G_STRUCT_OFFSET (SoundIOClass, overrun),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__VOID,
		 G_TYPE_NONE, 0);
  }
}
static void
sound_io_base_finalize (AudioSourceClass *source_class)
{
  sound_io_base_init_count--;
  if (sound_io_base_init_count == 0) {
  }
}

GType
sound_io_get_type()
{
  static GType sound_io_type = 0;

  if (!sound_io_type) {
    static const GTypeInfo sound_io_info =
      {
        sizeof (AudioSourceClass),
        (GBaseInitFunc) sound_io_base_init,		/* base_init */
        (GBaseFinalizeFunc) sound_io_base_finalize, /* base_finalize */
      };
    
    sound_io_type = g_type_register_static(G_TYPE_INTERFACE, "SoundIO",
					   &sound_io_info, 0);
    g_type_interface_add_prerequisite (sound_io_type, G_TYPE_OBJECT);
  }
  
  return sound_io_type;
}

FrameLength
sound_io_sample_rate(SoundIO *io)
{
  return SOUND_IO_GET_CLASS(io)->sample_rate(io);
}
