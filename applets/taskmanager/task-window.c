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
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <libwnck/libwnck.h>
#include "libawn/gseal-transition.h"

#undef G_DISABLE_SINGLE_INCLUDES
#include <glibtop/procargs.h>

#include "task-window.h"
#include "task-settings.h"
#include "xutils.h"
#include "util.h"

#if !GTK_CHECK_VERSION(2,14,0)
#define GTK_ICON_LOOKUP_FORCE_SIZE 0
#endif

G_DEFINE_TYPE (TaskWindow, task_window, TASK_TYPE_ITEM)

#define TASK_WINDOW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_WINDOW, \
  TaskWindowPrivate))

struct _TaskWindowPrivate
{
  WnckWindow *window;

  WnckWindow *last_active_non_taskmanager_window;
  // Workspace where the window should be in, before being visible.
  // NULL if it isn't important
  WnckWorkspace *workspace;

  // Is this window in the workspace. If workspace is NULL, this is always TRUE;
  gboolean in_workspace;

  AwnApplet   *applet;
  
  /* Properties */
  gchar   *message;
  gfloat   progress;

  /*Controls whether the item has been explicitly hidden using the dbus api.
   This is no longer related to the hide/show controled by the show_all_windows
   config key*/
  gboolean hidden;
  gboolean needs_attention;
  gboolean is_active;
  gboolean highlighted;
  
  gint     use_win_icon;
  
  gint     activate_behavior;
  
  GtkWidget         *menu;
  
  gchar * special_id; /*Thank you OpenOffice*/
  
  GtkWidget *box;
  GtkWidget *name;    /*name label*/
  GtkWidget *image;   /*placed in button (TaskItem) with label*/

  /*number of application initiated icon changes*/
  guint     icon_changes;

  gchar     *client_name;
};

enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_ACTIVATE_BEHAVIOR,
  PROP_USE_WIN_ICON,
  PROP_HIGHLIGHTED
};

enum
{
  ACTIVE_CHANGED,
  NEEDS_ATTENTION,
  WORKSPACE_CHANGED,
  MESSAGE_CHANGED,
  PROGRESS_CHANGED,
  HIDDEN_CHANGED,

  LAST_SIGNAL
};

typedef enum 
{
  AWN_ACTIVATE_DEFAULT,
  AWN_ACTIVATE_MOVE_TO_TASK_VIEWPORT,
  AWN_ACTIVATE_MOVE_TASK_TO_ACTIVE_VIEWPORT
} AwnActivateBehavior;

static guint32 _window_signals[LAST_SIGNAL] = { 0 };

/* Forwards */
static const gchar * _get_name        (TaskItem       *item);
static GdkPixbuf   * _get_icon        (TaskItem       *item);
static gboolean      _is_visible      (TaskItem       *item);
static void          _left_click      (TaskItem *item, GdkEventButton *event);
static GtkWidget *   _right_click     (TaskItem *item, GdkEventButton *event);
static guint         _match           (TaskItem *item, TaskItem *item_to_match);
static GtkWidget*    _get_image_widget (TaskItem *item);

static void   task_window_set_window (TaskWindow *window,
                                      WnckWindow *wnckwin);
static void   task_window_check_for_special_case (TaskWindow * window);

static void   _active_window_changed  (WnckScreen *screen,
                                      WnckWindow *previously_active_window,
                                      TaskWindow * item);

static void   theme_changed_cb (GtkIconTheme *icon_theme,TaskWindow * window);

