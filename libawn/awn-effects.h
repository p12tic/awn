/*
 *  Copyright (C) 2007 Michal Hruby <michal.mhr@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
 *
*/

/*! \file awn-effects.h */

#ifndef __AWN_EFFECTS_H__
#define __AWN_EFFECTS_H__

#include <gtk/gtk.h>

#include "awn-defines.h"
#include "awn-gconf.h"
#include "awn-title.h"

G_BEGIN_DECLS

typedef enum {
        AWN_EFFECT_NONE,
        AWN_EFFECT_OPENING,
	AWN_EFFECT_LAUNCHING,
        AWN_EFFECT_HOVER,
        AWN_EFFECT_ATTENTION,
        AWN_EFFECT_CLOSING,
        AWN_EFFECT_CHANGE_NAME,
	AWN_EFFECT_FADE,
	AWN_EFFECT_FADE_IN,
	AWN_EFFECT_FADE_OUT
} AwnEffect;

typedef const gchar* (*AwnTitleCallback)(GObject*);
typedef void (*AwnEventNotify)(GObject*);
typedef gboolean (*AwnEffectCondition)(GObject*);

typedef struct _AwnEffects AwnEffects;

struct _AwnEffects
{
	GObject *self;
	AwnSettings *settings;
	AwnTitle *title;
	AwnTitleCallback get_title;
	AwnEventNotify start_anim, stop_anim;

	gboolean is_closing;
	gboolean hover;
	
	gint icon_width, icon_height;
	
	 /* EFFECT VARIABLES */
	gboolean effect_lock;
	AwnEffect effect_sheduled;
	AwnEffect current_effect;
	gint effect_direction;
	gint count;

	gdouble x_offset;
	gdouble y_offset;
	gint width;
	gint height;
	gdouble rotate_degrees;
	gfloat alpha;

	guint enter_notify;
	guint leave_notify;
};

//! Initializes AwnEffects structure.
/*!
 * \param obj Managed window to which the effects will apply.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_effects_init(GObject *obj, AwnEffects *fx);

//! Registers enter-notify and leave-notify events for managed window.
/*!
 * \param obj Managed window to which the effects will apply.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_register_effects (GObject *obj, AwnEffects *fx);

//! Unregisters events for managed window.
/*!
 * \param obj Managed window to which the effects apply.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_unregister_effects (GObject *obj, AwnEffects *fx);

//! Schedules single effect with one loop or HOVER effect.
/*!
 * \param timeout Frame time in miliseconds.
 * \param effect Effect to schedule.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_schedule_effect(const gint timeout, const AwnEffect effect, AwnEffects *fx);

//! Schedules repeating effect.
/*!
 * \param timeout Frame time in miliseconds.
 * \param effect Effect to schedule.
 * \param fx Pointer to AwnEffects structure.
 * \param condition Pointer to control function, the animation will stop looping when this function returns 0 (FALSE).
 * \param max_loops Maximum number of loops, leave 0 for unlimited looping.
 */
void
awn_schedule_repeating_effect(const gint timeout, const AwnEffect effect, AwnEffects *fx, AwnEffectCondition condition, const gint max_loops);

//! Makes AwnTitle appear on event-notify.
/*!
 * \param fx Pointer to AwnEffects structure.
 * \param title Pointer to AwnTitle instance.
 * \param title_func Pointer to function which returns desired title text.
 */
void
awn_effects_set_title(AwnEffects *fx, AwnTitle *title, AwnTitleCallback title_func);

//! Provides callbacks for animation start and end.
/*!
 * \param fx Pointer to AwnEffects structure.
 * \param start Function which will be called when animation starts.
 * \param stop Function which will be called when animation finishes.
 */
void
awn_effects_set_notify(AwnEffects *fx, AwnEventNotify start, AwnEventNotify stop);

G_END_DECLS

#endif
