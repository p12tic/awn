/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *             Hannes Verschore <hv1989@gmail.com>
 */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>

#include <libdesktop-agnostic/desktop-entry.h>
#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>

#include <libawn/libawn.h>

#include "task-launcher.h"
#include "task-item.h"
#include "task-window.h"

#include "task-settings.h"
#include "xutils.h"

G_DEFINE_TYPE (TaskLauncher, task_launcher, TASK_TYPE_ITEM)

#define TASK_LAUNCHER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_LAUNCHER, \
  TaskLauncherPrivate))

struct _TaskLauncherPrivate
{
  gchar *path;
  DesktopAgnosticDesktopEntryBackend *entry;

  const gchar *name;
  const gchar *exec;
  const gchar *icon_name;
  GPid   pid;
};

enum
{
  PROP_0,
  PROP_DESKTOP_FILE
};

/* Forwards */
static const gchar * _get_name        (TaskItem       *item);
static GdkPixbuf   * _get_icon        (TaskItem       *item);
static gboolean      _is_visible      (TaskItem       *item);
static void          _left_click      (TaskItem       *item,
                                       GdkEventButton *event);
static void          _right_click     (TaskItem       *item,
                                       GdkEventButton *event);
static void          _middle_click     (TaskItem       *item,
                                       GdkEventButton *event);
static guint         _match           (TaskItem       *item,
                                       TaskItem       *item_to_match);

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
  G_OBJECT_CLASS (task_launcher_parent_class)->dispose (object);
}

static void
task_launcher_finalize (GObject *object)
{ 
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
  	
  priv = launcher->priv = TASK_LAUNCHER_GET_PRIVATE (launcher);
  
  priv->path = NULL;
  priv->entry = NULL;
}

TaskItem * 
task_launcher_new_for_desktop_file (const gchar *path)
{
  TaskItem *item = NULL;

  if (!g_file_test (path, G_FILE_TEST_EXISTS))
    return NULL;

  item = g_object_new (TASK_TYPE_LAUNCHER,
                       "desktopfile", path,
                       NULL);

  return item;
}

const gchar   * 
task_launcher_get_desktop_path     (TaskLauncher *launcher)
{
  g_return_val_if_fail (TASK_IS_LAUNCHER (launcher), NULL);

  return launcher->priv->path;
}

static void
task_launcher_set_desktop_file (TaskLauncher *launcher, const gchar *path)
{
  TaskLauncherPrivate *priv;
  DesktopAgnosticVFSFile *file;
  GError *error = NULL;
  GdkPixbuf *pixbuf;
 
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));
  priv = launcher->priv;

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
    g_critical ("File not found: '%s'", path);
    return;
  }

  priv->entry = desktop_agnostic_desktop_entry_new_for_file (file, &error);

  if (error)
  {
    g_critical ("Error when trying to load the launcher: %s", error->message);
    g_error_free (error);
    return;
  }

  if (priv->entry == NULL)
  {
    return;
  }

  priv->name = desktop_agnostic_desktop_entry_backend_get_name (priv->entry);

  priv->exec = g_strstrip (desktop_agnostic_desktop_entry_backend_get_string (priv->entry, "Exec"));
  
  priv->icon_name = desktop_agnostic_desktop_entry_backend_get_icon (priv->entry);

  task_item_emit_name_changed (TASK_ITEM (launcher), priv->name);
  pixbuf = _get_icon (TASK_ITEM (launcher));
  task_item_emit_icon_changed (TASK_ITEM (launcher), pixbuf);
  g_object_unref (pixbuf);
  task_item_emit_visible_changed (TASK_ITEM (launcher), TRUE);

  g_debug ("LAUNCHER: %s", priv->name);
}

/*
 * Implemented functions for a standard window without a launcher
 */
static const gchar * 
_get_name (TaskItem *item)
{
  return TASK_LAUNCHER (item)->priv->name;
}

