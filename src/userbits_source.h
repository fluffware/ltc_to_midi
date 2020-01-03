#ifndef __USERBITS_SOURCE_H__
#define __USERBITS_SOURCE_H__

#include <glib-object.h>

typedef unsigned int UserBits;

#define G_TYPE_USERBITS_SOURCE (userbits_source_get_type())
#define USERBITS_SOURCE(obj) \
(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_USERBITS_SOURCE, UserbitsSource))
#define USERBITS_SOURCE_CLASS(klass) \
G_TYPE_CHECK_CLASS_CAST((klass), G_TYPE_USERBITS_SOURCE, UserbitsSourceClass)

#define IS_USERBITS_SOURCE(object) \
G_TYPE_CHECK_INSTANCE_TYPE((object), G_TYPE_USERBITS_SOURCE)

#define IS_USERBITS_SOURCE_CLASS(klass) \
G_TYPE_CHECK_CLASS_TYPE((object), G_TYPE_USERBITS_SOURCE)

#define USERBITS_SOURCE_GET_CLASS(object) \
G_TYPE_INSTANCE_GET_INTERFACE((object), G_TYPE_USERBITS_SOURCE, UserbitsSourceClass)

#define USERBITS_SOURCE_TYPE(object) (G_TYPE_FROM_INTERFACE (object))
#define USERBITS_SOURCE_TYPE_NAME(object) (g_type_name(GTK_OBJECT_TYPE (object)))

typedef struct _UserbitsSource UserbitsSource;
typedef struct _UserbitsSourceClass UserbitsSourceClass;

struct _UserbitsSourceClass
{
  GTypeInterface base_iface;
  /* Last read userbits. */
  UserBits (*userbits)(UserbitsSource *source);

  /* Signals */

  /* Sent whenever the userbits of the source changes. when indicate what
     sytem time the userbits correspnds to. */
  void (*userbits_changed)(UserbitsSource *source, UserBits user_bits);
};

GType
userbits_source_get_type();

UserBits
userbits_source_userbits(UserbitsSource *source);

#endif /* __USERBITS_SOURCE_H__ */
