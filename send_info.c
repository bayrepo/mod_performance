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
 * send_info.c
 *
 *  Created on: Aug 8, 2014
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

#define SEC2NANO 1000000000.0

#ifdef _LIBBUILD
#define apr_cpystrn strncpy
#endif

#include "send_info.h"

pid_t gettid(void) {
#if defined(linux)
  return syscall (__NR_gettid);
#endif

#if defined(__FreeBSD__)
  if (pthread_main_np () == 0)
    {
      long lwpid;
#if __FreeBSD_version < 900031
      thr_self (&lwpid);
#else
      lwpid = pthread_getthreadid_np ();
#endif
      return lwpid;
    }
  return (getpid ());
#endif

#if defined(__FreeBSD__)
  long lwpid;
#if __FreeBSD_version < 900031
  thr_self (&lwpid);
#else
  lwpid = pthread_getthreadid_np ();
#endif
  return lwpid;
#endif
}

void modperformance_sendbegin_info_send_info(performance_module_send_req *req,
		char *uri, char *filename, char *hostname, char *method, char *args,
		char *canonical_filename, int use_tid, server_rec *server,
		apr_pool_t *pool, int performance_use_cononical_name) {

	double time_start;
	struct timeval tm;
	gettimeofday(&tm, NULL);
	time_start = (double) tm.tv_sec + (double) tm.tv_usec / SEC2NANO;

	req->current_pid = getpid();
	req->time_start = time_start;
	req->current_tid = use_tid ? gettid() : getpid();
	req->srv = server;
	if (hostname)
		apr_cpystrn(req->hostname, hostname, sizeof(req->hostname));
	else
		apr_cpystrn(req->hostname, "", sizeof(req->hostname));
	if (uri)
		apr_cpystrn(req->uri, uri, sizeof(req->uri));
	else
		apr_cpystrn(req->uri, "", sizeof(req->uri));

	char *str = NULL;
#ifndef _LIBBUILD
	if(!pool){
#endif
	char str_b[512] = "";
	snprintf(str_b, 512, "%s:%s%s%s", method, req->uri,
			args ? "?" : "", args ? args : "");
	str = str_b;
#ifndef _LIBBUILD
	} else {
		str = apr_psprintf(pool, "%s:%s%s%s", method, req->uri,
			args ? "?" : "", args ? args : "");
	}
#endif
	apr_cpystrn(req->uri, str, sizeof(req->uri));


	if (performance_use_cononical_name) {
		if (canonical_filename)
			apr_cpystrn(req->script, canonical_filename, sizeof(req->script));
		else
			apr_cpystrn(req->script, "", sizeof(req->script));
	} else {
		if (filename)
			apr_cpystrn(req->script, filename, sizeof(req->script));
		else
			apr_cpystrn(req->script, "", sizeof(req->script));
	}

#if defined(linux)
	glibtop_get_cpu_own(&req->cpu1);
	glibtop_get_proc_time_own(&req->cpu2, req->current_pid,
			use_tid ? req->current_tid : (long) -1);
	//glibtop_get_mem_own(&req->mem1);
	glibtop_get_proc_mem_own(&req->mem2, req->current_pid,
			use_tid ? req->current_tid : (long) -1);

	get_io_stat_current(&req->io, req->current_pid,
			use_tid ? req->current_tid : (long) -1);
#endif
#if defined(__FreeBSD__)
	get_freebsd_cpu_info (NULL, &req->cpu1);
	get_freebsd_cpukvm_info (NULL, req->current_pid, &req->cpu2);
	get_freebsd_mem_info (NULL, req->current_pid, &req->mem2);
	get_freebsd_io_info (NULL, req->current_pid, &req->io);
#endif

	req->command = 0;
}

void modperformance_sendend_info_send_info(performance_module_send_req *req,
		char *uri, char *hostname, char *method, char *args, int use_tid,
		server_rec *server, apr_pool_t *pool) {
	double time_start2;
	struct timeval tm;
	gettimeofday(&tm, NULL);
	time_start2 = (double) tm.tv_sec + (double) tm.tv_usec / SEC2NANO;

	req->current_pid = getpid();
	req->time_start = time_start2;
	req->current_tid = use_tid ? gettid() : getpid();
	req->srv = server;

	if (hostname)
		apr_cpystrn(req->hostname, hostname, sizeof(req->hostname));
	else
		apr_cpystrn(req->hostname, "", sizeof(req->hostname));

	if (uri)
		apr_cpystrn(req->uri, uri, sizeof(req->uri));
	else
		apr_cpystrn(req->uri, "", sizeof(req->uri));

	char *str = NULL;
#ifndef _LIBBUILD
	if(!pool){
#endif
		char str_b[512] = "";
		snprintf(str_b, 512, "%s:%s%s%s", method, req->uri,
			args ? "?" : "", args ? args : "");
		str = str_b;
#ifndef _LIBBUILD
	} else {

		str = apr_psprintf(pool, "%s:%s%s%s", method, req->uri,
			args ? "?" : "", args ? args : "");
	}
#endif
	apr_cpystrn(req->uri, str, sizeof(req->uri));

	apr_cpystrn(req->script, "", sizeof(req->script));

#if defined(linux)
	glibtop_get_cpu_own(&req->cpu1);
	glibtop_get_proc_time_own(&req->cpu2, req->current_pid,
			use_tid ? req->current_tid : (long) -1);
	//glibtop_get_mem_own(&req->mem1);
	glibtop_get_proc_mem_own(&req->mem2, req->current_pid,
			use_tid ? req->current_tid : (long) -1);

	get_io_stat_current(&req->io, req->current_pid,
			use_tid ? req->current_tid : (long) -1);
#endif
#if defined(__FreeBSD__)
	get_freebsd_cpu_info (NULL, &req->cpu1);
	get_freebsd_cpukvm_info (NULL, req->current_pid, &req->cpu2);
	get_freebsd_mem_info (NULL, req->current_pid, &req->mem2);
	get_freebsd_io_info (NULL, req->current_pid, &req->io);
#endif

	req->command = 1;
}
