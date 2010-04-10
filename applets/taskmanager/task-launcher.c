/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 * Copyright (C) 2009 Rodney Cryderman <rcryderman@gmail.com>
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *             Hannes Verschore <hv1989@gmail.com>
 *             Rodney Cryderman <rcryderman@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>
#include <glib/gi18n.h>

#include <libdesktop-agnostic/fdo.h>
#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>
#include <glibtop/procuid.h>
#include <sys/types.h>
#include <unistd.h>
#include <libawn/libawn.h>
#include <libawn/awn-utils.h>
#include <libawn/awn-pixbuf-cache.h>

#include "task-launcher.h"
#include "task-window.h"

#include "task-settings.h"
#include "xutils.h"
#include "util.h"

#if !GTK_CHECK_VERSION(2,14,0)
#define GTK_ICON_LOOKUP_FORCE_SIZE 0
#endif

G_DEFINE_TYPE (TaskLauncher, task_launcher, TASK_TYPE_ITEM)

#define TASK_LAUNCHER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_LAUNCHER, \
  TaskLauncherPrivate))

struct _TaskLauncherPrivate
{
  gchar *path;
  DesktopAgnosticFDODesktopEntry *entry;
  DesktopAgnosticVFSFile* file_vfs;
  DesktopAgnosticVFSFileMonitor* monitor_vfs;
  
  const gchar *name;
  gchar *exec;
  const gchar *icon_name;
  GPid   pid;
  glong timestamp;
  
  GtkWidget     *menu;
  
  gchar         *special_id;    /*AKA OpenOffice ***** */
  
  GtkWidget *box;
  GtkWidget *name_label;    /*name label*/
  GtkWidget *image;   /*placed in button (TaskItem) with label*/
  GtkWidget *launcher_image;

};

enum
{
  PROP_0,
  PROP_DESKTOP_FILE
};

//#define DEBUG 1

/* Forwards */
static const gchar * _get_name        (TaskItem       *item);
static GdkPixbuf   * _get_icon        (TaskItem       *item);
static gboolean      _is_visible      (TaskItem       *item);
static void          _left_click      (TaskItem       *item,
                                       GdkEventButton *event);
static GtkWidget *   _right_click     (TaskItem       *item,
                                       GdkEventButton *event);
static void          _middle_click     (TaskItem       *item,
                                       GdkEventButton *event);
static guint         _match           (TaskItem       *item,
                                       TaskItem       *item_to_match);
static void         _name_change      (TaskItem *item, 
                                       const gchar *name);
static GtkWidget*    _get_image_widget (TaskItem *item);

static void   task_launcher_set_desktop_file (TaskLauncher *launcher,
                                              const gchar  *path);

