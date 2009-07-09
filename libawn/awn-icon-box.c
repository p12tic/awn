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
#include "awn-utils.h"

G_DEFINE_TYPE (AwnIconBox, awn_icon_box, GTK_TYPE_BOX)

#define AWN_ICON_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_ICON_BOX, \
  AwnIconBoxPrivate))

struct _AwnIconBoxPrivate
{
  AwnOrientation orient;
  gint           offset;

  AwnApplet     *applet;

  /* Current box class */
  GtkWidgetClass  *klass;
};

enum
{
  PROP_0,

  PROP_APPLET
};

/* FORWARDS */
static void awn_icon_box_set_applet (AwnIconBox *box, AwnApplet *applet);

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
awn_icon_box_child_size_allocate (GtkWidget *widget,
                                  GtkAllocation *alloc,
                                  AwnIconBox *box)
{
  AwnIconBoxPrivate *priv = AWN_ICON_BOX (box)->priv;

  g_return_if_fail (AWN_IS_ICON (widget));

  if (priv->applet == NULL) return;

  gint x = alloc->x + alloc->width / 2;
  gint y = alloc->y + alloc->height / 2;
  gint offset = awn_applet_get_offset_at (priv->applet, x, y);
  awn_icon_set_offset (AWN_ICON (widget), offset);
}

static void
awn_icon_box_dispose (GObject *object)
{
  AwnIconBoxPrivate *priv = AWN_ICON_BOX_GET_PRIVATE (object);

  if (priv->klass)
  {
    g_type_class_unref (priv->klass);
    priv->klass = NULL;
  }

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

  gtk_box_set_child_packing (GTK_BOX (container), child, FALSE, FALSE, 0,
                              GTK_PACK_START);
  awn_icon_set_offset      (AWN_ICON (child),
                            AWN_ICON_BOX (container)->priv->offset);
  awn_icon_set_orientation (AWN_ICON (child), 
                            AWN_ICON_BOX (container)->priv->orient);
  g_signal_connect (child, "size-allocate",
                    G_CALLBACK (awn_icon_box_child_size_allocate), container);
}

static void
awn_icon_box_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_icon_box_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  switch (prop_id)
  {
    case PROP_APPLET:
      awn_icon_box_set_applet (AWN_ICON_BOX (object),
                               g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_icon_box_class_init (AwnIconBoxClass *klass)
{
  GObjectClass      *obj_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *wid_class = GTK_WIDGET_CLASS (klass);

  obj_class->get_property = awn_icon_box_get_property;
  obj_class->set_property = awn_icon_box_set_property;
  obj_class->dispose = awn_icon_box_dispose;

  wid_class->size_request  = awn_icon_box_size_request;
  wid_class->size_allocate = awn_icon_box_size_allocate;

  /* FIXME: offset and orient should be properties as well */

  g_object_class_install_property (obj_class,
    PROP_APPLET,
    g_param_spec_object ("applet",
                         "Applet",
                         "AwnApplet from which offset and orientation "
                         "properties are read",
                         AWN_TYPE_APPLET,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

  g_type_class_add_private (obj_class, sizeof (AwnIconBoxPrivate));
}

static void
awn_icon_box_init (AwnIconBox *icon_box)
{
  AwnIconBoxPrivate *priv;
  priv = icon_box->priv = AWN_ICON_BOX_GET_PRIVATE (icon_box);

  priv->orient = AWN_ORIENTATION_BOTTOM;
  priv->offset = 0;
  priv->applet = NULL;
  priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_HBOX));

  g_signal_connect_after (icon_box, "add", 
                          G_CALLBACK (awn_icon_box_add), icon_box);

  awn_utils_ensure_transparent_bg (GTK_WIDGET (icon_box));
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

static void
on_applet_size_changed (AwnApplet *applet, gint size, AwnIconBox *box)
{
  // FIXME: why doesn't this set size of the icons?
}

static void
on_applet_orient_changed (AwnApplet      *applet, 
                         AwnOrientation  orient, 
                         AwnIconBox     *box)
{
  awn_icon_box_set_orientation (box, orient);
}

static void
on_applet_offset_changed (AwnApplet  *applet, 
                         gint        offset, 
                         AwnIconBox *box)
{
  awn_icon_box_set_offset (box, offset);
}

GtkWidget *   
awn_icon_box_new_for_applet (AwnApplet *applet)
{
  GtkWidget *icon_box = NULL;

  icon_box = g_object_new (AWN_TYPE_ICON_BOX,
                           "homogeneous", FALSE,
                           "spacing", 0,
                           "applet", applet,
                           NULL);

  return icon_box;
}

static void
awn_icon_box_set_applet (AwnIconBox *box, AwnApplet *applet)
{
  if (AWN_IS_APPLET (applet))
  {
    AWN_ICON_BOX(box)->priv->applet = applet;

    g_signal_connect (applet, "size-changed", 
                      G_CALLBACK (on_applet_size_changed), box);
    g_signal_connect (applet, "orientation-changed",
                      G_CALLBACK (on_applet_orient_changed), box);
    g_signal_connect (applet, "offset-changed",
                      G_CALLBACK (on_applet_offset_changed), box);
    awn_icon_box_set_orientation (AWN_ICON_BOX (box), 
                                  awn_applet_get_orientation (applet));
    awn_icon_box_set_offset (AWN_ICON_BOX (box),
                             awn_applet_get_offset (applet));
  }
}

/*
 * Public functions 
 */
void 
awn_icon_box_set_orientation (AwnIconBox     *icon_box,
                              AwnOrientation  orient)
{
  AwnIconBoxPrivate *priv;
  GList *children, *c;

  g_return_if_fail (AWN_IS_ICON_BOX (icon_box));
  priv = icon_box->priv;

  if (priv->klass)
  {
    g_type_class_unref (priv->klass);
    priv->klass = NULL;
  }

  priv->orient = orient;

  switch (priv->orient)
  {
    case AWN_ORIENTATION_TOP:
    case AWN_ORIENTATION_BOTTOM:
#if GTK_CHECK_VERSION(2, 15, 0)
      gtk_orientable_set_orientation (GTK_ORIENTABLE (icon_box), GTK_ORIENTATION_HORIZONTAL);
#endif
      priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_HBOX));
      break;
    
    case AWN_ORIENTATION_RIGHT:
    case AWN_ORIENTATION_LEFT:
#if GTK_CHECK_VERSION(2, 15, 0)
      gtk_orientable_set_orientation (GTK_ORIENTABLE (icon_box), GTK_ORIENTATION_VERTICAL);
#endif
      priv->klass = GTK_WIDGET_CLASS (g_type_class_ref (GTK_TYPE_VBOX));
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

void 
awn_icon_box_set_offset (AwnIconBox *icon_box,
                         gint        offset)
{
  AwnIconBoxPrivate *priv;
  GList *children, *c;

  g_return_if_fail (AWN_IS_ICON_BOX (icon_box));
  priv = icon_box->priv;

  priv->offset = offset;
 
  /* Update the children with the new orientation */
  children = gtk_container_get_children (GTK_CONTAINER (icon_box));
  for (c = children; c; c = c->next)
  {
    AwnIcon *icon = c->data;

    if (AWN_IS_ICON (icon))
    {
      if (priv->applet)
      {
        GtkAllocation *alloc = &(GTK_WIDGET (icon)->allocation);
        gint x = alloc->x + alloc->width / 2;
        gint y = alloc->y + alloc->height / 2;
        offset = awn_applet_get_offset_at (priv->applet, x, y);
      }
      awn_icon_set_offset (icon, offset);
    }
  }
  g_list_free (children);
}

