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

#include "awn-overlayable.h"

static void awn_overlayable_base_init (AwnOverlayableIface *iface);

GType
awn_overlayable_get_type (void)
{
  static GType overlayable_type = 0;

  if (G_UNLIKELY (overlayable_type == 0))
  {
    const GTypeInfo overlayable_info =
    {
      sizeof (AwnOverlayableIface),
      (GBaseInitFunc)     awn_overlayable_base_init,
      (GBaseFinalizeFunc) NULL,
    };

    overlayable_type = g_type_register_static (G_TYPE_INTERFACE,
                                               "AwnOverlayable",
                                               &overlayable_info, 0);
  }

  return overlayable_type;
}

static void
awn_overlayable_base_init (AwnOverlayableIface *iface)
{
  iface->get_effects = NULL; // perhaps show a warning?
}

/**
 * awn_overlayable_get_effects:
 * @self: #AwnOverlayable instance.
 *
 * Returns: #AwnEffects instance managing animations for this instance.
 */
AwnEffects*
awn_overlayable_get_effects (AwnOverlayable* self)
{
  g_return_val_if_fail (AWN_IS_OVERLAYABLE (self), NULL);

  return AWN_OVERLAYABLE_GET_INTERFACE (self)->get_effects (self);
}

/**
 * awn_overlayable_add_overlay:
 * @self: #AwnOverlayable instance.
 * @overlay: #AwnOverlay instance which should be added.
 *
 * Adds an overlay to the list of rendered overlays.
 */
void
awn_overlayable_add_overlay (AwnOverlayable* self, AwnOverlay *overlay)
{
  awn_effects_add_overlay (awn_overlayable_get_effects (self), overlay);
}

/**
 * awn_overlayable_remove_overlay:
 * @self: #AwnOverlayable instance.
 * @overlay: #AwnOverlay which was previously added using
 *  awn_overlayable_add_overlay.
 *
 * Removes overlay from the list of rendered overlays.
 */
void
awn_overlayable_remove_overlay (AwnOverlayable *self, AwnOverlay *overlay)
{
  awn_effects_remove_overlay (awn_overlayable_get_effects (self), overlay);
}

/**
 * awn_overlayable_get_overlays:
 * @self: #AwnOverlayable instance.
 *
 * Returns: a newly-allocated list of the overlays added for this instance.
 */
GList*
awn_overlayable_get_overlays (AwnOverlayable *self)
{
  return awn_effects_get_overlays (awn_overlayable_get_effects (self));
}