/* GObject stuff */
static void
task_launcher_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  TaskLauncher *launcher = TASK_LAUNCHER (object);
  
  switch (prop_id)
  {
    case PROP_DESKTOP_FILE:
      g_value_set_string (value, launcher->priv->path); 
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_launcher_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  TaskLauncher *launcher = TASK_LAUNCHER (object);

  switch (prop_id)
  {
    case PROP_DESKTOP_FILE:
      task_launcher_set_desktop_file (launcher, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_launcher_dispose (GObject *object)
{ 
  TaskLauncherPrivate *priv = TASK_LAUNCHER_GET_PRIVATE (object);

  if (priv->menu)
  {
    gtk_widget_destroy (priv->menu);
    priv->menu = NULL;
  }
  if (priv->file_vfs)
  {
    g_object_unref (priv->file_vfs);
    priv->file_vfs = NULL;
  }
  if (priv->monitor_vfs)
  {
    g_object_unref (priv->monitor_vfs);
    priv->monitor_vfs=NULL;
  }
  if (priv->entry)
  {
    g_object_unref (priv->entry);
    priv->entry = NULL;
  }
  if (priv->box)
  {
    gtk_widget_destroy (priv->box);
    priv->box=NULL;
  }
  G_OBJECT_CLASS (task_launcher_parent_class)->dispose (object);
}

static void
task_launcher_finalize (GObject *object)
{ 
  TaskLauncher *launcher = TASK_LAUNCHER (object);
  TaskLauncherPrivate *priv = TASK_LAUNCHER_GET_PRIVATE (launcher);

  g_free (priv->special_id);
  g_free (priv->path);
  
  G_OBJECT_CLASS (task_launcher_parent_class)->finalize (object);
}

static void
task_launcher_class_init (TaskLauncherClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass  *obj_class = G_OBJECT_CLASS (klass);
  TaskItemClass *item_class = TASK_ITEM_CLASS (klass);

  obj_class->set_property = task_launcher_set_property;
  obj_class->get_property = task_launcher_get_property;
  obj_class->finalize = task_launcher_finalize;
  obj_class->dispose = task_launcher_dispose;

  /* We implement the necessary funtions for a normal window */
  item_class->get_name         = _get_name;
  item_class->get_icon         = _get_icon;
  item_class->is_visible       = _is_visible;
  item_class->match            = _match;
  item_class->left_click       = _left_click;
  item_class->right_click      = _right_click;
  item_class->middle_click      = _middle_click; 
  item_class->name_change       = _name_change;
  item_class->get_image_widget= _get_image_widget;
  
  /* Install properties */
  pspec = g_param_spec_string ("desktopfile",
                               "DesktopFile",
                               "Desktop File Path",
                               NULL,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_DESKTOP_FILE, pspec);

  g_type_class_add_private (obj_class, sizeof (TaskLauncherPrivate));
}

static void
task_launcher_init (TaskLauncher *launcher)
{
  TaskLauncherPrivate *priv;
  GdkPixbuf           *launcher_pbuf;
  gint                icon_height;
  gint                icon_width;
  GtkIconTheme        *theme;	
  priv = launcher->priv = TASK_LAUNCHER_GET_PRIVATE (launcher);
  
  priv->path = NULL;
  priv->entry = NULL;
  
  /* let this button listen to every event */
  gtk_widget_add_events (GTK_WIDGET (launcher), GDK_ALL_EVENTS_MASK);

  /* for looks */
  gtk_button_set_relief (GTK_BUTTON (launcher), GTK_RELIEF_NONE);

  /* create content */
  priv->box = gtk_hbox_new (FALSE, 10);

  gtk_container_add (GTK_CONTAINER (launcher), priv->box);
  gtk_container_set_border_width (GTK_CONTAINER (priv->box), 1);

  priv->image = GTK_WIDGET (awn_image_new ());
  gtk_box_pack_start (GTK_BOX (priv->box), priv->image, FALSE, FALSE, 0);
  
  priv->name_label = gtk_label_new ("");
  /*
   TODO once get/set prop is available create this a config key and bind
   */
  gtk_label_set_max_width_chars (GTK_LABEL(priv->name_label), MAX_TASK_ITEM_CHARS);
  gtk_label_set_ellipsize (GTK_LABEL(priv->name_label),PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (priv->box), priv->name_label, TRUE, TRUE, 10);
  
  priv->launcher_image = GTK_WIDGET (awn_image_new ());  
  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON,&icon_width,&icon_height);
  /*repress annoying gtk icon theme spam*/
  launcher_pbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
                                          "gtk-knows-best",
                                          icon_height,
                                          GTK_ICON_LOOKUP_FORCE_SIZE,
                                            NULL);
  if (launcher_pbuf)
  {
    g_object_unref (launcher_pbuf);
  }

  theme = awn_themed_icon_get_awn_theme (NULL);
  launcher_pbuf = awn_pixbuf_cache_lookup (awn_pixbuf_cache_get_default(),
                                      NULL,
                                      awn_utils_get_gtk_icon_theme_name(theme),
                                      "launcher-program",
                                      -1,
                                      icon_height,
                                      NULL);
  if (!launcher_pbuf)
  {
    launcher_pbuf = gtk_icon_theme_load_icon (theme,
                                            "launcher-program",
                                            icon_height,
                                            GTK_ICON_LOOKUP_FORCE_SIZE,
                                              NULL);
    if (launcher_pbuf)
    {
      awn_pixbuf_cache_insert_pixbuf (awn_pixbuf_cache_get_default(),
                                      launcher_pbuf,
                                      NULL,
                                      awn_utils_get_gtk_icon_theme_name(theme),
                                      "launcher-program");
    }
  }
  
  if (!launcher_pbuf)
  {
    theme = gtk_icon_theme_get_default();
    launcher_pbuf = awn_pixbuf_cache_lookup (awn_pixbuf_cache_get_default(),
                                        NULL,
                                        awn_utils_get_gtk_icon_theme_name(theme),
                                        "launcher-program",
                                        -1,
                                        icon_height,
                                        NULL);
    if (launcher_pbuf)
    {
      awn_pixbuf_cache_insert_pixbuf (awn_pixbuf_cache_get_default(),
                                      launcher_pbuf,
                                      NULL,
                                      awn_utils_get_gtk_icon_theme_name(theme),
                                      "launcher-program");
    }  
  }
  
  if (launcher_pbuf)
  {
    gtk_image_set_from_pixbuf (GTK_IMAGE (priv->launcher_image), launcher_pbuf);
    g_object_unref (launcher_pbuf);
  }
  gtk_box_pack_end (GTK_BOX (priv->box), priv->launcher_image, FALSE, FALSE, 0);  
  
}

TaskItem * 
task_launcher_new_for_desktop_file (AwnApplet * applet, const gchar *path)
{
  TaskItem *item = NULL;

  if (!g_file_test (path, G_FILE_TEST_EXISTS))
    return NULL;

  item = g_object_new (TASK_TYPE_LAUNCHER,
                       "applet",applet,
                       "desktopfile", path,
                       NULL);

  return item;
}

const gchar   * 
task_launcher_get_desktop_path (TaskLauncher *launcher)
{
  g_return_val_if_fail (TASK_IS_LAUNCHER (launcher), NULL);

  return launcher->priv->path;
}

/*
 TODO 
 there's a fair amount of code duplication between this and 
 task_launcher_set_desktop_file().  
 */
 
static void
_desktop_changed (DesktopAgnosticVFSFileMonitor* monitor, 
                       DesktopAgnosticVFSFile* self,                  
                       DesktopAgnosticVFSFile* other, 
                       DesktopAgnosticVFSFileMonitorEvent event,
                       TaskLauncher * launcher
                  )
{
  TaskLauncherPrivate *priv;
  DesktopAgnosticFDODesktopEntry *entry;
  GError * error = NULL;
  gchar * needle;
  gchar * exec_key; 
  GdkPixbuf * pixbuf;
  GdkPixbuf * scaled;  
  gint height,width,scaled_width,scaled_height;
  
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));

  priv = launcher->priv;  
  
  entry = desktop_agnostic_fdo_desktop_entry_new_for_file (self, &error);
  if (error)
  {
    g_critical ("Error when trying to load the launcher: %s", error->message);
    g_error_free (error);
    return;
  }
  if (!usable_desktop_entry (entry) )
  {
    g_critical ("%s: Invalid desktop file, retaining existing valid entries until applet shutdown",__func__);
    return;
  }

  g_object_unref (priv->entry);    
  g_free (priv->special_id);
  priv->entry = entry;
  priv->special_id = get_special_id_from_desktop(priv->entry);
  priv->name = _desktop_entry_get_localized_name (priv->entry);
  task_item_emit_name_changed (TASK_ITEM (launcher), priv->name);

  exec_key = g_strstrip (desktop_agnostic_fdo_desktop_entry_get_string (priv->entry, "Exec"));  
  needle = strchr (exec_key,'%');
  if (needle)
  {
          *needle = '\0';
          g_strstrip (exec_key);
  }
  g_strstrip (exec_key);
  priv->exec = exec_key;
  
  priv->icon_name = desktop_agnostic_fdo_desktop_entry_get_icon (priv->entry);

  pixbuf = _get_icon (TASK_ITEM (launcher));
  
  height = gdk_pixbuf_get_height (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON,&scaled_width,&scaled_height);  
  if (height != scaled_height)
  {
    scaled = gdk_pixbuf_scale_simple (pixbuf,scaled_width,scaled_height,GDK_INTERP_BILINEAR);    
  }
  else
  {
    scaled = pixbuf;
    g_object_ref (scaled);
  }
  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), scaled);
  g_object_unref (scaled);
  
  task_item_emit_icon_changed (TASK_ITEM (launcher), pixbuf);
  g_object_unref (pixbuf);
  task_item_emit_visible_changed (TASK_ITEM (launcher), TRUE);  
}


