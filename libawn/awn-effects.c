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
#define  AWN_FRAME_RATE 40
#define  AWN_TASK_EFFECT_DIR_UP			0
#define  AWN_TASK_EFFECT_DIR_DOWN		1
#define  AWN_TASK_EFFECT_BOUNCE_SLEEP		0
#define  AWN_TASK_EFFECT_BOUNCE_SQEEZE_DOWN	1
#define  AWN_TASK_EFFECT_BOUNCE_SQEEZE_UP	2
#define  AWN_TASK_EFFECT_BOUNCE_JUMP_UP		3
#define  AWN_TASK_EFFECT_BOUNCE_JUMP_DOWN	4
#define  AWN_TASK_EFFECT_BOUNCE_SQEEZE_DOWN2	5
#define  AWN_TASK_EFFECT_BOUNCE_SQEEZE_UP2	6
#define  AWN_TASK_EFFECT_TURN_1			0
#define  AWN_TASK_EFFECT_TURN_2			1
#define  AWN_TASK_EFFECT_TURN_3			2
#define  AWN_TASK_EFFECT_TURN_4			3


/* FORWARD DECLARATIONS */
static gboolean awn_task_icon_spotlight_effect (AwnEffects *fx, int j);
       gboolean awn_task_icon_wrapper( AwnEffects *fx, int i );
       gboolean awn_tasks_icon_effects(gboolean *true);
static gboolean awn_on_enter_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data);
static gboolean awn_on_leave_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data);
void		awn_task_icon_spotlight_init (AwnEffects *fx);

GList *effect_list;
gboolean effectbusy;
GdkEventMotion *effectevent;

void
awn_effects_init(GObject* self, AwnEffects *fx) {
	fx->self = self;
	fx->settings = awn_get_settings();
	fx->focus_window = NULL;
	fx->title = NULL;
	fx->get_title = NULL;
	
	fx->is_active = FALSE;
	fx->is_closing.state = FALSE;
	fx->is_closing.max_loop = 0;
	fx->is_opening.state = TRUE;
	fx->is_opening.max_loop = 0;
	fx->is_opening_launcher.state = FALSE;
	fx->is_opening_launcher.max_loop = 1000;
	fx->is_asking_attention.state = FALSE;
	fx->is_asking_attention.max_loop = 0;
	fx->first_run = TRUE;
	fx->hover.state = FALSE;
	fx->hover.max_loop = 0;
	fx->kill = FALSE;	
	
	fx->bounce_offset = 0;
	fx->effect_y_offset = 0;
	fx->icon_width = 0;
	fx->icon_height = 0;
	fx->current_width = 0;	
	fx->normal_width = 0;
	fx->current_height = 0;
	fx->normal_height = 0;

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
	fx->alpha = 0.0;
	fx->spotlight_alpha = 0.0;

	fx->enter_notify = 0;
	fx->leave_notify = 0;
}

void
awn_effects_set_title(AwnEffects *fx, AwnTitle* title, AwnTitleCallback title_func) {
	fx->title = title;
	fx->get_title = title_func;
}

void
awn_effect_force_quit(const AwnEffect effect, AwnEffects *fx)
{
	switch (effect) {
		case AWN_EFFECT_NONE:
			return;
		case AWN_EFFECT_OPENING:
			if(fx->is_opening.force_loop_stop)
				fx->is_opening.force_loop_stop(fx->self);
			break;
		case AWN_EFFECT_LAUNCHING:
			if(fx->is_opening_launcher.force_loop_stop)
				fx->is_opening_launcher.force_loop_stop(fx->self);
			break;
		case AWN_EFFECT_HOVER:
			if(fx->hover.force_loop_stop)
				fx->hover.force_loop_stop(fx->self);
			break;
		case AWN_EFFECT_ATTENTION:
			if(fx->is_asking_attention.force_loop_stop)
				fx->is_asking_attention.force_loop_stop(fx->self);
			break;
		case AWN_EFFECT_CLOSING:
			if(fx->is_closing.force_loop_stop)
				fx->is_closing.force_loop_stop(fx->self);		
			break;
		case AWN_EFFECT_CHANGE_NAME:
			
			break;
	}
	awn_stop_effect(effect,fx);
}


