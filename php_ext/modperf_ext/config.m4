dnl $Id$
dnl config.m4 for extension modperf_ext

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(modperf_ext, for modperf_ext support,
dnl Make sure that the comment is aligned:
dnl [  --with-modperf_ext             Include modperf_ext support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(modperf_ext, whether to enable modperf_ext support,
Make sure that the comment is aligned:
[  --enable-modperf_ext           Enable modperf_ext support])

if test "$PHP_MODPERF_EXT" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-modperf_ext -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/modperf_ext.h"  # you most likely want to change this
  dnl if test -r $PHP_MODPERF_EXT/$SEARCH_FOR; then # path given as parameter
  dnl   MODPERF_EXT_DIR=$PHP_MODPERF_EXT
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for modperf_ext files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       MODPERF_EXT_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$MODPERF_EXT_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the modperf_ext distribution])
  dnl fi

  dnl # --with-modperf_ext -> add include path
  dnl PHP_ADD_INCLUDE($MODPERF_EXT_DIR/include)

  dnl # --with-modperf_ext -> check for lib and symbol presence
  dnl LIBNAME=modperf_ext # you may want to change this
  dnl LIBSYMBOL=modperf_ext # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $MODPERF_EXT_DIR/lib, MODPERF_EXT_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_MODPERF_EXTLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong modperf_ext lib version or lib not found])
  dnl ],[
  dnl   -L$MODPERF_EXT_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(MODPERF_EXT_SHARED_LIBADD)

  PHP_NEW_EXTENSION(modperf_ext, modperf_ext.c, $ext_shared)
fi
