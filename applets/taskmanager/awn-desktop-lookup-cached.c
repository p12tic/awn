/*
 * Copyright (C) 2009,2010 Rodney Cryderman <rcryderman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 *
 * Authored by Rodney Cryderman <rcryderman@gmail.com>
 */


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
  GHashTable * desktops_hash;    /*desktop file names, without paths*/
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
    gchar * new_path = g_strdup_printf ("%s/%s",applications_dir,fname);
    if ( g_file_test (new_path,G_FILE_TEST_IS_DIR) )
    {
//      g_message ("Adding %s",new_path);
      awn_desktop_lookup_cached_add_dir (lookup,new_path);
    }
    else
    {
      DesktopAgnosticFDODesktopEntry *entry=NULL;
      DesktopAgnosticVFSFile *file;
      file = desktop_agnostic_vfs_file_new_for_path (new_path, NULL);
      if (file)
      {
        if (desktop_agnostic_vfs_file_exists (file) && g_strstr_len (new_path,-1,".desktop") )
        {
          entry = desktop_agnostic_fdo_desktop_entry_new_for_file (file, NULL);
          if (entry && desktop_agnostic_fdo_desktop_entry_key_exists (entry,"NoDisplay"))
          {
            if (desktop_agnostic_fdo_desktop_entry_get_boolean (entry,"NoDisplay"))
            {
              goto NO_DISPLAY;
            }
          }
          if (entry && desktop_agnostic_fdo_desktop_entry_key_exists (entry,"Name")
              &&
              desktop_agnostic_fdo_desktop_entry_key_exists (entry,"Exec"))
          {
            /*
             Be careful.  Not duplicating these strings for each data structure
             */
            gchar * name = _desktop_entry_get_localized_name (entry);
            gchar * exec = desktop_agnostic_fdo_desktop_entry_get_string (entry, "Exec");
            gchar * copy_path = NULL;
            gchar * search = NULL;
            gchar * name_lwr = g_utf8_strdown (name,-1);
            gchar * startup_wm = NULL;
            gchar * desktop_name = g_strdup (fname);
            DesktopNode * node;
                        
            g_strdelimit (exec,"%",'\0');
            g_strstrip (exec);

            if ( name_lwr && (search = g_hash_table_lookup (priv->name_hash,name_lwr) ))
            {
//              g_warning ("%s: Name (%s) collision between %s and %s",__func__,name,search,new_path);
              g_free (name_lwr);
              name_lwr = NULL;
            }

            if ( exec && (search = g_hash_table_lookup (priv->exec_hash,exec)))
            {
              /* This gets hit when we refresh the list due to an new installations etc.
               If we hit this then it's more or less a duplicate of an existing desktop
               or we have a refresh for some reason.  Either way we ignore it.*/
//              g_warning ("%s: Exec Name (%s) collision between %s and %s",__func__,exec,search,new_path);
              g_free (name);
              g_free (name_lwr);
              g_free (exec);
              g_free (desktop_name);
              goto NAME_COLLSION;
            }

            if ( desktop_name && (search = g_hash_table_lookup (priv->desktops_hash,desktop_name)))
            {
              /*Happens often enough (ex.  "Terminal" ).  Not a big deal, we're 
               relatively conservative in using name for matching purposes*/
              g_free (desktop_name);
              desktop_name = NULL;
            }

            if (desktop_agnostic_fdo_desktop_entry_key_exists (entry,"StartupWMClass"))
            {
              startup_wm = desktop_agnostic_fdo_desktop_entry_get_string (entry, "StartupWMClass");
              search = g_hash_table_lookup (priv->startup_wm_hash,startup_wm);
              if ( g_strcmp0 (startup_wm,"Wine")==0)
              {
                g_free (startup_wm);
                startup_wm = NULL;
              }
              else if (search)
              {
                /*if we hit this then I'm interested in knowing about it*/
                g_warning ("%s: StartuWM Name (%s) collision between %s and %s",__func__,startup_wm,search,new_path);
                g_free (startup_wm);
                startup_wm = NULL;
              }
            }
            copy_path = g_strdup (new_path);
            if (name_lwr)
            {
              g_hash_table_insert (priv->name_hash,name_lwr,copy_path);
            }
            if (exec)
            {
              g_hash_table_insert (priv->exec_hash,exec,copy_path);
            }
            if (desktop_name)
            {
              g_hash_table_insert (priv->desktops_hash,desktop_name,copy_path);
            }
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
NO_DISPLAY:
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
_data_dir_changed (DesktopAgnosticVFSFileMonitor* monitor, 
                       DesktopAgnosticVFSFile* self,                  
                       DesktopAgnosticVFSFile* other, 
                       DesktopAgnosticVFSFileMonitorEvent event,
                       AwnDesktopLookupCached * lookup
                  )
{
  gchar * path = desktop_agnostic_vfs_file_get_path (self);
  if ( g_file_test (path,G_FILE_TEST_IS_DIR))
  {
    awn_desktop_lookup_cached_add_dir (lookup, path);
  }
}

static void
awn_desktop_lookup_cached_constructed (GObject *object)
{
  const gchar* const * system_dirs = NULL;
  GStrv iter = NULL;
  AwnDesktopLookupCachedPrivate * priv = GET_PRIVATE (object);
  gchar * applications_dir;
  
  if ( G_OBJECT_CLASS (awn_desktop_lookup_cached_parent_class)->constructed)
  {
    G_OBJECT_CLASS (awn_desktop_lookup_cached_parent_class)->constructed (object);
  }

  system_dirs = g_get_system_data_dirs ();
  for (iter = (GStrv)system_dirs; *iter; iter++)
  {
    GError * error = NULL;
    DesktopAgnosticVFSFileMonitor* monitor_vfs;
    DesktopAgnosticVFSFile* file_vfs;
    applications_dir = g_strdup_printf ("%s/applications/",*iter);

    if (! g_file_test (applications_dir,G_FILE_TEST_IS_DIR ))
    {
      g_free (applications_dir);
      continue;
    }
//    g_message ("Adding %s",applications_dir);
    awn_desktop_lookup_cached_add_dir (AWN_DESKTOP_LOOKUP_CACHED(object),applications_dir);

    file_vfs = desktop_agnostic_vfs_file_new_for_path (applications_dir,&error);
    if (error)
    {
      g_warning ("%s: Error with file monitor.  %s",__func__,error->message);
      g_error_free (error);
      error = NULL;
    }
    monitor_vfs = desktop_agnostic_vfs_file_monitor (file_vfs);
    g_signal_connect (G_OBJECT(monitor_vfs),"changed", G_CALLBACK(_data_dir_changed),object);
    g_object_weak_ref (object,(GWeakNotify)g_object_unref,file_vfs);
    g_object_weak_ref (object,(GWeakNotify)g_object_unref,monitor_vfs);
    g_free (applications_dir);
  }
  applications_dir = g_strdup_printf ("%s/applications/",g_get_user_data_dir ());
//  g_message ("Adding %s",applications_dir);
  awn_desktop_lookup_cached_add_dir (AWN_DESKTOP_LOOKUP_CACHED(object),applications_dir);
  g_free (applications_dir);

//  awn_desktop_lookup_cached_add_dir (AWN_DESKTOP_LOOKUP_CACHED(object),"/var/lib/menu-xdg/applications/");

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
  priv->desktops_hash = g_hash_table_new_full (g_str_hash,
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
_search_exec_sub (DesktopNode *a, gchar * b)
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
//  g_debug ("matching %s, %s",a->exec,b);
  if (g_strstr_len (a->exec,-1,b) )
  {
    return 0;
  }
  return -1;                 
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

static int
_search_path_base_cmp (DesktopNode *a, gchar * b)
{
  int result = -1;
  if (!a->path)
  {
    return result;
  }
  gchar * bname = g_path_get_basename (a->path);
  int a_len = strlen (a->path);
  int b_len = strlen (b);
  if (a_len >2 && b_len>2)
  {
    result = g_strcmp0 (bname,b );
  }
  g_free (bname);
  return result;
}

const gchar *
awn_desktop_lookup_search_by_wnck_window (AwnDesktopLookupCached * lookup, WnckWindow * win)
{
  /*TODO
   search through the wnck window list for the xid then call ^*/
  const gchar * extensions[] = {".exe",".EXE",
                                "-bin","-BIN",
                                ".bin",".BIN",
                                ".sh",".py",".pl",NULL};
  const gchar * result = NULL;
  AwnDesktopLookupCachedPrivate * priv = GET_PRIVATE (lookup);
  gchar * res_name = NULL;
  gchar * class_name = NULL;
  gchar * res_name_lwr = NULL;
  gchar * class_name_lwr = NULL;
  gchar * res_name_no_ext = NULL;
  gchar * class_name_no_ext = NULL;
  gchar * res_name_no_ext_lwr = NULL;
  gchar * full_cmd = NULL;
  gchar * cmd = NULL;
  gchar * cmd_basename = NULL;
  glibtop_proc_args buf;
  gulong xid = wnck_window_get_xid (win);
  GSList *l = NULL;
  const gchar * title;
  gint  hit_method = 0;

  title = wnck_window_get_name (win);
  _wnck_get_wmclass (xid,&res_name, &class_name);
  if (res_name)
  {
    res_name_lwr =  g_utf8_strdown (res_name,-1);
  }
  for (gchar **i= (gchar **)extensions; *i; i++)
  {
    gchar * search = NULL;
    if (res_name)
    {
      search = g_strrstr_len (res_name,-1,*i);
      if ( search )
      {
        if ( strlen (res_name)>(strlen(*i)+3) &&  (strlen (search) == strlen (*i)) )
        {
          res_name_no_ext = g_strdup (res_name);
          res_name_no_ext [strlen (res_name_no_ext) - strlen(*i)]='\0';
        }
      }
    }
    if (class_name)
    {
      search = g_strrstr_len (class_name,-1,*i);
      if ( search )
      {
        if ( strlen (class_name)>(strlen(*i)+3) &&  (strlen (search) == strlen (*i)) )
        {
          class_name_no_ext = g_strdup (class_name);
          class_name_no_ext [strlen (class_name_no_ext) - strlen(*i)]='\0';
        }
      }
    }
  }
  if (res_name_no_ext)
  {
    res_name_no_ext_lwr =  g_utf8_strdown (res_name_no_ext,-1);
  }
  if (class_name)
  {
    class_name_lwr = g_utf8_strdown (class_name,-1);
  }
  cmd = glibtop_get_proc_args (&buf,wnck_window_get_pid (win),1024);    
  full_cmd = get_full_cmd_from_pid ( wnck_window_get_pid (win));
  if (full_cmd)
  {
    g_strstrip (full_cmd);
  }
  if (cmd)
  {
    cmd_basename = g_path_get_basename (cmd);
  }
//  g_debug ("cmd = %s, full_cmd = %s, cmd_basename = %s",cmd,full_cmd,cmd_basename);
//  g_debug ("%s: res_name = '%s', class_name = '%s'",__func__,res_name,class_name);

  /* Checked the special cased data*/
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
        gchar * build_name = g_strdup_printf ("%s.desktop",(gchar *)iter->data);
        l = g_slist_find_custom (priv->desktop_list,
                                    build_name,
                                    (GCompareFunc)_search_path_base_cmp);
        g_free (build_name);
        if (l)
        {
          result = ((DesktopNode*) (l->data))->path;
          break;
        }
      }
      g_slist_free (desktops);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
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
        gchar * build_name = g_strdup_printf ("%s.desktop",(gchar *)iter->data);
        l = g_slist_find_custom (priv->desktop_list,
                                    build_name,
                                    (GCompareFunc)_search_path_base_cmp);
        g_free (build_name);
        if (l)
        {
          result = ((DesktopNode*) (l->data))->path;
          break;
        }
      }
      g_slist_free (desktops);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

  /*
   Look for the full cmd in the exec table
   */
  if (!result)
  {
    if (full_cmd)
    {
      result = g_hash_table_lookup (priv->exec_hash,full_cmd);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

  /*  class name in startupwm hash table?*/
  if (!result)
  {
    if (class_name)
    {
      result = g_hash_table_lookup (priv->startup_wm_hash,class_name);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

  /*  class_name_no_ext in startupwm hash table?*/
  if (!result)
  {
    if (class_name_no_ext)
    {
      result = g_hash_table_lookup (priv->startup_wm_hash,class_name_no_ext);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

  /*res name in startup hash?*/
  if (!result)
  {
    if (res_name)
    {
      result = g_hash_table_lookup (priv->startup_wm_hash,res_name);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

  /*look for %res_name%.desktop */
  if (!result)
  {
    if (res_name)
    {
      gchar * tmp = g_strdup_printf ("%s.desktop",res_name);
      result = g_hash_table_lookup (priv->desktops_hash,tmp);
      g_free (tmp);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

    /*look for res_name_no_ext in startup hash*/
  if (!result)
  {
    if (res_name_no_ext)
    {
      result = g_hash_table_lookup (priv->startup_wm_hash,res_name_no_ext);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

    /*look for %res_name_no_ext%.desktop */
  if (!result)
  {
    if (res_name_no_ext)
    {
      gchar * tmp = g_strdup_printf ("%s.desktop",res_name_no_ext);
      result = g_hash_table_lookup (priv->desktops_hash,tmp);
      g_free (tmp);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (res_name_lwr)
    {
      gchar * tmp = g_strdup_printf ("%s.desktop",res_name_lwr);
      result = g_hash_table_lookup (priv->desktops_hash,tmp);
      g_free (tmp);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

  if (!result)
  {
    if (res_name_no_ext_lwr)
    {
      gchar * tmp = g_strdup_printf ("%s.desktop",res_name_no_ext_lwr);
      result = g_hash_table_lookup (priv->desktops_hash,tmp);
      g_free (tmp);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;

  if (!result)
  {
    if (cmd)
    {
      result = g_hash_table_lookup (priv->exec_hash,cmd);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (cmd_basename)
    {
      result = g_hash_table_lookup (priv->exec_hash,cmd_basename);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (full_cmd)
    {
      l = g_slist_find_custom (priv->desktop_list,
                                    full_cmd,
                                    (GCompareFunc)_search_exec);
      if (l)
      {
        result = ((DesktopNode*) (l->data))->path;
      }
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (full_cmd)
    {
      l = g_slist_find_custom (priv->desktop_list,full_cmd,(GCompareFunc)_search_exec_sub);
      while (l)
      {
        if (g_strstr_len (title,-1,((DesktopNode*) (l->data))->name))
        {
          result = ((DesktopNode*) (l->data))->path;
        }
        l = l->next;
        if (l)
        {
          l = g_slist_find_custom (l,full_cmd,(GCompareFunc)_search_exec_sub);
        }
      }
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (full_cmd)
    {
      l = g_slist_find_custom (priv->desktop_list,
                                    full_cmd,
                                    (GCompareFunc)_search_exec_sub);
      if (l)
      {
        result = ((DesktopNode*) (l->data))->path;
      }
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (cmd)
    {
      gchar * d_filename = g_strdup_printf ("%s.desktop",cmd);
      l = g_slist_find_custom (priv->desktop_list,
                                    d_filename,
                                    (GCompareFunc)_search_path);
      g_free (d_filename);
      if (l)
      {
        result = ((DesktopNode*) (l->data))->path;
      }
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (cmd)
    {
      result = g_hash_table_lookup (priv->name_hash,cmd);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (res_name)
    {
      result = g_hash_table_lookup (priv->exec_hash,res_name);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (res_name_lwr)
    {
      result = g_hash_table_lookup (priv->exec_hash,res_name_lwr);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (res_name)
    {
      result = g_hash_table_lookup (priv->name_hash,res_name);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
  
  if (!result)
  {
    if (res_name_lwr)
    {
      result = g_hash_table_lookup (priv->name_hash,res_name_lwr);
    }
    hit_method ++;
  }
  result = result?(g_file_test(result,G_FILE_TEST_EXISTS)?result:NULL):NULL;
#ifdef DEBUG  
  if (hit_method)
  {
    g_message ("%s: Hit method = %d",__func__,hit_method);
  }
#endif
  g_free (full_cmd);
  g_free (cmd);
  g_free (cmd_basename);
  g_free (res_name);
  g_free (class_name);
  g_free (res_name_lwr);
  g_free (class_name_lwr);
  g_free (res_name_no_ext);
  g_free (class_name_no_ext);
  g_free (res_name_no_ext_lwr);  
  return result;
}