static void
task_launcher_set_desktop_file (TaskLauncher *launcher, const gchar *path)
{
  TaskLauncherPrivate *priv;
  DesktopAgnosticVFSFile *file;
  GError *error = NULL;
  GdkPixbuf *pixbuf;
  gchar * exec_key = NULL;
  gchar * needle;
  GdkPixbuf    *scaled;
  gint  height;
  gint  width;
  gint  scaled_height;
  gint  scaled_width;  
  
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));
  priv = launcher->priv;

  if (priv->path)
  {
    g_free (priv->path);
  }
  priv->path = g_strdup (path);

  file = desktop_agnostic_vfs_file_new_for_path (path, &error);

  if (error)
  {
    g_critical ("Error when trying to load the launcher: %s", error->message);
    g_error_free (error);
    return;
  }

  if (file == NULL || !desktop_agnostic_vfs_file_exists (file))
  {
    if (file)
    {
      g_object_unref (file);
    }
    g_critical ("File not found: '%s'", path);
    return;
  }

  if (priv->entry)
  {
    g_object_unref (priv->entry);
  }
  priv->entry = desktop_agnostic_fdo_desktop_entry_new_for_file (file, &error);
  
  if (error)
  {
    g_critical ("Error when trying to load the launcher: %s", error->message);
    g_error_free (error);
    g_object_unref (file);    
    return;
  }

  if (!usable_desktop_entry (priv->entry) )
  {
    g_critical ("%s: Invalid desktop file for %s",__func__,path);
    g_object_unref (priv->entry);
    priv->entry = NULL;
    return;
  }

  if (priv->file_vfs)
  {
    g_object_unref (priv->file_vfs);
  }
  priv->file_vfs = desktop_agnostic_vfs_file_new_for_path (path,&error);

  if (error)
  {
    g_warning ("Unable to Monitor %s: %s",path, error->message);
    g_error_free (error);
  }
  else
  {
    if (priv->monitor_vfs)
    {
      g_object_unref (priv->monitor_vfs);
    }
    priv->monitor_vfs = desktop_agnostic_vfs_file_monitor (priv->file_vfs);
    g_signal_connect (G_OBJECT(priv->monitor_vfs),"changed", G_CALLBACK(_desktop_changed),launcher);
  }
  
  g_object_unref (file);  
  if (priv->entry == NULL)
  {
    return;
  }

  if (priv->special_id)
  {
    g_free (priv->special_id);
  }
  priv->special_id = get_special_id_from_desktop(priv->entry);
  priv->name = _desktop_entry_get_localized_name (priv->entry);

  exec_key = g_strstrip (desktop_agnostic_fdo_desktop_entry_get_string (priv->entry, "Exec"));
  
  /*do we have have any % chars? if so... then find the first one , 
   and truncate
   
   There is an open question if we should remove any of other command line 
   args... for now leaving things alone as long as their is no %
   */
  needle = strchr (exec_key,'%');
  if (needle)
  {
          *needle = '\0';
          g_strstrip (exec_key);
  }
  g_strstrip (exec_key);
  if (priv->exec)
  {
    g_free (priv->exec);
  }
  priv->exec = exec_key;
  
  priv->icon_name = desktop_agnostic_fdo_desktop_entry_get_icon (priv->entry);

  task_item_emit_name_changed (TASK_ITEM (launcher), priv->name);

  pixbuf = _get_icon (TASK_ITEM (launcher));
  
  height = gdk_pixbuf_get_height (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  gtk_icon_size_lookup (GTK_ICON_SIZE_BUTTON,&scaled_width,&scaled_height);  
  if (height != scaled_height)
  {
    scaled = gdk_pixbuf_scale_simple (pixbuf,scaled_width,scaled_height,GDK_INTERP_BILINEAR);    
  }
  else
  {
    scaled = pixbuf;
    g_object_ref (scaled);
  }
  
  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), scaled);
  g_object_unref (scaled);
  
  task_item_emit_icon_changed (TASK_ITEM (launcher), pixbuf);
  g_object_unref (pixbuf);
  task_item_emit_visible_changed (TASK_ITEM (launcher), TRUE);
