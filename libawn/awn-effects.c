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
#include <string.h>

typedef enum {
	AWN_EFFECT_PRIORITY_HIGHEST,
	AWN_EFFECT_PRIORITY_HIGH,
	AWN_EFFECT_PRIORITY_ABOVE_NORMAL,
	AWN_EFFECT_PRIORITY_NORMAL,
	AWN_EFFECT_PRIORITY_BELOW_NORMAL,
	AWN_EFFECT_PRIORITY_LOW,
	AWN_EFFECT_PRIORITY_LOWEST
} AwnEffectPriority;

typedef struct _AwnEffectsPrivate AwnEffectsPrivate;

struct _AwnEffectsPrivate {
	AwnEffects *effects;
	AwnEffect this_effect;
	gint max_loops;
	AwnEffectPriority priority;
	AwnEventNotify start, stop;
};

#define  M_PI	3.14159265358979323846

#define AWN_EFFECT_DIR_DOWN			0
#define AWN_EFFECT_DIR_UP			1
#define  AWN_FRAME_RATE 			40
#define  AWN_EFFECT_BOUNCE_SLEEP		0
#define  AWN_EFFECT_BOUNCE_SQEEZE_DOWN		1
#define  AWN_EFFECT_BOUNCE_SQEEZE_UP		2
#define  AWN_EFFECT_BOUNCE_JUMP_UP		3
#define  AWN_EFFECT_BOUNCE_JUMP_DOWN		4
#define  AWN_EFFECT_BOUNCE_SQEEZE_DOWN2		5
#define  AWN_EFFECT_BOUNCE_SQEEZE_UP2		6
#define  AWN_EFFECT_TURN_1			0
#define  AWN_EFFECT_TURN_2			1
#define  AWN_EFFECT_TURN_3			2
#define  AWN_EFFECT_TURN_4			3

const char *EFFECT_NAMES[] = {
	"AWN_EFFECT_NONE",
	"AWN_EFFECT_OPENING",
	"AWN_EFFECT_LAUNCHING",
	"AWN_EFFECT_HOVER",
	"AWN_EFFECT_ATTENTION",
	"AWN_EFFECT_CLOSING",
	"AWN_EFFECT_CHANGE_NAME"
};

/* FORWARD DECLARATIONS */

//static gboolean awn_task_icon_spotlight_effect (AwnEffects *fx, int j);
//static gboolean awn_task_icon_wrapper( AwnEffects *fx, int i );
static void awn_task_icon_spotlight_init (AwnEffects *fx);

// effect functions
static gboolean bounce_effect (AwnEffectsPrivate *priv);
static gboolean fading_effect (AwnEffectsPrivate *priv);

static gboolean awn_on_enter_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data);
static gboolean awn_on_leave_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data);

static void main_effect_loop(AwnEffects *fx);

void
awn_effects_init(GObject* self, AwnEffects *fx) {
	fx->self = self;
	fx->settings = awn_get_settings();
	fx->focus_window = NULL;
	fx->title = NULL;
	fx->get_title = NULL;
	fx->effect_queue = NULL;
	
	fx->effect_y_offset = 0;

	fx->window_width = 0;
	fx->window_height = 0;
	fx->icon_width = 48;
	fx->icon_height = 48;
	fx->delta_width = 0;
	fx->delta_height = 0;

	/* EFFECT VARIABLES */
	fx->effect_lock = FALSE;
	fx->effect_direction = 0;
	fx->count = 0;

	fx->x_offset = 0;
	fx->y_offset = 0;
	fx->rotate_degrees = 0.0;
	fx->alpha = 1.0;
	fx->spotlight_alpha = 1.0;

	fx->enter_notify = 0;
	fx->leave_notify = 0;
}

void
awn_effects_finalize(AwnEffects *fx) {
	awn_unregister_effects(fx);
	fx->focus_window = NULL;
	fx->self = NULL;
}

inline gboolean awn_effect_check_max_loops(AwnEffectsPrivate *priv) {
	gboolean max_reached = FALSE;
	if (priv->max_loops > 0) {
		priv->max_loops--;
		max_reached = priv->max_loops <= 0;
	}
	if (max_reached) awn_effect_stop(priv->effects, priv->this_effect);
	return max_reached;
}

