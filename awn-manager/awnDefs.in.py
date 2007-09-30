#
#  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA.
#
#  Author: Neil Jagdish Patel <njpatel@gmail.com>
#
#  Notes: Avant Window Navigator preferences window

import sys, os

PREFIX = "@PREFIX@"

LOCALEDIR = os.path.join (PREFIX, "share", "locale")
PKGDATADIR = os.path.join (PREFIX, "share", "avant-window-navigator")

HOME_CONFIG_DIR = os.path.join(os.path.expanduser("~"), ".config", "awn")
HOME_APPLET_DIR = os.path.join(HOME_CONFIG_DIR, "applets")
HOME_LAUNCHERS_DIR = os.path.join(HOME_CONFIG_DIR, "launchers")
HOME_THEME_DIR = os.path.join(HOME_CONFIG_DIR, "themes")
HOME_CUSTOM_ICONS_DIR = os.path.join(HOME_CONFIG_DIR, "custom-icons")


# GCONF KEYS
AWN_PATH                = "/apps/avant-window-navigator"
AWN_AUTO_HIDE           = AWN_PATH + "/auto_hide"                   #bool
AWN_PANEL_MODE          = AWN_PATH + "/panel_mode"                  #bool
AWN_KEEP_BELOW          = AWN_PATH + "/keep_below"                  #bool

BAR_PATH                = AWN_PATH + "/bar"
BAR_ROUNDED_CORNERS     = BAR_PATH + "/rounded_corners"             #bool
BAR_CORNER_RADIUS       = BAR_PATH + "/corner_radius"               #float
BAR_RENDER_PATTERN      = BAR_PATH + "/render_pattern"              #bool
BAR_PATTERN_URI         = BAR_PATH + "/pattern_uri"                 #string
BAR_PATTERN_ALPHA       = BAR_PATH + "/pattern_alpha"               #float
BAR_GLASS_STEP_1        = BAR_PATH + "/glass_step_1"                #string
BAR_GLASS_STEP_2        = BAR_PATH + "/glass_step_2"                #string
BAR_GLASS_HISTEP_1      = BAR_PATH + "/glass_histep_1"              #string
BAR_GLASS_HISTEP_2      = BAR_PATH + "/glass_histep_2"              #string
BAR_BORDER_COLOR        = BAR_PATH + "/border_color"                #string
BAR_HILIGHT_COLOR       = BAR_PATH + "/hilight_color"               #string
BAR_SHOW_SEPARATOR      = BAR_PATH + "/show_separator"              #bool
BAR_SEP_COLOR           = BAR_PATH + "/sep_color"

BAR_HEIGHT              = BAR_PATH + "/bar_height"                  #int
BAR_ANGLE               = BAR_PATH + "/bar_angle"                   #int
BAR_ICON_OFFSET         = BAR_PATH + "/icon_offset"                 #int

WINMAN_PATH             = AWN_PATH + "/window_manager"
WINMAN_SHOW_ALL_WINS    = WINMAN_PATH + "/show_all_windows"         #bool
WINMAN_LAUNCHERS        = WINMAN_PATH + "/launchers"                #list

APP_PATH                = AWN_PATH + "/app"
APP_ACTIVE_PNG          = APP_PATH + "/active_png"                  #string
APP_ARROW_COLOR         = APP_PATH + "/arrow_color"                 #color
APP_TASKS_H_ARROWS      = APP_PATH + "/tasks_have_arrows"           #bool
APP_ARROW_OFFSET        = APP_PATH + "/arrow_offset"
APP_ICON_EFFECT         = APP_PATH + "/icon_effect"                 #int
APP_NAME_CHANGE_NOTIFY  = APP_PATH + "/name_change_notify"          #bool

TITLE_PATH              = AWN_PATH + "/title"
TITLE_TEXT_COLOR        = TITLE_PATH + "/text_color"                #color
TITLE_SHADOW_COLOR      = TITLE_PATH + "/shadow_color"              #color
TITLE_BACKGROUND        = TITLE_PATH + "/background"                #color
TITLE_FONT_FACE         = TITLE_PATH + "/font_face"                 #string

APPLETS_PATH            = AWN_PATH + "/applets"
APPLET_LIST             = AWN_PATH + "/applets_list"                #list

# i18n-related
I18N_DOMAIN = "avant-window-navigator"

def i18nize(symbol_table):
    import locale
    import gettext
    locale.setlocale(locale.LC_ALL, '')
    gettext.bindtextdomain(I18N_DOMAIN, LOCALEDIR)
    gettext.textdomain(I18N_DOMAIN)
    symbol_table['_'] = gettext.gettext
