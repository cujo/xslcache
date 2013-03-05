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

/* $Id: php_xsl.c 289012 2009-09-30 18:53:10Z indeyets $ */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_xsl.h"

/****************** JAKE *******************/
int le_stylesheet;

/* If you declare any globals in php_xsl.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(xsl)
*/
zend_class_entry *xsl_xsltcache_class_entry;
static zend_object_handlers xslcache_object_handlers;

/* {{{ xsl_functions[]
 *
 * Every user visible function must have an entry in xsl_functions[].
 */
zend_function_entry xslcache_functions[] = {
	{NULL, NULL, NULL}  /* Must be the last line in xsl_functions[] */
};
/* }}} */

static zend_module_dep xslcache_deps[] = {
	ZEND_MOD_REQUIRED("libxml")
	ZEND_MOD_REQUIRED("dom")
	{NULL, NULL, NULL}
};

/* {{{ xslcache_module_entry
 */
zend_module_entry xslcache_module_entry = {
#if ZEND_MODULE_API_NO >= 20050617
	STANDARD_MODULE_HEADER_EX, NULL,
	xslcache_deps,
#elif ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"xslcache",
	xslcache_functions,
	PHP_MINIT(xslcache),
	PHP_MSHUTDOWN(xslcache),
	PHP_RINIT(xslcache),
	PHP_RSHUTDOWN(xslcache),
	PHP_MINFO(xslcache),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_XSLCACHE_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_XSLCACHE
ZEND_GET_MODULE(xslcache)
#endif

extern int parse_stylesheet(persist_xsl_object *obj, const char *path);

void pdestroy_stylesheet_wrapper(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	persist_xsl_object *persist = (persist_xsl_object *) rsrc->ptr;

	if (persist->sheet_paths) {
		zend_hash_destroy(persist->sheet_paths);
		pefree(persist->sheet_paths, 1);
	}

	if (persist->sheetp) {
		/* free wrapper */

		//!!! FIXME?
		if (((xsltStylesheetPtr) persist->sheetp)->_private != NULL) {
			((xsltStylesheetPtr) persist->sheetp)->_private = NULL;
		}

		xsltFreeStylesheet((xsltStylesheetPtr) persist->sheetp);
		persist->sheetp = NULL;
	}

	pefree(persist->persist_key, 1);
	pefree(persist, 1);
}

int cached_sheet_stale(persist_xsl_object *xslp)
{
	char *key;
	uint keylen;
	struct stat statBuf;
	unsigned long idx;
	int rc;
	char *file;

	if (xslp == NULL) {
		//php_error(E_WARNING, "Null xsl_object passed into cached_sheet_stale");
		return 0;
	}

	for (zend_hash_internal_pointer_reset(xslp->sheet_paths);
		 	zend_hash_has_more_elements(xslp->sheet_paths) == SUCCESS;
		 	zend_hash_move_forward(xslp->sheet_paths))
	{
		rc = zend_hash_get_current_key_ex(xslp->sheet_paths, &key, &keylen, &idx, 0, NULL);

		if (rc != HASH_KEY_IS_STRING) {
			php_error(E_WARNING, "Key is not string in sheet_paths in cached_sheet_stale");
			continue;
		}

		if (keylen == 0) {
			php_error(E_WARNING, "Empty key in sheet_paths in cached_sheet_stale");
			continue;
		}

		file = estrndup(key, keylen);

		// key is a file path
		if (stat(file, &statBuf) == 0) {
			efree(file);
			if (statBuf.st_mtime > xslp->create_time) {
				zend_hash_internal_pointer_end_ex(xslp->sheet_paths, NULL);
				return 1;
			}
		} else {
			php_error(E_WARNING, "Stat failed on file: %s", file);
			efree(file);
			zend_hash_internal_pointer_end_ex(xslp->sheet_paths, NULL);
			return 1;
		}
	}

	return 0;
}

/* {{{ xsl_objects_free_storage */
void xslcache_objects_free_storage(void *object TSRMLS_DC)
{
	int i;
	xsl_object *intern = (xsl_object *)object;

	zend_object_std_dtor(&intern->std TSRMLS_CC);

	zend_hash_destroy(intern->parameter);
	FREE_HASHTABLE(intern->parameter);

	zend_hash_destroy(intern->registered_phpfunctions);
	FREE_HASHTABLE(intern->registered_phpfunctions);

	if (intern->node_list) {
		zend_hash_destroy(intern->node_list);
		FREE_HASHTABLE(intern->node_list);
	}

	if (intern->doc) {
		php_libxml_decrement_doc_ref(intern->doc TSRMLS_CC);
		efree(intern->doc);
		intern->doc = NULL;
	}

	if (intern->xslp && !intern->xslp->keep_cached) {
		/* delete the cached object, destructor called when object is removed */
		i = zend_hash_del(&EG(persistent_list), intern->xslp->persist_key, strlen(intern->xslp->persist_key));
	}

	efree(intern);
}
/* }}} */
/* {{{ xsl_objects_new */
zend_object_value xslcache_objects_new(zend_class_entry *class_type TSRMLS_DC)
{
	zend_object_value retval;
	xsl_object *intern;
	zval *tmp;

	intern = emalloc(sizeof(xsl_object));
	intern->xslp = NULL;
	intern->prop_handler = NULL;
	intern->parameter = NULL;
	intern->hasKeys = 0;
	intern->registerPhpFunctions = 0;
	intern->registered_phpfunctions = NULL;
	intern->doc = NULL;

	zend_object_std_init(&intern->std, class_type TSRMLS_CC);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(intern->std.properties, &class_type->default_properties, (copy_ctor_func_t) zval_add_ref, (void *) &tmp, sizeof(zval *));
#else
	object_properties_init(&intern->std, class_type);
#endif
	ALLOC_HASHTABLE(intern->parameter);
	zend_hash_init(intern->parameter, 0, NULL, ZVAL_PTR_DTOR, 0);
	ALLOC_HASHTABLE(intern->registered_phpfunctions);
	zend_hash_init(intern->registered_phpfunctions, 0, NULL, ZVAL_PTR_DTOR, 0);
	intern->node_list = NULL;
	retval.handle = zend_objects_store_put(intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, (zend_objects_free_object_storage_t) xslcache_objects_free_storage, NULL TSRMLS_CC);
	intern->handle = retval.handle;
	retval.handlers = &xslcache_object_handlers;
	return retval;
}
/* }}} */


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(xslcache)
{
	zend_class_entry ce;

	memcpy(&xslcache_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	xslcache_object_handlers.clone_obj = NULL;

	REGISTER_XSLCACHE_CLASS(ce, "XSLTCache", NULL, php_xslcache_xsltcache_class_functions, xsl_xsltcache_class_entry);
#if HAVE_XSL_EXSLT
	exsltRegisterAll();
#endif

	xsltRegisterExtModuleFunction(
		(const xmlChar *) "functionString",
		(const xmlChar *) "http://php.net/xsl",
		xsl_ext_function_string_php
	);
	xsltRegisterExtModuleFunction(
		(const xmlChar *) "function",
		(const xmlChar *) "http://php.net/xsl",
		xsl_ext_function_object_php
	);

//!!!	REGISTER_LONG_CONSTANT("XSL_CLONE_AUTO",      0,     CONST_CS | CONST_PERSISTENT);
//	REGISTER_LONG_CONSTANT("XSL_CLONE_NEVER",    -1,     CONST_CS | CONST_PERSISTENT);
//	REGISTER_LONG_CONSTANT("XSL_CLONE_ALWAYS",    1,     CONST_CS | CONST_PERSISTENT);

//	REGISTER_LONG_CONSTANT("LIBXSLT_VERSION",           LIBXSLT_VERSION,            CONST_CS | CONST_PERSISTENT);
//	REGISTER_STRING_CONSTANT("LIBXSLT_DOTTED_VERSION",  LIBXSLT_DOTTED_VERSION,     CONST_CS | CONST_PERSISTENT);

#if HAVE_XSL_EXSLT
//	REGISTER_LONG_CONSTANT("LIBEXSLT_VERSION",           LIBEXSLT_VERSION,            CONST_CS | CONST_PERSISTENT);
//	REGISTER_STRING_CONSTANT("LIBEXSLT_DOTTED_VERSION",  LIBEXSLT_DOTTED_VERSION,     CONST_CS | CONST_PERSISTENT);
#endif

	le_stylesheet = zend_register_list_destructors_ex(NULL, pdestroy_stylesheet_wrapper, "Cached Stylesheets", module_number);
	return SUCCESS;
}
/* }}} */

/* {{{ xslcache_object_get_data */
zval *xslcache_object_get_data(void *obj)
{
	zval *dom_wrapper;
	dom_wrapper = ((xsltStylesheetPtr) obj)->_private;
	return dom_wrapper;
}
/* }}} */

/* {{{ xsl_object_set_data */
static void xslcache_object_set_data(void *obj, zval *wrapper TSRMLS_DC)
{
//	((xsltStylesheetPtr) obj)->_private = wrapper;
}
/* }}} */

/* {{{ php_xslcache_set_object */
void php_xslcache_set_object(zval *wrapper, void *obj TSRMLS_DC)
{
	xsl_object *object;

	object = (xsl_object *)zend_objects_get_address(wrapper TSRMLS_CC);
//!!!FIXME	object->ptr = obj;
//	xslcache_object_set_data(obj, wrapper TSRMLS_CC);
}
/* }}} */

/* {{{ php_xslcache_create_object */
//!!!!!
zval *php_xslcache_create_object(xsltStylesheetPtr obj, int *found, zval *wrapper_in, zval *return_value  TSRMLS_DC)
{
	zval *wrapper = NULL;
	zend_class_entry *ce;

	*found = 0;

	//!!!! FORCE RETURN NULL FOR TESTING
	ZVAL_NULL(wrapper);
	return wrapper;

	if (!obj) {
		if(!wrapper_in) {
			ALLOC_ZVAL(wrapper);
		} else {
			wrapper = wrapper_in;
		}
		ZVAL_NULL(wrapper);
		return wrapper;
	}

	if ((wrapper = (zval *) xslcache_object_get_data((void *) obj))) {
		zval_add_ref(&wrapper);
		*found = 1;
		return wrapper;
	}

	if(!wrapper_in) {
		wrapper = return_value;
	} else {
		wrapper = wrapper_in;
	}


	ce = xsl_xsltcache_class_entry;

	if(!wrapper_in) {
		object_init_ex(wrapper, ce);
	}
	php_xslcache_set_object(wrapper, (void *) obj TSRMLS_CC);
	return (wrapper);
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(xslcache)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/

	xsltUnregisterExtModuleFunction ((const xmlChar *) "functionString",
				   (const xmlChar *) "http://php.net/xsl");
	xsltUnregisterExtModuleFunction ((const xmlChar *) "function",
				   (const xmlChar *) "http://php.net/xsl");

	xsltCleanupGlobals();

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(xslcache)
{
  xsltSetGenericErrorFunc(NULL, php_libxml_error_handler);
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(xslcache)
{
	xsltSetGenericErrorFunc(NULL, NULL);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(xslcache)
{
	php_info_print_table_start();
	{
		char buffer[128];
		int major, minor, subminor;

		php_info_print_table_row(2, "XSLCACHE", "enabled");
		major = xsltLibxsltVersion/10000;
		minor = (xsltLibxsltVersion - major * 10000) / 100;
		subminor = (xsltLibxsltVersion - major * 10000 - minor * 100);
		snprintf(buffer, 128, "%d.%d.%d", major, minor, subminor);
		php_info_print_table_row(2, "libxslt Version", buffer);
		major = xsltLibxmlVersion/10000;
		minor = (xsltLibxmlVersion - major * 10000) / 100;
		subminor = (xsltLibxmlVersion - major * 10000 - minor * 100);
		snprintf(buffer, 128, "%d.%d.%d", major, minor, subminor);
		php_info_print_table_row(2, "libxslt compiled against libxml Version", buffer);
	}
#if HAVE_XSL_EXSLT
		php_info_print_table_row(2, "EXSLT", "enabled");
		php_info_print_table_row(2, "libexslt Version", LIBEXSLT_DOTTED_VERSION);
#endif
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