inline gboolean awn_effect_check_top_effect(AwnEffectsPrivate *priv, gboolean *stopped) {
	*stopped = TRUE;
	AwnEffects *fx = priv->effects;
	GList *queue = fx->effect_queue;
	AwnEffectsPrivate *item;
	while (queue) {
		item = queue->data;
		if (item->this_effect == priv->this_effect) {
			*stopped = FALSE;
			break;
		}
		queue = g_list_next(queue);
	}
	if (!fx->effect_queue) return FALSE;
	item = fx->effect_queue->data;
	return item->this_effect == priv->this_effect;
}

inline gboolean awn_effect_handle_repeating(AwnEffectsPrivate *priv) {
	gboolean effect_stopped = TRUE;
	gboolean max_reached = awn_effect_check_max_loops(priv);
	gboolean repeat = !max_reached && awn_effect_check_top_effect(priv, &effect_stopped);
	if (!repeat) {
		gboolean unregistered = FALSE;
		AwnEffects *fx = priv->effects;
		fx->current_effect = AWN_EFFECT_NONE;
		fx->effect_lock = FALSE;
		if (effect_stopped) {
			if (priv->stop) priv->stop(fx->self);
			unregistered = fx->self == NULL;
			g_free(priv);
		}
		if (!unregistered) main_effect_loop(fx);
	}
	return repeat;
}


void
awn_effects_set_title(AwnEffects *fx, AwnTitle* title, AwnTitleCallback title_func) {
	fx->title = title;
	fx->get_title = title_func;
}

void
awn_effect_force_quit(AwnEffects *fx)
{
	// TODO: UNSAFE right now, better don't use
	GList* queue = fx->effect_queue;
	while (queue) {
		g_free(queue->data);
		queue->data = NULL;
		queue = g_list_next(queue);
	}
	g_list_free(fx->effect_queue);
	fx->effect_queue = NULL;
}