/* GObject stuff */
static void
task_window_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  TaskWindow *taskwin = TASK_WINDOW (object);
  TaskWindowPrivate *priv;
  priv = TASK_WINDOW_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, taskwin->priv->window); 
      break;

    case PROP_ACTIVATE_BEHAVIOR:
      g_value_set_int (value, taskwin->priv->activate_behavior); 
      break;

    case PROP_USE_WIN_ICON:
      g_value_set_int (value, taskwin->priv->use_win_icon); 
      break;

    case PROP_HIGHLIGHTED:
      g_value_set_boolean (value, taskwin->priv->highlighted); 
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_window_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  TaskWindow *taskwin = TASK_WINDOW (object);
  TaskWindowPrivate *priv;
  priv = TASK_WINDOW_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      task_window_set_window (taskwin, g_value_get_object (value));
      break;
      
    case PROP_ACTIVATE_BEHAVIOR:
      priv->activate_behavior = g_value_get_int (value);
      break;

    case PROP_USE_WIN_ICON:
      priv->use_win_icon = g_value_get_int (value);
      break;

    case PROP_HIGHLIGHTED:
      task_window_set_highlighted (taskwin,g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static gboolean
do_bind_property (DesktopAgnosticConfigClient *client, const gchar *key,
                  GObject *object, const gchar *property)
{
  GError *error = NULL;
  
  desktop_agnostic_config_client_bind (client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       key, object, property, TRUE,
                                       DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       &error);
  if (error)
  {
    g_warning ("Could not bind property '%s' to key '%s': %s", property, key,
               error->message);
    g_error_free (error);
    return FALSE;
  }
	
  return TRUE;
}

static void
task_window_constructed (GObject *object)
{
  TaskWindowPrivate *priv;
  
  priv = TASK_WINDOW_GET_PRIVATE (object);
  
  g_object_get (object,
                "applet",&priv->applet,
                NULL);
  
 /*  TaskWindowPrivate *priv = TASK_WINDOW (object)->priv;*/  
  if ( G_OBJECT_CLASS (task_window_parent_class)->constructed)
  {
    G_OBJECT_CLASS (task_window_parent_class)->constructed (object);
  }
  g_signal_connect (wnck_screen_get_default(),"active-window-changed",
                    G_CALLBACK(_active_window_changed),object);
  g_signal_connect(G_OBJECT(gtk_icon_theme_get_default()),
                   "changed",
                   G_CALLBACK(theme_changed_cb),object);

  if (!do_bind_property (awn_config_get_default_for_applet (priv->applet, NULL), "activate_behavior", object,
                         "activate_behavior"))
  {
    return;
  }
  priv->client_name = NULL;
}

static void
task_window_dispose (GObject *object)
{
  TaskWindowPrivate *priv = TASK_WINDOW (object)->priv; 
  GError  * err = NULL;
  /*TaskItem will also do this, so it shouldn't be necessary in TaskWindow.*/
  if (priv->applet)
  {
    desktop_agnostic_config_client_unbind_all_for_object (awn_config_get_default_for_applet (priv->applet, NULL), object, &err);
    if (err)
    {
      g_warning ("%s: Failed to unbind_all: %s",__func__,err->message);
      g_error_free (err);
    }
    priv->applet = NULL;
  }  
  if (priv->menu)
  {
    gtk_widget_destroy (priv->menu);
    priv->menu=NULL;
  }

  if (priv->box)
  {
    gtk_widget_destroy (priv->box);
    priv->box=NULL;
  }
  
  G_OBJECT_CLASS (task_window_parent_class)->dispose (object);
}


static void
task_window_finalize (GObject *object)
{
  TaskWindowPrivate *priv = TASK_WINDOW (object)->priv; 
  g_signal_handlers_disconnect_by_func(wnck_screen_get_default(), 
                                       G_CALLBACK (_active_window_changed), 
                                       object);
  g_free (priv->client_name);
  g_free (priv->special_id);
  g_free (priv->message);
  g_signal_handlers_disconnect_by_func (G_OBJECT(gtk_icon_theme_get_default()),
                          G_CALLBACK(theme_changed_cb),object);  

  G_OBJECT_CLASS (task_window_parent_class)->finalize (object);
  
}


static void
task_window_class_init (TaskWindowClass *klass)
{
  GParamSpec    *pspec;
  GObjectClass  *obj_class  = G_OBJECT_CLASS (klass);
  TaskItemClass *item_class = TASK_ITEM_CLASS (klass);

  obj_class->set_property = task_window_set_property;
  obj_class->get_property = task_window_get_property;
  obj_class->constructed  = task_window_constructed;
  obj_class->finalize = task_window_finalize;
  obj_class->dispose = task_window_dispose;

  /* We implement the necessary funtions for an item */
  item_class->get_name        = _get_name;
  item_class->get_icon        = _get_icon;
  item_class->is_visible      = _is_visible;
  item_class->left_click      = _left_click;
  item_class->right_click      = _right_click;
  item_class->match           = _match;
  item_class->get_image_widget= _get_image_widget;
  
  /* Install signals */
  _window_signals[ACTIVE_CHANGED] =
		g_signal_new ("active-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, active_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN); 

  _window_signals[NEEDS_ATTENTION] =
		g_signal_new ("needs-attention",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, needs_attention),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN);   

  _window_signals[WORKSPACE_CHANGED] =
		g_signal_new ("workspace-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, workspace_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
            1, WNCK_TYPE_WORKSPACE);

  _window_signals[MESSAGE_CHANGED] =
		g_signal_new ("message-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, message_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE,
            1, G_TYPE_STRING);

  _window_signals[PROGRESS_CHANGED] =
		g_signal_new ("progress-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, progress_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
            1, G_TYPE_FLOAT);

  _window_signals[HIDDEN_CHANGED] =
		g_signal_new ("hidden-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskWindowClass, hidden_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN);

  /* Install properties */
  pspec = g_param_spec_object ("taskwindow",
                               "Window",
                               "WnckWindow",
                               WNCK_TYPE_WINDOW,
                               G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_WINDOW, pspec);  
  
  pspec = g_param_spec_int ("activate_behavior",
                               "Activate Behavior",
                               "Activate Behavior",
                               AWN_ACTIVATE_DEFAULT,
                               AWN_ACTIVATE_MOVE_TASK_TO_ACTIVE_VIEWPORT,
                               AWN_ACTIVATE_DEFAULT,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_ACTIVATE_BEHAVIOR, pspec);  

  pspec = g_param_spec_int ("use_win_icon",
                               "Use the Applications Window icon",
                               "Use the Applications Window icon",
                                USE_DEFAULT,
                                USE_NEVER,
                                1,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_USE_WIN_ICON, pspec);  

  pspec = g_param_spec_boolean ("highlighted",
                               "Highlight",
                               "Highlight the item",
                                FALSE,
                                G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_HIGHLIGHTED, pspec);  

  g_type_class_add_private (obj_class, sizeof (TaskWindowPrivate));
}

static void
task_window_init (TaskWindow *window)
{
  TaskWindowPrivate *priv;
  GtkWidget         *alignment;
  
  priv = window->priv = TASK_WINDOW_GET_PRIVATE (window);

  priv->workspace = NULL;
  priv->in_workspace = TRUE;
  priv->message = NULL;
  priv->progress = 0;
  priv->hidden = FALSE;
  priv->needs_attention = FALSE;
  priv->is_active = FALSE;
  priv->use_win_icon = USE_DEFAULT;

  /* let this button listen to every event */
  gtk_widget_add_events (GTK_WIDGET (window), GDK_ALL_EVENTS_MASK);

  /* for looks */
  gtk_button_set_relief (GTK_BUTTON (window), GTK_RELIEF_NONE);

  /* create content */
  priv->box = gtk_hbox_new (FALSE, 10);

  alignment = gtk_alignment_new (0.0,0.5,0.0,0.0);
  gtk_container_add (GTK_CONTAINER(alignment),priv->box);

  gtk_container_add (GTK_CONTAINER (window), alignment);
  gtk_container_set_border_width (GTK_CONTAINER (priv->box), 1);

  priv->image = GTK_WIDGET (awn_image_new ());
  gtk_box_pack_start (GTK_BOX (priv->box), priv->image, FALSE, FALSE, 0);
  
  priv->name = gtk_label_new ("");
  priv->icon_changes = 0;
  /*
   TODO once get/set prop is available create this a config key and bind
   */
  gtk_label_set_max_width_chars (GTK_LABEL(priv->name), MAX_TASK_ITEM_CHARS);
  gtk_label_set_ellipsize (GTK_LABEL(priv->name),PANGO_ELLIPSIZE_END);
  gtk_box_pack_start (GTK_BOX (priv->box), priv->name, TRUE, FALSE, 0);
  
}

TaskItem *
task_window_new (AwnApplet * applet, WnckWindow *window)
{
  TaskItem *win = NULL;

  win = g_object_new (TASK_TYPE_WINDOW,
                      "taskwindow", window,
                      "applet",applet,
                      NULL);

  return win;
}


static void
task_window_check_for_special_case (TaskWindow * window)
{
  gchar * full_cmd;
  gchar   *res_name = NULL;
  gchar   *class_name = NULL;
  TaskWindowPrivate *priv = window->priv;
  
  full_cmd = get_full_cmd_from_pid (task_window_get_pid(window));
  task_window_get_wm_class(window, &res_name, &class_name);  
  priv->special_id = get_special_id_from_window_data (full_cmd, 
                                    res_name, 
                                    class_name,
                                    task_window_get_name (window));
  g_free (full_cmd);    
  g_free (res_name);
  g_free (class_name);
}
/*
 * Handling of the main WnckWindow
 */
static void
window_closed (TaskWindow *window, WnckWindow *old_window)
{  
  gtk_widget_destroy (GTK_WIDGET (window));
}

static void
on_window_name_changed (WnckWindow *wnckwin, TaskWindow *window)
{
  TaskWindowPrivate *priv;
  gchar * markup;
  const gchar * name = NULL;
  
  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));
  priv = window->priv;

  name = wnck_window_get_name (wnckwin);
  if (priv->highlighted)
  {
    markup = g_markup_printf_escaped ("<span font_style=\"italic\" font_weight=\"heavy\" font_family=\"Sans\" font_stretch=\"ultracondensed\">%s</span>", name);
  }
  else
  {
    markup = g_markup_printf_escaped ("<span font_family=\"Sans\" font_stretch=\"ultracondensed\">%s</span>", name);
  }
  gtk_label_set_markup (GTK_LABEL (priv->name), markup);
  g_free (markup);  
  task_item_emit_name_changed (TASK_ITEM (window), name);  
}

