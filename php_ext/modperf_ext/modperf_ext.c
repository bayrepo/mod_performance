#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dlfcn.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "SAPI.h"
#include "php_modperf_ext.h"

ZEND_DECLARE_MODULE_GLOBALS(modperf_ext)

static int le_modperf_ext;

#ifndef PHP_FE_END
#define PHP_FE_END {NULL, NULL, NULL}
#endif

/* {{{ modperf_ext_functions[]
 *
 */
const zend_function_entry modperf_ext_functions[] = {
PHP_FE(modperf_info, NULL)
PHP_FE_END };
/* }}} */

/* {{{ modperf_ext_module_entry
 */
zend_module_entry modperf_ext_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
		STANDARD_MODULE_HEADER,
#endif
		"modperf_ext", modperf_ext_functions,
		PHP_MINIT(modperf_ext),
		PHP_MSHUTDOWN(modperf_ext),
		PHP_RINIT(modperf_ext),
		PHP_RSHUTDOWN(modperf_ext),
		PHP_MINFO(modperf_ext),
#if ZEND_MODULE_API_NO >= 20010901
		PHP_MODPERF_EXT_VERSION,
#endif
		STANDARD_MODULE_PROPERTIES };
/* }}} */

#ifdef COMPILE_DL_MODPERF_EXT
ZEND_GET_MODULE(modperf_ext)
#endif

/* {{{ PHP_INI
 */

PHP_INI_BEGIN() STD_PHP_INI_ENTRY("modperf_ext.enabled", "0", PHP_INI_SYSTEM, OnUpdateBool, gModPerfEnabled, zend_modperf_ext_globals, modperf_ext_globals)
PHP_INI_ENTRY("modperf_ext.socket", "/opt/performance/perfsock", PHP_INI_SYSTEM, NULL)
PHP_INI_ENTRY("modperf_ext.libpath", "libmodperformance.so", PHP_INI_SYSTEM, NULL)
PHP_INI_END()

/* }}} */

/* {{{ */

static inline char *_modperf_ext_fetch_global_var(char *name,
		int name_size TSRMLS_DC) {
	char *res = NULL;

#if PHP_MAJOR_VERSION < 7
	zval **tmp;
    zend_is_auto_global("_SERVER", sizeof("_SERVER") - 1 TSRMLS_CC);

	if (PG(http_globals)[TRACK_VARS_SERVER]
			&& zend_hash_find(HASH_OF(PG(http_globals)[TRACK_VARS_SERVER]),
					name, name_size, (void **) &tmp) != FAILURE &&
			Z_TYPE_PP(tmp) == IS_STRING && Z_STRLEN_PP(tmp) > 0) {
		res = estrdup(Z_STRVAL_PP(tmp));
	} else {
		res = estrdup("");
	}
#else
	zval *tmp;
	if ((Z_TYPE(PG(http_globals)[TRACK_VARS_SERVER]) == IS_ARRAY || zend_is_auto_global_str(ZEND_STRL("_SERVER"))) &&
				(tmp = zend_hash_str_find(Z_ARRVAL_P(&PG(http_globals)[TRACK_VARS_SERVER]), name, name_size)) == NULL
			) {
		res = estrdup("");
	} else {
		res = estrdup(Z_STRVAL_P(tmp));
	}
#endif
	return res;

}
/* }}} */

/* {{{ php_modperf_ext_init_globals
 */

