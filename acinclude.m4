dnl AC_PYTHON_DEVEL([version])
dnl 
dnl Originally copied from http://autoconf-archive.cryp.to/ac_python_devel.html
dnl (Original last modified 2007-07-31)
dnl 
dnl Copyright © 2007 Sebastian Huber <sebastian-huber@web.de>
dnl Copyright © 2007 Alan W. Irwin <irwin@beluga.phys.uvic.ca>
dnl Copyright © 2007 Rafael Laboissiere <rafael@laboissiere.net>
dnl Copyright © 2007 Andrew Collier <colliera@ukzn.ac.za>
dnl Copyright © 2007 Matteo Settenvini <matteo@member.fsf.org>
dnl Copyright © 2007 Horst Knorr <hk_classes@knoda.org>
dnl
dnl This program is free software: you can redistribute it and/or modify it
dnl under the terms of the GNU General Public License as published by the Free
dnl Software Foundation, either version 3 of the License, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful, but WITHOUT
dnl ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
dnl FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
dnl more details.
dnl
dnl You should have received a copy of the GNU General Public License along with
dnl this program. If not, see <http://www.gnu.org/licenses/>.
dnl
dnl As a special exception, the respective Autoconf Macro's copyright owner
dnl gives unlimited permission to copy, distribute and modify the configure
dnl scripts that are the output of Autoconf when processing the Macro. You need
dnl not follow the terms of the GNU General Public License when using or
dnl distributing such scripts, even though portions of the text of the Macro
dnl appear in them. The GNU General Public License (GPL) does govern all other
dnl use of the material that constitutes the Autoconf Macro.
dnl
dnl This special exception to the GPL applies to versions of the Autoconf Macro
dnl released by the Autoconf Macro Archive. When you make and distribute a
dnl modified version of the Autoconf Macro, you may extend this special
dnl exception to the GPL to apply to your modified version as well.
dnl
AC_DEFUN([AC_PYTHON_DEVEL],[
        #
        # Allow the use of a (user set) custom python version
        #
        AC_ARG_VAR([PYTHON_VERSION],[The installed Python
                version to use, for example '2.3'. This string
                will be appended to the Python interpreter
                canonical name.])

        AC_PATH_PROG([PYTHON],[python[$PYTHON_VERSION]])
        if test -z "$PYTHON"; then
           AC_MSG_ERROR([Cannot find python$PYTHON_VERSION in your system path])
           PYTHON_VERSION=""
        fi

        #
        # Check for a version of Python >= 2.1.0
        #
        AC_MSG_CHECKING([for a version of Python >= 2.1.0])
        ac_supports_python_ver=`$PYTHON -c "import sys, string; \
                ver = string.split(sys.version)[[0]]; \
                print ver >= '2.1.0'"`
        if test "$ac_supports_python_ver" != "True"; then
                if test -z "$PYTHON_NOVERSIONCHECK"; then
                        AC_MSG_RESULT([no])
                        AC_MSG_FAILURE([
This version of the AC@&t@_PYTHON_DEVEL macro
doesn't work properly with versions of Python before
2.1.0. You may need to re-run configure, setting the
variables PYTHON_CPPFLAGS, PYTHON_LDFLAGS, PYTHON_SITE_PKG,
PYTHON_EXTRA_LIBS and PYTHON_EXTRA_LDFLAGS by hand.
Moreover, to disable this check, set PYTHON_NOVERSIONCHECK
to something else than an empty string.
])
                else
                        AC_MSG_RESULT([skip at user request])
                fi
        else
                AC_MSG_RESULT([yes])
        fi

        #
        # if the macro parameter ``version'' is set, honour it
        #
        if test -n "$1"; then
                AC_MSG_CHECKING([for a version of Python >= $1])
                ac_supports_python_ver=`$PYTHON -c "import sys, string; \
                        ver = string.split(sys.version)[[0]]; \
                        print ver >= '$1'"`
                if test "$ac_supports_python_ver" = "True"; then
                   AC_MSG_RESULT([yes])
                else
                        AC_MSG_RESULT([no])
                        AC_MSG_ERROR([this package requires Python $1.
If you have it installed, but it isn't the default Python
interpreter in your system path, please pass the PYTHON_VERSION
variable to configure. See ``configure --help'' for reference.
])
                        PYTHON_VERSION=""
                fi
        fi

        #
        # Check if you have distutils, else fail
        #
        AC_MSG_CHECKING([for the distutils Python package])
        ac_distutils_result=`$PYTHON -c "import distutils" 2>&1`
        if test -z "$ac_distutils_result"; then
                AC_MSG_RESULT([yes])
        else
                AC_MSG_RESULT([no])
                AC_MSG_ERROR([cannot import Python module "distutils".
Please check your Python installation. The error was:
$ac_distutils_result])
                PYTHON_VERSION=""
        fi

        #
        # Check for Python include path
        #
        AC_MSG_CHECKING([for Python include path])
        if test -z "$PYTHON_CPPFLAGS"; then
                python_path=`$PYTHON -c "import distutils.sysconfig; \
                        print distutils.sysconfig.get_python_inc();"`
                if test -n "${python_path}"; then
                        python_path="-I$python_path"
                fi
                PYTHON_CPPFLAGS=$python_path
        fi
        AC_MSG_RESULT([$PYTHON_CPPFLAGS])
        AC_SUBST([PYTHON_CPPFLAGS])

        #
        # Check for Python library path
        #
        AC_MSG_CHECKING([for Python library path])
        if test -z "$PYTHON_LDFLAGS"; then
                # (makes two attempts to ensure we've got a version number
                # from the interpreter)
                py_version=`$PYTHON -c "from distutils.sysconfig import *; \
                        from string import join; \
                        print join(get_config_vars('VERSION'))"`
                if test "$py_version" == "[None]"; then
                        if test -n "$PYTHON_VERSION"; then
                                py_version=$PYTHON_VERSION
                        else
                                py_version=`$PYTHON -c "import sys; \
                                        print sys.version[[:3]]"`
                        fi
                fi

                PYTHON_LDFLAGS=`$PYTHON -c "from distutils.sysconfig import *; \
                        from string import join; \
                        print '-L' + get_python_lib(0,1), \
                        '-lpython';"`$py_version
        fi
        AC_MSG_RESULT([$PYTHON_LDFLAGS])
        AC_SUBST([PYTHON_LDFLAGS])

        #
        # Check for the site packages
        #
        AC_MSG_CHECKING([for Python site-packages path])
        AC_SUBST([PYTHON_EXEC_PREFIX], ['${exec_prefix}'])
        if test -z "$PYTHON_SITE_PKG"; then
                PYTHON_SITE_PKG=`$PYTHON -c "import distutils.sysconfig; \
                        print distutils.sysconfig.get_python_lib(0,0,prefix='${PYTHON_EXEC_PREFIX}');"`
        fi
        AC_MSG_RESULT([$PYTHON_SITE_PKG])
        AC_SUBST([PYTHON_SITE_PKG])

        #
        # libraries which must be linked in when embedding
        #
        AC_MSG_CHECKING(python extra libraries)
        if test -z "$PYTHON_EXTRA_LIBS"; then
           PYTHON_EXTRA_LIBS=`$PYTHON -c "import distutils.sysconfig; \
                conf = distutils.sysconfig.get_config_var; \
                print conf('LOCALMODLIBS'), conf('LIBS')"`
        fi
        AC_MSG_RESULT([$PYTHON_EXTRA_LIBS])
        AC_SUBST(PYTHON_EXTRA_LIBS)

        #
        # linking flags needed when embedding
        #
        AC_MSG_CHECKING(python extra linking flags)
        if test -z "$PYTHON_EXTRA_LDFLAGS"; then
                PYTHON_EXTRA_LDFLAGS=`$PYTHON -c "import distutils.sysconfig; \
                        conf = distutils.sysconfig.get_config_var; \
                        print conf('LINKFORSHARED')"`
        fi
        AC_MSG_RESULT([$PYTHON_EXTRA_LDFLAGS])
        AC_SUBST(PYTHON_EXTRA_LDFLAGS)

        #
        # final check to see if everything compiles alright
        #
        AC_MSG_CHECKING([consistency of all components of python development environment])
        AC_LANG_PUSH([C])
        # save current global flags
        LIBS="$ac_save_LIBS $PYTHON_LDFLAGS"
        CPPFLAGS="$ac_save_CPPFLAGS $PYTHON_CPPFLAGS"
        AC_TRY_LINK([
                #include <Python.h>
        ],[
                Py_Initialize();
        ],[pythonexists=yes],[pythonexists=no])

        AC_MSG_RESULT([$pythonexists])

        if test ! "$pythonexists" = "yes"; then
           AC_MSG_ERROR([
  Could not link test program to Python. Maybe the main Python library has been
  installed in some non-standard library path. If so, pass it to configure,
  via the LDFLAGS environment variable.
  Example: ./configure LDFLAGS="-L/usr/non-standard-path/python/lib"
  ============================================================================
   ERROR!
   You probably have to install the development version of the Python package
   for your distribution.  The exact name of this package varies among them.
  ============================================================================
           ])
          PYTHON_VERSION=""
        fi
        AC_LANG_POP
        # turn back to default flags
        CPPFLAGS="$ac_save_CPPFLAGS"
        LIBS="$ac_save_LIBS"

        #
        # all done!
        #
])

