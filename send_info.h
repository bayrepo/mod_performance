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
 * send_info.h
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
#ifndef SEND_INFO_H_
#define SEND_INFO_H_

#ifdef _LIBBUILD
typedef void server_rec;
typedef void apr_pool_t;
#else
#include <httpd.h>
#include <apr_pools.h>
#include <apr_strings.h>
#endif

typedef struct {
	pid_t current_pid;
	pid_t current_tid;
	char script[256];
	char hostname[128];
	char uri[512];
	char req[256];
	double time_start;
	server_rec *srv;
	int command;
	glibtop_cpu_own cpu1;
	glibtop_proc_time_own cpu2;
	iostat io;
	glibtop_mem_own mem1;
	glibtop_proc_mem_own mem2;
} performance_module_send_req;

void modperformance_sendbegin_info_send_info(performance_module_send_req *req,
		char *uri, char *filename, char *hostname, char *method, char *args,
		char *canonical_filename, int use_tid, server_rec *server,
		apr_pool_t *pool, int performance_use_cononical_name);
void modperformance_sendend_info_send_info(performance_module_send_req *req,
		char *uri, char *hostname, char *method, char *args, int use_tid,
		server_rec *server, apr_pool_t *pool);

pid_t gettid(void);

#endif /* SEND_INFO_H_ */
