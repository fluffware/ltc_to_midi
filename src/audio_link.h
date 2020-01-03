#ifndef __AUDIO_LINK_H__
#define __AUDIO_LINK_H__


typedef struct _AudioSink AudioSink;
typedef struct _AudioSource AudioSource;

gboolean
audio_link_connect(AudioSource *source, AudioSink *sink);

/* Connect all objects given as parameters together.
   The first object implements AudioSource.
   The last object implements AudioSink.
   The ones in between implements both AudioSource and AudioSink.
   Last parameter should be NULL. */
gboolean
audio_link_connect_chain(gpointer first_source, ...);

#endif /* __AUDIO_LINK_H__ */
