/* Xlib utils */

/*
 * Copyright (C) 2001 Havoc Pennington
 *               2009 Michal Hruby <michal.mhr@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include "xutils.h"

enum {
	STRUT_LEFT = 0,
	STRUT_RIGHT = 1,
	STRUT_TOP = 2,
	STRUT_BOTTOM = 3,
	STRUT_LEFT_START = 4,
	STRUT_LEFT_END = 5,
	STRUT_RIGHT_START = 6,
	STRUT_RIGHT_END = 7,
	STRUT_TOP_START = 8,
	STRUT_TOP_END = 9,
	STRUT_BOTTOM_START = 10,
	STRUT_BOTTOM_END = 11
};

//pulled out of xutils.c
static Atom net_wm_strut              = 0;
static Atom net_wm_strut_partial      = 0;

/* xutils_set_strut:
 * @gdk_window: window to set struts on
 * @position: position of the window, top=0, right=1, bottom=2, left=3
 * @strut: the height of the strut
 * @strut_start: the start position of the strut
 * @strut_end: the end position of the strut
 *
 * Sets struts for a given @gdk_window on the specified @position. 
 * To let a maximised window not cover the given struts
 *
 * - partially pulled out of xutils.c -
 */
void
xutils_set_strut (GdkWindow        *gdk_window,
                  GtkPositionType   position,
                  guint32           strut,
                  guint32           strut_start,
                  guint32           strut_end)
{
  Display *display;
  Window   window;
  gulong   struts [12] = { 0, };

  g_return_if_fail (GDK_IS_WINDOW (gdk_window));

  display = GDK_WINDOW_XDISPLAY (gdk_window);
  window  = GDK_WINDOW_XWINDOW (gdk_window);

  if (net_wm_strut == None)
    net_wm_strut = XInternAtom (display, "_NET_WM_STRUT", False);
  if (net_wm_strut_partial == None)
    net_wm_strut_partial = XInternAtom (display, "_NET_WM_STRUT_PARTIAL", False);

  switch (position) {
    case GTK_POS_TOP:
      struts [STRUT_TOP] = strut;
      struts [STRUT_TOP_START] = strut_start;
      struts [STRUT_TOP_END] = strut_end;
      break;
    case GTK_POS_RIGHT:
      struts [STRUT_RIGHT] = strut;
      struts [STRUT_RIGHT_START] = strut_start;
      struts [STRUT_RIGHT_END] = strut_end;
      break;
    case GTK_POS_BOTTOM:
      struts [STRUT_BOTTOM] = strut;
      struts [STRUT_BOTTOM_START] = strut_start;
      struts [STRUT_BOTTOM_END] = strut_end;
      break;
    case GTK_POS_LEFT:
      struts [STRUT_LEFT] = strut;
      struts [STRUT_LEFT_START] = strut_start;
      struts [STRUT_LEFT_END] = strut_end;
      break;
  }

  gdk_error_trap_push ();
  XChangeProperty (display, window, net_wm_strut,
                   XA_CARDINAL, 32, PropModeReplace,
                   (guchar *) &struts, 4);
  XChangeProperty (display, window, net_wm_strut_partial,
                   XA_CARDINAL, 32, PropModeReplace,
                   (guchar *) &struts, 12);
  gdk_error_trap_pop ();
}

GdkRegion*
xutils_get_input_shape (GdkWindow *window)
{
  GdkRegion *region = gdk_region_new ();

  GdkRectangle rect;
  XRectangle *rects;
  int count = 0;
  int ordering = 0;

  gdk_error_trap_push ();
  rects = XShapeGetRectangles (GDK_WINDOW_XDISPLAY (window),
                               GDK_WINDOW_XID (window),
                               ShapeInput, &count, &ordering);
  gdk_error_trap_pop ();

  for (int i=0; i<count; ++i)
  {
    rect.x = rects[i].x;
    rect.y = rects[i].y;
    rect.width = rects[i].width;
    rect.height = rects[i].height;

    gdk_region_union_with_rect (region, &rect);
  }
  if (rects) free(rects);

  return region;
}

GdkWindow*
xutils_get_window_at_pointer (GdkDisplay *display)
{
  GdkWindow *result = NULL;
  Window root;
  Window child;
  int root_x;
  int root_y;
  int win_x;
  int win_y;
  unsigned int flags;

  Bool res = XQueryPointer (GDK_DISPLAY_XDISPLAY (display),
                            GDK_ROOT_WINDOW(), &root, &child,
                            &root_x, &root_y, &win_x, &win_y, &flags);
  if (!res) return NULL;

  result = gdk_window_lookup_for_display (display, child);
  if (!result && child != None)
    result = gdk_window_foreign_new (child);

  return result;
}

gboolean
xutils_is_window_minimized (GdkWindow *window)
{
  gboolean result = FALSE;

  g_return_val_if_fail (GDK_IS_WINDOW (window), FALSE);

  Atom type;
  int format;
  gulong nitems;
  gulong bytes_after;
  gulong *num;
  Atom wm_state = gdk_x11_atom_to_xatom (
      gdk_atom_intern_static_string ("WM_STATE"));

  gdk_error_trap_push ();
  type = None;
  XGetWindowProperty (GDK_WINDOW_XDISPLAY (window),
                      GDK_WINDOW_XWINDOW (window),
                      wm_state,
                      0,G_MAXLONG,
                      False, wm_state, &type, &format, &nitems,
                      &bytes_after, (void*)&num);

  if (gdk_error_trap_pop ())
  {
    return FALSE;
  }

  if (type != wm_state)
  {
    XFree (num);
    return FALSE;
  }

  result = *num == IconicState;

  XFree (num);

  return result;
}

