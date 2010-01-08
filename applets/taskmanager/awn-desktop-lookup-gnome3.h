/* awn-desktop-lookup-gnome3.h */

#ifndef _AWN_DESKTOP_LOOKUP_GNOME3
#define _AWN_DESKTOP_LOOKUP_GNOME3

#include "awn-desktop-lookup.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define AWN_TYPE_DESKTOP_LOOKUP_GNOME3 awn_desktop_lookup_gnome3_get_type()

#define AWN_DESKTOP_LOOKUP_GNOME3(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_DESKTOP_LOOKUP_GNOME3, AwnDesktopLookupGnome3))

#define AWN_DESKTOP_LOOKUP_GNOME3_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_DESKTOP_LOOKUP_GNOME3, AwnDesktopLookupGnome3Class))

#define AWN_IS_DESKTOP_LOOKUP_GNOME3(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_DESKTOP_LOOKUP_GNOME3))

#define AWN_IS_DESKTOP_LOOKUP_GNOME3_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_DESKTOP_LOOKUP_GNOME3))

#define AWN_DESKTOP_LOOKUP_GNOME3_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_DESKTOP_LOOKUP_GNOME3, AwnDesktopLookupGnome3Class))

typedef struct {
  AwnDesktopLookup parent;
} AwnDesktopLookupGnome3;

typedef struct {
  AwnDesktopLookupClass parent_class;
} AwnDesktopLookupGnome3Class;

GType awn_desktop_lookup_gnome3_get_type (void);

AwnDesktopLookupGnome3* awn_desktop_lookup_gnome3_new (void);

G_END_DECLS

#endif /* _AWN_DESKTOP_LOOKUP_GNOME3 */