#ifdef DEBUG
  g_debug ("LAUNCHER: %s", priv->name);
#endif
}

/*
 * Implemented functions for a standard window without a launcher
 */
static const gchar * 
_get_name (TaskItem *item)
{
  return TASK_LAUNCHER (item)->priv->name;
}

const gchar * 
task_launcher_get_icon_name (TaskItem *item)
{
  return TASK_LAUNCHER (item)->priv->icon_name;
}

const gchar * 
task_launcher_get_exec (const TaskItem *item)
{
  return TASK_LAUNCHER (item)->priv->exec;
}

static GdkPixbuf * 
_get_icon (TaskItem *item)
{
  TaskLauncherPrivate *priv = TASK_LAUNCHER (item)->priv;
  TaskSettings *s = task_settings_get_default (NULL);
  GError *error = NULL;
  GdkPixbuf *pixbuf = NULL;

  if (priv->icon_name != NULL)
  {
    if (g_path_is_absolute (priv->icon_name))
    {
      pixbuf = gdk_pixbuf_new_from_file_at_scale (priv->icon_name,
                                                  s->panel_size, s->panel_size,
                                                  TRUE, &error);
    }
  }
  else
  {
    // use fallback
    priv->icon_name = g_strdup ("gtk-missing-image");
  }

  if (pixbuf == NULL)
  {
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
    pixbuf = awn_pixbuf_cache_lookup (awn_pixbuf_cache_get_default(),
                                      NULL,
                                      awn_utils_get_gtk_icon_theme_name(icon_theme),
                                      priv->icon_name,
                                      -1,
                                      s->panel_size,
                                      NULL);
    if (!pixbuf)
    {
      pixbuf = gtk_icon_theme_load_icon (icon_theme, priv->icon_name,
                                   s->panel_size, 0, &error);
      if (pixbuf)
      {
        awn_pixbuf_cache_insert_pixbuf (awn_pixbuf_cache_get_default(),
                                        pixbuf,
                                        NULL,
                                        awn_utils_get_gtk_icon_theme_name(icon_theme),
                                        priv->icon_name);
      }
    }
  }
  if (error)
  {
    g_warning ("The launcher '%s' could not load the icon '%s': %s",
               priv->path, priv->icon_name, error->message);
    g_error_free (error);
  }

  return pixbuf;
}

