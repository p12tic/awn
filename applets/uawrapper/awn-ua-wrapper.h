/* awn-ua-wrapper.h */

#ifndef _AWN_UA_WRAPPER
#define _AWN_UA_WRAPPER

#include <glib-object.h>
#include <libawn/libawn.h>

G_BEGIN_DECLS

#define AWN_TYPE_UA_WRAPPER awn_ua_wrapper_get_type()

#define AWN_UA_WRAPPER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_UA_WRAPPER, AwnUAWrapper))

#define AWN_UA_WRAPPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_UA_WRAPPER, AwnUAWrapperClass))

#define AWN_IS_UA_WRAPPER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_UA_WRAPPER))

#define AWN_IS_UA_WRAPPER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_UA_WRAPPER))

#define AWN_UA_WRAPPER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_UA_WRAPPER, AwnUAWrapperClass))

typedef struct {
  AwnApplet parent;
} AwnUAWrapper;

typedef struct {
  AwnAppletClass parent_class;
} AwnUAWrapperClass;

GType awn_ua_wrapper_get_type (void);

AwnUAWrapper* awn_ua_wrapper_new (gchar* uid, gint panel_id);

G_END_DECLS

#endif /* _AWN_UA_WRAPPER */