/*
void
awn_task_icon_spotlight_init (AwnEffects *fx)
{
	fx->y_offset = 0;
	fx->effect_y_offset = fx->settings->bar_height;
	GError *error = NULL;

	if(fx->settings->icon_effect_spotlight == 1)
		fx->spotlight = gdk_pixbuf_new_from_file_at_scale("/usr/local/share/avant-window-navigator/active/spotlight1.png",75,100,TRUE, &error);
	else
		fx->spotlight = gdk_pixbuf_new_from_file_at_scale("/usr/local/share/avant-window-navigator/active/spotlight2.png",75,100,TRUE, &error);		
	fx->alpha = 1.0;
	fx->current_width = fx->icon_width/2;
}


static gboolean
 awn_task_icon_spotlight_effect (AwnEffects *fx, int j)
 {
	const gint max = fx->settings->bar_height + 2;
 	
	if(fx->first_run)
		awn_task_icon_spotlight_init ((gpointer)fx);
 	
 	if(fx->is_closing.state)
 	{		
		if( fx->is_closing.max_loop != 0 && fx->is_closing.loop > fx->is_closing.max_loop ) {
			fx->kill = TRUE;
			awn_stop_effect(AWN_EFFECT_CLOSING, fx);			
			return TRUE;
		} 
		else if ( fx->effect_y_offset >= max ) {
			fx->kill = TRUE;
			awn_stop_effect(AWN_EFFECT_CLOSING, fx);			
			return TRUE;
		}
		else {
 			fx->spotlight_alpha += AWN_FRAME_RATE * 0.2;
 			fx->effect_y_offset += AWN_FRAME_RATE * 0.05;
 			if(fx->current_width > 0+AWN_FRAME_RATE * 0.05)	
 				fx->current_width -= AWN_FRAME_RATE * 0.05;	
 			gtk_widget_queue_draw(GTK_WIDGET(fx->self));
 			return TRUE;
 		}
		fx->is_closing.loop++;
 	}
 	if(fx->is_opening.state)
 	{		
 		if ((fx->is_opening.max_loop != 0 && fx->is_opening.loop > fx->is_opening.max_loop) || fx->effect_y_offset <= 0) {
 			fx->y_offset = 0;
			fx->effect_y_offset = 0;
			fx->current_width = fx->icon_width;
			fx->current_height = fx->icon_height;
			gtk_widget_queue_draw(GTK_WIDGET(fx->self)); 			
			if( fx->effect_y_offset <= 0 )			
				awn_stop_effect(AWN_EFFECT_OPENING, fx);
			else
				awn_effect_force_quit(AWN_EFFECT_OPENING, fx);
			return TRUE;
 		} else {
 			fx->effect_y_offset -= AWN_FRAME_RATE * 0.05;
 			fx->spotlight_alpha = 100;
 			if(fx->current_width < fx->icon_width)	
 				fx->current_width += AWN_FRAME_RATE * 0.05;	
 			gtk_widget_queue_draw(GTK_WIDGET(fx->self));
			fx->is_opening.loop++;
			return TRUE;
 		}
 	}
 	if( fx->is_opening_launcher.state )
 	{
 		if (fx->is_opening_launcher.max_loop != 0 && fx->is_opening_launcher.loop > fx->is_opening_launcher.max_loop ) {
 			awn_effect_force_quit(AWN_EFFECT_LAUNCHING, fx);
			return TRUE;
		}
		
		if( fx->spotlight_alpha < 75 )
 			fx->spotlight_alpha += AWN_FRAME_RATE * 0.2;
 		else if( fx->spotlight_alpha > 75 )
 			fx->spotlight_alpha -= AWN_FRAME_RATE * 0.2;	
 		gtk_widget_queue_draw(GTK_WIDGET(fx->self));
		fx->is_opening_launcher.loop++;
 		return TRUE;	
 	}
 	if( fx->is_asking_attention.state )
 	{
 		if (fx->is_asking_attention.max_loop != 0 && fx->is_asking_attention.loop > fx->is_asking_attention.max_loop ) {
 			awn_effect_force_quit(AWN_EFFECT_ATTENTION, fx);
			return TRUE;
		}

		if( fx->spotlight_alpha < 100 )
 			fx->spotlight_alpha += AWN_FRAME_RATE * 0.2;
 		else if( fx->spotlight_alpha > 100 )
 			fx->spotlight_alpha -= AWN_FRAME_RATE * 0.2;	
 		gtk_widget_queue_draw(GTK_WIDGET(fx->self));
		fx->is_asking_attention.loop++;
 		return TRUE;
 	}
 
 	if(!fx->hover.state && fx->spotlight_alpha == 0)
 		return FALSE;
 	
 
 	if(fx->hover.state && fx->spotlight_alpha<100)
 		fx->spotlight_alpha = 100;
 	else
 	{
 		if(fx->spotlight_alpha>0)
 			fx->spotlight_alpha -= AWN_FRAME_RATE * 0.2;
 		else
 			fx->spotlight_alpha = 0;
 	}
 
	if(fx->hover.state)
	{
		if (fx->hover.max_loop != 0 && fx->hover.loop > fx->hover.max_loop ) {
 			awn_effect_force_quit(AWN_EFFECT_HOVER, fx);
			return TRUE;
		}
		fx->hover.loop++;
	}
 	gtk_widget_queue_draw(GTK_WIDGET(fx->self));
 	return TRUE;
 }


gboolean
awn_task_icon_wrapper( AwnEffects *fx, int i )
{
 		
 	//if(strcmp(fx->settings->icon_effect_active,"BOUNCE")==0)
 	//	return awn_task_icon_bounce_effect (priv);
 	//else if(strcmp(fx->settings->icon_effect_active,"TURN")==0)
 	//	return awn_task_icon_turn_effect(task);
 	//else if(strcmp(fx->settings->icon_effect_active,"ZOOM")==0)
 	//	return awn_task_icon_zoom_effect(task);
 	//else if(strcmp(fx->settings->icon_effect_active,"ZOOM2")==0)
 	//	return awn_task_icon_zoom2_effect(task);
 	//else if(strcmp(fx->settings->icon_effect_active,"FADE")==0)
 	//	return awn_task_icon_fade_effect(task);
 	//else if(strcmp(fx->settings->icon_effect_active,"OSXZOOM")==0)
 	//	return awn_task_icon_osx_effect(task);
 	//else 
	if(strcmp(fx->settings->icon_effect_active,"SPOTLIGHT")==0)
 		return awn_task_icon_spotlight_effect(fx,i);
 	
 	return FALSE;
}*/

