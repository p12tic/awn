#include "awn-desktop-lookup-client.h"

G_DEFINE_TYPE (AwnDesktopLookupClient, awn_desktop_lookup_client, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_DESKTOP_LOOKUP_CLIENT, AwnDesktopLookupClientPrivate))

typedef struct _AwnDesktopLookupClientPrivate AwnDesktopLookupClientPrivate;

struct _AwnDesktopLookupClientPrivate {
    int dummy;
};

static void
awn_desktop_lookup_client_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_client_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_client_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_client_parent_class)->dispose (object);
}

static void
awn_desktop_lookup_client_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_client_parent_class)->finalize (object);
}

static void
awn_desktop_lookup_client_class_init (AwnDesktopLookupClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnDesktopLookupClientPrivate));

  object_class->get_property = awn_desktop_lookup_client_get_property;
  object_class->set_property = awn_desktop_lookup_client_set_property;
  object_class->dispose = awn_desktop_lookup_client_dispose;
  object_class->finalize = awn_desktop_lookup_client_finalize;
}

static void
awn_desktop_lookup_client_init (AwnDesktopLookupClient *self)
{
}

AwnDesktopLookupClient*
awn_desktop_lookup_client_new (void)
{
  return g_object_new (AWN_TYPE_DESKTOP_LOOKUP_CLIENT, NULL);
}