static void
on_window_icon_changed (WnckWindow *wnckwin, TaskWindow *window)
{
  TaskSettings *s = task_settings_get_default (NULL);
  GdkPixbuf    *pixbuf;
  GdkPixbuf    *scaled;
  gint  height;
  gint  width;
  gint  scaled_height;
  gint  scaled_width;  
  TaskWindowPrivate *priv;

  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));

  priv = window->priv;
  
  pixbuf = _wnck_get_icon_at_size (wnckwin, s->panel_size, s->panel_size);
  
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

  priv->icon_changes ++;
  task_item_emit_icon_changed (TASK_ITEM (window), pixbuf);
  g_object_unref (pixbuf);
}

static void
on_window_workspace_changed (WnckWindow *wnckwin, TaskWindow *window)
{
  TaskWindowPrivate *priv;
    
  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));

  priv = window->priv;
  if (priv->workspace==NULL)
    priv->in_workspace = TRUE;
  else
    priv->in_workspace = wnck_window_is_in_viewport (priv->window, priv->workspace);

  if (priv->in_workspace && !priv->hidden)
  {
    gtk_widget_show (GTK_WIDGET(window));
    task_item_emit_visible_changed (TASK_ITEM (window), TRUE);
  }
  else
  {
    gtk_widget_hide (GTK_WIDGET(window));
    task_item_emit_visible_changed (TASK_ITEM (window), FALSE);
  }
  g_signal_emit (window, _window_signals[WORKSPACE_CHANGED], 
                 0, wnck_window_get_workspace (wnckwin));
}