// simple bounce effect based on sin function
static gboolean
bounce_effect (AwnEffectsPrivate *priv)
{
        AwnEffects *fx = priv->effects;
	if (!fx->effect_lock) {
		fx->effect_lock = TRUE;
		// effect start initialize values
		fx->count = 0;
		if (priv->start) priv->start(fx->self);
		// TODO: set priv->start to NULL, so it's not called multiple times while moving in the queue?
	}

	const gdouble MAX_BOUNCE_OFFSET = 15.0;
	const gint PERIOD = 20;

	fx->y_offset = sin(++fx->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;

	// repaint widget
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	gboolean repeat = TRUE;
	if (fx->count >= PERIOD) {
		fx->count = 0;
		// check for repeating
		repeat = awn_effect_handle_repeating(priv);
	}
	return repeat;
}

static gboolean
bounce_effect2 (AwnEffectsPrivate *priv)
{
        AwnEffects *fx = priv->effects;
	if (!fx->effect_lock) {
		fx->effect_lock = TRUE;
		// effect start initialize values
		fx->count = 0;
		fx->delta_width = 0;
		fx->delta_height = 0;
		fx->effect_direction = AWN_EFFECT_BOUNCE_SQEEZE_DOWN;
		if (priv->start) priv->start(fx->self);
		// TODO: set priv->start to NULL, so it's not called multiple times while moving in the queue?
	}

	const gdouble MAX_BOUNCE_OFFSET = 15.0;
	const gint PERIOD = 28;
	
	if(fx->effect_direction == AWN_EFFECT_BOUNCE_SQEEZE_DOWN)
	{
		fx->delta_width += (fx->icon_width*3/4)/(PERIOD/4);
		fx->delta_height -= (fx->icon_height*3/4)/(PERIOD/4);
		fx->effect_y_offset = -fx->delta_height;
		if(fx->delta_height <= fx->icon_height*-1/4)
			fx->effect_direction = AWN_EFFECT_BOUNCE_SQEEZE_UP;
	}
	else if(fx->effect_direction == AWN_EFFECT_BOUNCE_SQEEZE_UP)
	{
		fx->delta_width -= (fx->icon_width*3/4)/(PERIOD/4);
		fx->delta_height += (fx->icon_height*3/4)/(PERIOD/4);
		fx->effect_y_offset = -fx->delta_height;
		if(fx->delta_height >= 0)
			fx->effect_direction = AWN_EFFECT_BOUNCE_SLEEP;
	}
	else if(fx->effect_direction == AWN_EFFECT_BOUNCE_SLEEP)
	{
		fx->y_offset = sin(++fx->count * M_PI * 2 / PERIOD) * MAX_BOUNCE_OFFSET;
		if(fx->count == PERIOD/2)
			fx->effect_direction = AWN_EFFECT_BOUNCE_SQEEZE_DOWN2;
	}
	else if(fx->effect_direction == AWN_EFFECT_BOUNCE_SQEEZE_DOWN2)
	{
		fx->delta_width += (fx->icon_width*3/4)/(PERIOD/4);
		fx->delta_height -= (fx->icon_height*3/4)/(PERIOD/4);
		fx->effect_y_offset = -fx->delta_height;
		if(fx->delta_height <= fx->icon_height*-1/4)
			fx->effect_direction = AWN_EFFECT_BOUNCE_SQEEZE_UP2;
	}
	else if(fx->effect_direction == AWN_EFFECT_BOUNCE_SQEEZE_UP2)
	{
		fx->delta_width -= (fx->icon_width*3/4)/(PERIOD/4);
		fx->delta_height += (fx->icon_height*3/4)/(PERIOD/4);
		fx->effect_y_offset = -fx->delta_height;
	}
	
	// repaint widget
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	gboolean repeat = TRUE;
	if (fx->effect_direction == AWN_EFFECT_BOUNCE_SQEEZE_UP2 && fx->delta_height >= 0) {
		fx->count = 0;
		fx->effect_direction = AWN_EFFECT_BOUNCE_SLEEP;
		fx->delta_width = 0;
		fx->delta_height = 0;
		// check for repeating
		repeat = awn_effect_handle_repeating(priv);
	}
	return repeat;
}

static gboolean
fade_out_effect (AwnEffectsPrivate *priv)
{
        AwnEffects *fx = priv->effects;
	if (!fx->effect_lock) {
		fx->effect_lock = TRUE;
		// effect start initialize values
		fx->count = 0;
		fx->alpha = 1.0;
		if (priv->start) priv->start(fx->self);
	}

	const gdouble MAX_OFFSET = 50.0;
	const gint PERIOD = 20;

	fx->y_offset = ++fx->count * (MAX_OFFSET/PERIOD);
	fx->alpha = fx->count*(-1.0/PERIOD)+1;
	fx->delta_width = cos(fx->count * M_PI/3/PERIOD) * fx->icon_width - fx->icon_width;
	fx->delta_height = fx->delta_width;
 
	// repaint widget
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	gboolean repeat = TRUE;
	if (fx->count >= PERIOD) {
		fx->count = 0;
		// check for repeating
		repeat = awn_effect_handle_repeating(priv);
	}
	return repeat;
}

static gboolean
bounce_opening_effect (AwnEffectsPrivate *priv)
{
        AwnEffects *fx = priv->effects;
	if (!fx->effect_lock) {
		fx->effect_lock = TRUE;
		// effect start initialize values
		fx->count = 0;
		fx->effect_direction = AWN_EFFECT_DIR_UP;
		fx->delta_width = -fx->icon_width;
		fx->delta_height = -fx->icon_height;
		if (priv->start) priv->start(fx->self);
	}

	const gdouble MAX_BOUNCE_OFFSET = 15.0;
	const gint PERIOD = 20;

	fx->y_offset = sin(++fx->count * M_PI / PERIOD) * MAX_BOUNCE_OFFSET;
	if(fx->delta_width < 0)
		fx->delta_width += fx->icon_width*2/PERIOD;
	else
		fx->delta_width = 0;
	if(fx->delta_height < 0) {
		fx->delta_height += fx->icon_height*2/PERIOD;
		fx->effect_y_offset = -fx->delta_height/2;
	} else {
		fx->delta_height = 0;
		fx->effect_y_offset = 0;
	}
	
	// repaint widget
	gtk_widget_queue_draw(GTK_WIDGET(fx->self));

	gboolean repeat = TRUE;
	if (fx->count >= PERIOD) {
		fx->count = 0;
		// check for repeating
		repeat = awn_effect_handle_repeating(priv);
	}
	return repeat;
}


static gboolean
fading_effect (AwnEffectsPrivate *priv)
{
        AwnEffects *fx = priv->effects;
	if (!fx->effect_lock) {
		fx->effect_lock = TRUE;
		// effect start initialize values
		fx->alpha = 1.0;
		fx->effect_direction = AWN_EFFECT_DIR_DOWN;
		if (priv->start) priv->start(fx->self);
		// TODO: set priv->start to NULL, so it's not called multiple times while moving in the queue?
	}
	const gdouble MIN_ALPHA = 0.35;
	const gdouble ALPHA_STEP = 0.05;

	gboolean repeat = TRUE;
	if (fx->effect_direction == AWN_EFFECT_DIR_DOWN) {
		fx->alpha -= ALPHA_STEP;
		if (fx->alpha <= MIN_ALPHA)
			fx->effect_direction = AWN_EFFECT_DIR_UP;
		// repaint widget
		gtk_widget_queue_draw(GTK_WIDGET(fx->self));
	} else {
		fx->alpha += ALPHA_STEP * 1.5;
		// repaint widget
		gtk_widget_queue_draw(GTK_WIDGET(fx->self));
		if (fx->alpha >= 1) {
			fx->alpha = 1.0;
			fx->effect_direction = AWN_EFFECT_DIR_DOWN;
			repeat = awn_effect_handle_repeating(priv);
		}
	}

	return repeat;
}

 
static gboolean awn_on_enter_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	
	AwnEffects *fx = (AwnEffects*)data;

	if (fx->settings) fx->settings->hiding = FALSE;

	if (fx->title && fx->get_title) {
		awn_title_show(fx->title, fx->focus_window, fx->get_title(fx->self));
	}

	awn_effect_start(fx, AWN_EFFECT_HOVER);
	return FALSE;
}