static gboolean
_is_visible (TaskItem *item)
{
  return TRUE;
}

/**
 * Match the launcher with the provided window. 
 * The higher the number it returns the more it matches the window.
 * 100 = definitly matches
 * 0 = doesn't match
 */

/*
FIXME,  ugly.
*/

static guint   
_match (TaskItem *item,
        TaskItem *item_to_match)
{
  TaskLauncherPrivate *priv;
  TaskLauncher *launcher;
  TaskWindow   *window;
  gchar   *res_name = NULL;
  gchar   *class_name = NULL;
  gchar   *res_name_lower = NULL;
  gchar   *class_name_lower = NULL;  
  gint     pid;
  gchar   *cmd = NULL;
  gchar   *full_cmd = NULL;
  gchar   *search_result= NULL;
  gchar * id = NULL;
  
  glibtop_proc_args buf;
  glibtop_proc_uid buf_proc_uid;
  glibtop_proc_uid ppid_buf_proc_uid;  
  glong   timestamp;
  GTimeVal timeval;
  gint    result = 0;
  gchar buffer[256];
  gchar * client_name = NULL;
  const gchar * client_name_to_match = NULL;
  gchar * startup_wm_class = NULL;
  
  gboolean ignore_wm_client_name;
    
  g_return_val_if_fail (TASK_IS_LAUNCHER(item), 0);

  if (!TASK_IS_WINDOW (item_to_match)) 
  {
    return 0;
  }

  launcher = TASK_LAUNCHER (item);
  priv = launcher->priv;
  timestamp = priv->timestamp;
  priv->timestamp = 0;
  window = TASK_WINDOW (item_to_match);

  g_object_get (item,
                "ignore_wm_client_name",&ignore_wm_client_name,
                NULL);
  if (!ignore_wm_client_name)
  {
    gethostname (buffer, sizeof(buffer));
    buffer [sizeof(buffer) - 1] = '\0';
    client_name = g_strdup (buffer);

    client_name_to_match = task_window_get_client_name (TASK_WINDOW(item_to_match));    
    if (!client_name_to_match)
    {
      /*
       WM_CLIENT_NAME is not necessarily set... in those case we'll assume 
       that it's the host
       */
      gethostname (buffer, sizeof(buffer));
      buffer [sizeof(buffer) - 1] = '\0';
      client_name_to_match = buffer;
    }
    if (g_strcmp0(client_name,client_name_to_match)!=0)
    {
      g_free (client_name);      
      return 0;
    }
    g_free (client_name);  
  }
  
  pid = task_window_get_pid(window);
  glibtop_get_proc_uid (&buf_proc_uid,pid);
  glibtop_get_proc_uid (&ppid_buf_proc_uid,buf_proc_uid.ppid);  
  g_get_current_time (&timeval);
  cmd = glibtop_get_proc_args (&buf,pid,1024);
  full_cmd = get_full_cmd_from_pid (pid);
  
  task_window_get_wm_class(window, &res_name, &class_name);  
  if (res_name)
  {
    res_name_lower = g_utf8_strdown (res_name, -1);
  }
  if (class_name)
  {
    class_name_lower = g_utf8_strdown (class_name, -1);
  }
#ifdef DEBUG
  g_debug ("res name lower = %s", res_name_lower);
  g_debug ("class name lower = %s",class_name_lower);
  g_debug ("cmd = %s",cmd);
  g_debug ("fullcmd = %s",full_cmd);
  g_debug ("exec = %s",priv->exec);
#endif
  id = get_special_id_from_window_data (full_cmd, res_name,class_name,task_window_get_name (window));

  
  /* 
   the open office clause follows 
   If either the launcher or the window is special cased then that is the 
   only comparision that will be done.  It's either a match or not on that 
   basis.
   */
  if (priv->special_id && id)
  {
    if (g_strcmp0 (priv->special_id,id) == 0)
    {
      result = 100;
      goto  finished;
    }
  }
  
  if (priv->special_id || id)
  {
    goto finished;  /* result is initialized to 0*/
  }
  
  /*
   Did the pid last launched from the launcher match the pid of the window?
   Note that if each launch starts a new process then those will get matched up
   in the TaskIcon match functions for older windows
   */
#ifdef DEBUG
  g_debug ("window pid = %d, launch pid = %d",pid,priv->pid);
#endif  
  if ( pid && (priv->pid == pid))
  {
    result = 95;
    goto finished;
  } 
  
  /*
   Check the parent PID also
   */
  /*
   Removing to test if they're resulting in some incorrect matches*/   
  /* 
#ifdef DEBUG
  g_debug ("ppid of window pid = %d, launch pid = %d",buf_proc_uid.ppid,priv->pid);
#endif    
  if (pid && buf_proc_uid.ppid)
  {
    if ( buf_proc_uid.ppid == priv->pid)
    {
      result = 92;
      goto finished;
    }

      
#ifdef DEBUG
  g_debug ("ppid of parent pid = %d, launch pid = %d",ppid_buf_proc_uid.ppid,priv->pid);
#endif    
  if (pid && buf_proc_uid.ppid && ppid_buf_proc_uid.ppid)
  {
    if ( ppid_buf_proc_uid.ppid == priv->pid)
    {
      result = 91;
      goto finished;
    }
  }
   */

  if (desktop_agnostic_fdo_desktop_entry_key_exists (priv->entry,"StartupWMClass"))
  {
    startup_wm_class = desktop_agnostic_fdo_desktop_entry_get_string (priv->entry, "StartupWMClass");
    if ( (g_strcmp0 (startup_wm_class,res_name)==0) || (g_strcmp0(startup_wm_class,class_name)==0))
    {
      g_free (startup_wm_class);
      return 94;
    }
    g_free (startup_wm_class);
  }
  
  /*
   Does the command line of the process match exec exactly? 
   Not likely but damn likely to be the correct match if it does
   Note that this will only match a case where there are _no_ arguments.
   full_cmd contains the arg list.
   */
  if (cmd)
  {
    if (g_strcmp0 (cmd, priv->exec)==0)
    {
      result = 90;
      goto finished;
    }
  }
  
  /* 
   Now try resource name, which should (hopefully) be 99% of the cases.
   See if the resouce name is the exec and check if the exec is in the resource
   name.
   */

  if (res_name_lower)
  {
    if ( strlen(res_name_lower)>1 && priv->exec)
    {
      if ( g_strstr_len (priv->exec, strlen (priv->exec), res_name_lower) ||
           g_strstr_len (res_name_lower, strlen (res_name_lower), priv->exec))
      {
        result = 70;
        goto finished;
      }
    }
  }

  /* 
   Try a class_name to exec line match. Same theory as res_name
   */
  if (class_name_lower)
  {
    if (strlen(class_name_lower)>1 && priv->exec)
    {
      if (g_strstr_len (priv->exec, strlen (priv->exec), class_name_lower))
      {
        result = 50;
        goto finished;
      }
    }
  }

  /*
   Is does priv->exec match the end of cmd?
   */
  if (cmd)
  {
    search_result = g_strrstr (cmd, priv->exec);
    if (search_result && 
        ((search_result + strlen(priv->exec)) == (cmd + strlen(cmd))))
    {
      result = 20;
      goto finished;
    }
  }

  /*
   Dubious... thus the rating of 1.
   Let's see how it works in practice
   This may work well enough with some additional fuzzy heuristics.
   */
  if ( timestamp)
  {
    /* was this launcher used in the last 10 seconds?*/
    if (timeval.tv_sec - timestamp < 10)
    {
      /* is the launcher pid set?*/
      if (priv->pid)
      {
        gchar *name = _desktop_entry_get_localized_name (priv->entry);
        GStrv tokens = g_strsplit (name, " ",-1);
        if (tokens && tokens[0] && (strlen (tokens[0])>5) )
        {
          gchar * lower = g_utf8_strdown (tokens[0],-1);          
          if ( g_strstr_len (res_name_lower, -1, lower) )
          {
            g_free (lower);              
            g_strfreev (tokens);
            g_free (name);
            result = 1;
            goto finished;
          }
          g_free (lower);          
        }
        g_strfreev (tokens);
        g_free (name);
      }
    }
  }

finished:
  g_free (res_name);
  g_free (class_name);
  g_free (res_name_lower);
  g_free (class_name_lower);
  g_free (cmd);
  g_free (full_cmd);
  g_free (id);
  return result;
}

