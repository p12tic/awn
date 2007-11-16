## this one is commonly used with AM_PATH_PYTHONDIR ...
dnl AM_CHECK_PYMOD(MODNAME [,SYMBOL [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]]])
dnl Check if a module containing a given symbol is visible to python.
AC_DEFUN([AM_CHECK_PYMOD],
[AC_REQUIRE([AM_PATH_PYTHON])
py_mod_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING(for ifelse([$2],[],,[$2 in ])python module $1)
AC_CACHE_VAL(py_cv_mod_$py_mod_var, [
ifelse([$2],[], [prog="
import sys
try:
        import $1
except ImportError:
        sys.exit(1)
except:
        sys.exit(0)
sys.exit(0)"], [prog="
import $1
$1.$2"])
if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
  then
    eval "py_cv_mod_$py_mod_var=yes"
  else
    eval "py_cv_mod_$py_mod_var=no"
  fi
])
py_val=`eval "echo \`echo '$py_cv_mod_'$py_mod_var\`"`
if test "x$py_val" != xno; then
  AC_MSG_RESULT(yes)
  ifelse([$3], [],, [$3
])dnl
else
  AC_MSG_RESULT(no)
  ifelse([$4], [],, [$4
])dnl
fi
])

dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for headers required to compile python extensions)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
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