static gboolean awn_on_leave_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	AwnEffects *fx = (AwnEffects*)data;

	if (fx->title) {
		awn_title_hide(fx->title, fx->focus_window);
	}
	
	awn_effect_stop(fx, AWN_EFFECT_HOVER);
	return FALSE;
}

static gint awn_effect_sort(gconstpointer a, gconstpointer b) {
	const AwnEffectsPrivate *data1 = (AwnEffectsPrivate*)a;
	const AwnEffectsPrivate *data2 = (AwnEffectsPrivate*)b;
	return (gint)(data1->priority - data2->priority);
}

inline AwnEffectPriority awn_effect_get_priority(const AwnEffect effect) {
	switch (effect) {
		case AWN_EFFECT_OPENING: return AWN_EFFECT_PRIORITY_HIGH;
		case AWN_EFFECT_LAUNCHING: return AWN_EFFECT_PRIORITY_NORMAL;
		case AWN_EFFECT_HOVER: return AWN_EFFECT_PRIORITY_LOW;
		case AWN_EFFECT_ATTENTION: return AWN_EFFECT_PRIORITY_ABOVE_NORMAL;
		case AWN_EFFECT_CLOSING: return AWN_EFFECT_PRIORITY_HIGHEST;
		case AWN_EFFECT_CHANGE_NAME: return AWN_EFFECT_PRIORITY_NORMAL;
		default: return AWN_EFFECT_PRIORITY_BELOW_NORMAL;
	}
}