static void
_left_click (TaskItem *item, GdkEventButton *event)
{
  TaskLauncherPrivate *priv;
  TaskLauncher *launcher;
  GError *error = NULL;
  GTimeVal timeval;
  gboolean startup_set = FALSE;
  
  g_return_if_fail (TASK_IS_LAUNCHER (item));
  
  launcher = TASK_LAUNCHER (item);
  priv = launcher->priv;

  if (desktop_agnostic_fdo_desktop_entry_key_exists (priv->entry,G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY))
  {
    GStrv tokens1;
    GStrv tokens2;
    gchar * screen_name = NULL;
    gchar * id = g_strdup_printf("awn_task_manager_%u_TIME%u",getpid(),event?event->time:gtk_get_current_event_time ());
    gchar * display_name = gdk_screen_make_display_name (gdk_screen_get_default());
    tokens1 = g_strsplit (display_name,":",2);
    if (tokens1 && tokens1[1])
    {
      tokens2 = g_strsplit(tokens1[1],".",2);
      g_strfreev (tokens1);
      if (tokens2 && tokens2[1])
      {
        screen_name = g_strdup (tokens2[1]);
        g_strfreev (tokens2);
      }
      else
      {
        if (tokens2)
        {
          g_strfreev (tokens2);
          screen_name = g_strdup ("0");          
        }
      }
    }
    else
    {
      if (tokens1)
      {
        g_strfreev (tokens1);
      }
      screen_name = g_strdup ("0");
    }
    
    gdk_x11_display_broadcast_startup_message (gdk_display_get_default(),
                                               "new",
                                               "ID",id,
                                               "NAME",priv->name,
                                               "SCREEN",screen_name,
                                               NULL);
    //                                               "PID",pid,
    g_setenv ("DESKTOP_STARTUP_ID",id,TRUE);
    startup_set = TRUE;
    g_free (id);
    g_free (screen_name);
  }
  priv->pid = desktop_agnostic_fdo_desktop_entry_launch (priv->entry,
                                               0, NULL, &error);  
  if (startup_set)
  {
    g_unsetenv ("DESKTOP_STARTUP_ID");
  }
  g_get_current_time (&timeval);
  priv->timestamp = timeval.tv_sec;

#ifdef DEBUG  
  g_debug ("%s: current time = %ld",__func__,timeval.tv_sec);  
  g_debug ("%s: launch pid = %d",__func__,priv->pid);
#endif
  if (error)
  {
    g_warning ("Unable to launch %s: %s", 
               task_item_get_name (item),
               error->message);
    g_error_free (error);
  }
}