static void
on_window_state_changed (WnckWindow      *wnckwin,
                         WnckWindowState  changed_mask,
                         WnckWindowState  state,
                         TaskWindow      *window)
{
  TaskWindowPrivate *priv;
  gboolean           visible = TRUE;
  gboolean           needs_attention = FALSE;
    
  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WINDOW (wnckwin));
  priv = window->priv;

  if ( state & WNCK_WINDOW_STATE_SKIP_TASKLIST)
  {
    visible = FALSE;
  }
  
  if (gtk_widget_get_visible (GTK_WIDGET (window)) != visible)
  {
    if (!visible || priv->hidden)
    {
      gtk_widget_hide (GTK_WIDGET(window));
    }
    else
    {
      gtk_widget_show (GTK_WIDGET(window));
    }

    if (priv->in_workspace && visible && gtk_widget_get_visible (GTK_WIDGET (window)) && !priv->hidden)
      task_item_emit_visible_changed (TASK_ITEM (window), TRUE);
    else
      task_item_emit_visible_changed (TASK_ITEM (window), FALSE);      
  }

  needs_attention = wnck_window_or_transient_needs_attention (wnckwin);

  if (priv->needs_attention != needs_attention)
  {
    priv->needs_attention = needs_attention;
    g_signal_emit (window, _window_signals[NEEDS_ATTENTION], 
                   0, needs_attention);
  }
  
}

static void   _active_window_changed  (WnckScreen *screen,
                                      WnckWindow *previously_active_window,
                                      TaskWindow * item)
{
  TaskWindowPrivate *priv;
  priv = item->priv;
  WnckWindow * win = wnck_screen_get_active_window (screen);
  if (!win)
  {
    win = previously_active_window;
  }
  if ( win && ( getpid() != wnck_window_get_pid ( win) ) )
  {
    priv->last_active_non_taskmanager_window = win;
  }
}
/**
 * TODO: remove old signals and weak_ref...
 */
static void
task_window_set_window (TaskWindow *window, WnckWindow *wnckwin)
{
  TaskWindowPrivate *priv;
  GdkPixbuf    *pixbuf;
  gchar * markup;
  TaskSettings *s = task_settings_get_default (NULL);
  
  g_return_if_fail (TASK_IS_WINDOW (window));

  priv = window->priv;
  priv->window = wnckwin;
  task_window_check_for_special_case (window);
  g_object_weak_ref (G_OBJECT (priv->window), 
                     (GWeakNotify)window_closed, window);

  g_signal_connect (wnckwin, "name-changed", 
                    G_CALLBACK (on_window_name_changed), window);
  g_signal_connect (wnckwin, "icon-changed",
                    G_CALLBACK (on_window_icon_changed), window);
  g_signal_connect (wnckwin, "workspace-changed",
                    G_CALLBACK (on_window_workspace_changed), window);
  g_signal_connect (wnckwin, "state-changed", 
                    G_CALLBACK (on_window_state_changed), window);

  if (priv->highlighted)
  {
    markup = g_markup_printf_escaped ("<span font_style=\"italic\" font_weight=\"heavy\" font_family=\"Sans\" font_stretch=\"ultracondensed\">%s</span>", wnck_window_get_name (wnckwin));
  }
  else
  {
    markup = g_markup_printf_escaped ("<span font_family=\"Sans\" font_stretch=\"ultracondensed\">%s</span>", wnck_window_get_name (wnckwin));
  }
  task_item_emit_name_changed (TASK_ITEM (window), markup);
  on_window_name_changed (wnckwin,window);
  on_window_icon_changed (wnckwin,window);
  g_free (markup);
  pixbuf = _wnck_get_icon_at_size (wnckwin, s->panel_size, s->panel_size);
  task_item_emit_icon_changed (TASK_ITEM (window), pixbuf);
  g_object_unref (pixbuf);
  task_item_emit_visible_changed (TASK_ITEM (window), TRUE);
}

/*
 * Public functions
 */

void
task_window_set_highlighted (TaskWindow *window, gboolean highlight_state)
{
  TaskWindowPrivate *priv;
  
  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  if (highlight_state != priv->highlighted)
  {
    const gchar * name;
    gchar * markup;
    
    priv->highlighted = highlight_state;
    name = wnck_window_get_name (priv->window);
    if (priv->highlighted)
    {
      markup = g_markup_printf_escaped ("<span font_style=\"italic\" font_weight=\"heavy\" font_family=\"Sans\" font_stretch=\"ultracondensed\">%s</span>", name);
    }
    else
    {
      markup = g_markup_printf_escaped ("<span font_family=\"Sans\" font_stretch=\"ultracondensed\">%s</span>", name);
    }
    gtk_label_set_markup (GTK_LABEL (priv->name), markup);
    g_free (markup);
  }
}

