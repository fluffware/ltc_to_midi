#include <userbits_source.h>
#include <gmarshal.h>

static guint userbits_source_base_init_count = 0;

static void
userbits_source_base_init (UserbitsSourceClass *source_class)
{
  userbits_source_base_init_count++;
  if (userbits_source_base_init_count == 1) {
    g_signal_new("userbits-changed",
		 G_OBJECT_CLASS_TYPE(source_class),
		 0,
		 G_STRUCT_OFFSET (UserbitsSourceClass, userbits_changed),
		 NULL, NULL,
		 g_cclosure_marshal_VOID__UINT,
		 G_TYPE_NONE, 1, G_TYPE_UINT);
  }
}
static void
userbits_source_base_finalize (UserbitsSourceClass *source_class)
{
  userbits_source_base_init_count--;
  if (userbits_source_base_init_count == 0) {
  }
}

GType
userbits_source_get_type()
{
  static GType userbits_source_type = 0;

  if (!userbits_source_type) {
    static const GTypeInfo userbits_source_info =
      {
        sizeof (UserbitsSourceClass),
        (GBaseInitFunc) userbits_source_base_init,		/* base_init */
        (GBaseFinalizeFunc) userbits_source_base_finalize, /* base_finalize */
      };
    
    userbits_source_type = g_type_register_static(G_TYPE_INTERFACE,
						  "UserbitsSource",
						  &userbits_source_info, 0);
    g_type_interface_add_prerequisite (userbits_source_type, G_TYPE_OBJECT);
    }

  return userbits_source_type;

}

/**
 * userbits_source_userbits:
 * @source: a #UserbitsSource
 * @Returns: last read user bits
 *
 * Get userbits.
 **/

UserBits
userbits_source_userbits(UserbitsSource *source)
{
  return USERBITS_SOURCE_GET_CLASS(source)->userbits(source);
}