static GtkWidget *
_right_click (TaskItem *item, GdkEventButton *event)
{
  TaskLauncherPrivate *priv;
  TaskLauncher *launcher;
  GtkWidget *menu_item;
  gint width,height;  
  
  GdkPixbuf * launcher_pbuf = NULL;
  
  g_return_val_if_fail (TASK_IS_LAUNCHER (item),NULL);
  
  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU,&width,&height);

  launcher_pbuf = awn_pixbuf_cache_lookup (awn_pixbuf_cache_get_default(),
                                      NULL,
                                      awn_utils_get_gtk_icon_theme_name(gtk_icon_theme_get_default()),
                                      "launcher-program",
                                      -1,
                                      height,
                                      NULL);
  if (!launcher_pbuf)
  {
    launcher_pbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default(),
                                            "launcher-program",
                                            height,
                                            GTK_ICON_LOOKUP_FORCE_SIZE,
                                            NULL);
    if (launcher_pbuf)
    {
      awn_pixbuf_cache_insert_pixbuf (awn_pixbuf_cache_get_default(),
                                      launcher_pbuf,
                                      NULL,
                                      awn_utils_get_gtk_icon_theme_name(gtk_icon_theme_get_default()),
                                      "launcher-program");
    }
  }

  launcher = TASK_LAUNCHER (item);
  priv = launcher->priv;

  if (!priv->menu)
  {
    priv->menu = gtk_menu_new ();

    menu_item = gtk_separator_menu_item_new();
    gtk_widget_show_all(menu_item);
    gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), menu_item);

    menu_item = awn_applet_create_pref_item();
    gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), menu_item);

    menu_item = gtk_separator_menu_item_new();
    gtk_widget_show(menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), menu_item);
    
    menu_item = gtk_image_menu_item_new_with_label (_("Launch"));
    if (launcher_pbuf)
    {
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                  gtk_image_new_from_pixbuf (launcher_pbuf));
    }
    gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), menu_item);
    gtk_widget_show (menu_item);
    g_signal_connect_swapped (menu_item,"activate",
                      G_CALLBACK(_middle_click),
                      item);        
  }