void awn_effect_start(AwnEffects *fx, const AwnEffect effect) {
	awn_effect_start_ex(fx, effect, NULL, NULL, 0);
}

void
awn_effect_start_ex(AwnEffects *fx, const AwnEffect effect, AwnEventNotify start, AwnEventNotify stop, gint max_loops) {
	if (effect == AWN_EFFECT_NONE || fx->self == NULL) return;

	AwnEffectsPrivate *queue_item;
	GList *queue = fx->effect_queue;
	// dont start the effect if already in queue
	while (queue) {
		queue_item = queue->data;
		if (queue_item->this_effect == effect) return;
		queue = g_list_next(queue);
	}
	AwnEffectsPrivate *priv = g_new(AwnEffectsPrivate, 1);
	priv->effects = fx;
	priv->this_effect = effect;
	priv->priority = awn_effect_get_priority(effect);
	priv->max_loops = max_loops;
	priv->start = start;
	priv->stop = stop;

	fx->effect_queue = g_list_insert_sorted(fx->effect_queue, priv, awn_effect_sort);
	main_effect_loop(fx);
}

void awn_effect_stop(AwnEffects *fx, const AwnEffect effect) {
	if (effect == AWN_EFFECT_NONE) return;

	AwnEffectsPrivate *queue_item;
	GList *queue = fx->effect_queue;
	// remove the effect if in queue
	while (queue) {
		queue_item = queue->data;
		if (queue_item->this_effect == effect) break;
		queue = g_list_next(queue);
	}
	if (queue) {
		gboolean dispose = queue_item->this_effect != fx->current_effect;
		fx->effect_queue = g_list_remove(fx->effect_queue, queue_item);
		if (dispose) g_free(queue_item);
	}
}

static void 
main_effect_loop(AwnEffects *fx) {
	if (fx->current_effect != AWN_EFFECT_NONE || fx->effect_queue == NULL) return;

	GSourceFunc animation = NULL;
	AwnEffectsPrivate *topEffect = (AwnEffectsPrivate*)(fx->effect_queue->data);
	switch (topEffect->this_effect) {
		case AWN_EFFECT_OPENING:
			animation = (GSourceFunc)bounce_opening_effect;
			break;
		case AWN_EFFECT_HOVER:
			// TODO: apply possible settings
			if (fx->settings && fx->settings->icon_effect)
				animation = (GSourceFunc)fading_effect;
			else
				animation = (GSourceFunc)bounce_effect2;
			break;
		case AWN_EFFECT_CLOSING:
			animation = (GSourceFunc)fade_out_effect;
			break;
		default: animation = (GSourceFunc)bounce_effect;
	}
	if (animation) {
		g_timeout_add(AWN_FRAME_RATE, animation, topEffect);
		fx->current_effect = topEffect->this_effect;
		fx->effect_lock = FALSE;
	}
}

