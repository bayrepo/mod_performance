/*
 * client-example.c
 *
 *  Created on: Aug 5, 2014
 *      Author: Alexey Berezhok
 *		E-mail: alexey_com@ukr.net
 */

//THIS SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS," AND COPYRIGHT HOLDERS
//MAKE NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
//LIMITED TO, WARRANTIES OF MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE
//OR THAT THE USE OF THE SOFTWARE OR DOCUMENTATION WILL NOT INFRINGE ANY THIRD
//PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS.
//COPYRIGHT HOLDERS WILL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL OR
//CONSEQUENTIAL DAMAGES ARISING OUT OF ANY USE OF THE SOFTWARE OR DOCUMENTATION.
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

void *modperflib_handle = NULL;
int (*modperformance_sendbegin_info)(char *, char *, char *,
		char *, char *, char *) = NULL;
void (*modperformance_sendend_info)(int *) = NULL;

int main(int argc, char **argv) {

	char modperflib_error_buf[128];
	char *modperflib_error;
	int sd = -1;

	modperflib_handle = dlopen("libmodperformance.so", RTLD_LAZY);
	if (!modperflib_handle) {
		snprintf(modperflib_error_buf, 128, "%s", dlerror());
		printf("%s\n", modperflib_error_buf);
		exit(1);
	}

	modperformance_sendbegin_info = dlsym(modperflib_handle,
			"modperformance_sendbegin_info");
	if ((modperflib_error = dlerror()) != NULL) {
		snprintf(modperflib_error_buf, 128, "%s", dlerror());
		printf("%s\n", modperflib_error_buf);
		exit(1);
	}

	modperformance_sendend_info = dlsym(modperflib_handle,
			"modperformance_sendend_info");
	if ((modperflib_error = dlerror()) != NULL) {
		snprintf(modperflib_error_buf, 128, "%s", dlerror());
		printf("%s\n", modperflib_error_buf);
		exit(1);
	}

	if(modperformance_sendend_info && modperformance_sendbegin_info && getenv("MODPERFORMANCE_SOCKET")){
		sd = modperformance_sendbegin_info(getenv("MODPERFORMANCE_SOCKET"), "test_uri", "test_path", "test_hostname", "method", "args");
	}

	//Make load

	if(modperformance_sendend_info && modperformance_sendbegin_info && (sd>=0)){
		modperformance_sendend_info(NULL);
	}

	dlclose(modperflib_handle);
	return 0;
}
