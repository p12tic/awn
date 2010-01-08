/* awn-desktop-lookup-cached.c */


#include "xutils.h"
#include <libdesktop-agnostic/fdo.h>
#include "awn-desktop-lookup-cached.h"
#include "libawn/libawn.h"
#include "util.h"

#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>

G_DEFINE_TYPE (AwnDesktopLookupCached, awn_desktop_lookup_cached, AWN_TYPE_DESKTOP_LOOKUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), AWN_TYPE_DESKTOP_LOOKUP_CACHED, AwnDesktopLookupCachedPrivate))

typedef struct _AwnDesktopLookupCachedPrivate AwnDesktopLookupCachedPrivate;

typedef struct
{
  gchar * path;
  gchar * exec;
  gchar * name;
}DesktopNode;

struct _AwnDesktopLookupCachedPrivate {
  GHashTable * name_hash;
  GHashTable * exec_hash;
  GHashTable * startup_wm_hash;

  GSList * desktop_list;  /*For when the fast lookups don't work*/
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
awn_desktop_lookup_cached_add_dir (AwnDesktopLookupCached * lookup,const gchar * applications_dir)
{
  GDir        * dir = NULL;
  const gchar * fname = NULL;
  AwnDesktopLookupCachedPrivate * priv = GET_PRIVATE (lookup);
  static int call_depth = 0;

  call_depth ++;
  if (call_depth>10)
  {
    g_debug ("%s: resursive depth = %d.  bailing at %s",__func__,call_depth,applications_dir);
  }
  dir = g_dir_open (applications_dir,0,NULL);
  while ( (fname = g_dir_read_name (dir)))
  {
    gchar * new_path = g_strdup_printf ("%s%s",applications_dir,fname);
    if ( g_file_test (new_path,G_FILE_TEST_IS_DIR) )
    {
      awn_desktop_lookup_cached_add_dir (lookup,new_path);
    }
    else
    {
      DesktopAgnosticFDODesktopEntry *entry=NULL;
      DesktopAgnosticVFSFile *file;
      file = desktop_agnostic_vfs_file_new_for_path (new_path, NULL);
      if (file)
      {
        if (desktop_agnostic_vfs_file_exists (file) )
        {
          entry = desktop_agnostic_fdo_desktop_entry_new_for_file (file, NULL);
          if (entry && desktop_agnostic_fdo_desktop_entry_key_exists (entry,"Name")
              &&
              desktop_agnostic_fdo_desktop_entry_key_exists (entry,"Exec"))
          {
            /*
             Be careful.  Not duplicating these strings for each data structure
             */
            gchar * name = desktop_agnostic_fdo_desktop_entry_get_name (entry);
            gchar * exec = desktop_agnostic_fdo_desktop_entry_get_string (entry, "Exec");
            gchar * copy_path = NULL;
            gchar * search = NULL;
            gchar * tmp;
            gchar * startup_wm = NULL;
            DesktopNode * node;
            
            tmp =  g_utf8_strdown (name,-1);
            g_free (name);
            name = tmp;
            
            g_strdelimit (exec,"%",'\0');
            g_strstrip (exec);
            
            if ( (search = g_hash_table_lookup (priv->name_hash,name) ))
            {
//              g_warning ("%s: Name (%s) collision between %s and %s",__func__,name,search,new_path);
              g_free (name);
              g_free (exec);
              goto NAME_COLLSION;
            }

            if ( (search = g_hash_table_lookup (priv->exec_hash,exec)))
            {
//              g_warning ("%s: Exec Name (%s) collision between %s and %s",__func__,exec,search,new_path);
              g_free (name);
              g_free (exec);
              goto NAME_COLLSION;
            }

            if (desktop_agnostic_fdo_desktop_entry_key_exists (entry,"StartupWMClass"))
            {
              startup_wm = desktop_agnostic_fdo_desktop_entry_get_string (entry, "StartupWMClass");
              if ((search = g_hash_table_lookup (priv->startup_wm_hash,startup_wm) ))
              {
//                g_warning ("%s: StartuWM Name (%s) collision between %s and %s",__func__,startup_wm,search,new_path);
                g_free (name);
                g_free (exec);
                g_free (startup_wm);
                goto NAME_COLLSION;
              }
            }
            copy_path = g_strdup (new_path);
            g_hash_table_insert (priv->name_hash,name,copy_path);
            g_hash_table_insert (priv->exec_hash,exec,copy_path);
            if (startup_wm)
            {
              g_hash_table_insert (priv->startup_wm_hash,startup_wm,copy_path);
            }
            node = g_malloc (sizeof(DesktopNode));
            node->path = copy_path;
            node->name = name;
            node->exec = exec;
            priv->desktop_list = g_slist_prepend (priv->desktop_list,node);
          }
NAME_COLLSION:
          if (entry)
          {
            g_object_unref (entry);
          }
        }
        g_object_unref (file);
      }
    }
    g_free (new_path);
  }
  g_dir_close (dir);
  call_depth --;
}

static void
awn_desktop_lookup_cached_constructed (GObject *object)
{
  const gchar* const * system_dirs = NULL;
  GStrv iter = NULL;
  AwnDesktopLookupCachedPrivate * priv = GET_PRIVATE (object);
  
  if ( G_OBJECT_CLASS (awn_desktop_lookup_cached_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_desktop_lookup_cached_parent_class)->constructed (object);
  }
  
  system_dirs = g_get_system_data_dirs ();
  for (iter = (GStrv)system_dirs; *iter; iter++)
  {
    gchar * applications_dir = g_strdup_printf ("%s/applications/",*iter);
    awn_desktop_lookup_cached_add_dir (AWN_DESKTOP_LOOKUP_CACHED(object),
                                       applications_dir);
    g_free (applications_dir);
  }
  awn_desktop_lookup_cached_add_dir (AWN_DESKTOP_LOOKUP_CACHED(object),
                                     g_get_user_data_dir ());
  /*
   entries originally prepended in order found.  Reversing on the premise that
   data dirs early in the list are more likely to have the desktop file we
   are looking for
   */
  priv->desktop_list = g_slist_reverse (priv->desktop_list);
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
  object_class->constructed = awn_desktop_lookup_cached_constructed;
}

static void
awn_desktop_lookup_cached_init (AwnDesktopLookupCached *self)
{
  AwnDesktopLookupCachedPrivate * priv = GET_PRIVATE (self);

  priv->name_hash = g_hash_table_new_full (g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           NULL);
  priv->exec_hash = g_hash_table_new_full (g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           NULL);
  priv->startup_wm_hash = g_hash_table_new_full (g_str_hash,
                                           g_str_equal,
                                           g_free,
                                           NULL);
  priv->desktop_list = NULL;
}

AwnDesktopLookupCached*
awn_desktop_lookup_cached_new (void)
{
  return g_object_new (AWN_TYPE_DESKTOP_LOOKUP_CACHED, NULL);
}

static int
_search_exec (DesktopNode *a, gchar * b)
{
  if (!a->exec)
  {
    return -1;
  }
  int a_len = strlen (a->exec);
  int b_len = strlen (b);
  if (a_len <3 || b_len<3)
  {
    return -1;
  }
  return strncmp (a->exec,b, a_len>b_len?b_len:a_len);
}
  
static int
_search_path (DesktopNode *a, gchar * b)
{
  if (!a->path)
  {
    return -1;
  }
  int a_len = strlen (a->path);
  int b_len = strlen (b);
  if (a_len <3 || b_len<3)
  {
    return -1;
  }
  if (g_strstr_len (a->path,-1,b) )
  {
    return 0;
  }
  return -1;                 
}

const gchar *
awn_desktop_lookup_search_by_wnck_window (AwnDesktopLookupCached * lookup, WnckWindow * win)
{
  /*TODO
   search through the wnck window list for the xid then call ^*/
  const gchar * result = NULL;
  AwnDesktopLookupCachedPrivate * priv = GET_PRIVATE (lookup);
  gchar * res_name = NULL;
  gchar * class_name = NULL;
  gchar * res_name_lwr = NULL;
  gchar * class_name_lwr = NULL;
  gchar * full_cmd = NULL;
  gchar * cmd = NULL;
  gchar * cmd_basename = NULL;
  glibtop_proc_args buf;
  gulong xid = wnck_window_get_xid (win);
  GSList *l = NULL;
  const gchar * title;

  title = wnck_window_get_name (win);
  _wnck_get_wmclass (xid,&res_name, &class_name);
  if (res_name)
  {
    res_name_lwr =  g_utf8_strdown (res_name,-1);
  }
  if (class_name)
  {
    class_name_lwr = g_utf8_strdown (class_name,-1);
  }
  cmd = glibtop_get_proc_args (&buf,wnck_window_get_pid (win),1024);    
  full_cmd = get_full_cmd_from_pid ( wnck_window_get_pid (win));
  if (cmd)
  {
    cmd_basename = g_path_get_basename (cmd);
  }
//  g_debug ("cmd = %s, full_cmd = %s, cmd_basename = %s",cmd,full_cmd,cmd_basename);

//  g_debug ("%s: res_name = '%s', class_name = '%s'",__func__,res_name,class_name);

  if (class_name)
  {
    if (class_name)
    {
      result = g_hash_table_lookup (priv->startup_wm_hash,class_name);
    }
  }
  if (!result)
  {
    if (res_name)
    {
      result = g_hash_table_lookup (priv->name_hash,res_name);
    }
  }
  if (!result)
  {
    if (res_name_lwr)
    {
      result = g_hash_table_lookup (priv->name_hash,res_name_lwr);
    }
  }
  if (!result)
  {
    if (cmd)
    {
      result = g_hash_table_lookup (priv->name_hash,cmd);
    }
  }
  
  if (!result)
  {
    if (res_name)
    {
      result = g_hash_table_lookup (priv->exec_hash,res_name);
    }
  }
  if (!result)
  {
    if (res_name_lwr)
    {
      result = g_hash_table_lookup (priv->exec_hash,res_name_lwr);
    }
  }
  if (!result)
  {
    if (res_name_lwr)
    {
      result = g_hash_table_lookup (priv->exec_hash,res_name_lwr);
    }
  }
  if (!result)
  {
    if (cmd)
    {
      result = g_hash_table_lookup (priv->exec_hash,cmd);
    }
  }
  if (!result)
  {
    if (cmd_basename)
    {
      result = g_hash_table_lookup (priv->exec_hash,cmd_basename);
    }
  }
  if (!result)
  {
    if (full_cmd)
    {
      result = g_hash_table_lookup (priv->exec_hash,full_cmd);
    }
  }
  if (!result)
  {
    if (full_cmd)
    {
      l = g_slist_find_custom (priv->desktop_list,
                                    full_cmd,
                                    (GCompareFunc)_search_exec);
      if (l)
      {
        result = l->data;
      }
    }
  }

  if (!result)
  {
    GSList * desktops = get_special_desktop_from_window_data (full_cmd,
                                                              res_name,
                                                              class_name,
                                                              title);
    if (desktops)
    {
      GSList * iter;
      for (iter = desktops; iter; iter = iter->next)
      {
        l = g_slist_find_custom (priv->desktop_list,
                                    iter->data,
                                    (GCompareFunc)_search_path);
        if (l)
        {
          result = ((DesktopNode*) (l->data))->path;
        }
      }
      g_slist_free (desktops);
    }
  }

  if (!result)
  {
    GSList * desktops = get_special_desktop_from_window_data (cmd,
                                                              res_name,
                                                              class_name,
                                                              title);
    if (desktops)
    {
      GSList * iter;
      for (iter = desktops; iter; iter = iter->next)
      {
        l = g_slist_find_custom (priv->desktop_list,
                                    iter->data,
                                    (GCompareFunc)_search_path);
        if (l)
        {
          result = l->data;
        }
      }
      g_slist_free (desktops);
    }
  }

  g_free (full_cmd);
  g_free (cmd);
  g_free (cmd_basename);
  g_free (res_name);
  g_free (class_name);
  g_free (res_name_lwr);
  g_free (class_name_lwr);
  return result;
}
