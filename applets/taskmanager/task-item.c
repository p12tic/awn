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
 * Authored by Hannes Verschore <hv1989@gmail.com>
 *
 */

#include "task-item.h"

#include <libawn/libawn.h>

G_DEFINE_ABSTRACT_TYPE (TaskItem, task_item, GTK_TYPE_EVENT_BOX)

#define TASK_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  TASK_TYPE_ITEM, \
  TaskItemPrivate))

struct _TaskItemPrivate
{
  GtkWidget *box;
  GtkWidget *name;
  GtkWidget *image;
};

enum
{
  PROP_0
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
static void task_item_name_changed      (TaskItem *item, const gchar   *name);
static void task_item_icon_changed      (TaskItem *item, GdkPixbuf     *icon);
static void task_item_visible_changed   (TaskItem *item, gboolean       visible);

static gboolean  task_item_button_release_event (GtkWidget      *widget,
                                                 GdkEventButton *event);
static gboolean  task_item_button_press_event   (GtkWidget      *widget,
                                                 GdkEventButton *event);

/* GObject stuff */
static void
task_item_class_init (TaskItemClass *klass)
{
  //GParamSpec   *pspec;
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

  wid_class->button_release_event = task_item_button_release_event;
  wid_class->button_press_event   = task_item_button_press_event;
  
  /* We implement the necessary funtions for a normal item */
  klass->get_name        = NULL;
  klass->get_icon        = NULL;
  klass->is_visible      = NULL;
  klass->match           = NULL;

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

  g_type_class_add_private (obj_class, sizeof (TaskItemPrivate));
}

static void
task_item_init (TaskItem *item)
{
  TaskItemPrivate *priv;

  /* get and save private struct */
  priv = item->priv = TASK_ITEM_GET_PRIVATE (item);

  /* let this eventbox listen to every events */
  gtk_event_box_set_above_child (GTK_EVENT_BOX (item), TRUE);
  
  /* create content */
  priv->box = gtk_hbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (item), priv->box);

  priv->image = GTK_WIDGET (awn_image_new ());
  gtk_box_pack_start (GTK_BOX (priv->box), priv->image, FALSE, FALSE, 0);
  
  priv->name = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (priv->box), priv->name, TRUE, FALSE, 10);

  /* connect to signals */
  g_signal_connect (G_OBJECT (item), "name-changed",
                    G_CALLBACK (task_item_name_changed), NULL);
  g_signal_connect (G_OBJECT (item), "icon-changed",
                    G_CALLBACK (task_item_icon_changed), NULL);
  g_signal_connect (G_OBJECT (item), "visible-changed",
                    G_CALLBACK (task_item_visible_changed), NULL);
}

static gboolean
task_item_button_release_event (GtkWidget      *widget,
                                GdkEventButton *event)
{
  g_return_val_if_fail (TASK_IS_ITEM (widget), FALSE);

  task_item_left_click (TASK_ITEM (widget), event);

  return TRUE;
}

static gboolean
task_item_button_press_event (GtkWidget      *widget,
                              GdkEventButton *event)
{
  g_return_val_if_fail (TASK_IS_ITEM (widget), FALSE);

  if (event->button != 3) return FALSE;

  task_item_right_click (TASK_ITEM (widget), event);

  return TRUE;
}

static void 
task_item_name_changed (TaskItem *item, const gchar *name)
{
  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (item);

  gtk_label_set_text (GTK_LABEL (priv->name), name);
}


static void 
task_item_icon_changed (TaskItem *item, GdkPixbuf *icon)
{
  TaskItemPrivate *priv = TASK_ITEM_GET_PRIVATE (item);

  gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), icon);
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
task_item_get_icon (TaskItem    *item)
{
  TaskItemClass *klass;

  g_return_val_if_fail (TASK_IS_ITEM (item), NULL);
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_val_if_fail (klass->get_icon, NULL);
        
  return klass->get_icon (item);
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

void
task_item_right_click (TaskItem *item, GdkEventButton *event)
{
  TaskItemClass *klass;

  g_return_if_fail (TASK_IS_ITEM (item));
  
  klass = TASK_ITEM_GET_CLASS (item);
  g_return_if_fail (klass->right_click);
        
  klass->right_click (item, event);
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
