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
 * Authors: Michal Hruby <michal.mhr@gmail.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "awn-alignment.h"
#include "gseal-transition.h"

G_DEFINE_TYPE (AwnAlignment, awn_alignment, GTK_TYPE_ALIGNMENT)

#define AWN_ALIGNMENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj),\
                                      AWN_TYPE_ALIGNMENT, AwnAlignmentPrivate))

struct _AwnAlignmentPrivate
{
  AwnApplet *applet;

  GtkPositionType position;
  gint offset_modifier;
  gfloat offset_multiplier;
  gint last_offset;
  gfloat scale;

  gulong position_changed_id;
  gulong offset_changed_id;
};

enum
{
  PROP_0,

  PROP_APPLET,
  PROP_SCALE,
  PROP_OFFSET_MOD,
  PROP_OFFSET_MULT
};

/* Forwards */
static void awn_alignment_set_applet (AwnAlignment *alignment,
                                      AwnApplet *applet);

static void on_position_changed      (AwnAlignment *alignment,
                                      GtkPositionType position);

static void ensure_alignment         (AwnAlignment *alignment);

/* GObject Stuff */
static void
awn_alignment_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  AwnAlignmentPrivate *priv;

  g_return_if_fail (AWN_IS_ALIGNMENT (object));
  priv = AWN_ALIGNMENT (object)->priv;

  switch (prop_id)
  {
    case PROP_APPLET:
      g_value_set_object (value, priv->applet);
      break;
    case PROP_OFFSET_MOD:
      g_value_set_int (value, priv->offset_modifier);
      break;
    case PROP_OFFSET_MULT:
      g_value_set_float (value, priv->offset_multiplier);
      break;
    case PROP_SCALE:
      g_value_set_float (value, priv->scale);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_alignment_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  AwnAlignmentPrivate *priv;

  g_return_if_fail (AWN_IS_ALIGNMENT (object));
  priv = AWN_ALIGNMENT (object)->priv;

  switch (prop_id)
  {
    case PROP_APPLET:
      awn_alignment_set_applet (AWN_ALIGNMENT (object),
                                AWN_APPLET (g_value_get_object (value)));
      break;
    case PROP_OFFSET_MOD:
      awn_alignment_set_offset_modifier (AWN_ALIGNMENT (object),
                                         g_value_get_int (value));
      break;
    case PROP_OFFSET_MULT:
      priv->offset_multiplier = g_value_get_float (value);
      ensure_alignment (AWN_ALIGNMENT (object));
      break;
    case PROP_SCALE:
      priv->scale = g_value_get_float (value);
      on_position_changed (AWN_ALIGNMENT (object), priv->position);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
awn_alignment_dispose (GObject *obj)
{
  AwnAlignmentPrivate *priv = AWN_ALIGNMENT_GET_PRIVATE (obj);

  if (priv->position_changed_id)
  {
    g_signal_handler_disconnect (priv->applet, priv->position_changed_id);
    priv->position_changed_id = 0;
  }

  if (priv->offset_changed_id)
  {
    g_signal_handler_disconnect (priv->applet, priv->offset_changed_id);
    priv->offset_changed_id = 0;
  }

  G_OBJECT_CLASS (awn_alignment_parent_class)->dispose (obj);
}

static void
awn_alignment_class_init (AwnAlignmentClass *klass)
{
  GObjectClass   *obj_class = G_OBJECT_CLASS (klass);

  obj_class->get_property = awn_alignment_get_property;
  obj_class->set_property = awn_alignment_set_property;
  obj_class->dispose      = awn_alignment_dispose;

  /* Class properties */
  g_object_class_install_property (obj_class,
    PROP_APPLET,
    g_param_spec_object ("applet",
                         "Applet",
                         "Applet from which properties are read out",
                         AWN_TYPE_APPLET,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /*
   *  Can be set to negative value to get some extra space where you can paint,
   *  or positive value for extra padding besides the offset.
   */
  g_object_class_install_property (obj_class,
    PROP_OFFSET_MOD,
    g_param_spec_int ("offset-modifier",
                      "Offset modifier",
                      "Offset modifier",
                      G_MININT, G_MAXINT, 0,
                      G_PARAM_CONSTRUCT | G_PARAM_READWRITE |
                      G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_OFFSET_MULT,
    g_param_spec_float ("offset-multiplier",
                        "Offset multiplier",
                        "Offset multiplier",
                        0.0, 1.0, 1.0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (obj_class,
    PROP_SCALE,
    g_param_spec_float ("scale",
                        "Scale",
                        "If available space is bigger than needed for "
                        "the child, how much of it to use for the child. "
                        "0.0 means none, 1.0 means all.",
                        0.0, 1.0, 1.0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (obj_class, sizeof (AwnAlignmentPrivate));
}

static void
awn_alignment_init (AwnAlignment *alignment)
{
  AwnAlignmentPrivate *priv;

  priv = alignment->priv = AWN_ALIGNMENT_GET_PRIVATE (alignment);

  priv->last_offset = 0;
  priv->scale = 1.0;
  priv->offset_multiplier = 1.0;

  g_signal_connect (alignment, "size-allocate",
                    G_CALLBACK (ensure_alignment), NULL);
}
 
GtkWidget*
awn_alignment_new_for_applet (AwnApplet *applet)
{
  GtkWidget *alignment;

  alignment = g_object_new(AWN_TYPE_ALIGNMENT,
                           "applet", applet,
                           NULL);

  return alignment;
}

static void
on_position_changed (AwnAlignment *alignment, GtkPositionType position)
{
  AwnAlignmentPrivate *priv;
  GtkAlignment *align;

  g_return_if_fail (AWN_IS_ALIGNMENT (alignment));

  align = GTK_ALIGNMENT (alignment);
  priv = alignment->priv;

  priv->position = position;

  switch (priv->position)
  {
    case GTK_POS_TOP:
      gtk_alignment_set (align, 0.5, 0.0, priv->scale, 0.0);
      break;
    case GTK_POS_BOTTOM:
      gtk_alignment_set (align, 0.5, 1.0, priv->scale, 0.0);
      break;
    case GTK_POS_LEFT:
      gtk_alignment_set (align, 0.0, 0.5, 0.0, priv->scale);
      break;
    case GTK_POS_RIGHT:
      gtk_alignment_set (align, 1.0, 0.5, 0.0, priv->scale);
      break;
  }

  /* make sure offset gets recalculated */
  priv->last_offset = 0;

  ensure_alignment (alignment);
}

static void
awn_alignment_set_applet (AwnAlignment *alignment,
                          AwnApplet    *applet)
{
  AwnAlignmentPrivate *priv;

  g_return_if_fail (AWN_IS_ALIGNMENT (alignment));
  priv = alignment->priv;

  if (priv->applet)
  {
    g_warning ("Applet was already set for this AwnAlignment!");
  }

  if (!AWN_IS_APPLET (applet))
    return;

  priv->applet = applet;

  priv->position = awn_applet_get_pos_type (applet);

  priv->position_changed_id =
    g_signal_connect_swapped (applet, "position-changed",
                              G_CALLBACK (on_position_changed), alignment);
  priv->offset_changed_id =
    g_signal_connect_swapped (applet, "offset-changed",
                              G_CALLBACK (ensure_alignment), alignment);

  /* will set proper align and padding */
  on_position_changed (alignment, priv->position);
}

gint
awn_alignment_get_offset_modifier (AwnAlignment *alignment)
{
  g_return_val_if_fail (AWN_IS_ALIGNMENT (alignment), 0);

  return alignment->priv->offset_modifier;
}

void
awn_alignment_set_offset_modifier (AwnAlignment *alignment, gint modifier)
{
  AwnAlignmentPrivate *priv;

  g_return_if_fail (AWN_IS_ALIGNMENT (alignment));
  priv = alignment->priv;

  priv->offset_modifier = modifier;

  ensure_alignment (alignment);
}

static void
ensure_alignment (AwnAlignment *alignment)
{
  AwnAlignmentPrivate *priv;
  GtkAlignment *align;
  GtkAllocation alloc;
  gint x, y, offset;

  g_return_if_fail (AWN_IS_ALIGNMENT (alignment));

  priv = alignment->priv;

  if (priv->applet == NULL) return;

  align = GTK_ALIGNMENT (alignment);
  gtk_widget_get_allocation (GTK_WIDGET (alignment), &alloc);

  x = alloc.x + alloc.width / 2;
  y = alloc.y + alloc.height / 2;

  offset = (awn_applet_get_offset_at (priv->applet, x, y) *
            priv->offset_multiplier) + priv->offset_modifier;
  if (offset < 0) offset = 0;

  /* 
   * Make sure we don't set the offset multiple times, otherwise we'll end up
   * in infinite loop.
   */
  if (priv->last_offset == offset) return;

  priv->last_offset = offset;

  switch (priv->position)
  {
    case GTK_POS_TOP:
      gtk_alignment_set_padding (align, offset, 0, 0, 0);
      break;
    case GTK_POS_BOTTOM:
      gtk_alignment_set_padding (align, 0, offset, 0, 0);
      break;
    case GTK_POS_LEFT:
      gtk_alignment_set_padding (align, 0, 0, offset, 0);
      break;
    case GTK_POS_RIGHT:
      gtk_alignment_set_padding (align, 0, 0, 0, offset);
      break;
  }
}