static void php_modperf_ext_init_globals(
		zend_modperf_ext_globals *modperf_ext_globals) {
	modperf_ext_globals->gModPerfEnabled = 0;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(modperf_ext) {
	ZEND_INIT_MODULE_GLOBALS(modperf_ext, php_modperf_ext_init_globals, NULL);
	REGISTER_INI_ENTRIES();

	MODPERF_EXT_G(gModPerfFileDescr) = -1;
	MODPERF_EXT_G(gModPerfLibHandler) = NULL;
	MODPERF_EXT_G(modperformance_sendbegin_info) = NULL;
	MODPERF_EXT_G(modperformance_sendend_info) = NULL;

	if (MODPERF_EXT_G(gModPerfEnabled)) {

		MODPERF_EXT_G(gModPerfLibHandler) = dlopen(
				INI_STR("modperf_ext.libpath"), RTLD_LAZY);
		if (!MODPERF_EXT_G(gModPerfLibHandler)) {
			php_error(E_WARNING,
					"modperf_ext: Can't load libmodperformance.so");
		} else {
			MODPERF_EXT_G(modperformance_sendbegin_info) = dlsym(
					MODPERF_EXT_G(gModPerfLibHandler),
					"modperformance_sendbegin_info");
			if (dlerror() != NULL) {
				php_error(E_WARNING,
						"modperf_ext: Can't load modperformance_sendbegin_info");
				dlclose(MODPERF_EXT_G(gModPerfLibHandler));
				MODPERF_EXT_G(gModPerfLibHandler) = NULL;
				MODPERF_EXT_G(modperformance_sendbegin_info) = NULL;
				MODPERF_EXT_G(modperformance_sendend_info) = NULL;
			}

			MODPERF_EXT_G(modperformance_sendend_info) = dlsym(
					MODPERF_EXT_G(gModPerfLibHandler),
					"modperformance_sendend_info");
			if (dlerror() != NULL) {
				php_error(E_WARNING,
						"modperf_ext: Can't load modperformance_sendend_info");
				dlclose(MODPERF_EXT_G(gModPerfLibHandler));
				MODPERF_EXT_G(gModPerfLibHandler) = NULL;
				MODPERF_EXT_G(modperformance_sendbegin_info) = NULL;
				MODPERF_EXT_G(modperformance_sendend_info) = NULL;
			}
		}
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(modperf_ext) {
	if (MODPERF_EXT_G(gModPerfLibHandler)) {

		dlclose(MODPERF_EXT_G(gModPerfLibHandler));
		MODPERF_EXT_G(gModPerfLibHandler) = NULL;
		MODPERF_EXT_G(modperformance_sendbegin_info) = NULL;
		MODPERF_EXT_G(modperformance_sendend_info) = NULL;
	}

	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(modperf_ext) {

	if ((MODPERF_EXT_G(modperformance_sendbegin_info))
			&& (MODPERF_EXT_G(modperformance_sendend_info))
			&& (MODPERF_EXT_G(gModPerfEnabled))) {
		char *modperformance_hostname = _modperf_ext_fetch_global_var(
				"SERVER_NAME", sizeof("SERVER_NAME") TSRMLS_CC);
		if(!modperformance_hostname || !modperformance_hostname[0]) {
			return SUCCESS;
		}
		MODPERF_EXT_G(gModPerfFileDescr) =
		MODPERF_EXT_G(modperformance_sendbegin_info)(
				INI_STR("modperf_ext.socket"), //socket pathc
				SG(request_info).request_uri, //uri
				SG(request_info).path_translated, //file name
				modperformance_hostname,
				(char *) SG(request_info).request_method,
				SG(request_info).query_string);
	}

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(modperf_ext) {

	if ((MODPERF_EXT_G(modperformance_sendbegin_info))
			&& (MODPERF_EXT_G(modperformance_sendend_info))
			&& (MODPERF_EXT_G(gModPerfFileDescr) >= 0)) {
		MODPERF_EXT_G(modperformance_sendend_info)(
				MODPERF_EXT_G(gModPerfFileDescr));
	}
	MODPERF_EXT_G(gModPerfFileDescr) = -1;

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(modperf_ext) {
	php_info_print_table_start();
	php_info_print_table_header(2, "modperf_ext support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();

}
/* }}} */


/* {{{ proto string modperf_info()
 */
PHP_FUNCTION(modperf_info) {
	char *output_result = NULL;
	int len = 0;
	if (zend_parse_parameters_none() == FAILURE) {
		return;
	}

	len = spprintf(&output_result, 0, "<table>\n"
			"<tr>\n<th>Option name</th><th>Value</th></tr>\n"
			"<tr>\n<td>modperf_ext.socket</td><td>%s</td></tr>\n"
			"<tr>\n<td>modperf_ext.enabled</td><td>%d</td></tr>\n"
			"<tr>\n<td>modperf_ext.libpath</td><td>%s</td></tr>\n"
			"<tr>\n<td>STATUS</td><td>%d</td></tr>\n",
			INI_STR("modperf_ext.socket"), MODPERF_EXT_G(gModPerfEnabled),
			INI_STR("modperf_ext.libpath"), MODPERF_EXT_G(gModPerfFileDescr));
#if PHP_MAJOR_VERSION < 7
	RETURN_STRINGL(output_result, len, 0);
#else
	RETURN_STRINGL(output_result, len);
#endif

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
