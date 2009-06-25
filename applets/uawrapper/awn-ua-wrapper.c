/* awn-ua-wrapper.c */

#include "awn-ua-wrapper.h"

G_DEFINE_TYPE (AwnUAWrapper, awn_ua_wrapper, AWN_TYPE_APPLET)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_UA_WRAPPER, AwnUAWrapperPrivate))

typedef struct _AwnUAWrapperPrivate AwnUAWrapperPrivate;

struct _AwnUAWrapperPrivate {
    int dummy;
};

static void
awn_ua_wrapper_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_ua_wrapper_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_ua_wrapper_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_ua_wrapper_parent_class)->dispose (object);
}

static void
awn_ua_wrapper_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_ua_wrapper_parent_class)->finalize (object);
}

static void
awn_ua_wrapper_class_init (AwnUAWrapperClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnUAWrapperPrivate));

  object_class->get_property = awn_ua_wrapper_get_property;
  object_class->set_property = awn_ua_wrapper_set_property;
  object_class->dispose = awn_ua_wrapper_dispose;
  object_class->finalize = awn_ua_wrapper_finalize;
}

static void
awn_ua_wrapper_init (AwnUAWrapper *self)
{
}

AwnUAWrapper*
awn_ua_wrapper_new (gchar* uid, gint panel_id)
{
  return g_object_new (AWN_TYPE_UA_WRAPPER,
                       "uid",uid,
                       "panel-id",panel_id,
                       NULL);
}

