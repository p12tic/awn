/*
 * Copyright (C) 2009 Michal Hruby <michal.mhr@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "awn-box.h"

G_DEFINE_TYPE (AwnBox, awn_box, GTK_TYPE_BOX)

#define AWN_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_BOX, \
  AwnBoxPrivate))

struct _AwnBoxPrivate
{
  GtkOrientation orient;
  /* Current box class */
  GtkWidgetClass *klass;
};

enum
{
  PROP_0,

  PROP_ORIENT
};

/* GObject stuff */
static void
awn_box_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  AwnBoxPrivate *priv = AWN_BOX (widget)->priv;

  priv->klass->size_request (widget, requisition);
}

static void
awn_box_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  AwnBoxPrivate *priv = AWN_BOX (widget)->priv;

  priv->klass->size_allocate (widget, allocation);
}

static void
awn_box_finalize (GObject *object)
{
  AwnBoxPrivate *priv = AWN_BOX_GET_PRIVATE (object);

  if (priv->klass)
  {
    g_type_class_unref (priv->klass);
    priv->klass = NULL;
  }

  G_OBJECT_CLASS (awn_box_parent_class)->finalize (object);
}

static void
awn_box_get_property (GObject    *object,
                      guint       prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  AwnBoxPrivate *priv = AWN_BOX_GET_PRIVATE (object);
  switch (prop_id)
  {
    case PROP_ORIENT:
      g_value_set_enum (value, priv->orient);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_box_set_property (GObject      *object,
                      guint         prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  AwnBoxPrivate *priv = AWN_BOX_GET_PRIVATE (object);

  switch (prop_id)
  {
    case PROP_ORIENT:
      if (priv->klass)
      {
        g_type_class_unref (priv->klass);
        priv->klass = NULL;
      }

      priv->orient = g_value_get_enum (value);

      switch (priv->orient)
      {
        case GTK_ORIENTATION_HORIZONTAL:
          priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_HBOX));
          break;
        
        case GTK_ORIENTATION_VERTICAL:
          priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_VBOX));
          break;

        default:
          g_assert_not_reached ();
          break;
      }

      gtk_widget_queue_resize (GTK_WIDGET (object));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_box_class_init (AwnBoxClass *klass)
{
  GObjectClass      *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->get_property = awn_box_get_property;
  obj_class->set_property = awn_box_set_property;
  obj_class->finalize = awn_box_finalize;

  wid_class->size_request  = awn_box_size_request;
  wid_class->size_allocate = awn_box_size_allocate;

#if GTK_CHECK_VERSION(2, 15, 0)
  // GtkBox implements GtkOrientable interface and therefore does have
  //  "orientation" property, which we don't want to override
#else
  g_object_class_install_property (obj_class,
    PROP_ORIENT,
    g_param_spec_enum ("orientation",
                       "Orientation",
                       "The orientation of the box",
                       GTK_TYPE_ORIENTATION,
                       GTK_ORIENTATION_HORIZONTAL,
                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif

  g_type_class_add_private (obj_class, sizeof (AwnBoxPrivate));
}

static void
awn_box_init (AwnBox *box)
{
  AwnBoxPrivate *priv;
  priv = box->priv = AWN_BOX_GET_PRIVATE (box);

  priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_HBOX));
  priv->orient = GTK_ORIENTATION_HORIZONTAL;
}

static GtkOrientation
awn_box_orientation_from_pos_type (GtkPositionType pos_type)
{
  switch (pos_type)
  {
    case GTK_POS_LEFT:
    case GTK_POS_RIGHT:
      return GTK_ORIENTATION_VERTICAL;

    case GTK_POS_TOP:
    case GTK_POS_BOTTOM:
      return GTK_ORIENTATION_HORIZONTAL;

    default:
      g_assert_not_reached ();
      return GTK_ORIENTATION_HORIZONTAL;
  }
}

GtkWidget *
awn_box_new (GtkOrientation orient)
{
  GtkWidget *box = NULL;

  box = g_object_new (AWN_TYPE_BOX, "orientation", orient, NULL);

  return box;
}

/*
 * Public functions 
 */

// this function can be shadowed by bindings, but that does't matter, cause
// it'd just call the base function
void
awn_box_set_orientation (AwnBox         *box,
                         GtkOrientation  orient)
{
  g_return_if_fail (AWN_IS_BOX (box));

#if GTK_CHECK_VERSION(2, 15, 0)
  gtk_orientable_set_orientation (GTK_ORIENTABLE (box), orient);
#else
  g_object_set ((GObject*)box, "orientation", orient, NULL);
#endif
}

void
awn_box_set_orientation_from_pos_type (AwnBox *box,
                                       GtkPositionType pos_type)
{
  awn_box_set_orientation (box, awn_box_orientation_from_pos_type (pos_type));
}

