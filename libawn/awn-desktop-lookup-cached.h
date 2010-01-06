/* awn-desktop-lookup-cached.h */

#ifndef _AWN_DESKTOP_LOOKUP_CACHED
#define _AWN_DESKTOP_LOOKUP_CACHED

#include "awn-desktop-lookup.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define AWN_TYPE_DESKTOP_LOOKUP_CACHED awn_desktop_lookup_cached_get_type()

#define AWN_DESKTOP_LOOKUP_CACHED(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCached))

#define AWN_DESKTOP_LOOKUP_CACHED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCachedClass))

#define AWN_IS_DESKTOP_LOOKUP_CACHED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), AWN_TYPE_DESKTOP_LOOKUP_CACHED))

#define AWN_IS_DESKTOP_LOOKUP_CACHED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), AWN_TYPE_DESKTOP_LOOKUP_CACHED))

#define AWN_DESKTOP_LOOKUP_CACHED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCachedClass))

typedef struct {
  AwnDesktopLookup parent;
} AwnDesktopLookupCached;

typedef struct {
  AwnDesktopLookupClass parent_class;
} AwnDesktopLookupCachedClass;

GType awn_desktop_lookup_cached_get_type (void);

AwnDesktopLookupCached* awn_desktop_lookup_cached_new (void);

G_END_DECLS

#endif /* _AWN_DESKTOP_LOOKUP_CACHED */

