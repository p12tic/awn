/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
 *
*/

#ifndef _AWN_SETTINGS_H
#define _AWN_SETTINGS_H

#include <glib.h>

#include <libawn/awn-cairo-utils.h>

typedef struct
{
  gint     bar_height;
  gint     bar_angle;
  
  gint     icon_effect;
  gfloat   icon_alpha;
  gfloat   reflection_alpha_mult;
  gint     frame_rate;
  gboolean icon_depth_on;

  gint     icon_offset;
  gint     reflection_offset;
  gboolean show_shadows;

  gboolean hiding;

} AwnEffectsSettings;

AwnEffectsSettings * awn_effects_settings_get_default (void);

#endif /* _AWN_SETTINGS_EFFECTS_H */