/**
 * Returns the name of the WnckWindow.
 */
const gchar *
task_window_get_name (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), "");

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_name (window->priv->window);
  
  return "";
}

gboolean 
task_window_get_icon_is_fallback(TaskWindow * window)
{
  TaskWindowPrivate *priv;
  
  g_return_val_if_fail (TASK_IS_WINDOW (window), TRUE);
  priv = window->priv;
  
  return wnck_window_get_icon_is_fallback (priv->window);
}

const gchar *
task_window_get_client_name (TaskWindow *window)
{
  TaskWindowPrivate *priv;
  
  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);
  priv = window->priv;
  
  if (!priv->client_name)
  {
    task_window_get_wm_client(window, &priv->client_name);
  }
  return priv->client_name;
}


/*
 return the total number of icon changes 
 */
guint
task_window_get_icon_changes (TaskWindow * window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), 0);

  return window->priv->icon_changes;
}

WnckScreen * 
task_window_get_screen (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), wnck_screen_get_default ());

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_screen (window->priv->window);
  
  return wnck_screen_get_default ();
}

gulong 
task_window_get_xid (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), 0);

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_xid (window->priv->window);
  
  return 0;
}

gint 
task_window_get_pid (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), -1);
  
	gint pid = 0;
  if (WNCK_IS_WINDOW (window->priv->window))
	{
    pid = wnck_window_get_pid (window->priv->window);
	}
  
	return pid;  
}

WnckApplication *
task_window_get_application (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_application (window->priv->window);

  return NULL;
}

WnckWindow *
task_window_get_window (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);

  if (WNCK_IS_WINDOW (window->priv->window))
    return window->priv->window;

  return NULL;
}

gboolean   
task_window_get_wm_class (TaskWindow    *window,
                          gchar        **res_name,
                          gchar        **class_name)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);
 
  *res_name = NULL;
  *class_name = NULL;
  
  if (WNCK_IS_WINDOW (window->priv->window))
  {
    _wnck_get_wmclass (wnck_window_get_xid (window->priv->window),
                       res_name, class_name);
    
    if (*res_name || *class_name)
      return TRUE;
  }

  return FALSE;
}

gboolean   
task_window_get_wm_client (TaskWindow    *window,
                          gchar        **client_name)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);
 
  *client_name = NULL;
  
  if (WNCK_IS_WINDOW (window->priv->window))
  {
    _wnck_get_client_name (wnck_window_get_xid (window->priv->window),client_name);
    
    if (*client_name)
      return TRUE;
  }

  return FALSE;
}

gboolean   
task_window_is_active (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_is_active (window->priv->window);
  
  return FALSE;
}

void 
task_window_set_is_active  (TaskWindow    *window,
                            gboolean       is_active)
{
  g_return_if_fail (TASK_IS_WINDOW (window));

  window->priv->is_active = is_active;
  
  g_signal_emit (window, _window_signals[ACTIVE_CHANGED], 0, is_active);
}

gboolean 
task_window_get_needs_attention (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  if (WNCK_IS_WINDOW (window->priv->window))
  {
    window->priv->needs_attention = wnck_window_or_transient_needs_attention 
                              (window->priv->window);
    return window->priv->needs_attention;
  }

  return FALSE;
}

const gchar * 
task_window_get_message (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), NULL);

  return window->priv->message;
}

gfloat   
task_window_get_progress (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), 0.0);

  return window->priv->progress;
}

gboolean
task_window_is_hidden (TaskWindow    *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  return !gtk_widget_get_visible (GTK_WIDGET (window));
}

/*hidden should be a prop*/
void
task_window_set_hidden (TaskWindow *window,gboolean hidden)
{
  TaskWindowPrivate * priv;
  g_return_if_fail (TASK_IS_WINDOW(window));
  priv=window->priv;

  priv->hidden = hidden;
  if (priv->in_workspace && !hidden)
  {
    gtk_widget_show (GTK_WIDGET(window));
  }
  else
  {
    gtk_widget_hide (GTK_WIDGET(window));
  }
  task_item_emit_visible_changed (TASK_ITEM (window), !hidden);
}

void
task_window_set_active_workspace   (TaskWindow    *window,
                                    WnckWorkspace *space)
{
  TaskWindowPrivate *priv;

  g_return_if_fail (TASK_IS_WINDOW (window));
  g_return_if_fail (WNCK_IS_WORKSPACE (space) || space == NULL);

  priv = window->priv;
  priv->workspace = space;
  priv->in_workspace = (space==NULL)?TRUE:wnck_window_is_in_viewport (priv->window, space);
  
  if (priv->in_workspace && !priv->hidden)
    task_item_emit_visible_changed (TASK_ITEM (window), TRUE);
  else
    task_item_emit_visible_changed (TASK_ITEM (window), FALSE);
}

