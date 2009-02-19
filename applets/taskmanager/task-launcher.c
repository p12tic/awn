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
 *
 */

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>

#include <libawn/libawn.h>

#include "task-launcher.h"
#include "task-window.h"

#include "task-settings.h"
#include "xutils.h"

G_DEFINE_TYPE (TaskLauncher, task_launcher, TASK_TYPE_WINDOW);

#define TASK_LAUNCHER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_LAUNCHER, \
  TaskLauncherPrivate))

#define GET_WINDOW_PRIVATE(obj) (TASK_WINDOW(obj)->priv)

struct _TaskLauncherPrivate
{
  gchar          *path;
  AwnDesktopItem *item;

  gchar *name;
  gchar *exec;
  gchar *icon_name;
  gint   pid;
};

enum
{
  PROP_0,
  PROP_DESKTOP_FILE
};

/* Forwards */
static gint          _get_pid         (TaskWindow     *window);
static const gchar * _get_name        (TaskWindow     *window);
static GdkPixbuf   * _get_icon        (TaskWindow     *window);
static gboolean      _is_on_workspace (TaskWindow     *window,
                                       WnckWorkspace  *space);
static void          _activate        (TaskWindow     *window,
                                       guint32         timestamp);
static void          _popup_menu      (TaskWindow     *window,
                                       GtkMenu        *menu);

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
task_launcher_class_init (TaskLauncherClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass    *obj_class = G_OBJECT_CLASS (klass);
  TaskWindowClass *win_class = TASK_WINDOW_CLASS (klass);

  obj_class->set_property = task_launcher_set_property;
  obj_class->get_property = task_launcher_get_property;

  /* We implement the necessary funtions for a normal window */
  win_class->get_pid         = _get_pid;
  win_class->get_name        = _get_name;
  win_class->get_icon        = _get_icon;
  win_class->is_on_workspace = _is_on_workspace;
  win_class->activate        = _activate;
  win_class->popup_menu      = _popup_menu;

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
  priv->item = NULL;
}

TaskLauncher   * 
task_launcher_new_for_desktop_file (const gchar *path)
{
  TaskLauncher *win = NULL;

  if (!g_file_test (path, G_FILE_TEST_EXISTS))
    return NULL;

  win = g_object_new (TASK_TYPE_LAUNCHER,
                      "desktopfile", path,
                      NULL);

  return win;
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
  gchar * exec_key = NULL;
  gchar * needle = NULL;
 
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));
  priv = launcher->priv;

  priv->path = g_strdup (path);

  priv->item = awn_desktop_item_new (path);

  if (priv->item == NULL)
    return;

  priv->name = awn_desktop_item_get_name (priv->item);

  exec_key = g_strstrip (awn_desktop_item_get_exec (priv->item) );
  
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
  priv->exec = exec_key;
  priv->icon_name = awn_desktop_item_get_icon_name (priv->item);

  g_debug ("LAUNCHER: %s", priv->name);
}

/*
 * Implemented functions for a standard window without a launcher
 */
static gint 
_get_pid (TaskWindow *window)
{
  TaskLauncher *launcher = TASK_LAUNCHER (window);
  
  if (WNCK_IS_WINDOW (window->priv->window))
  {
		gint value = -1;
    value = wnck_window_get_pid (window->priv->window);
		value = value ? value : -1;
		return value;
  }
  else
  {
    return launcher->priv->pid;
  }
}

static const gchar * 
_get_name (TaskWindow    *window)
{
  TaskLauncher *launcher = TASK_LAUNCHER (window);

  if (WNCK_IS_WINDOW (window->priv->window))
  {
    return wnck_window_get_name (window->priv->window);
  }
  else
  {
    return launcher->priv->name;
  }
}

static GdkPixbuf * 
_get_icon (TaskWindow    *window)
{
  TaskLauncher *launcher = TASK_LAUNCHER (window);
  TaskSettings *s = task_settings_get_default ();

  if (WNCK_IS_WINDOW (window->priv->window))
  {
    return _wnck_get_icon_at_size (window->priv->window, 
                                   s->panel_size, s->panel_size);
  }
  else
  {
    return awn_desktop_item_get_icon (launcher->priv->item, s->panel_size);
  }
}

static gboolean 
_is_on_workspace (TaskWindow *window,
                  WnckWorkspace *space)
{
  return TRUE;
}