static GdkPixbuf * 
_get_icon (TaskItem *item)
{
  TaskLauncherPrivate *priv = TASK_LAUNCHER (item)->priv;
  TaskSettings *s = task_settings_get_default ();
  GError *error = NULL;
  GdkPixbuf *pixbuf = NULL;

  if (g_path_is_absolute (priv->icon_name))
  {
    pixbuf = gdk_pixbuf_new_from_file_at_scale (priv->icon_name,
                                                s->panel_size, s->panel_size,
                                                TRUE, &error);
  }
  else
  {
    GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();

    pixbuf = gtk_icon_theme_load_icon (icon_theme, priv->icon_name,
                                       s->panel_size, 0, &error);
  }
  if (error)
  {
    g_warning ("The launcher '%s' could not load the icon '%s': %s",
               priv->path, priv->icon_name, error->message);
    g_error_free (error);
    return NULL;
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
static guint   
_match (TaskItem *item,
        TaskItem *item_to_match)
{
  TaskLauncherPrivate *priv;
  TaskLauncher *launcher;
  TaskWindow   *window;
  gchar   *res_name = NULL;
  gchar   *class_name = NULL;
  gchar   *temp;
  gint     pid;
  glibtop_proc_args buf;
  gchar   *cmd;
  gchar   *search_result;

  g_return_val_if_fail (TASK_IS_LAUNCHER(item), 0);

  if (!TASK_IS_WINDOW (item_to_match)) 
  {
    return 0;
  }

  launcher = TASK_LAUNCHER (item);
  priv = launcher->priv;
  
  window = TASK_WINDOW (item_to_match);

  /* Try simple pid-match first */
  pid = task_window_get_pid(window);
  
#ifdef DEBUG
  g_debug ("%s:  Pid = %d,  win pid = %d",__func__,pid,priv->pid);
#endif 
  if ( pid && (priv->pid == pid))
  {
    return 100;
  }

  /*does the command line of the process match exec exactly... not likely but
  damn likely to be the correct match if it does*/
  cmd = glibtop_get_proc_args (&buf,pid,1024);    
  #ifdef DEBUG
  g_debug ("%s:  cmd = '%s', exec = '%s'",__func__,cmd,priv->exec);
  #endif  
  if (cmd)
  {
    if (g_strcmp0 (cmd, priv->exec)==0)
    {
      #ifdef DEBUG
      g_debug ("%s:  strcmp match ",__func__);
      #endif
      g_free (cmd);
      return 90;
    }
  }
  
  /* Now try resource name, which should (hopefully) be 99% of the cases */
  task_window_get_wm_class(window, &res_name, &class_name);

  if (res_name)
  {
    temp = res_name;
    res_name = g_utf8_strdown (temp, -1);
    g_free (temp);

    if ( strlen(res_name) && priv->exec)
    {
      #ifdef DEBUG
      g_debug ("%s: 70  res_name = %s,  exec = %s",__func__,res_name,priv->exec);
      #endif 
      if ( g_strstr_len (priv->exec, strlen (priv->exec), res_name) ||
           g_strstr_len (res_name, strlen (res_name), priv->exec))
      {
        g_free (res_name);
        g_free (class_name);
        g_free (cmd);
        return 70;
      }
    }
  }

  /* Try a class_name to exec line match */
  if (class_name)
  {
    temp = class_name;
    class_name = g_utf8_strdown (temp, -1);
    g_free (temp);

    if (strlen(class_name) && priv->exec)
    {
      #ifdef DEBUG
      g_debug ("%s: 50  priv->exec = %s,  class_name = %s",__func__,priv->exec,class_name);
      #endif 
      if (g_strstr_len (priv->exec, strlen (priv->exec), class_name))
      {
        g_free (res_name);
        g_free (class_name);
        g_free (cmd);        
        return 50;
      }
    }
  }

  g_free (res_name);
  g_free (class_name);
  
  if (cmd)
  {
    search_result = g_strrstr (cmd, priv->exec);
    #ifdef DEBUG
    g_debug ("cmd = %p, search_result = %p, strlen(exec) = %u, strlen (cmd) =%u",
             cmd,search_result,(guint)strlen(priv->exec),(guint)strlen(cmd));
    #endif
    if (search_result && 
        ((search_result + strlen(priv->exec)) == (cmd + strlen(cmd))))
    {
      #ifdef DEBUG
      g_debug ("exec matches end of command line.");
      #endif
      g_free (cmd);
      return 20;
    }
  }
  g_free (cmd);
  return 0; 
}

static void
_left_click (TaskItem *item, GdkEventButton *event)
{
  TaskLauncherPrivate *priv;
  TaskLauncher *launcher;
  GError *error = NULL;
  
  g_return_if_fail (TASK_IS_LAUNCHER (item));
  
  launcher = TASK_LAUNCHER (item);
  priv = launcher->priv;

  priv->pid = 
    desktop_agnostic_desktop_entry_backend_launch (priv->entry,
                                                   0, NULL, &error);

#ifdef DEBUG  
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

static void
_right_click (TaskItem *item, GdkEventButton *event)
{
  TaskLauncherPrivate *priv;
  TaskLauncher *launcher;
  GtkWidget *menu_item,
            *menu;
  
  g_return_if_fail (TASK_IS_LAUNCHER (item));
  
  launcher = TASK_LAUNCHER (item);
  priv = launcher->priv;

  menu = gtk_menu_new ();
  menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_EXECUTE, NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show (menu_item);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, 
                  NULL, NULL, event->button, event->time);
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

  priv->pid = desktop_agnostic_desktop_entry_backend_launch (priv->entry, 0,
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
    desktop_agnostic_desktop_entry_backend_launch (launcher->priv->entry,
                                                   0, list, &error);

  if (error)
  {
    g_warning ("Unable to launch %s: %s", 
               launcher->priv->name,
               error->message);
    g_error_free (error);
  }
}