gboolean   
task_window_is_on_workspace (TaskWindow    *window,
                             WnckWorkspace *space)
{
  TaskWindowPrivate *priv;
  
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);
  g_return_val_if_fail (WNCK_IS_WORKSPACE (space), FALSE);

  priv = window->priv;

  if (WNCK_IS_WINDOW (priv->window))
    return wnck_window_is_in_viewport (priv->window, space);

  return FALSE;
}

/*
 * Lifted from libwnck/tasklist.c
 */
static gboolean
really_activate (WnckWindow *window, guint32 timestamp)
{
  WnckWindowState  state;
  WnckWorkspace   *active_ws;
  WnckWorkspace   *window_ws;
  WnckScreen      *screen;
  gboolean         switch_workspace_on_unminimize = TRUE;
  gboolean         was_minimised = FALSE;

  screen = wnck_window_get_screen (window);
  state = wnck_window_get_state (window);
  active_ws = wnck_screen_get_active_workspace (screen);
  window_ws = wnck_window_get_workspace (window);
	
  if (state & WNCK_WINDOW_STATE_MINIMIZED)
  {
    if (window_ws &&
        active_ws != window_ws &&
        switch_workspace_on_unminimize)
    {
      wnck_workspace_activate (window_ws, timestamp);
    }
    wnck_window_activate_transient (window, timestamp);
  }
  else
  {
    if ((wnck_window_is_active (window) ||
         wnck_window_transient_is_most_recently_activated (window)) &&
         (!window_ws || active_ws == window_ws))
    {
      wnck_window_minimize (window);
      was_minimised = TRUE;
    }
    else
    {
      /* FIXME: THIS IS SICK AND WRONG AND BUGGY. See the end of
       * http://mail.gnome.org/archives/wm-spec-list/005-July/msg0003.html
       * There should only be *one* activate call.
       */
      if (window_ws)
        wnck_workspace_activate (window_ws, timestamp);
     
      wnck_window_activate_transient (window, timestamp);
    }
  } 
  return was_minimised;
}

void 
task_window_activate (TaskWindow    *window,
                      guint32        timestamp)
{
  TaskWindowPrivate *priv;
  WnckScreen *screen;
  WnckWorkspace *active_workspace, *window_workspace;

  g_return_if_fail (TASK_IS_WINDOW (window));

  priv = window->priv;
  if (WNCK_IS_WINDOW (priv->window))
  {
    switch (priv->activate_behavior)
    {
      case AWN_ACTIVATE_MOVE_TO_TASK_VIEWPORT:
        screen = wnck_window_get_screen(priv->window);
        active_workspace = wnck_screen_get_active_workspace (screen);
        window_workspace = wnck_window_get_workspace (priv->window);

        if (active_workspace && window_workspace &&
              !wnck_window_is_in_viewport(priv->window, active_workspace))
        {
          wnck_workspace_activate(window_workspace, timestamp);
        }
        really_activate (window->priv->window, timestamp);
        break;

      case AWN_ACTIVATE_MOVE_TASK_TO_ACTIVE_VIEWPORT:
        screen = wnck_window_get_screen(priv->window);
        active_workspace = wnck_screen_get_active_workspace (screen);

        wnck_window_move_to_workspace(priv->window, active_workspace);
        wnck_window_activate (window->priv->window, timestamp);
        break;

      case AWN_ACTIVATE_DEFAULT:
      default:
        really_activate (window->priv->window, timestamp);        
        break;
    }
    

  }
}

void  
task_window_minimize (TaskWindow    *window)
{
  g_return_if_fail (TASK_IS_WINDOW (window));

  if (WNCK_IS_WINDOW (window->priv->window))
    wnck_window_minimize (window->priv->window);

}

void     
task_window_close (TaskWindow    *window,
                   guint32        timestamp)
{
  g_return_if_fail (TASK_IS_WINDOW (window));

  if (WNCK_IS_WINDOW (window->priv->window))
    wnck_window_close (window->priv->window, timestamp);

}

GtkWidget *   
task_window_popup_context_menu (TaskWindow     *window,
                                GdkEventButton *event)
{
  TaskWindowClass *klass;
  TaskWindowPrivate *priv;
  GtkWidget         *item;

  g_return_val_if_fail (TASK_IS_WINDOW (window),NULL);
  g_return_val_if_fail (event,NULL);
  priv = window->priv;
  
  klass = TASK_WINDOW_GET_CLASS (window);

  if (priv->menu)
  {
    gtk_widget_destroy (priv->menu);
  }
  priv->menu = wnck_action_menu_new (priv->window);
  
  item = gtk_separator_menu_item_new();
  gtk_widget_show_all(item);
  gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), item);

  item = awn_applet_create_pref_item();
  gtk_menu_shell_prepend(GTK_MENU_SHELL(priv->menu), item);

  item = gtk_separator_menu_item_new();
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(priv->menu), item);
  
  gtk_menu_popup (GTK_MENU (priv->menu), NULL, NULL, 
                  NULL, NULL, event->button, event->time);
  return priv->menu;
}

