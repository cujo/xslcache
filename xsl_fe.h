/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Christian Stocker <chregu@php.net>                           |
  +----------------------------------------------------------------------+
*/

/* $Id: xsl_fe.h 281492 2009-06-01 09:32:03Z indeyets $ */

#ifndef XSL_FE_H
#define XSL_FE_H

extern zend_function_entry php_xslcache_xsltcache_class_functions[];
extern zend_class_entry *xsl_xsltcache_class_entry;

PHP_FUNCTION(xsl_xsltcache_import_stylesheet);
PHP_FUNCTION(xsl_xsltcache_transform_to_doc);
PHP_FUNCTION(xsl_xsltcache_transform_to_uri);
PHP_FUNCTION(xsl_xsltcache_transform_to_xml);
PHP_FUNCTION(xsl_xsltcache_set_parameter);
PHP_FUNCTION(xsl_xsltcache_get_parameter);
PHP_FUNCTION(xsl_xsltcache_remove_parameter);
PHP_FUNCTION(xsl_xsltcache_has_exslt_support);
PHP_FUNCTION(xsl_xsltcache_register_php_functions);

#endif
