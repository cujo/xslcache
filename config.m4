dnl
dnl $Id: config.m4 287745 2009-08-26 13:59:16Z indeyets $
dnl

PHP_ARG_WITH(xslcache, for XSL support,
[  --with-xslcache[=DIR]        Include new XSL support (requires libxslt >= 1.1.0).
                          DIR is the libxslt install directory])

if test "$PHP_XSLCACHE" != "no"; then

  if test "$PHP_LIBXML" = "no"; then
    AC_MSG_ERROR([XSL extension requires LIBXML extension, add --enable-libxml])
  fi
  
  if test "$PHP_DOM" = "no"; then
    AC_MSG_ERROR([XSL extension requires DOM extension, add --enable-dom])
  fi

  for i in $PHP_XSLCACHE /usr/local /usr; do
    if test -x "$i/bin/xslt-config"; then
      XSLT_CONFIG=$i/bin/xslt-config
      break
    fi
  done

  if test -z "$XSLT_CONFIG"; then
    AC_MSG_ERROR([xslt-config not found. Please reinstall the libxslt >= 1.1.0 distribution])
  else
    libxslt_full_version=`$XSLT_CONFIG --version`
    ac_IFS=$IFS
    IFS="."
    set $libxslt_full_version
    IFS=$ac_IFS
    LIBXSLT_VERSION=`expr [$]1 \* 1000000 + [$]2 \* 1000 + [$]3`
    if test "$LIBXSLT_VERSION" -ge "1001000"; then
      XSL_LIBS=`$XSLT_CONFIG --libs`
      XSL_INCS=`$XSLT_CONFIG --cflags`
      PHP_EVAL_LIBLINE($XSL_LIBS, XSLCACHE_SHARED_LIBADD)
      PHP_EVAL_INCLINE($XSL_INCS)

      AC_MSG_CHECKING([for EXSLT support])
      for i in $PHP_XSLCACHE /usr/local /usr; do
        if test -r "$i/include/libexslt/exslt.h"; then
          PHP_XSLCACHE_EXSL_DIR=$i
          break
        fi
      done
      if test -z "$PHP_XSLCACHE_EXSL_DIR"; then
        AC_MSG_RESULT(not found)
      else 
        AC_MSG_RESULT(found)
        PHP_ADD_LIBRARY_WITH_PATH(exslt, $PHP_XSLCACHE_EXSL_DIR/$PHP_LIBDIR, XSLCACHE_SHARED_LIBADD)
        PHP_ADD_INCLUDE($PHP_XSLCACHE_EXSL_DIR/include)
        AC_DEFINE(HAVE_XSLCACHE_EXSLT,1,[ ])
      fi
    else
      AC_MSG_ERROR([libxslt version 1.1.0 or greater required.])
    fi
    
  
  fi
  
  AC_DEFINE(HAVE_XSLCACHE,1,[ ])
  PHP_NEW_EXTENSION(xslcache, php_xsl.c xsltcache.c, $ext_shared)
  PHP_SUBST(XSLCACHE_SHARED_LIBADD)
  PHP_ADD_EXTENSION_DEP(xslcache, libxml)
fi