#if GTK_CHECK_VERSION (2,16,0)	
  /* null op if GTK+ < 2.16.0*/
  awn_utils_show_menu_images (GTK_MENU(priv->menu));
#endif
  
  gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, 
                  NULL, NULL, event->button, event->time);
  g_object_unref (launcher_pbuf);  
  return priv->menu;
}

static void 
_middle_click (TaskItem *item, GdkEventButton *event)
{
  TaskLauncherPrivate *priv;
  TaskLauncher *launcher;
  GError *error = NULL;
  
  g_return_if_fail (TASK_IS_LAUNCHER (item));
  
  launcher = TASK_LAUNCHER (item);
  priv = launcher->priv;

  priv->pid = desktop_agnostic_fdo_desktop_entry_launch (priv->entry, 0,
                                                         NULL, &error);

  if (error)
  {
    g_warning ("Unable to launch %s: %s", priv->name, error->message);
    g_error_free (error);
  }
}


/*
 * Public functions
 */

void 
task_launcher_launch_with_data (TaskLauncher *launcher,
                                GSList       *list)
{
  GError *error = NULL;
  
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));

  launcher->priv->pid =
    desktop_agnostic_fdo_desktop_entry_launch (launcher->priv->entry,
                                               0, list, &error);

  if (error)
  {
    g_warning ("Unable to launch %s: %s", 
               launcher->priv->name,
               error->message);
    g_error_free (error);
  }
}

static void 
_name_change (TaskItem *item, const gchar *name)
{
  TaskLauncherPrivate *priv;
  
  g_return_if_fail (TASK_IS_LAUNCHER (item));
  gchar * tmp;
  gchar * markup;

  priv = TASK_LAUNCHER (item)->priv;  
  
  tmp = g_strdup_printf (_("Launch %s"),name);
  markup = g_markup_printf_escaped ("<span font_family=\"Sans\" font_weight=\"bold\">%s</span>", tmp);
  gtk_label_set_markup (GTK_LABEL (priv->name_label), markup);  
  TASK_ITEM_CLASS (task_launcher_parent_class)->name_change (item, name);  
  g_free (tmp);
  g_free (markup);
}

static GtkWidget *
_get_image_widget (TaskItem *item)
{
  TaskLauncherPrivate *priv = TASK_LAUNCHER (item)->priv;
  
  return priv->image;
}
