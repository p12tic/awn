/*
 * Copyright (C) 2008 Neil Jagdish Patel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 
 * 2 or later as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */
#include "awn-icon-box.h"
#include "awn-icon.h"

G_DEFINE_TYPE (AwnIconBox, awn_icon_box, GTK_TYPE_BOX);

#define AWN_ICON_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_ICON_BOX, \
  AwnIconBoxPrivate))

struct _AwnIconBoxPrivate
{
  AwnOrientation orient;

  /* Current box class */
  GtkWidgetClass  *klass;
};

/* GObject stuff */
static void
awn_icon_box_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  AwnIconBoxPrivate *priv = AWN_ICON_BOX (widget)->priv;
  
  priv->klass->size_request (widget, requisition);
}

static void
awn_icon_box_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  AwnIconBoxPrivate *priv = AWN_ICON_BOX (widget)->priv;
   
  priv->klass->size_allocate (widget, allocation);
}

static void
awn_icon_box_dispose (GObject *object)
{
  G_OBJECT_CLASS (awn_icon_box_parent_class)->dispose (object);
}

static void
awn_icon_box_add (GtkContainer *container, GtkWidget *child)
{
  if (!AWN_IS_ICON (child))
  {
    g_warning ("AwnIconBox only accepts AwnIcons as children");
    return;
  }

  awn_icon_set_orientation (AWN_ICON (child), 
                            AWN_ICON_BOX (container)->priv->orient);
  GTK_CONTAINER_CLASS (awn_icon_box_parent_class)->add (container, child);
}

static void
awn_icon_box_class_init (AwnIconBoxClass *klass)
{
  GObjectClass      *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *wid_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *con_class = GTK_CONTAINER_CLASS (klass);

  obj_class->dispose = awn_icon_box_dispose;

  wid_class->size_request  = awn_icon_box_size_request;
  wid_class->size_allocate = awn_icon_box_size_allocate;

  con_class->add = awn_icon_box_add;

  g_type_class_add_private (obj_class, sizeof (AwnIconBoxPrivate));
}

static void
awn_icon_box_init (AwnIconBox *icon_box)
{
  AwnIconBoxPrivate *priv;

  priv = icon_box->priv = AWN_ICON_BOX_GET_PRIVATE (icon_box);

  priv->orient = AWN_ORIENTATION_BOTTOM;
  priv->klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_HBOX));
}

GtkWidget *
awn_icon_box_new (void)

{
  GtkWidget *icon_box = NULL;

  icon_box = g_object_new (AWN_TYPE_ICON_BOX,
                           "homogeneous", FALSE,
                           "spacing", 0,
                           NULL);

  return icon_box;
}

void 
awn_icon_box_set_orientation (AwnIconBox     *icon_box,
                              AwnOrientation  orient)
{
  AwnIconBoxPrivate *priv;
  GList *children, *c;

  g_return_if_fail (AWN_IS_ICON_BOX (icon_box));
  priv = icon_box->priv;
  
  priv->orient = orient;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
      priv->klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_HBOX));
      break;
    
    case AWN_ORIENTATION_RIGHT:
    case AWN_ORIENTATION_LEFT:
      priv->klass = GTK_WIDGET_CLASS (gtk_type_class (GTK_TYPE_VBOX));
      break;

    default:
      g_assert_not_reached ();
      priv->klass = NULL;
      break;
  }

  /* Update the children with the new orientation */
  children = gtk_container_get_children (GTK_CONTAINER (icon_box));
  for (c = children; c; c = c->next)
  {
    AwnIcon *icon = c->data;

    if (AWN_IS_ICON (icon))
      awn_icon_set_orientation (icon, orient);
  }
  g_list_free (children);
}