void    
task_window_set_icon_geometry (TaskWindow    *window,
                               gint           x,
                               gint           y,
                               gint           width,
                               gint           height)
{
  TaskWindowPrivate *priv;

  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

  /* FIXME: Do something interesting like dividing the width by the number of
   * WnckWindows so the user can scrub through them
   */
  if (WNCK_IS_WINDOW (priv->window))
    wnck_window_set_icon_geometry (priv->window, x, y, width, height);

}

gboolean    
task_window_get_is_running (TaskWindow *window)
{
  g_return_val_if_fail (TASK_IS_WINDOW (window), FALSE);

  return WNCK_IS_WINDOW (window->priv->window);
}

gboolean
task_window_matches_wmclass (TaskWindow * task_window,const gchar * name)
{
  gchar * res_name = NULL;
  gchar * class_name = NULL;
  gboolean result = FALSE;
  TaskWindowPrivate *priv;
  
  g_return_val_if_fail (TASK_IS_WINDOW (task_window),FALSE);

  priv = task_window->priv;
  _wnck_get_wmclass (wnck_window_get_xid (priv->window),&res_name, &class_name);
  result = ( (g_strcmp0 (res_name, name) == 0) || (g_strcmp0 (class_name, name) == 0) );
  g_free (res_name);
  g_free (class_name);
  return result;
}
/*
 * Implemented functions for a window
 */

static const gchar * 
_get_name (TaskItem    *item)
{
  TaskWindow *window = TASK_WINDOW (item);
  
  if (WNCK_IS_WINDOW (window->priv->window))
    return wnck_window_get_name (window->priv->window);

  return NULL;
}

static GdkPixbuf * 
_get_icon (TaskItem    *item)
{
  /*
   TODO:
   Once the pixbuf lookups and caching get moved into a separate object in
   AwnThemedIcon then this static can be replaced by use of that new object
   */
  static GdkPixbuf * fallback=NULL;
  TaskSettings *s = task_settings_get_default (NULL);
  TaskWindow *window = TASK_WINDOW (item);
  TaskWindowPrivate *priv = TASK_WINDOW (item)->priv;

  if (WNCK_IS_WINDOW (window->priv->window))
  {
    if (wnck_window_get_icon_is_fallback (priv->window))
    {
      if (fallback && (gdk_pixbuf_get_height (fallback) != s->panel_size))
      {
        g_object_unref (fallback);
        fallback = NULL;        
      }
      if (!fallback)
      {
        fallback = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                             "awn-window-fallback",
                                             s->panel_size,
                                             GTK_ICON_LOOKUP_FORCE_SIZE,
                                             NULL);
      }
      if (fallback)
      {
        g_object_ref (fallback);
        return fallback;
      }
      g_warning ("%s: Failed to load awn fallback.  Falling back to wnck fallback.",__func__);
    }
    return _wnck_get_icon_at_size (window->priv->window, 
                                 s->panel_size, s->panel_size);
  }
  return NULL;
}

static gboolean
_is_visible (TaskItem *item)
{
  TaskWindowPrivate *priv = TASK_WINDOW (item)->priv;
  
  return priv->in_workspace && !priv->hidden && gtk_widget_get_visible (GTK_WIDGET (item));
}

static GtkWidget *
_get_image_widget (TaskItem *item)
{
  TaskWindowPrivate *priv = TASK_WINDOW (item)->priv;
  
  return priv->image;
}

static void
theme_changed_cb (GtkIconTheme *icon_theme,TaskWindow * window)
{
  TaskWindowPrivate *priv;
  
  g_return_if_fail (TASK_IS_WINDOW (window));
  priv = window->priv;

}

static void
_left_click (TaskItem *item, GdkEventButton *event)
{
  TaskWindowPrivate *priv = TASK_WINDOW (item)->priv;  
  guint timestamp = event?event->time:gtk_get_current_event_time ();
  
  if ( (priv->window == priv->last_active_non_taskmanager_window) && 
      !wnck_window_is_minimized(priv->last_active_non_taskmanager_window))
  {
    /*activate before we minimize so the dialog loses focus*/
    task_window_activate (TASK_WINDOW (item), timestamp);
    task_window_minimize (TASK_WINDOW (item) );
  }
  else
  {
    task_window_activate (TASK_WINDOW (item), timestamp);
  }
}

/*
 FIXME,  ugly, ugly,ugly.
 */
static GtkWidget *
_right_click (TaskItem *item, GdkEventButton *event)
{
  return task_window_popup_context_menu (TASK_WINDOW (item), event);
}