dnl
dnl JH_ADD_CFLAG(FLAG)
dnl checks whether the C compiler supports the given flag, and if so, adds
dnl it to $CFLAGS.  If the flag is already present in the list, then the
dnl check is not performed.
AC_DEFUN([JH_ADD_CFLAG],
[
case " $CFLAGS " in
*@<:@\	\ @:>@$1@<:@\	\ @:>@*)
  ;;
*)
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"
  AC_MSG_CHECKING([whether [$]CC understands $1])
  AC_TRY_COMPILE([], [], [jh_has_option=yes], [jh_has_option=no])
  AC_MSG_RESULT($jh_has_option)
  if test $jh_has_option = no; then
    CFLAGS="$save_CFLAGS"
  fi
  ;;
esac])
dnl AM_GCONF_SOURCE_2
dnl Defines GCONF_SCHEMA_CONFIG_SOURCE which is where you should install schemas
dnl  (i.e. pass to gconftool-2
dnl Defines GCONF_SCHEMA_FILE_DIR which is a filesystem directory where
dnl  you should install foo.schemas files
dnl

AC_DEFUN([AM_GCONF_SOURCE_2],
[
  if test "x$GCONF_SCHEMA_INSTALL_SOURCE" = "x"; then
    GCONF_SCHEMA_CONFIG_SOURCE=`gconftool-2 --get-default-source`
  else
    GCONF_SCHEMA_CONFIG_SOURCE=$GCONF_SCHEMA_INSTALL_SOURCE
  fi

  AC_ARG_WITH(gconf-source, 
  [  --with-gconf-source=sourceaddress      Config database for installing schema files.],GCONF_SCHEMA_CONFIG_SOURCE="$withval",)

  AC_SUBST(GCONF_SCHEMA_CONFIG_SOURCE)
  AC_MSG_RESULT([Using config source $GCONF_SCHEMA_CONFIG_SOURCE for schema installation])

  if test "x$GCONF_SCHEMA_FILE_DIR" = "x"; then
    GCONF_SCHEMA_FILE_DIR='$(sysconfdir)/gconf/schemas'
  fi

  AC_ARG_WITH(gconf-schema-file-dir, 
  [  --with-gconf-schema-file-dir=dir        Directory for installing schema files.],GCONF_SCHEMA_FILE_DIR="$withval",)

  AC_SUBST(GCONF_SCHEMA_FILE_DIR)
  AC_MSG_RESULT([Using $GCONF_SCHEMA_FILE_DIR as install directory for schema files])

  AC_ARG_ENABLE(schemas-install,
     [  --disable-schemas-install	Disable the schemas installation],
     [case ${enableval} in
       yes|no) ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-schemas-install) ;;
      esac])
  AM_CONDITIONAL([GCONF_SCHEMAS_INSTALL], [test "$enable_schemas_install" != no])
])
