#include <libawn/awn-applet-simple.h>

AwnApplet*
awn_applet_factory_initp ( gchar* uid, gint orient, gint height )
{
	AwnApplet *applet;
  	
  applet = (AwnApplet*)awn_applet_simple_new ( uid, orient, height );
  awn_applet_simple_set_icon_name (AWN_APPLET_SIMPLE (applet), 
                                   "taskmanager", "gtk-apply");

  return applet;
}
