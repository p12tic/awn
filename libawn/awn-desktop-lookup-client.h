/* awn-desktop-lookup-client.h */

#ifndef _AWN_DESKTOP_LOOKUP_CLIENT
#define _AWN_DESKTOP_LOOKUP_CLIENT

#include <glib-object.h>

G_BEGIN_DECLS

#define AWN_TYPE_DESKTOP_LOOKUP_CLIENT awn_desktop_lookup_client_get_type()

#define AWN_DESKTOP_LOOKUP_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_DESKTOP_LOOKUP_CLIENT, AwnDesktopLookupClient))

#define AWN_DESKTOP_LOOKUP_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_DESKTOP_LOOKUP_CLIENT, AwnDesktopLookupClientClass))

#define AWN_IS_DESKTOP_LOOKUP_CLIENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_DESKTOP_LOOKUP_CLIENT))

#define AWN_IS_DESKTOP_LOOKUP_CLIENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_DESKTOP_LOOKUP_CLIENT))

#define AWN_DESKTOP_LOOKUP_CLIENT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_DESKTOP_LOOKUP_CLIENT, AwnDesktopLookupClientClass))

typedef struct {
  GObject parent;
} AwnDesktopLookupClient;

typedef struct {
  GObjectClass parent_class;
} AwnDesktopLookupClientClass;

GType awn_desktop_lookup_client_get_type (void);

AwnDesktopLookupClient* awn_desktop_lookup_client_new (void);

G_END_DECLS

#endif /* _AWN_DESKTOP_LOOKUP_CLIENT */

/* awn-desktop-lookup-client.c */

