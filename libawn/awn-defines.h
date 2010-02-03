/*
 *  Copyright (C) 2007 Neil Jagdish Patel <njpatel@gmail.com>
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
 *  Author : Neil Jagdish Patel <njpatel@gmail.com>
*/

#ifndef __AWN_DEFINES_H__
#define __AWN_DEFINES_H__

/**
 * AWN_MAX_HEIGHT: 
 *
 *The maximum size of the panel
 */

#define AWN_MAX_HEIGHT 100

/**
 * AWN_MIN_HEIGHT: 
 *
 *The minimum size of the panel
 */
#define AWN_MIN_HEIGHT 12

typedef enum
{
  AWN_APPLET_FLAGS_NONE     = 0,
  AWN_APPLET_EXPAND_MINOR   = 1 << 0,
  AWN_APPLET_EXPAND_MAJOR   = 1 << 1,
  AWN_APPLET_IS_EXPANDER    = 1 << 2,
  AWN_APPLET_IS_SEPARATOR   = 1 << 3,
  AWN_APPLET_HAS_SHAPE_MASK = 1 << 4,

  AWN_APPLET_DOCKLET_HANDLES_POSITION_CHANGE = 1 << 10,
  AWN_APPLET_DOCKLET_CLOSE_ON_MOUSE_OUT = 1 << 11
} AwnAppletFlags;

/**
 * AwnAppletLicense:
 * @AWN_APPLET_LICENSE_GPLV2: GPL version 2 or later
 * @AWN_APPLET_LICENSE_GPLV3: GPL version 3 or later
 * @AWN_APPLET_LICENSE_LGPLV2_1: LGPL version 2.1 or later
 * @AWN_APPLET_LICENSE_LGPLV3: LGPL version 3 or later
 *
 * The license to use for the applet's about dialog.
 * Starting the acceptable values at 10 makes it rather unlikely
 * that someone can specify a license type by accident.
 */
typedef enum
{
  AWN_APPLET_LICENSE_GPLV2 = 10,
  AWN_APPLET_LICENSE_GPLV3 = 11,
  AWN_APPLET_LICENSE_LGPLV2_1 = 12,
  AWN_APPLET_LICENSE_LGPLV3 = 13
} AwnAppletLicense;

/**
 * AwnPathType:
 * @AWN_PATH_LINEAR: Standard (non-curved) panel layout.
 * @AWN_PATH_ELLIPSE: Elliptical (curved) panel layout.
 * @AWN_PATH_LAST: Placeholder value.
 *
 * Describes the layout of icons on the panel.
 */

typedef enum
{
	AWN_PATH_LINEAR=0,
	AWN_PATH_ELLIPSE,

	AWN_PATH_LAST

} AwnPathType;

/**
 * AWN_PANEL_ID_DEFAULT:
 *
 * The default panel ID.
 */
#define AWN_PANEL_ID_DEFAULT 1

/**
 * AWN_FONT_SIZE_EXTRA_SMALL: 
 *
 * Extra small font size. For use with #AwnOverlayText.
 * Standardized font sizes to be used with #AwnOverlayText.  Corresponds to 
 * standard Pango font size units for standard %PANGO_SCALE when the awn icon
 * size is 48.0 pixels.
 */

/**
 * AWN_FONT_SIZE_SMALL: 
 *
 * Small font size. For use with #AwnOverlayText.
 * Standardized font sizes to be used with #AwnOverlayText.  Corresponds to 
 * standard Pango font size units for standard %PANGO_SCALE when the awn icon
 * size is 48.0 pixels.
 */

/**
 * AWN_FONT_SIZE_MEDIUM: 
 *
 * Medium font size. For use with #AwnOverlayText.
 * Standardized font sizes to be used with #AwnOverlayText.  Corresponds to 
 * standard Pango font size units for standard %PANGO_SCALE when the awn icon
 * size is 48.0 pixels.
 */

/**
 * AWN_FONT_SIZE_LARGE: 
 *
 * Large font size. For use with #AwnOverlayText.
 * Standardized font sizes to be used with #AwnOverlayText.  Corresponds to 
 * standard Pango font size units for standard %PANGO_SCALE when the awn icon
 * size is 48.0 pixels.
 */

/**
 * AWN_FONT_SIZE_EXTRA_LARGE: 
 *
 * Extra large font size. For use with #AwnOverlayText.
 * Standardized font sizes to be used with #AwnOverlayText.  Corresponds to 
 * standard Pango font size units for standard %PANGO_SCALE when the awn icon
 * size is 48.0 pixels.
 */

#define AWN_FONT_SIZE_EXTRA_SMALL 6.0
#define AWN_FONT_SIZE_SMALL       9.0
#define AWN_FONT_SIZE_MEDIUM      12.0
#define AWN_FONT_SIZE_LARGE       15.0
#define AWN_FONT_SIZE_EXTRA_LARGE 18.0
#endif