void
awn_task_icon_spotlight_init (AwnEffects *fx)
{
	fx->first_run = FALSE;
	fx->y_offset = 0;
	fx->effect_y_offset = fx->settings->bar_height;
	GError *error = NULL;

	if(fx->settings->icon_effect_spotlight == 1)
		fx->spotlight = gdk_pixbuf_new_from_file_at_scale("/usr/local/share/avant-window-navigator/active/spotlight1.png",75,100,TRUE, &error);
	else
		fx->spotlight = gdk_pixbuf_new_from_file_at_scale("/usr/local/share/avant-window-navigator/active/spotlight2.png",75,100,TRUE, &error);		
	fx->alpha = 1.0;
	fx->normal_width = fx->icon_width;
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
			fx->current_width = fx->normal_width;
			fx->current_height = fx->normal_height;
			gtk_widget_queue_draw(GTK_WIDGET(fx->self)); 			
			if( fx->effect_y_offset <= 0 )			
				awn_stop_effect(AWN_EFFECT_OPENING, fx);
			else
				awn_effect_force_quit(AWN_EFFECT_OPENING, fx);
			return TRUE;
 		} else {
 			fx->effect_y_offset -= AWN_FRAME_RATE * 0.05;
 			fx->spotlight_alpha = 100;
 			if(fx->current_width < fx->normal_width)	
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
 }
 
 gboolean
 awn_tasks_icon_effects(gboolean *true)
 {	
 	//if(effectevent && effectevent->x_root && (int)effectevent->x_root != 0)
 	//	mousex = (int)effectevent->x_root;
 	
 	int i = 0;
 	gboolean change = FALSE;
 	GList *list = effect_list; 
 	while(list)
 	{
 		AwnEffects *fx = list->data;
		if(fx->kill)
			effect_list = g_list_remove(effect_list, list->data); 		
		else if( awn_task_icon_wrapper(fx, i) )
 			change = TRUE;	
		list = g_list_next(list);
 		i++;
 	}
 	g_list_free(list);
 	if(!change)
 		effectbusy = FALSE;

 	return change;
 }


static gboolean check_hover(AwnEffects *fx) {
	return fx->hover.state;
}

static gboolean awn_on_enter_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	
	AwnEffects *fx = (AwnEffects*)data;

	fx->hover.state = TRUE;
	if (fx->settings) fx->settings->hiding = FALSE;

	if (fx->title && fx->get_title) {
		awn_title_show(fx->title, fx->focus_window, fx->get_title(fx->self));
	}

	awn_start_effect(AWN_EFFECT_HOVER, fx);	
	return FALSE;
}

static gboolean awn_on_leave_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data) {
	AwnEffects *fx = (AwnEffects*)data;
	fx->hover.state = FALSE;

	if (fx->title) {
		awn_title_hide(fx->title, fx->focus_window);
	}
	
	awn_stop_effect(AWN_EFFECT_HOVER, fx);	
	return FALSE;
}

void awn_start_effect(const AwnEffect effect, AwnEffects *fx) {
	switch (effect) {
		case AWN_EFFECT_NONE:
			return;
		case AWN_EFFECT_OPENING:
			fx->is_opening.state = TRUE;
			if(fx->is_opening.start)
				fx->is_opening.start(fx->self);
			break;
		case AWN_EFFECT_LAUNCHING:
			fx->is_opening_launcher.state = TRUE;
			if(fx->is_opening_launcher.start)
				fx->is_opening_launcher.start(fx->self);
			break;
		case AWN_EFFECT_HOVER:
			fx->hover.state = TRUE;
			if(fx->hover.start)
				fx->hover.start(fx->self);
			break;
		case AWN_EFFECT_ATTENTION:
			fx->is_asking_attention.state = TRUE;
			if(fx->is_asking_attention.start)
				fx->is_asking_attention.start(fx->self);
			break;
		case AWN_EFFECT_CLOSING:
			fx->is_closing.state = TRUE;
			if(fx->is_closing.start)
				fx->is_closing.start(fx->self);
			break;
		case AWN_EFFECT_CHANGE_NAME:
			
			break;
	}
	
	if( !effectbusy )
	{
		effectbusy = TRUE;
		g_timeout_add(AWN_FRAME_RATE, (GSourceFunc)awn_tasks_icon_effects,(gpointer)1);
	}
}

