#ifndef __SOUNDIO_ALSA_H__
#define __SOUNDIO_ALSA_H__


#include <soundio.h>
#include <glib-object.h>

typedef struct _SoundIOAlsaClass SoundIOAlsaClass;
typedef struct _SoundIOAlsa SoundIOAlsa;

#define G_TYPE_SOUND_IO_ALSA (sound_io_alsa_get_type())
#define SOUND_IO_ALSA(object)  \
(G_TYPE_CHECK_INSTANCE_CAST ((object), G_TYPE_SOUND_IO_ALSA, SoundIOAlsa))
#define SOUND_IO_ALSA_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_SOUND_IO_ALSA, SoundIOAlsaClass)

#define IS_SOUND_IO_ALSA(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_SOUND_IO_ALSA)

#define IS_SOUND_IO_ALSA_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_SOUND_IO_ALSA)

#define SOUND_IO_ALSA_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_CLASS((object), G_TYPE_SOUND_IO_ALSA, SoundIOAlsaClass)

#define SOUND_IO_ALSA_TYPE(object) (G_TYPE_FROM_INSTANCE (object))
#define SOUND_IO_ALSA_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))

SoundIO *
sound_io_alsa_new(const char *device,
		  Channels channels, FrameLength sample_rate,
		  gboolean use_input, gboolean use_output);

GType
sound_io_alsa_get_type();

#endif /* __SOUNDIO_ALSA_H__ */
