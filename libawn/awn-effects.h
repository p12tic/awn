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
} AwnEffect;

typedef const gchar* (*AwnTitleCallback)(GObject*);
typedef void (*AwnEventNotify)(GObject*);
typedef gboolean (*AwnEffectCondition)(GObject*);

typedef struct _AwnEffectProperties AwnEffectProperties;

struct _AwnEffectProperties {
        gboolean state;
        AwnEventNotify start;
	AwnEventNotify stop;
	AwnEventNotify force_loop_stop;
	gint max_loop;
	gint loop;
};

typedef struct _AwnEffects AwnEffects;

struct _AwnEffects
{
	GObject *self;
	GtkWidget *focus_window;
	AwnSettings *settings;
	AwnTitle *title;
	AwnTitleCallback get_title;
	AwnEventNotify start_anim, stop_anim;

	gboolean is_active;
	AwnEffectProperties is_closing;
	AwnEffectProperties is_opening;
	AwnEffectProperties is_opening_launcher;
	AwnEffectProperties is_asking_attention;
	gboolean first_run;
	AwnEffectProperties hover;
	gboolean kill;

	gint icon_width, icon_height;
	
	 /* EFFECT VARIABLES */
	gboolean effect_lock;
	AwnEffect effect_sheduled;
	AwnEffect current_effect;
	gint effect_direction;
	gint count;

	gdouble x_offset;
	gdouble y_offset;
	gint bounce_offset;
	gdouble effect_y_offset;
	gdouble previous_effect_y_offset;
	gint current_width;
	gint current_height;
	gint normal_width;
	gint normal_height;
	gint previous_width;
	gint previous_height;
	gint height;
	gint width;

	
	gdouble rotate_degrees;
	gfloat alpha;
	gfloat spotlight_alpha;

	GdkPixbuf *spotlight;

	guint enter_notify;
	guint leave_notify;
};

//! Initializes AwnEffects structure.
/*!
 * \param obj Object which will be passed to all callback functions, this object is also passed to gtk_widget_queue_draw() during the animation.
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

//! Start a single effect.
/*!
 * \param effect Effect to schedule.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_start_effect(const AwnEffect effect, AwnEffects *fx);

//! Stop a single effect.
/*!
 * \param effect Effect to stop.
 * \param fx Pointer to AwnEffects structure.
 */

void
awn_stop_effect(const AwnEffect effect, AwnEffects *fx);

//! Force a single effect to stop. This should be called when the effects is using more than the max_loop.  
/*!
 * \param effect Effect to stop.
 * \param fx Pointer to AwnEffects structure.
 */
void
awn_effect_force_quit(const AwnEffect effect, AwnEffects *fx);

//! Returns the progress of a specific effect (You get a percentage 0->100 of loop/max_loop)
/*!
 * \param fx Pointer to AwnEffects structure.
 * \param effect Effect to show the progres of.
 */

gint
awn_effect_progress(AwnEffects *fx, const AwnEffect effect);

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
awn_effects_set_notify(AwnEffects *fx, const AwnEffect effect, AwnEventNotify start, AwnEventNotify stop, AwnEventNotify force_loop_stop, gint max_loop);

G_END_DECLS

#endif
