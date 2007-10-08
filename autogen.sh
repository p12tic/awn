#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="avant-window-navigator"

(test -f $srcdir/configure.in \
  && test -f $srcdir/autogen.sh) || {
    echo -n "**Error**: Directory \`$srcdir' does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

if which gnome-autogen.sh ; then
  REQUIRED_AUTOMAKE_VERSION=1.9 . gnome-autogen.sh
else
  if which xdt-autogen ; then
    . xdt-autogen
  else
    echo "No build script available.  You have two choices:"
    echo "1. You need to install the gnome-common module and make"
    echo "   sure the gnome-autogen.sh script is in your \$PATH."
    echo "2. You need to install the xfce4-dev-tools module and make"
    echo "   sure the xdt-autogen script is in your \$PATH."
    exit 1
  fi
fi

