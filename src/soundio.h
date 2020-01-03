#ifndef __SOUNDIO_H__
#define __SOUNDIO_H__

#include <audio_buffer.h>

typedef unsigned int SoundIOResult;
#define SOUNDIO_OK 0
#define SOUNDIO_ERROR_IO 0x11
#define SOUNDIO_ERROR_MEM 0x12

#define G_TYPE_SOUND_IO (sound_io_get_type())
#define SOUND_IO(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SOUND_IO, SoundIO))
#define SOUND_IO_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_SOUND_IO, SoundIOClass)

#define IS_SOUND_IO(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_SOUND_IO)

#define IS_SOUND_IO_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_SOUND_IO)

#define SOUND_IO_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_INTERFACE((object), G_TYPE_SOUND_IO, SoundIOClass)

#define SOUND_IO_TYPE(object) (G_TYPE_FROM_INTERFACE (object))
#define SOUND_IO_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))

typedef struct _SoundIO SoundIO;
typedef struct _SoundIOClass SoundIOClass;


struct _SoundIOClass
{
  GTypeInterface base_iface;
  FrameLength (*sample_rate)(SoundIO *io);
  
  /* Signals */
  void (*underrun)(SoundIO *io);
  void (*overrun)(SoundIO *io);
};

GType
sound_io_get_type();

FrameLength
sound_io_buffered(SoundIO *io);

FrameLength
sound_io_sample_rate(SoundIO *io);

#endif /* __SOUNDIO_H__ */
