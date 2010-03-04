/*
 * Copyright (C) 2008 Neil Jagdish Patel <njpatel@gmail.com>
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
 * Authored by Hannes Verschore <hv1989@gmail.com>
 *
 */

#include "task-item.h"

#include <libawn/libawn.h>

#define TASK_ITEM_ICON_SCALE 0.65

G_DEFINE_ABSTRACT_TYPE (TaskItem, task_item, GTK_TYPE_BUTTON)

#define TASK_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ITEM, \
  TaskItemPrivate))

struct _TaskItemPrivate
{
  GdkPixbuf *icon;

  TaskIcon *task_icon;
  AwnApplet * applet;
  gboolean  ignore_wm_client_name;
};

enum
{
  PROP_0,
  PROP_APPLET,
  PROP_IGNORE_WM_CLIENT_NAME
};

enum
{
  NAME_CHANGED,
  ICON_CHANGED,
  VISIBLE_CHANGED,

  LAST_SIGNAL
};
static guint32 _item_signals[LAST_SIGNAL] = { 0 };

/* Forwards */

static void task_item_icon_changed      (TaskItem *item, GdkPixbuf     *icon);
static void task_item_visible_changed   (TaskItem *item, gboolean       visible);
static void task_item_name_changed      (TaskItem *item, const gchar   *name);
static gboolean  task_item_button_release_event (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_item_button_press_event   (GtkWidget      *widget,
                                                 GdkEventButton *event);
static void task_item_activate (GtkWidget *widget, gpointer null);

/* GObject stuff */

static void
task_item_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  TaskItem        *icon = TASK_ITEM (object);
  TaskItemPrivate *priv = icon->priv;

  switch (prop_id)
  {
    case PROP_APPLET:
      g_value_set_object (value,priv->applet);
      break;
    case PROP_IGNORE_WM_CLIENT_NAME:
      g_value_set_boolean (value,priv->ignore_wm_client_name);
      break;      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
task_item_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  TaskItem *icon = TASK_ITEM (object);
  TaskItemPrivate *priv = icon->priv;
  
  switch (prop_id)
  {
    case PROP_APPLET:
      priv->applet = g_value_get_object (value);
      break;
    case PROP_IGNORE_WM_CLIENT_NAME:
      priv->ignore_wm_client_name = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}


static void
task_item_dispose (GObject *object)
{
  TaskItem *item = TASK_ITEM (object);
  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (object);
  GError  * err = NULL;
 
  if (priv->icon)
  {
    g_object_unref (priv->icon);
    priv->icon = NULL;
  }

  // this removes the overlays from the associated TaskIcon
  task_item_set_task_icon (item, NULL);

  item->text_overlay = NULL;
  item->progress_overlay = NULL;
  item->icon_overlay = NULL;

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
  
  G_OBJECT_CLASS (task_item_parent_class)->dispose (object);
}


static void
task_item_finalize (GObject *object)
{
  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (object);
  GError  * err = NULL;

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
  G_OBJECT_CLASS (task_item_parent_class)->finalize (object);
}

static void
task_item_constructed (GObject *object)
{
  TaskItemClass *klass;  
  klass = TASK_ITEM_GET_CLASS (object);  
  GError *error = NULL;
  DesktopAgnosticConfigClient *client;
  TaskItemPrivate *priv = TASK_ITEM(object)->priv;
  
  g_return_if_fail (klass->is_visible);
  
  if (G_OBJECT_CLASS (task_item_parent_class)->constructed)
  {
    G_OBJECT_CLASS (task_item_parent_class)->constructed (object);
  }

  g_assert (priv->applet);
  client = awn_config_get_default_for_applet (priv->applet, &error);
  desktop_agnostic_config_client_bind (client,
                                       DESKTOP_AGNOSTIC_CONFIG_GROUP_DEFAULT,
                                       "ignore_wm_client_name", object, "ignore_wm_client_name", 
                                       TRUE,DESKTOP_AGNOSTIC_CONFIG_BIND_METHOD_FALLBACK,
                                       &error);
  if (error)
  {
    g_warning ("Could not bind property ignore_wm_client_name:%s",error->message);
    g_error_free (error);
  }

  /* connect to signals */
  g_signal_connect (G_OBJECT (object), "name-changed",
                    G_CALLBACK (klass->name_change), NULL);
  g_signal_connect (G_OBJECT (object), "visible-changed",
                    G_CALLBACK (task_item_visible_changed), NULL);
  g_signal_connect (G_OBJECT (object), "activate",
                    G_CALLBACK (task_item_activate), NULL);
  g_signal_connect (G_OBJECT (object), "icon-changed",
                    G_CALLBACK (task_item_icon_changed), NULL);  
}


static void
task_item_class_init (TaskItemClass *klass)
{
  GParamSpec   *pspec;
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->dispose = task_item_dispose;
  obj_class->finalize = task_item_finalize;
  obj_class->constructed = task_item_constructed;  
  obj_class->set_property = task_item_set_property;
  obj_class->get_property = task_item_get_property;

  wid_class->button_release_event = task_item_button_release_event;
  wid_class->button_press_event   = task_item_button_press_event;
  
  /* We implement the necessary funtions for a normal item */
  klass->get_name        = NULL;
  klass->get_icon        = NULL;
  klass->is_visible      = NULL;
  klass->match           = NULL;
  klass->name_change    = task_item_name_changed;
  klass->get_image_widget = task_item_get_image_widget;
  
  /* Install signals */
  _item_signals[NAME_CHANGED] =
		g_signal_new ("name-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskItemClass, name_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING, 
			      G_TYPE_NONE,
            1, G_TYPE_STRING); 

  _item_signals[ICON_CHANGED] =
		g_signal_new ("icon-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskItemClass, icon_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT, 
			      G_TYPE_NONE,
            1, GDK_TYPE_PIXBUF);

  _item_signals[VISIBLE_CHANGED] =
		g_signal_new ("visible-changed",
			      G_OBJECT_CLASS_TYPE (obj_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TaskItemClass, visible_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN, 
			      G_TYPE_NONE,
            1, G_TYPE_BOOLEAN);

  pspec = g_param_spec_object ("applet",
                               "Applet",
                               "AwnApplet this item belongs to",
                               AWN_TYPE_APPLET,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_APPLET, pspec);

  pspec = g_param_spec_boolean ("ignore_wm_client_name",
                               "Ignore WM_CLIENT_NAME",
                               "Ignore WM_CLIENT_NAME for grouping purposes",
                               FALSE,
                               G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
  g_object_class_install_property (obj_class, PROP_IGNORE_WM_CLIENT_NAME, pspec);

  g_type_class_add_private (obj_class, sizeof (TaskItemPrivate));
}

static void
task_item_init (TaskItem *item)
{
  TaskItemPrivate *priv;
  TaskItemClass *klass;

  klass = TASK_ITEM_GET_CLASS (item);  
  
  /* get and save private struct */
  priv = item->priv = TASK_ITEM_GET_PRIVATE (item);  
}


static void
unset_inhibit_focus_loss_cb (TaskIcon * icon)
{
      task_icon_set_inhibit_focus_loss (icon,FALSE);
}

static gboolean
task_item_button_release_event (GtkWidget      *widget,
                                GdkEventButton *event)
{
  GtkWidget * menu;
  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (widget);

  g_return_val_if_fail (TASK_IS_ITEM (widget), FALSE);

  switch (event->button)
  {
    case 1:
      task_item_left_click (TASK_ITEM (widget), event);
      break;
    case 2:
      task_item_middle_click (TASK_ITEM (widget), event);
      break;
    case 3:
      /*
       Crazy hoops to keep the dialog open when bringing up a context menu.
      Keeps it from closing due to focus loss.
       */
      task_icon_set_inhibit_focus_loss (priv->task_icon,TRUE);
      menu = task_item_right_click (TASK_ITEM (widget), event);
      g_signal_connect_swapped (menu,"deactivate", 
                                G_CALLBACK(gtk_widget_hide),
                                task_icon_get_dialog(priv->task_icon));
      g_signal_connect_swapped (menu,"deactivate",
                                G_CALLBACK(unset_inhibit_focus_loss_cb),
                                priv->task_icon);
      break;
  }

  return FALSE;
}

static gboolean
task_item_button_press_event (GtkWidget      *widget,
                              GdkEventButton *event)
{
  g_return_val_if_fail (TASK_IS_ITEM (widget), FALSE);
  gtk_widget_queue_draw (widget);

  return FALSE;
}

static void 
task_item_activate (GtkWidget *widget, gpointer null)
{
  g_return_if_fail (TASK_IS_ITEM (widget));
  
  /*
   FIXME TODO
   This works for now as the GdkEventButton arg has already
   been checked before this.. and it's not checked 
   by any of the called code.
   
   That being said... It probably makes sense to remove
   the event arg from the parameters to task_item_*_click()
   and the functions called by them.  If it doesn't make sense
   then this needs to this function call needs to be replaced 
   with something else :-)
   */
  task_item_left_click (TASK_ITEM(widget), NULL);
}

static void 
task_item_name_changed (TaskItem *item, const gchar *name)
{
/*  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (item); */
}

static void 
task_item_icon_changed (TaskItem *item, GdkPixbuf *icon)
{
  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (item);
  
  g_return_if_fail (icon);
  g_return_if_fail (GDK_IS_PIXBUF(icon));
  if (priv->icon)
  {
    g_object_unref (priv->icon);
  }
  priv->icon = icon;
  g_object_ref (icon);
  
  /* height should be equal to width... but just in case */  
}

static void
task_item_visible_changed (TaskItem *item, gboolean visible)
{
  if (visible)
    gtk_widget_show (GTK_WIDGET (item));
  else
    gtk_widget_hide (GTK_WIDGET (item));
}

/**
 * Public functions
 */

const gchar * 
task_item_get_name (TaskItem    *item)
{
  TaskItemClass *klass;

  g_return_val_if_fail (TASK_IS_ITEM (item), NULL);
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_val_if_fail (klass->get_name, NULL);
        
  return klass->get_name (item);
}

GdkPixbuf *
task_item_get_icon ( const TaskItem *item)
{
  TaskItemClass *klass;

  g_return_val_if_fail (TASK_IS_ITEM (item), NULL);
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_val_if_fail (klass->get_icon, NULL);
        
  return klass->get_icon ((TaskItem*)item);
}

gboolean
task_item_is_visible (TaskItem    *item)
{
  TaskItemClass *klass;

  g_return_val_if_fail (TASK_IS_ITEM (item), FALSE);
  
  klass = TASK_ITEM_GET_CLASS (item);  
  g_return_val_if_fail (klass->is_visible, FALSE);
        
  return klass->is_visible (item);
}

void
task_item_left_click (TaskItem *item, GdkEventButton *event)
{
  TaskItemClass *klass;

  g_return_if_fail (TASK_IS_ITEM (item));
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_if_fail (klass->left_click);
        
  klass->left_click (item, event);
}

GtkWidget *
task_item_right_click (TaskItem *item, GdkEventButton *event)
{
  TaskItemClass *klass;

  g_return_val_if_fail (TASK_IS_ITEM (item),NULL);
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_val_if_fail (klass->right_click,NULL);
        
  return klass->right_click (item, event);
}

void
task_item_middle_click (TaskItem *item, GdkEventButton *event)
{
  TaskItemClass *klass;

  g_return_if_fail (TASK_IS_ITEM (item));
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_if_fail (klass->right_click);

  if (klass->middle_click)
  {
    klass->middle_click (item, event);
  }
}

guint
task_item_match (TaskItem *item, TaskItem *item_to_match)
{
  TaskItemClass *klass;

  g_return_val_if_fail (TASK_IS_ITEM (item), 0);
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_val_if_fail (klass->match, 0);
        
  return klass->match (item, item_to_match);
}

void
task_item_set_task_icon (TaskItem *item, TaskIcon *icon)
{
  g_return_if_fail (TASK_IS_ITEM (item));

  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (item);

  // add/remove overlays
  if (priv->task_icon)
  {
    AwnOverlayable *over = AWN_OVERLAYABLE (priv->task_icon);
    GList *overlays = awn_overlayable_get_overlays (over);

    if (item->text_overlay
        && g_list_find (overlays, item->text_overlay))
    {
      awn_overlayable_remove_overlay (over, AWN_OVERLAY (item->text_overlay));
    }
    if (item->progress_overlay
        && g_list_find (overlays, item->progress_overlay))
    {
      awn_overlayable_remove_overlay (over, AWN_OVERLAY (item->progress_overlay));
    }
    if (item->icon_overlay
        && g_list_find (overlays, item->icon_overlay))
    {
      awn_overlayable_remove_overlay (over, AWN_OVERLAY (item->icon_overlay));
    }

    g_list_free (overlays);
  }

  priv->task_icon = icon;
  if (icon)
  {
    AwnOverlayable *over = AWN_OVERLAYABLE (icon);
    // we can control what's on top here
    if (item->icon_overlay)
      awn_overlayable_add_overlay (over, AWN_OVERLAY (item->icon_overlay));
    if (item->progress_overlay)
      awn_overlayable_add_overlay (over, AWN_OVERLAY (item->progress_overlay));
    if (item->text_overlay)
      awn_overlayable_add_overlay (over, AWN_OVERLAY (item->text_overlay));
  }
}

void
task_item_update_overlay (TaskItem *item, const gchar *key, GValue *value)
{
  g_return_if_fail (TASK_IS_ITEM (item));

  if (strcmp ("icon-file", key) == 0)
  {
    g_return_if_fail (G_VALUE_HOLDS_STRING (value));

    if (item->icon_overlay == NULL)
    {
      item->icon_overlay = awn_overlay_pixbuf_file_new (NULL);
      g_object_set (G_OBJECT (item->icon_overlay),
                    "use-source-op", TRUE,
                    "scale", 1.0, NULL);
      GtkWidget *image = task_item_get_image_widget (item);
      AwnOverlayable *over = AWN_OVERLAYABLE (image);
      awn_overlayable_add_overlay (over,
                                   AWN_OVERLAY (item->icon_overlay));
    }

    const gchar* filename = g_value_get_string (value);
    g_object_set (G_OBJECT (item->icon_overlay),
                  "active", filename && filename[0] != '\0', NULL);
    if (filename && filename[0] != '\0')
    {
      g_object_set_property (G_OBJECT (item->icon_overlay),
                             "file-name", value);
    }

    // this refreshes the overlays on TaskIcon
    task_item_set_task_icon (item, task_item_get_task_icon (item));
  }
  else if (strcmp ("progress", key) == 0)
  {
    g_return_if_fail (G_VALUE_HOLDS_INT (value));

    if (item->progress_overlay == NULL)
    {
      item->progress_overlay = awn_overlay_progress_circle_new ();
      g_object_set (G_OBJECT (item->progress_overlay),
                    "gravity", GDK_GRAVITY_SOUTH_EAST,
                    "scale", 0.5, NULL);
      GtkWidget *image = task_item_get_image_widget (item);
      AwnOverlayable *over = AWN_OVERLAYABLE (image);
      awn_overlayable_add_overlay (over,
                                   AWN_OVERLAY (item->progress_overlay));
    }

    g_object_set (G_OBJECT (item->progress_overlay),
                  "active", g_value_get_int (value) != -1, NULL);
    if (g_value_get_int (value) != -1)
    {
      g_object_set_property (G_OBJECT (item->progress_overlay),
                             "percent-complete", value);
    }

    // this refreshes the overlays on TaskIcon
    task_item_set_task_icon (item, task_item_get_task_icon (item));
  }
  else if (strcmp ("message", key) == 0 || strcmp ("badge", key) == 0)
  {
    g_return_if_fail (G_VALUE_HOLDS_STRING (value));

    if (item->text_overlay == NULL)
    {
      item->text_overlay = awn_overlay_text_new ();
      GtkWidget *image = task_item_get_image_widget (item);
      AwnOverlayable *over = AWN_OVERLAYABLE (image);
      awn_overlayable_add_overlay (over, AWN_OVERLAY (item->text_overlay));
    }

    if (strcmp ("badge", key) == 0)
    {
      g_object_set (G_OBJECT (item->text_overlay),
                    "font-sizing", AWN_FONT_SIZE_MEDIUM,
                    "apply-effects", TRUE,
                    "gravity", GDK_GRAVITY_NORTH_EAST, 
                    "x-adj", -0.05,
                    "y-adj", +0.05, NULL);
    }
    else
    {
      g_object_set (G_OBJECT (item->text_overlay),
                    "font-sizing", AWN_FONT_SIZE_LARGE,
                    "apply-effects", FALSE,
                    "gravity", GDK_GRAVITY_CENTER,
                    "x-adj", 0.0,
                    "y-adj", 0.0, NULL);
    }

    const gchar* text = g_value_get_string (value);
    g_object_set (G_OBJECT (item->text_overlay),
                  "active", text && text[0] != '\0', NULL);
    if (text && text[0] != '\0')
    {
      g_object_set_property (G_OBJECT (item->text_overlay), "text", value);
    }

    // this refreshes the overlays on TaskIcon
    task_item_set_task_icon (item, task_item_get_task_icon (item));
  }
  else if (strcmp ("visible", key) == 0)
  {
    // we do support this key, though not here
  }
  else
  {
    g_debug ("TaskItem doesn't support key: \"%s\"", key);
  }
}

TaskIcon*
task_item_get_task_icon (TaskItem *item)
{
  g_return_val_if_fail (TASK_IS_ITEM (item), NULL);

  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (item);

  return priv->task_icon; 
}

GtkWidget*
task_item_get_image_widget (TaskItem *item)
{
  TaskItemClass *klass;

  g_return_val_if_fail (TASK_IS_ITEM (item), NULL);
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_val_if_fail (klass->get_name, NULL);

  if (klass->get_image_widget)
  {
    return klass->get_image_widget (item);
  }
  else
  {
    return NULL;
  }
}

/**
 * Protected functions (used only by derived classes)
 */

void
task_item_emit_name_changed (TaskItem *item, const gchar *name)
{
  g_return_if_fail (TASK_IS_ITEM (item));
  
  g_signal_emit (item, _item_signals[NAME_CHANGED], 0, name);
}

void
task_item_emit_icon_changed (TaskItem *item, GdkPixbuf *icon)
{
  g_return_if_fail (TASK_IS_ITEM (item));

  g_signal_emit (item, _item_signals[ICON_CHANGED], 0, icon);
}

void
task_item_emit_visible_changed (TaskItem *item, gboolean visible)
{
  g_return_if_fail (TASK_IS_ITEM (item));

  g_signal_emit (item, _item_signals[VISIBLE_CHANGED], 0, visible);
}
