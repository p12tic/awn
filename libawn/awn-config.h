/*
 * Copyright (C) 2009 Mark Lee <avant-wn@lazymalevolence.com>
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
 * Author : Mark Lee <avant-wn@lazymalevolence.com>
 */

#ifndef __LIBAWN_AWN_CONFIG_H__
#define __LIBAWN_AWN_CONFIG_H__

#include <glib.h>
#include <libdesktop-agnostic/config.h>
#include "awn-applet.h"

G_BEGIN_DECLS

DesktopAgnosticConfigClient* awn_config_get_default                    (gint panel_id, GError **error);
DesktopAgnosticConfigClient* awn_config_get_default_for_applet         (AwnApplet *applet, GError **error);
DesktopAgnosticConfigClient* awn_config_get_default_for_applet_by_info (const gchar *name, const gchar *uid, GError **error);
void                         awn_config_free                           (void);

G_END_DECLS

#endif