static guint   
_match (TaskItem *item,
        TaskItem *item_to_match)
{
  TaskWindowPrivate *priv;
  TaskWindow   *window;
  TaskWindow   *window_to_match;
  gchar   *res_name = NULL;
  gchar   *class_name = NULL;
  gchar   *res_name_to_match = NULL;
  gchar   *class_name_to_match = NULL;  
  gchar   *temp;
  gint      pid;
  gint      pid_to_match;
  gchar   *full_cmd_to_match;
  gchar   *full_cmd = NULL;
  gchar   *id;
  gboolean ignore_wm_client_name;
  
  g_return_val_if_fail (TASK_IS_WINDOW(item), 0);

  if (!TASK_IS_WINDOW (item_to_match)) 
  {
    return 0;
  }

  window = TASK_WINDOW (item);
  priv = window->priv;

  g_object_get (item,
                "ignore_wm_client_name",&ignore_wm_client_name,
                NULL);
  /*
  TODO:
   Stuff the client_names into TaskWindow....  call task_window_get_wm_client
   once and save in a field
   */
  if (!ignore_wm_client_name)
  {
    gchar buffer[256];
    gchar buffer_to_match[256];
    const gchar   *client_name = NULL;
    const gchar   *client_name_to_match = NULL;

    /*check the client names to begin with*/
    client_name=task_window_get_client_name (TASK_WINDOW(item));
    if (!client_name)
    {
      /*
       WM_CLIENT_NAME is not necessarily set... in those case we'll assume 
       that it's the host
       */
      gethostname (buffer, sizeof(buffer));
      buffer [sizeof(buffer) - 1] = '\0';
      client_name = buffer;
    }
    client_name_to_match = task_window_get_client_name (TASK_WINDOW(item_to_match));
    if (!client_name_to_match)
    {
      /*
       WM_CLIENT_NAME is not necessarily set... in those case we'll assume 
       that it's the host
       */
      gethostname (buffer_to_match, sizeof(buffer_to_match));
      buffer_to_match [sizeof(buffer_to_match) - 1] = '\0';
      client_name_to_match = buffer_to_match;
    }

    if (g_strcmp0(client_name,client_name_to_match)!=0)
    {
      return 0;
    }
  }  
  window_to_match = TASK_WINDOW (item_to_match);
  pid = task_window_get_pid (window);
  pid_to_match = task_window_get_pid (window_to_match);

//#define DEBUG 1
  /* special case? */
  full_cmd_to_match = get_full_cmd_from_pid (pid_to_match);
  task_window_get_wm_class(window_to_match, &res_name_to_match, &class_name_to_match);
  id = get_special_id_from_window_data (full_cmd_to_match, res_name_to_match,
                                        class_name_to_match,
                                        task_window_get_name (window_to_match));  
  /* the open office clause follows */
#ifdef DEBUG
    g_debug ("%s, compare %s,%s------------------------",__func__,priv->special_id,id);
#endif

  if (priv->special_id && id)
  {
    if (g_strcmp0 (priv->special_id,id) == 0)
    {
      g_free (res_name_to_match);
      g_free (class_name_to_match);
      g_free (full_cmd_to_match);
      g_free (id);
      return 99;
    }
  }
  if (priv->special_id || id)
  {
      g_free (res_name_to_match);
      g_free (class_name_to_match);
      g_free (full_cmd_to_match);
      g_free (id);
      return 0;
  }
  
  if (pid)
  {
    full_cmd = get_full_cmd_from_pid (pid);
  }
  if (full_cmd && g_strcmp0 (full_cmd, full_cmd_to_match) == 0)
  {
    g_free (res_name_to_match);
    g_free (class_name_to_match);
    g_free (full_cmd_to_match);
    g_free (full_cmd);
    g_free (id);    
    return 95;
  }
  
  g_free (full_cmd_to_match);
  g_free (id);
  
  /* Try simple pid-match next */

#ifdef DEBUG
  g_debug ("%s:  Pid to match = %d,  pid = %d",__func__,pid_to_match,pid);
#endif 
  if ( pid && ( pid_to_match == pid ))
  {
    g_free (full_cmd);    
    g_free (res_name_to_match);
    g_free (class_name_to_match);        
    return 94;
  }

  /* Now try resource name, which should (hopefully) be 99% of the cases */
  task_window_get_wm_class(window, &res_name, &class_name);
  
  if (res_name && res_name_to_match)
  {
    temp = res_name;
    res_name = g_utf8_strdown (temp, -1);
    g_free (temp);

    temp = res_name_to_match;
    res_name_to_match = g_utf8_strdown (temp, -1);
    g_free (temp);

    if ( strlen (res_name_to_match) && strlen (res_name) )
    {
      #ifdef DEBUG
      g_debug ("%s: 70  res_name = %s,  res_name_to_match = %s",__func__,res_name,res_name_to_match);
      #endif 
      if ( g_strcmp0 (res_name, res_name_to_match) == 0)
      {
        g_free (res_name);
        g_free (class_name);
        g_free (res_name_to_match);
        g_free (class_name_to_match);
        g_free (full_cmd);
        return 65;
      }
    }
  }
  g_free (full_cmd);
  g_free (res_name);
  g_free (class_name);
  g_free (res_name_to_match);
  g_free (class_name_to_match);
  return 0; 
}

WinIconUse
task_window_get_use_win_icon (TaskWindow * item)
{
  TaskWindowPrivate *priv;
  g_return_val_if_fail (TASK_IS_WINDOW(item),0);
  priv = item->priv;
  
  return priv->use_win_icon;
}

void
task_window_set_use_win_icon (TaskWindow * item, WinIconUse win_use)
{
  TaskWindowPrivate *priv;
  g_return_if_fail (TASK_IS_WINDOW(item));
  priv = item->priv;
  
  priv->use_win_icon = win_use;
}
  
