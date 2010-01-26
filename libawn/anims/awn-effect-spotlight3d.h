/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
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
 */


#ifndef __AWN_EFFECT_SPOTLIGHT3D_H__
#define __AWN_EFFECT_SPOTLIGHT3D_H__

#include "awn-effects-shared.h"

gboolean spotlight3D_effect(AwnEffectsAnimation * anim);
gboolean spotlight3D_hover_effect(AwnEffectsAnimation * anim);
gboolean spotlight3D_opening_effect(AwnEffectsAnimation * anim);
gboolean spotlight3D_closing_effect(AwnEffectsAnimation * anim);
gboolean spotlight3D_effect_finalize(AwnEffectsAnimation * anim);

#endif
