# -*- Mode: Python; py-indent-offset: 4 -*-

import sys, os
import getopt

# load the required modules:
import gobject as _gobject

ver = getattr(_gobject, 'pygobject_version', ())
if ver < (2, 11, 1):
  raise ImportError("PyGTK requires PyGObject 2.11.1 or higher, but %s was found" % (ver,))

from awn import *

uid = "0"
window = 0
orient = 0
height = 0

def init (argv):
  global uid
  global window
  global orient
  global height

  try: 
    opts, args = getopt.getopt (argv, "u:w:o:h:", 
                                ["uid=", "window=", "orient=", "height="])
  except getopt.GetoptError:
    print ("Unable to parse args")
    sys.exit (2)

  for opt, arg in opts:
    if opt in ("-u", "--uid"):
      uid = arg
      #print "uid = " + arg + " " + str (type (uid))
    elif opt in ("-w", "--window"):
      window = int (arg)
      #print "window = " + arg+ " " + str (type (window))
    elif opt in ("-o", "--orient"):
      orient = int (arg)
      #print "orient = " + arg + " " + str (type (orient))
    elif opt in ("-h", "--height"):
      height = int (arg)
      #print "height = " + arg + " " + str (type (height))

def init_applet (applet):
  global uid
  global orient
  global height
  global window
  plug = awn.Plug (applet)
  plug.add (applet)
  if (window):
    plug.construct (window)
  else:
    plug.construct (-1)
    plug.show_all ()
