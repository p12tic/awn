/* awn-desktop-lookup.c */

#include "awn-desktop-lookup.h"

G_DEFINE_TYPE (AwnDesktopLookup, awn_desktop_lookup, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_DESKTOP_LOOKUP, AwnDesktopLookupPrivate))

typedef struct _AwnDesktopLookupPrivate AwnDesktopLookupPrivate;

struct _AwnDesktopLookupPrivate {

  GHashTable * desktop_lookup_cache;
  
};

static void
awn_desktop_lookup_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
awn_desktop_lookup_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->dispose (object);
}

static void
awn_desktop_lookup_finalize (GObject *object)
{
  G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->finalize (object);
}

static void
awn_desktop_lookup_constructed (GObject *object)
{
  if ( G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_desktop_lookup_parent_class)->constructed (object);
  }
}

static void
awn_desktop_lookup_class_init (AwnDesktopLookupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (AwnDesktopLookupPrivate));

  object_class->get_property = awn_desktop_lookup_get_property;
  object_class->set_property = awn_desktop_lookup_set_property;
  object_class->dispose = awn_desktop_lookup_dispose;
  object_class->finalize = awn_desktop_lookup_finalize;
  object_class->constructed = awn_desktop_lookup_constructed;
}

static void
awn_desktop_lookup_init (AwnDesktopLookup *self)
{
  AwnDesktopLookupPrivate * priv = GET_PRIVATE(self);
  priv->desktop_lookup_cache = g_hash_table_new_full (g_str_hash,
                                                      (GEqualFunc)g_strcmp0,
                                                      g_free,
                                                      g_free);
}

AwnDesktopLookup*
awn_desktop_lookup_new (void)
{
  return g_object_new (AWN_TYPE_DESKTOP_LOOKUP, NULL);
}

gchar *
awn_desktop_lookup_search_cache (AwnDesktopLookup* * self,
                                 gchar * class_name,
                                 gchar * res_name,
                                 gchar * cmd, 
                                 gchar *id)
{
  AwnDesktopLookupPrivate * priv = GET_PRIVATE(self);
  gchar * key;
  gchar * desktop_path;

  key = g_strdup_printf("%s::%s::%s::%s",class_name,res_name,cmd,id);
  desktop_path = g_hash_table_lookup (priv->desktop_lookup_cache,key);

  g_free (key);
  key = NULL;
  return desktop_path;
}

/*
 If other attempts to find a desktop file fail then this function will
 typically get called which use tables of data to match problem apps to 
 desktop file names (refer to util.c)
 */
gchar *
awn_desktop_lookup_special_case (gchar * cmd, 
                                 gchar *res_name, 
                                 gchar * class_name,
                                 gchar * wm_icon_name,
                                 gchar * wm_name,
                                 gchar * window_role)
{
  gchar * result = NULL;
  return result;
}


gchar *
awn_desktop_lookup_search_for_desktop (gulong xid)
{
  gchar * found_desktop = NULL;
  return found_desktop;
}