static void   
_activate (TaskWindow    *window,
           guint32        timestamp)
{
  TaskLauncher *launcher = TASK_LAUNCHER (window);
  GError *error = NULL;

  launcher->priv->pid = awn_desktop_item_launch (launcher->priv->item,
                                                 NULL, &error);

  if (error)
  {
    g_warning ("Unable to launch %s: %s", 
               launcher->priv->name,
               error->message);
    g_error_free (error);
  }
}

/*
 * Public functions
 */
gboolean   
task_launcher_has_window (TaskLauncher *launcher)
{
  g_return_val_if_fail (TASK_IS_LAUNCHER (launcher), TRUE);

  if (WNCK_IS_WINDOW (TASK_WINDOW (launcher)->priv->window))
    return TRUE;
  return FALSE;
}

gboolean   
task_launcher_try_match (TaskLauncher *launcher,
                         gint          pid,
                         const gchar  *res_name,
                         const gchar  *class_name)
{
  TaskLauncherPrivate *priv;

  g_return_val_if_fail (launcher, FALSE);
  priv = launcher->priv;

  /* Try simple pid-match first */
  if ( pid && (priv->pid == pid))
    return TRUE;

  /* Now try resource name, which should (hopefully) be 99% of the cases */
  if ( strlen(res_name) && res_name && priv->exec)
  {
    if ( g_strstr_len (priv->exec, strlen (priv->exec), res_name) ||
         g_strstr_len (res_name, strlen (res_name), priv->exec))
    {
      return TRUE;
    }
  }
  
  /* Try a class_name to exec line match */
  if ( strlen(class_name) && class_name && priv->exec)
  {
    if (g_strstr_len (priv->exec, strlen (priv->exec), class_name))
      return TRUE;
  }

  return FALSE; 
}

static void
on_window_closed (TaskLauncher *launcher, WnckWindow *old_window)
{
  TaskWindowPrivate *priv;
  TaskSettings *s = task_settings_get_default ();
  GdkPixbuf *pixbuf;

  g_return_if_fail (TASK_IS_LAUNCHER (launcher));
  priv = TASK_WINDOW (launcher)->priv;

  /* NULLify the window pointer */
  priv->window = NULL;
  g_slist_free (priv->utilities);
  priv->utilities = NULL;

  /* Reset name */
  task_window_set_name (TASK_WINDOW (launcher), launcher->priv->name);  

  /* Reset icon */
  pixbuf = xutils_get_named_icon (launcher->priv->icon_name,
                                  s->panel_size, s->panel_size);

  task_window_update_icon (TASK_WINDOW (launcher), pixbuf);

  g_object_unref (pixbuf);
}

void       
task_launcher_set_window (TaskLauncher *launcher,
                          WnckWindow   *window)
{
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));
  g_return_if_fail (WNCK_IS_WINDOW (window));

  g_object_set (launcher, "taskwindow", window, NULL);

  task_window_set_name (TASK_WINDOW (launcher), wnck_window_get_name (window));

  g_object_weak_ref (G_OBJECT (window), 
                     (GWeakNotify)on_window_closed,
                     launcher);
}

static void 
_popup_menu (TaskWindow     *window,
             GtkMenu        *menu)
{
  TaskWindowPrivate *priv;
  GtkWidget         *item;
  
  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  if (!WNCK_IS_WINDOW (priv->window))
  {
    item = gtk_image_menu_item_new_from_stock (GTK_STOCK_EXECUTE, NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_show (item);
  }
}

void 
task_launcher_launch_with_data (TaskLauncher *launcher,
                                GSList       *list)
{
  GError *error = NULL;
  
  g_return_if_fail (TASK_IS_LAUNCHER (launcher));

  launcher->priv->pid = awn_desktop_item_launch (launcher->priv->item,
                                                 list, &error);

  if (error)
  {
    g_warning ("Unable to launch %s: %s", 
               launcher->priv->name,
               error->message);
    g_error_free (error);
  }
}

void 
task_launcher_middle_click (TaskLauncher   *launcher, 
                            GdkEventButton *event)
{
  TaskLauncherPrivate *priv;
  GError *error = NULL;

  g_return_if_fail (TASK_IS_LAUNCHER (launcher));
  priv = launcher->priv;

  if (WNCK_IS_WINDOW (TASK_WINDOW (launcher)->priv->window))
    awn_desktop_item_launch (priv->item, NULL, &error);
  else
    priv->pid = awn_desktop_item_launch (priv->item, NULL, &error);

  if (error)
  {
    g_warning ("Unable to launch %s: %s", priv->name, error->message);
    g_error_free (error);
  }
}
