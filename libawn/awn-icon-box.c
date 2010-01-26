/*
 * Copyright (C) 2008 Neil Jagdish Patel
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
 * Authored by Neil Jagdish Patel <njpatel@gmail.com>
 *
 */

#include "awn-icon-box.h"
#include "awn-icon.h"
#include "awn-overlayable.h"
#include "awn-alignment.h"
#include "awn-utils.h"
#include "gseal-transition.h"

G_DEFINE_TYPE (AwnIconBox, awn_icon_box, AWN_TYPE_BOX)

#define AWN_ICON_BOX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
  AWN_TYPE_ICON_BOX, \
  AwnIconBoxPrivate))

struct _AwnIconBoxPrivate
{
  GtkPositionType position;
  gint            offset;

  AwnApplet      *applet;
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
awn_icon_box_child_size_allocate (GtkWidget *widget,
                                  GtkAllocation *alloc,
                                  AwnIconBox *box)
{
  AwnIconBoxPrivate *priv = AWN_ICON_BOX (box)->priv;

  g_return_if_fail (AWN_IS_OVERLAYABLE (widget) || AWN_IS_ALIGNMENT (widget));

  if (priv->applet == NULL) return;

  gint x = alloc->x + alloc->width / 2;
  gint y = alloc->y + alloc->height / 2;
  gint offset = awn_applet_get_offset_at (priv->applet, x, y);
  if (AWN_IS_ICON (widget))
  {
    awn_icon_set_offset (AWN_ICON (widget), offset);
  }
  else if (AWN_IS_OVERLAYABLE (widget))
  {
    AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (widget));
    g_object_set (fx, "icon-offset", priv->offset, NULL);
  }
}

static void
awn_icon_box_add (GtkContainer *container, GtkWidget *child)
{
  AwnIconBoxPrivate *priv;
  g_return_if_fail (AWN_IS_ICON_BOX (container));

  // we'll accept AwnIcon, AwnAlignment or any AwnOverlayable
  if (!AWN_IS_ICON (child) && !AWN_IS_ALIGNMENT (child) && 
      !AWN_IS_OVERLAYABLE (child))
  {
    g_warning ("AwnIconBox only accepts AwnIcons as children");
    return;
  }

  priv = AWN_ICON_BOX (container)->priv;

  gtk_box_set_child_packing (GTK_BOX (container), child, FALSE, FALSE, 0,
                              GTK_PACK_START);
  if (AWN_IS_ICON (child))
  {
    awn_icon_set_offset (AWN_ICON (child), priv->offset);
    awn_icon_set_pos_type (AWN_ICON (child), priv->position);
  }
  else if (AWN_IS_OVERLAYABLE (child))
  {
    AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (child));
    g_object_set (fx, "position", priv->position, 
                  "icon-offset", priv->offset, NULL);
  }

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
awn_icon_box_orientation_notify (AwnIconBox *icon_box)
{
  AwnIconBoxPrivate *priv;
  GList *children, *c;
  
  g_return_if_fail (AWN_IS_ICON_BOX (icon_box));
  priv = icon_box->priv;

  /* Update the children with the new orientation */
  children = gtk_container_get_children (GTK_CONTAINER (icon_box));
  for (c = children; c; c = c->next)
  {
    GtkWidget *icon = c->data;

    if (AWN_IS_ICON (icon))
    {
      awn_icon_set_pos_type (AWN_ICON (icon), priv->position);
    }
    else if (AWN_IS_OVERLAYABLE (icon))
    {
      AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (icon));
      g_object_set (fx, "position", priv->position, NULL);
    }
  }
  g_list_free (children);
}

static void
awn_icon_box_class_init (AwnIconBoxClass *klass)
{
  GObjectClass      *obj_class = G_OBJECT_CLASS (klass);

  obj_class->get_property = awn_icon_box_get_property;
  obj_class->set_property = awn_icon_box_set_property;

  /* FIXME: offset and position should be properties as well */

  g_object_class_install_property (obj_class,
    PROP_APPLET,
    g_param_spec_object ("applet",
                         "Applet",
                         "AwnApplet from which offset and position "
                         "properties are read",
                         AWN_TYPE_APPLET,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE |
                         G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (obj_class, sizeof (AwnIconBoxPrivate));
}

static void
awn_icon_box_init (AwnIconBox *icon_box)
{
  AwnIconBoxPrivate *priv;
  priv = icon_box->priv = AWN_ICON_BOX_GET_PRIVATE (icon_box);

  priv->position = GTK_POS_BOTTOM;
  priv->offset = 0;
  priv->applet = NULL;

  g_signal_connect_after (icon_box, "add", 
                          G_CALLBACK (awn_icon_box_add), icon_box);

  g_signal_connect (icon_box, "notify::orientation", 
                    G_CALLBACK (awn_icon_box_orientation_notify), NULL);
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
  //   because AwnIcon doesn't allow setting size (only ThemedIcon does)
}

static void
on_applet_position_changed (AwnApplet      *applet, 
                            GtkPositionType position,
                            AwnIconBox     *box)
{
  awn_icon_box_set_pos_type (box, position);
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
    g_signal_connect (applet, "position-changed",
                      G_CALLBACK (on_applet_position_changed), box);
    g_signal_connect (applet, "offset-changed",
                      G_CALLBACK (on_applet_offset_changed), box);
    awn_icon_box_set_pos_type (AWN_ICON_BOX (box), 
                               awn_applet_get_pos_type (applet));
    awn_icon_box_set_offset (AWN_ICON_BOX (box),
                             awn_applet_get_offset (applet));
  }
}

/*
 * Public functions 
 */
void 
awn_icon_box_set_pos_type (AwnIconBox     *icon_box,
                           GtkPositionType  position)
{
  g_return_if_fail (AWN_IS_ICON_BOX (icon_box));

  icon_box->priv->position = position;

  awn_box_set_orientation_from_pos_type (AWN_BOX (icon_box), position);
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
 
  /* Update the children with the new position */
  children = gtk_container_get_children (GTK_CONTAINER (icon_box));
  for (c = children; c; c = c->next)
  {
    GtkWidget *icon = c->data;

    if (priv->applet)
    {
      GtkAllocation alloc;
      gtk_widget_get_allocation (icon, &alloc);
      gint x = alloc.x + alloc.width / 2;
      gint y = alloc.y + alloc.height / 2;
      offset = awn_applet_get_offset_at (priv->applet, x, y);
      if (AWN_IS_ICON (icon))
      {
        awn_icon_set_offset (AWN_ICON (icon), offset);
      }
      else if (AWN_IS_OVERLAYABLE (icon))
      {
        AwnEffects *fx = awn_overlayable_get_effects (AWN_OVERLAYABLE (icon));
        g_object_set (fx, "icon-offset", offset, NULL);
      }
    }
  }
  g_list_free (children);
}

