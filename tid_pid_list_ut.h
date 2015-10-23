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
 * tid_pid_list_ut.h
 *
 *  Created on: Jan 30, 2013
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


#ifndef TID_PID_LIST_UT_H_
#define TID_PID_LIST_UT_H_

#include "tid_pid_list.h"

#include "ut/uthash.h"

typedef struct __counters_pid_tid_list_item_key{
	long counter;
} counters_pid_tid_list_item_key;

typedef struct __counters_pid_tid_list_item_hh{
	counters_pid_tid_list_item_key counter;
	double cpu_usage;
	double io_usager;
	double io_usagew;
	double mem_usage;
	double mem_usage_mb;
	performance_module_send_req data;
	apr_pool_t *p;
	double req_time;
	server_rec *srv;
	UT_hash_handle hh;
} counters_pid_tid_list_item_hh;

typedef struct __tids_tid_pid_list_item_hh{
	int tid_pid;
	performance_module_send_req data;
	int fd;
	apr_pool_t *p;
	double max_mem;
	double max_mem_mb;
	server_rec *srv;
	UT_hash_handle hh;
} tids_tid_pid_list_item_hh;

typedef  void (*func_T_ut) (pid_t *, tids_tid_pid_list_item_hh *, apr_pool_t *);

void remove_pid_tid_data_fd_ut(int fd);
void add_new_pid_tid_data_ut(performance_module_send_req *data, int fd,
		apr_pool_t *pool, server_rec *srv);
tids_tid_pid_list_item_hh *get_tid_pid_data_ut(pid_t tid);
void save_counters_ut(tids_tid_pid_list_item_hh *old,
		performance_module_send_req *new, apr_pool_t *pool);
void remove_counters_ut();
void remove_tid_pid_data_ut(pid_t tid);
void get_memory_info_ut(pid_t *pid, tids_tid_pid_list_item_hh *item, apr_pool_t *pool);
void proceed_tid_pid_ut(func_T_ut func, apr_pool_t *pool);
void prcd_function_ut(apr_pool_t *p, double *old_tm, double new_tm);
void prcd_function2_ut(apr_pool_t *p, double *old_tm, double new_tm);
void remove_tid_bad_list_ut();
void remove_tid_bad_from_list_ut();
void add_tid_to_bad_list_ut(pid_t pid, apr_pool_t *pool);
void init_tid_pid_ut(apr_pool_t *pool);
void destroy_tid_pid_ut();
void debug_tid_pid_ut();

void add_item_to_list_ut(performance_module_send_req *item, int fd);
void clean_item_list_ut(server_rec *srv, apr_pool_t *pool);
void add_item_to_removelist_ut(int fd);


#endif /* TID_PID_LIST_UT_H_ */