void
awn_register_effects (GObject *obj, AwnEffects *fx) {
	fx->focus_window = GTK_WIDGET(obj);
	fx->enter_notify = g_signal_connect(obj, "enter-notify-event", G_CALLBACK(awn_on_enter_event), fx);
	fx->leave_notify = g_signal_connect(obj, "leave-notify-event", G_CALLBACK(awn_on_leave_event), fx);
}

void
awn_unregister_effects (AwnEffects *fx) {
	if (fx->enter_notify)
		g_signal_handler_disconnect(G_OBJECT(fx->focus_window), fx->enter_notify);
	if (fx->leave_notify)
		g_signal_handler_disconnect(G_OBJECT(fx->focus_window), fx->leave_notify);
	fx->enter_notify = 0;
	fx->leave_notify = 0;
}

void awn_draw_background(AwnEffects *fx, cairo_t *cr) {
	// TODO: paint possible background
}

void awn_draw_icons(AwnEffects *fx, cairo_t *cr, GdkPixbuf *icon, GdkPixbuf *reflect) {
	g_return_if_fail(icon != NULL && fx->window_width>0 && fx->window_height>0);
	gint x1 = (fx->window_width - fx->icon_width)/2;
	gint y1 = fx->y_offset;
	if (fx->settings) y1 = fx->settings->bar_height - fx->y_offset;

	/* refresh icon info */
	fx->icon_width = gdk_pixbuf_get_width(icon);
	fx->icon_height = gdk_pixbuf_get_height(icon);

	/* icon */
	gboolean free_reflect = FALSE;
	gboolean was_scaled = FALSE;
	GdkPixbuf *scaledIcon = NULL;
	if( fx->delta_width ||  fx->delta_height) {
		// scale icon
		was_scaled = TRUE;
		scaledIcon = gdk_pixbuf_scale_simple(icon, fx->icon_width + fx->delta_width, fx->icon_height + fx->delta_height, GDK_INTERP_BILINEAR);
		// refresh reflection, we scaled icon
		reflect = gdk_pixbuf_flip(scaledIcon, FALSE);
		free_reflect = TRUE;
		// TODO: stop using effect_y_offset, use y_offset instead
		// adjust offsets
		x1 = (fx->window_width - fx->icon_width - fx->delta_width)/2;
		y1 += fx->effect_y_offset;
		gdk_cairo_set_source_pixbuf(cr, scaledIcon, x1, y1);
	} else
		gdk_cairo_set_source_pixbuf(cr, icon, x1, y1);
	cairo_paint_with_alpha(cr, fx->alpha);
	if (was_scaled) g_object_unref(scaledIcon);

	/* reflection */
	if (fx->y_offset >= 0) {
		if (!reflect) {
			reflect = gdk_pixbuf_flip(icon, FALSE);
			free_reflect = TRUE;
		}
		y1 += fx->icon_height + fx->y_offset*2;
		if (was_scaled) y1 -= fx->effect_y_offset;
		gdk_cairo_set_source_pixbuf(cr, reflect, x1, y1);
		cairo_paint_with_alpha(cr, fx->alpha/3);
		if (free_reflect) g_object_unref(reflect);
	}

	/* 4px offset for 3D look */
	if (fx->settings && fx->settings->bar_angle != 0) {
		cairo_save(cr);
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba (cr, 1, 1, 1, 0);
		cairo_rectangle (cr, 0, ((fx->settings->bar_height *2)-4)+ fx->settings->icon_offset, fx->window_width,  4);
		cairo_fill (cr);
		cairo_restore (cr);
	}
}

void awn_draw_foreground(AwnEffects *fx, cairo_t *cr) {
	// TODO: paint possible foreground
}

void awn_draw_set_size(AwnEffects *fx, const gint width, const gint height) {
	fx->window_width = width;
	fx->window_height = height;
}

