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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: php_xsl.h 289013 2009-09-30 19:01:52Z indeyets $ */

#ifndef PHP_XSL_H
#define PHP_XSL_H

extern zend_module_entry xslcache_module_entry;
#define phpext_xslcache_ptr &xslcache_module_entry

#define PHP_XSLCACHE_VERSION "0.7.1"

#ifdef PHP_WIN32
# define PHP_XSLCACHE_API __declspec(dllexport)
#else
# define PHP_XSLCACHE_API
#endif

#ifdef ZTS
# include "TSRM.h"
#endif

#include <libxslt/xsltconfig.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>
#include <libxslt/transform.h>
#if HAVE_XSL_EXSLT
# include <libexslt/exslt.h>
# include <libexslt/exsltconfig.h>
#endif

#include "ext/dom/xml_common.h"
#include "xsl_fe.h"

#include <libxslt/extensions.h>
#include <libxml/xpathInternals.h>

typedef struct _persist_xsl_object {
	char *persist_key;
	xsltStylesheetPtr sheetp;
	time_t create_time;
	time_t lastused_time;
	HashTable *sheet_paths;
	zend_bool keep_cached;
} persist_xsl_object;

typedef struct _xsl_object {
	zend_object  std;
	HashTable *prop_handler;
	HashTable *node_list;

	zend_object_handle handle;
	HashTable *parameter;
	int hasKeys;
	int registerPhpFunctions;
	HashTable *registered_phpfunctions;
	php_libxml_node_object *doc;

	persist_xsl_object *xslp;
} xsl_object;

void php_xslcache_set_object(zval *wrapper, void *obj TSRMLS_DC);
void xslcache_objects_free_storage(void *object TSRMLS_DC);
zval *php_xslcache_create_object(xsltStylesheetPtr obj, int *found, zval *wrapper_in, zval *return_value  TSRMLS_DC);

void xsl_ext_function_string_php(xmlXPathParserContextPtr ctxt, int nargs);
void xsl_ext_function_object_php(xmlXPathParserContextPtr ctxt, int nargs);

#define REGISTER_XSLCACHE_CLASS(ce, name, parent_ce, funcs, entry) \
	INIT_CLASS_ENTRY(ce, name, funcs); \
	ce.create_object = xslcache_objects_new; \
	entry = zend_register_internal_class_ex(&ce, parent_ce, NULL TSRMLS_CC);

#define XSL_DOMOBJ_NEW(zval, obj, ret) \
	if (NULL == (zval = php_xslcache_create_object(obj, ret, zval, return_value TSRMLS_CC))) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cannot create required DOM object"); \
		RETURN_FALSE; \
	}

PHP_MINIT_FUNCTION(xslcache);
PHP_MSHUTDOWN_FUNCTION(xslcache);
PHP_RINIT_FUNCTION(xslcache);
PHP_RSHUTDOWN_FUNCTION(xslcache);
PHP_MINFO_FUNCTION(xslcache);

#if 0
ZEND_BEGIN_MODULE_GLOBALS(xslcache)
ZEND_END_MODULE_GLOBALS(xslcache)
#endif

#ifdef ZTS
# define XSLCACHE_G(v) TSRMG(xslcache_globals_id, zend_xslcache_globals *, v)
#else
# define XSLCACHE_G(v) (xslcache_globals.v)
#endif
#endif	/* PHP_XSL_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
