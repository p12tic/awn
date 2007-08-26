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

typedef struct _AwnEffectsPrivate AwnEffectsPrivate;

struct _AwnEffectsPrivate {
	AwnEffects *effects;
	AwnEffect next;
	gint max_loops;
	AwnEffectCondition condition;
	AwnEventNotify start, stop;
};

#define  M_PI	3.14159265358979323846

#define AWN_EFFECT_DIR_DOWN	0
#define AWN_EFFECT_DIR_UP	1
#define AWN_EFFECT_DIR_LEFT	2
#define AWN_EFFECT_DIR_RIGHT	3

#define AWN_EFFECT_INIT(fx, effect_name)			\
	if (fx->effect_lock) {					\
		if (fx->current_effect != effect_name)		\
			return TRUE;				\
	} else 
#define AWN_EFFECT_LOCK(fx, effect_name)			\
	fx->effect_lock = TRUE;					\
	fx->effect_sheduled = AWN_EFFECT_NONE;			\
	fx->current_effect = effect_name
#define AWN_EFFECT_FINISH(fx)					\
	fx->effect_lock = FALSE;				\
	fx->current_effect = AWN_EFFECT_NONE;			\
	fx->y_offset = 0

/* FORWARD DECLARATIONS */

static gboolean appear_effect (AwnEffectsPrivate *priv);
static gboolean disappear_effect (AwnEffectsPrivate *priv);
static gboolean bounce_effect (AwnEffectsPrivate *priv);
static gboolean multibounce_effect (AwnEffectsPrivate *priv);
static gboolean linearbounce_effect (AwnEffectsPrivate *priv);
static gboolean fade_effect (AwnEffectsPrivate *priv);
static gboolean fadein_effect (AwnEffectsPrivate *priv);
static gboolean fadeout_effect (AwnEffectsPrivate *priv);

void
awn_effects_init(GObject* self, AwnEffects *fx) {
	fx->self = self;
	fx->settings = awn_get_settings();
	fx->title = NULL;
	fx->get_title = NULL;
	fx->is_closing = FALSE;
	fx->hover = FALSE;

	fx->icon_width = 0;
	fx->icon_height = 0;

	/* EFFECT VARIABLES */
	fx->effect_lock = FALSE;
	fx->effect_sheduled = AWN_EFFECT_NONE;
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

void
awn_effects_set_title(AwnEffects *fx, AwnTitle* title, AwnTitleCallback title_func) {
	fx->title = title;
	fx->get_title = title_func;
}

void
awn_effects_set_notify(AwnEffects *fx, AwnEventNotify start, AwnEventNotify stop) {
	fx->start_anim = start;
	fx->stop_anim = stop;
}

static gboolean
appear_effect (AwnEffectsPrivate *priv)
{
	/* 
	 * This effect is made of two actual effects (appear and multibounce)
	 * When appearing ends, one loop of multibounce is scheduled.
	 */
	AwnEffects *fx = priv->effects;
	const gint PERIOD = 40;
	gint MAX_OFFSET = 50;
	if (fx->settings) MAX_OFFSET = fx->settings->bar_height;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		fx->count = 0;
		fx->y_offset = -MAX_OFFSET;
		if (priv->start) priv->start(fx->self);
	}
	// all other effects ended, our turn
	
	if (fx->count <= PERIOD/2) {
		fx->alpha = (double)fx->count / (PERIOD/2);
		fx->y_offset = cos(fx->count++ * M_PI / PERIOD) * MAX_OFFSET;
	} else {
		fx->y_offset = 0;
		fx->alpha = 1.0;
		AWN_EFFECT_FINISH(fx);
	}
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	gboolean repeat = fx->effect_lock;
	if (!repeat) {
		// don't dispose priv structure, we'll reuse it for multibounce
		fx->effect_sheduled = priv->next;
		priv->start = NULL;
		g_timeout_add(40, (GSourceFunc)multibounce_effect, priv);
	}
	return repeat;
}

static gboolean
disappear_effect (AwnEffectsPrivate *priv)
{
	AwnEffects *fx = priv->effects;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		fx->y_offset = 0;
		if (priv->start) priv->start(fx->self);
	}
	// all other effects ended, our turn

	gint MAX_OFFSET = 50;
	if (fx->settings) MAX_OFFSET = fx->settings->bar_height + 2;
	
	fx->y_offset++;
	fx->alpha = 1.0 - (fx->y_offset / MAX_OFFSET);
	if (fx->y_offset >= MAX_OFFSET) {
		fx->alpha = 0;
		AWN_EFFECT_FINISH(fx);
	}
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	gboolean repeat = fx->effect_lock;
	if (!repeat) {
		if (priv->stop) priv->stop(fx->self);
		g_free(priv);
	}
	return repeat;
}

