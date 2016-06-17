/*
 * Copyright 2012 Alexey Berezhok (alexey_com@ukr.net, bayrepo.info@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * lib-functions.c
 *
 *  Created on: Aug 4, 2014
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <pthread.h>
#if defined(linux)
#include <linux/unistd.h>
#endif
#if defined(__FreeBSD__)
#include <unistd.h>
#include <string.h>
#endif
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <asm/param.h>
#include <poll.h>

#if defined(linux)
#include "iostat.h"
#endif
#if defined(__FreeBSD__)
#include "freebsd_getsysinfo.h"
#endif

#include "lib-functions.h"
#include "send_info.h"

static int performance_send_data_to_inner(int fd, const void *buf,
		size_t buf_size) {
	int rc;

	do {
		rc = write(fd, buf, buf_size);
	} while (rc < 0 && errno == EINTR);
	if (rc < 0) {
		return errno;
	}

	return 0;
}

static int connect_to_daemon_inner(int * sdptr, char *socket_path) {
	struct sockaddr_un unix_addr;
	int sd;

	memset(&unix_addr, 0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	strncpy(unix_addr.sun_path, socket_path, (sizeof unix_addr.sun_path)-1);
	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		return -1;
	if (connect(sd, (struct sockaddr *) &unix_addr, sizeof(unix_addr)) < 0) {
		close(sd);
		return -1;
	}
	*sdptr = sd;
	return 0;
}

int modperformance_sendbegin_info(char *socket_path, char *uri, char *path,
		char *hostname, char *method, char *args) {
	int global_sd = -1;

	if (global_sd < 0) {
		if (connect_to_daemon_inner(&global_sd, socket_path) != 0) {
			global_sd = -1;
			return -1;
		}
	}

	performance_module_send_req *req = malloc(
			sizeof(performance_module_send_req));

	if (!req) {
		close(global_sd);
		return -1;
	}

	modperformance_sendbegin_info_send_info(req,
			uri, path, hostname, method, args,
			path, 0, NULL,
			NULL, 0);

	performance_send_data_to_inner(global_sd, (const void *) req,
			sizeof(performance_module_send_req));
	free(req);
	return global_sd;
}

void modperformance_sendend_info(int modperformance_sd) {

	if (modperformance_sd) {
		//double time_start2;
		//struct timeval tm;
		//gettimeofday(&tm, NULL);
		//time_start2 = (double) tm.tv_sec + (double) tm.tv_usec / 1000000.0;

		performance_module_send_req *req = malloc(
				sizeof(performance_module_send_req));

		if (req) {

			modperformance_sendend_info_send_info(req,
					"", "", "", "", 0,
					NULL, NULL);

			performance_send_data_to_inner(modperformance_sd, (const void *) req,
					sizeof(performance_module_send_req));
			free(req);
		}
		shutdown(modperformance_sd, SHUT_RDWR);
		close(modperformance_sd);

	}
}