void awn_stop_effect(const AwnEffect effect, AwnEffects *fx) {
	switch (effect) {
		case AWN_EFFECT_NONE:
			return;
		case AWN_EFFECT_OPENING:
			fx->is_opening.state = FALSE;
			fx->is_opening.loop = 0;
			if(fx->is_opening.stop)
				fx->is_opening.stop(fx->self);
			break;
		case AWN_EFFECT_LAUNCHING:
			fx->is_opening_launcher.state = FALSE;
			fx->is_opening_launcher.loop = 0;
			if(fx->is_opening_launcher.stop)
				fx->is_opening_launcher.stop(fx->self);
			break;
		case AWN_EFFECT_HOVER:
			fx->hover.state = FALSE;
			fx->hover.loop = 0;
			if(fx->hover.stop)
				fx->hover.stop(fx->self);
			break;
		case AWN_EFFECT_ATTENTION:
			fx->is_asking_attention.state = FALSE;
			fx->is_asking_attention.loop = 0;
			if(fx->is_asking_attention.stop)
				fx->is_asking_attention.stop(fx->self);
			break;
		case AWN_EFFECT_CLOSING:
			if(!fx->kill)
				g_print("Effect problem: You cannot stop the closing effect!!!!\n");
			if(fx->is_closing.stop)
				fx->is_closing.stop(fx->self);		
			break;
		case AWN_EFFECT_CHANGE_NAME:
			
			break;
	}

	if( !effectbusy )
	{
		effectbusy = TRUE;
		g_timeout_add(AWN_FRAME_RATE, (GSourceFunc)awn_tasks_icon_effects,(gpointer)1);
	}
}

void
awn_register_effects (GObject *obj, AwnEffects *fx) {
	fx->focus_window = GTK_WIDGET(obj);
	fx->enter_notify = g_signal_connect(obj, "enter-notify-event", G_CALLBACK(awn_on_enter_event), fx);
	fx->leave_notify = g_signal_connect(obj, "leave-notify-event", G_CALLBACK(awn_on_leave_event), fx);
	
	effect_list = g_list_append(effect_list, (gpointer)fx);
}

void
awn_unregister_effects (GObject *obj, AwnEffects *fx) {
	g_signal_handler_disconnect(obj, fx->enter_notify);
	g_signal_handler_disconnect(obj, fx->leave_notify);
}

void
awn_effects_set_notify(AwnEffects *fx, const AwnEffect effect, AwnEventNotify start, AwnEventNotify stop, AwnEventNotify force_loop_stop, gint max_loop) {
	switch (effect) {
		case AWN_EFFECT_NONE:
			return;
		case AWN_EFFECT_OPENING:
			fx->is_opening.start = start;
			fx->is_opening.start = stop;
			fx->is_opening.force_loop_stop = force_loop_stop;
			if(max_loop != -1)			
				fx->is_opening.max_loop = max_loop;			
			break;
		case AWN_EFFECT_LAUNCHING:
			fx->is_opening_launcher.start = start;
			fx->is_opening_launcher.stop = stop;
			fx->is_opening_launcher.force_loop_stop = force_loop_stop;
			if(max_loop != -1)
				fx->is_opening_launcher.max_loop = max_loop;
			break;
		case AWN_EFFECT_HOVER:
			fx->hover.start = start;
			fx->hover.stop = stop;
			fx->hover.force_loop_stop = force_loop_stop;
			if(max_loop != -1)
				fx->hover.max_loop = max_loop;
			break;
		case AWN_EFFECT_ATTENTION:
			fx->is_asking_attention.start = start;
			fx->is_asking_attention.stop = stop;
			fx->is_asking_attention.force_loop_stop = force_loop_stop;
			if(max_loop != -1)
				fx->is_asking_attention.max_loop = max_loop;
			break;
		case AWN_EFFECT_CLOSING:
			fx->is_closing.start = start;
			fx->is_closing.stop = stop;
			fx->is_closing.force_loop_stop = force_loop_stop;
			if(max_loop != -1)
				fx->is_closing.max_loop = max_loop;
			break;
		case AWN_EFFECT_CHANGE_NAME:
			
			break;
	}
}

gint
awn_effect_progress(AwnEffects *fx, const AwnEffect effect) {
	switch (effect) {
		case AWN_EFFECT_NONE:			
			return 0;
		case AWN_EFFECT_OPENING:
			if(!fx->is_opening.state)
				return 0;
			else
				return (fx->is_opening.loop/fx->is_opening.max_loop)*100;			
			break;
		case AWN_EFFECT_LAUNCHING:
			if(!fx->is_opening_launcher.state)
				return 0;
			else
				return (fx->is_opening_launcher.loop/fx->is_opening_launcher.max_loop)*100;
			break;
		case AWN_EFFECT_HOVER:
			if(!fx->hover.state)
				return 0;
			else
				return (fx->hover.loop/fx->hover.max_loop)*100;
			break;
		case AWN_EFFECT_ATTENTION:
			if(!fx->is_asking_attention.state)
				return 0;
			else
				return (fx->is_asking_attention.loop/fx->is_asking_attention.max_loop)*100;
			break;
		case AWN_EFFECT_CLOSING:
			if(!fx->is_closing.state)
				return 0;
			else
				return (fx->is_closing.loop/fx->is_closing.max_loop)*100;
			break;
		case AWN_EFFECT_CHANGE_NAME:
			
			break;
	}
}