static gboolean
linearbounce_effect (AwnEffectsPrivate *priv)
{
	AwnEffects *fx = priv->effects;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		fx->count = 0;
		fx->effect_direction = AWN_EFFECT_DIR_UP;
		if (priv->start) priv->start(fx->self);
	}
	// all other effects ended, our turn
	const gdouble MAX_BOUNCE_OFFSET = 15.0;

	gboolean repeat = TRUE;
	if (fx->effect_direction == AWN_EFFECT_DIR_UP) {
		if (++fx->y_offset >= MAX_BOUNCE_OFFSET)
			fx->effect_direction = AWN_EFFECT_DIR_DOWN;
	} else {
		if (--fx->y_offset < 1) {
			fx->effect_direction = AWN_EFFECT_DIR_UP;
			repeat = FALSE;
			gboolean max_reached = FALSE;
			if (priv->max_loops) {
				priv->max_loops--;
				max_reached = priv->max_loops <= 0;
			}
			if (priv->condition) repeat = priv->condition(fx->self);
			if (!repeat || fx->is_closing || max_reached) {
				/* finished bouncing, back to normal */
				AWN_EFFECT_FINISH(fx);
			}
		}
	}
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	repeat = fx->effect_lock;
	if (!repeat) {
		if (priv->stop) priv->stop(fx->self);
		g_free(priv);
	}
	return repeat;
}

static gboolean
bounce_effect (AwnEffectsPrivate *priv)
{
	AwnEffects *fx = priv->effects;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		fx->count = 0;
		if (priv->start) priv->start(fx->self);
	}
	// all other effects ended, our turn
	const gdouble MAX_BOUNCE_OFFSET = 15.0;
	const gint PERIOD = 20; // number of calls to this function for the entire animation

	fx->y_offset = sin(++fx->count * M_PI / PERIOD)
			 * MAX_BOUNCE_OFFSET;

	gboolean repeat = TRUE;
	if (fx->count >= PERIOD) {
		fx->count = 0;
		repeat = FALSE;
		gboolean max_reached = FALSE;
		if (priv->max_loops) {
			priv->max_loops--;
			max_reached = priv->max_loops <= 0;
		}
		// HACK! pass fx itself for HOVER effect
		if (priv->condition)
			repeat = priv->condition(priv->next == AWN_EFFECT_HOVER ? (GObject*)fx : fx->self);
		if (!repeat || fx->is_closing || max_reached) {
			/* finished bouncing, back to normal */
			AWN_EFFECT_FINISH(fx);
		}
	}
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	repeat = fx->effect_lock;
	if (!repeat) {
		if (priv->stop) priv->stop(fx->self);
		g_free(priv);
	}
	return repeat;
}

static gboolean
multibounce_effect (AwnEffectsPrivate *priv)
{
	AwnEffects *fx = priv->effects;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		fx->count = 0;
		fx->effect_direction = 1;
		if (priv->start) priv->start(fx->self);
	}
	// all other effects ended, our turn
	const gdouble MAX_BOUNCE_OFFSET = 15.0;
	const gint PERIOD = 20; // number of calls to this function for the entire animation

	fx->y_offset = sin(++fx->count * M_PI / PERIOD)
			 * MAX_BOUNCE_OFFSET / (double)fx->effect_direction;

	gboolean repeat = TRUE;
	if (fx->count >= PERIOD) {
		fx->count = 0;
		repeat = FALSE;
		gboolean max_reached = FALSE;
		if (priv->max_loops) {
			priv->max_loops--;
			max_reached = priv->max_loops <= 0;
		}
		if (priv->condition) repeat = priv->condition(fx->self);
		if (!repeat || fx->is_closing || max_reached) {
			/* finished bouncing, back to normal */
			fx->effect_direction++;
			if (fx->effect_direction >= 4) {
				fx->effect_direction = 1;
				AWN_EFFECT_FINISH(fx);
			}
		} else fx->effect_direction = 1;
	}
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	repeat = fx->effect_lock;
	if (!repeat) {
		if (priv->stop) priv->stop(fx->self);
		g_free(priv);
	}
	return repeat;
}

static gboolean
fade_effect (AwnEffectsPrivate *priv)
{
	AwnEffects *fx = priv->effects;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		fx->effect_direction = AWN_EFFECT_DIR_DOWN;
		if (priv->start) priv->start(fx->self);
	}
	// all other effects ended, our turn
	const gdouble MIN_FADEOUT = 0.2;

	gboolean repeat = TRUE;
	if (fx->effect_direction == AWN_EFFECT_DIR_DOWN) {
		fx->alpha -= 0.05;
		if (fx->alpha <= MIN_FADEOUT)
			fx->effect_direction = AWN_EFFECT_DIR_UP;
	} else {
		fx->alpha += 0.05;
		if (fx->alpha >= 1) {
			fx->alpha = 1.0;
			fx->effect_direction = AWN_EFFECT_DIR_DOWN;
			repeat = FALSE;
			gboolean max_reached = FALSE;
			if (priv->max_loops) {
				priv->max_loops--;
				max_reached = priv->max_loops <= 0;
			}
			// HACK! pass fx itself for HOVER effect
			if (priv->condition)
				repeat = priv->condition(priv->next == AWN_EFFECT_HOVER ? (GObject*)fx : fx->self);
			if (!repeat || fx->is_closing || max_reached) {
				/* finished bouncing, back to normal */
				AWN_EFFECT_FINISH(fx);
			}
		}
	}
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	repeat = fx->effect_lock;
	if (!repeat) {
		if (priv->stop) priv->stop(fx->self);
		g_free(priv);
	}
	return repeat;
}

