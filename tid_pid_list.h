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
 * tid_pid_list.h
 *
 *  Created on: Oct 10, 2012
 *      Author: alexey
 */

#ifndef TID_PID_LIST_H_
#define TID_PID_LIST_H_

#include "send_info.h"

#define SEC2NANO 1000000000.0

typedef struct __performance_module_send_req_list{
	performance_module_send_req req;
	int fd;
} performance_module_send_req_list;

typedef struct __bad_tids_pid_tid_list_item{
	pid_t tid;
	apr_pool_t *p;
} bad_pid_tid_list_item;

typedef struct __remove_ietm_list_item{
	int fd;
} remove_ietm_list_item;

typedef struct __counters_pid_tid_list_item{
	double cpu_usage;
	double io_usager;
	double io_usagew;
	double mem_usage;
	double mem_usage_mb;
	performance_module_send_req data;
	apr_pool_t *p;
	double req_time;
	server_rec *srv;
} counters_pid_tid_list_item;

typedef struct __tids_tid_pid_list_item{
	performance_module_send_req data;
	int fd;
	apr_pool_t *p;
	double max_mem;
	double max_mem_mb;
	server_rec *srv;
} tids_tid_pid_list_item;

typedef  void (*func_T) (pid_t *, tids_tid_pid_list_item *, apr_pool_t *);

void remove_pid_tid_data_fd(int fd);
void add_new_pid_tid_data(performance_module_send_req *data, int fd,
		apr_pool_t *pool, server_rec *srv);
tids_tid_pid_list_item *get_tid_pid_data(pid_t tid);
void save_counters(tids_tid_pid_list_item *old,
		performance_module_send_req *new, apr_pool_t *pool);
void remove_counters();
void remove_tid_pid_data(pid_t tid);
void get_memory_info(pid_t *pid, tids_tid_pid_list_item *item, apr_pool_t *pool);
void proceed_tid_pid(func_T func, apr_pool_t *pool);
void prcd_function(apr_pool_t *p, double *old_tm, double new_tm);
void prcd_function2(apr_pool_t *p, double *old_tm, double new_tm);
void remove_tid_bad_list();
void remove_tid_bad_from_list();
void add_tid_to_bad_list(pid_t pid, apr_pool_t *pool);
void init_tid_pid(apr_pool_t *pool);
void destroy_tid_pid();
void debug_tid_pid();

void add_item_to_list(performance_module_send_req *item, int fd);
void clean_item_list(server_rec *srv, apr_pool_t *pool);
void add_item_to_removelist(int fd);

#endif /* TID_PID_LIST_H_ */
