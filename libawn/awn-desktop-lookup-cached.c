/* awn-desktop-lookup-cached.c */

#include "awn-desktop-lookup-cached.h"

G_DEFINE_TYPE (AwnDesktopLookupCached, awn_desktop_lookup_cached, AWN_TYPE_DESKTOP_LOOKUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCachedPrivate))

typedef struct _AwnDesktopLookupCachedPrivate AwnDesktopLookupCachedPrivate;

struct _AwnDesktopLookupCachedPrivate {
    int dummy;
};

static void
awn_desktop_lookup_cached_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_cached_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_cached_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_cached_parent_class)->dispose (object);
}

static void
awn_desktop_lookup_cached_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_cached_parent_class)->finalize (object);
}

static void
awn_desktop_lookup_cached_class_init (AwnDesktopLookupCachedClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnDesktopLookupCachedPrivate));

  object_class->get_property = awn_desktop_lookup_cached_get_property;
  object_class->set_property = awn_desktop_lookup_cached_set_property;
  object_class->dispose = awn_desktop_lookup_cached_dispose;
  object_class->finalize = awn_desktop_lookup_cached_finalize;
}

static void
awn_desktop_lookup_cached_init (AwnDesktopLookupCached *self)
{
}

AwnDesktopLookupCached*
awn_desktop_lookup_cached_new (void)
{
  return g_object_new (AWN_TYPE_DESKTOP_LOOKUP_CACHED, NULL);
}

