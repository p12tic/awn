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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "awn-effects.h"
#include <math.h>

#define  M_PI	3.14159265358979323846
#define AWN_EFFECT_INIT(fx, effect_name)			\
	if (fx->effect_lock) {					\
		if (fx->current_effect != effect_name)		\
			return TRUE;				\
	} else 
#define AWN_EFFECT_LOCK(fx, effect_name)			\
	fx->effect_sheduled = FALSE;				\
	fx->effect_lock = TRUE;					\
	fx->current_effect = effect_name;

void
awn_effects_init(GObject* self, AwnEffects *fx) {
	fx->self = self;
	fx->settings = NULL; // TODO: not null
        fx->needs_attention = FALSE;
	fx->is_closing = FALSE;
	fx->hover = FALSE;

	fx->icon_width = 0;
	fx->icon_height = 0;

	/* EFFECT VARIABLES */
	fx->effect_sheduled = FALSE;
	fx->effect_lock = FALSE;
	fx->current_effect = AWN_EFFECT_NONE;
	fx->effect_direction = 0;
	fx->count = 0;

	fx->x_offset = 0;
	fx->y_offset = 0;
	fx->width = 0;
	fx->height = 0;
	fx->rotate_degrees = 0.0;
	fx->alpha = 1.0;

	fx->enter_notify = 0;
	fx->leave_notify = 0;
}

static gboolean
bounce_effect (AwnEffects *fx)
{
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, AWN_EFFECT_HOVER) {
		AWN_EFFECT_LOCK(fx, AWN_EFFECT_HOVER);
	}
	// all other effects ended, our turn
	const gdouble MAX_BOUNCE_OFFSET = 15.0;
	const gint PERIOD = 20; // number of calls to this function for the entire animation

	fx->y_offset = sin(++fx->count * M_PI / PERIOD)
			 * MAX_BOUNCE_OFFSET / (double)fx->effect_direction;

	if (fx->count >= PERIOD) {
		fx->count = 0;
		/* finished bouncing, back to normal */
		if (!fx->hover || fx->is_closing)  {
			if (++fx->effect_direction >= 4 || fx->is_closing) {
				fx->effect_direction = 1;
				fx->effect_lock = FALSE;
				fx->current_effect = AWN_EFFECT_NONE;
				fx->y_offset = 0;
			}
		}
	else
		fx->effect_direction = 1;
	}
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	return fx->effect_lock;
}

static gboolean awn_on_enter_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	AwnEffects *fx = (AwnEffects*)data;
	fx->hover = TRUE;

	if (!fx->effect_sheduled && fx->current_effect != AWN_EFFECT_HOVER) {
		g_timeout_add(40, (GSourceFunc)bounce_effect, fx);
		fx->effect_sheduled = TRUE;
	}
}

static gboolean awn_on_leave_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	AwnEffects *fx = (AwnEffects*)data;

	fx->hover = FALSE;
}

void
awn_register_effects (GObject *obj, AwnEffects *fx) {
	fx->enter_notify = g_signal_connect(obj, "enter-notify-event", G_CALLBACK(awn_on_enter_event), fx);
	fx->leave_notify = g_signal_connect(obj, "leave-notify-event", G_CALLBACK(awn_on_leave_event), fx);
}

void
awn_unregister_effects (GObject *obj, AwnEffects *fx) {
	g_signal_handler_disconnect(obj, fx->enter_notify);
	g_signal_handler_disconnect(obj, fx->leave_notify);
}