static gboolean
fadein_effect (AwnEffectsPrivate *priv)
{
	// should we even lock this simple effect?
	AwnEffects *fx = priv->effects;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		fx->count = 0;
		if (priv->start) priv->start(fx->self);
	}
	fx->alpha = 1.0;
	AWN_EFFECT_FINISH(fx);

	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	if (priv->stop) priv->stop(fx->self);
	g_free(priv);
	return FALSE;
}

static gboolean
fadeout_effect (AwnEffectsPrivate *priv)
{
	AwnEffects *fx = priv->effects;
	// waits for other effects to end
	AWN_EFFECT_INIT(fx, priv->next) {
		AWN_EFFECT_LOCK(fx, priv->next);
		// additional init possible here
		if (priv->start) priv->start(fx->self);
	}

        fx->alpha-=0.05;

	if (fx->alpha <= 0.2) {
		fx->alpha = 0.2;
		AWN_EFFECT_FINISH(fx);
	} else if (fx->hover) {
		// HACK to have same functionality as before awn-effects
		fx->alpha = 1.0;
		AWN_EFFECT_FINISH(fx);
	}

	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	gboolean repeat = fx->effect_lock;
	if (!repeat) {
		if (priv->stop) priv->stop(fx->self);
		g_free(priv);
	}
	return repeat;
}

static gboolean check_hover(AwnEffects *fx) {
	return fx->hover;
}

static gboolean awn_on_enter_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	const AwnEffect eff = AWN_EFFECT_HOVER;
	AwnEffects *fx = (AwnEffects*)data;

	fx->hover = TRUE;
	if (fx->settings) fx->settings->hiding = FALSE;

	if (fx->title && fx->get_title) {
		awn_title_show(fx->title, GTK_WIDGET(fx->self), fx->get_title(fx->self));
	}

	awn_schedule_repeating_effect(40, eff, fx, (AwnEffectCondition)check_hover, 0);
	return FALSE;
}

static gboolean awn_on_leave_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	AwnEffects *fx = (AwnEffects*)data;

	fx->hover = FALSE;

	if (fx->title) {
		awn_title_hide(fx->title, GTK_WIDGET(fx->self));
	}

	if (fx->settings && fx->settings->fade_effect) {
		fx->alpha = 0.2;
		gtk_widget_queue_draw(GTK_WIDGET(fx->self));
	}
	return FALSE;
}

void awn_schedule_effect(const gint timeout, const AwnEffect effect, AwnEffects *fx) {
	awn_schedule_repeating_effect(timeout, effect, fx, NULL, 0);
}

void 
awn_schedule_repeating_effect(const gint timeout, const AwnEffect effect, AwnEffects *fx, AwnEffectCondition condition, const gint max_loops) {
	GSourceFunc animation = NULL;

	if (fx->current_effect == effect || fx->effect_sheduled == effect) return;
	fx->effect_sheduled = effect;
	switch (effect) {
		case AWN_EFFECT_NONE:
			return;
		case AWN_EFFECT_OPENING:
			animation = (GSourceFunc)appear_effect;
			break;
		case AWN_EFFECT_LAUNCHING:
			animation = (GSourceFunc)bounce_effect;
			break;
		case AWN_EFFECT_HOVER:
			if (fx->settings && fx->settings->fade_effect) {
				animation = (GSourceFunc)fadein_effect;
			} else {
				animation = (GSourceFunc)bounce_effect;
			}
			break;
		case AWN_EFFECT_ATTENTION:
			animation = (GSourceFunc)bounce_effect;
			break;
		case AWN_EFFECT_CLOSING:
			animation = (GSourceFunc)disappear_effect;
			break;
		case AWN_EFFECT_CHANGE_NAME:
			animation = (GSourceFunc)bounce_effect;
			break;
		case AWN_EFFECT_FADE:
			animation = (GSourceFunc)fade_effect;
			break;
		case AWN_EFFECT_FADE_IN:
			animation = (GSourceFunc)fadein_effect;
			break;
		case AWN_EFFECT_FADE_OUT:
			animation = (GSourceFunc)fadeout_effect;
			break;
	}
	if (animation) {
		AwnEffectsPrivate *priv = g_new(AwnEffectsPrivate, 1);
		priv->effects = fx;
		priv->next = effect;
		priv->condition = condition;
		priv->max_loops = max_loops;
		priv->start = fx->start_anim;
		priv->stop = fx->stop_anim;
		g_timeout_add(timeout, animation, priv);
	}
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

