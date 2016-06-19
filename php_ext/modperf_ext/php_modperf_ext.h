
#ifndef PHP_MODPERF_EXT_H
#define PHP_MODPERF_EXT_H

extern zend_module_entry modperf_ext_module_entry;
#define phpext_modperf_ext_ptr &modperf_ext_module_entry

#define PHP_MODPERF_EXT_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_MODPERF_EXT_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_MODPERF_EXT_API __attribute__ ((visibility("default")))
#else
#	define PHP_MODPERF_EXT_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(modperf_ext);
PHP_MSHUTDOWN_FUNCTION(modperf_ext);
PHP_RINIT_FUNCTION(modperf_ext);
PHP_RSHUTDOWN_FUNCTION(modperf_ext);
PHP_MINFO_FUNCTION(modperf_ext);

PHP_FUNCTION(modperf_info);

ZEND_BEGIN_MODULE_GLOBALS(modperf_ext)
	int gModPerfFileDescr;
	zend_bool gModPerfEnabled;
	void *gModPerfLibHandler;
	int (*modperformance_sendbegin_info)(char *, char *, char *, char *, char *, char *);
	void (*modperformance_sendend_info)(int);
ZEND_END_MODULE_GLOBALS(modperf_ext)


#ifdef ZTS
#define MODPERF_EXT_G(v) TSRMG(modperf_ext_globals_id, zend_modperf_ext_globals *, v)
#else
#define MODPERF_EXT_G(v) (modperf_ext_globals.v)
#endif

#endif	/* PHP_MODPERF_EXT_H */

