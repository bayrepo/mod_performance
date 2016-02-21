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
 * tid_pid_list_ut.c
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
#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "unixd.h"
#include "ap_mpm.h"
#include "mod_log_config.h"

#include "http_protocol.h"

#include "apr_strings.h"
#include "apr_thread_proc.h"
#include "apr_signal.h"
#include "apr_general.h"
#include "apr_hash.h"
#include "apr_tables.h"
#include "apr_time.h"
#include "apr_file_io.h"
#include "apr_optional.h"

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

#include "debug.h"

#include "ut/utarray.h"
#include "ut/uthash.h"

#include "tid_pid_list_ut.h"

#if defined(__FreeBSD__)
#define utime process_cpu
#define stime system_cpu
#endif


glibtop_mem_own *get_global_mem();

void math_get_pcpu(double *dcpu, glibtop_cpu_own * current_cpu_beg,
		glibtop_proc_time_own * current_cpu_pid_beg, double current_tm_val_beg,
		glibtop_cpu_own * current_cpu_end,
		glibtop_proc_time_own * current_cpu_pid_end, double current_tm_val_end);
void math_get_io(double *dwrite, double *dread, iostat * old, iostat * new);
void math_get_mem(float *memrate, float *membytes, glibtop_mem_own * memory,
		glibtop_proc_mem_own * procmem);

void performance_module_save_to_db(double req_time, apr_pool_t *pool,
		server_rec *srv, server_rec *server,
		performance_module_send_req *req_begin, double dcpu, double dmem,
		double dmem_mb, double dwrite, double dread, double dt);

int get_use_tid();

long global_id_ut = 0;

#define SAVE_CURRENT_TIME(x) do{ \
		struct timespec cur_tm; \
		clock_gettime(CLOCK_REALTIME, &cur_tm); \
		x = (double)cur_tm.tv_sec	+ (double) cur_tm.tv_nsec / (double) SEC2NANO; \
	} while(0)

apr_thread_mutex_t *mutex_tid_ut = NULL;
apr_thread_mutex_t *mutex_counters_ut = NULL;
apr_thread_mutex_t *mutex_list_ut = NULL;
apr_thread_mutex_t *mutex_remove_list_ut = NULL;

tids_tid_pid_list_item_hh *tids_ut = NULL;
counters_pid_tid_list_item_hh *counters_ut = NULL;
UT_array *bad_tids_ut = NULL;

UT_array *list_1_ut = NULL;
UT_array *list_2_ut = NULL;

UT_array *remove_list_1_ut = NULL;
UT_array *remove_list_2_ut = NULL;

UT_icd bad_pid_tid_list_item_icd = { sizeof(bad_pid_tid_list_item), NULL, NULL,
		NULL };
UT_icd performance_module_send_req_list_icd = {
		sizeof(performance_module_send_req_list), NULL, NULL, NULL };
UT_icd remove_ietm_list_item_icd = { sizeof(remove_ietm_list_item), NULL, NULL,
		NULL };

void add_item_to_list_ut(performance_module_send_req *item, int fd) {
	apr_status_t rv;
	rv = apr_thread_mutex_trylock(mutex_list_ut);
	if (APR_STATUS_IS_EBUSY(rv)) {

		performance_module_send_req_list itm;
		itm.fd = fd;
		memcpy(&itm.req, item, sizeof(performance_module_send_req));
		utarray_push_back(list_2_ut, &itm);
	} else {
		performance_module_send_req_list itm;
		itm.fd = fd;
		memcpy(&itm.req, item, sizeof(performance_module_send_req));
		utarray_push_back(list_1_ut, &itm);
		apr_thread_mutex_unlock(mutex_list_ut);
	}
}

void add_item_to_removelist_ut(int fd) {
	apr_status_t rv;
	rv = apr_thread_mutex_trylock(mutex_remove_list_ut);
	if (APR_STATUS_IS_EBUSY(rv)) {
		remove_ietm_list_item itm;
		itm.fd = fd;
		utarray_push_back(remove_list_2_ut, &itm);
	} else {
		remove_ietm_list_item itm;
		itm.fd = fd;
		utarray_push_back(remove_list_1_ut, &itm);
		apr_thread_mutex_unlock(mutex_remove_list_ut);
	}
}

void add_item_to_removelist_tid_ut(int tid) {
	apr_status_t rv;
	tids_tid_pid_list_item_hh *item = get_tid_pid_data_ut(tid);
	if (item) {
		int fd = item->fd;
		rv = apr_thread_mutex_trylock(mutex_remove_list_ut);
		if (APR_STATUS_IS_EBUSY(rv)) {
			remove_ietm_list_item itm;
			itm.fd = fd;
			utarray_push_back(remove_list_2_ut, &itm);
		} else {
			remove_ietm_list_item itm;
			itm.fd = fd;
			utarray_push_back(remove_list_1_ut, &itm);
			apr_thread_mutex_unlock(mutex_remove_list_ut);
		}
	}
}

void proceed_list_need_ut(UT_array *lst, server_rec *srv, apr_pool_t *pool) {
	performance_module_send_req_list *s = NULL;

	while ((s = (performance_module_send_req_list*) utarray_next(lst,s))) {

		if (s) {
			if (s->req.command == 0) {
				write_debug_info(
						"Thread listen accept socket %d - Read command begin TID %d CPU %lld %s",
						s->fd, s->req.current_tid,
						s->req.cpu2.utime + s->req.cpu2.stime, s->req.uri);
				add_new_pid_tid_data_ut(&s->req, s->fd, pool, srv);
			} else {
				write_debug_info(
						"Thread listen accept socket %d - Read command end TID %d",
						s->fd, s->req.current_tid);
				tids_tid_pid_list_item_hh *tbl = get_tid_pid_data_ut(
						s->req.current_tid);
				if (tbl) {
					write_debug_info(
							"Thread listen accept socket %d - Read command end TID %d, get tid ok, CPU %lld",
							s->fd, s->req.current_tid,
							s->req.cpu2.utime + s->req.cpu2.stime);
					save_counters_ut(tbl, &s->req, pool);
					remove_tid_pid_data_ut(s->req.current_tid);
				}
			}
		}
	}
	utarray_clear(lst);
}

void proceed_remove_list_need_ut(UT_array *lst) {
	remove_ietm_list_item *s = NULL;
	while ((s = (remove_ietm_list_item*) utarray_next(lst,s))) {

		if (s) {
			remove_pid_tid_data_fd_ut(s->fd);
		}
	}
	utarray_clear(lst);
}

void clean_item_list_ut(server_rec *srv, apr_pool_t *pool) {
	apr_thread_mutex_lock(mutex_list_ut);
	proceed_list_need_ut(list_1_ut, srv, pool);
	apr_thread_mutex_unlock(mutex_list_ut);
	proceed_list_need_ut(list_2_ut, srv, pool);
	apr_thread_mutex_lock(mutex_remove_list_ut);
	proceed_remove_list_need_ut(remove_list_1_ut);
	apr_thread_mutex_unlock(mutex_remove_list_ut);
	proceed_remove_list_need_ut(remove_list_2_ut);
}

void remove_pid_tid_data_fd_ut(int fd) {
	tids_tid_pid_list_item_hh *s = NULL;
	apr_thread_mutex_lock(mutex_tid_ut);
	for (s = tids_ut; s != NULL;
			s = (tids_tid_pid_list_item_hh *) (s->hh.next)) {
		if (s->fd == fd) {
			HASH_DEL( tids_ut, s);
			free(s);
			break;
		}
	}
	apr_thread_mutex_unlock(mutex_tid_ut);
}

void add_new_pid_tid_data_ut(performance_module_send_req *data, int fd,
		apr_pool_t *pool, server_rec *srv) {
	pid_t tmp_key = data->current_tid;
	tids_tid_pid_list_item_hh *tmp = NULL;
	apr_thread_mutex_lock(mutex_tid_ut);
	HASH_FIND_INT(tids_ut, &tmp_key, tmp);
	if (tmp == NULL) {
		tmp = (tids_tid_pid_list_item_hh *) malloc(
				sizeof(tids_tid_pid_list_item_hh));
		memcpy(&tmp->data, data, sizeof(performance_module_send_req));
		tmp->tid_pid = (int) data->current_tid;
		tmp->fd = fd;
		tmp->p = pool;
		tmp->max_mem = 0;
		tmp->max_mem_mb = 0;
		tmp->srv = srv;
		HASH_ADD_INT( tids_ut, tid_pid, tmp);
	} else {
		if (tmp->fd)
			close(tmp->fd);
		memcpy(&tmp->data, data, sizeof(performance_module_send_req));
		tmp->fd = fd;
		tmp->max_mem = 0;
		tmp->max_mem_mb = 0;
	}
	apr_thread_mutex_unlock(mutex_tid_ut);
}

tids_tid_pid_list_item_hh *get_tid_pid_data_ut(pid_t tid) {
	pid_t tmp_key = tid;
	tids_tid_pid_list_item_hh *tmp = NULL;
	apr_thread_mutex_lock(mutex_tid_ut);
	HASH_FIND_INT(tids_ut, &tmp_key, tmp);
	apr_thread_mutex_unlock(mutex_tid_ut);
	return tmp;
}

#define DIFF_FIELD(x,y,z,t) x.t = y->t - z->data.t; \
							if (x.t<0) x.t = 0

#define ADD_FIELD(x,y,z) x->z+=y->z
#define GET_TRIPLE_MAX(x,y,z) (((x>y)?x:y)>z)?((x>y)?x:y):z

void save_counters_ut(tids_tid_pid_list_item_hh *old,
		performance_module_send_req *new, apr_pool_t *pool) {
	apr_thread_mutex_lock(mutex_counters_ut);
	global_id_ut++;
	double dcpu = 0.0;
	math_get_pcpu(&dcpu, &old->data.cpu1, &old->data.cpu2, old->data.time_start,
			&new->cpu1, &new->cpu2, new->time_start);
	double dwrite = 0.0, dread = 0.0;
	math_get_io(&dwrite, &dread, &old->data.io, &new->io);
	float max_mem1, max_mem11;
	float max_mem1_mb, max_mem11_mb;
	math_get_mem(&max_mem1, &max_mem1_mb, get_global_mem(), &old->data.mem2);
	math_get_mem(&max_mem11, &max_mem11_mb, get_global_mem(), &new->mem2);
	counters_pid_tid_list_item_hh *tmp = NULL;
	tmp = (counters_pid_tid_list_item_hh *) malloc(
			sizeof(counters_pid_tid_list_item_hh));
	tmp->counter.counter = global_id_ut;
	tmp->p = pool;
	tmp->cpu_usage = dcpu;
	tmp->mem_usage = GET_TRIPLE_MAX(old->max_mem, max_mem1, max_mem11);
	tmp->mem_usage_mb =
			GET_TRIPLE_MAX(old->max_mem_mb, max_mem1_mb, max_mem11_mb);
	tmp->io_usager = dread;
	tmp->io_usagew = dwrite;
	memcpy(&tmp->data, &old->data, sizeof(performance_module_send_req));
	tmp->req_time = new->time_start - old->data.time_start;
	tmp->srv = old->srv;
	write_debug_info(
			"Save counter info TID %d CPU %f MEM %f CPUB %lld CPUE %lld",
			tmp->data.current_tid, tmp->cpu_usage, tmp->mem_usage_mb,
			old->data.cpu2.stime + old->data.cpu2.utime,
			new->cpu2.stime + new->cpu2.utime);
	HASH_ADD(hh, counters_ut, counter, sizeof(counters_pid_tid_list_item_key),
			tmp);
	apr_thread_mutex_unlock(mutex_counters_ut);
}

void remove_counters_ut() {
	counters_pid_tid_list_item_hh *v = NULL, *tmp;
	write_debug_info("Remove counters begin-------------------------");
	apr_thread_mutex_lock(mutex_counters_ut);
	HASH_ITER(hh, counters_ut, v, tmp) {

		write_debug_info("Counter %ld Tid %d Req %f Cpu %f", v->counter.counter,
				v->data.current_tid, v->req_time, v->cpu_usage);

		//save counter
		performance_module_save_to_db(v->req_time, v->p, v->data.srv, v->srv,
				&v->data, v->cpu_usage, v->mem_usage, v->mem_usage_mb,
				v->io_usagew, v->io_usager, v->req_time);
		HASH_DEL(counters_ut, v);
		free(v);
	}

	apr_thread_mutex_unlock(mutex_counters_ut);
	write_debug_info("Remove counters end  -------------------------");
}

void remove_tid_pid_data_ut(pid_t tid) {
	tids_tid_pid_list_item_hh *s = NULL;
	apr_thread_mutex_lock(mutex_tid_ut);
	for (s = tids_ut; s != NULL;
			s = (tids_tid_pid_list_item_hh *) (s->hh.next)) {
		if (s->data.current_tid == tid) {
			HASH_DEL( tids_ut, s);
			free(s);
			break;
		}
	}
	apr_thread_mutex_unlock(mutex_tid_ut);
}

void get_memory_info_ut(pid_t *pid, tids_tid_pid_list_item_hh *item,
		apr_pool_t *pool) {
	float mem11, mem21;
	glibtop_proc_mem_own mem2;

	if ((glibtop_get_proc_mem_own_ret(&mem2, item->data.current_pid,
			get_use_tid() ? item->data.current_tid : (long) -1) < 0)) {
		add_tid_to_bad_list_ut(*pid, pool);
	} else {

		math_get_mem(&mem11, &mem21, get_global_mem(), &mem2);
		write_debug_info("Get memory info %f TID %d", mem21,
				item->data.current_tid);
		if (item->max_mem < mem11) {
			item->max_mem = mem11;
		}

		if (item->max_mem_mb < mem21) {
			item->max_mem_mb = mem21;
		}
	}
}

void proceed_tid_pid_ut(func_T_ut func, apr_pool_t *pool) {
	tids_tid_pid_list_item_hh *s = NULL;
	apr_thread_mutex_lock(mutex_tid_ut);
	for (s = tids_ut; s != NULL;
			s = (tids_tid_pid_list_item_hh *) (s->hh.next)) {
		func((pid_t *)&s->tid_pid, s, pool);
	}

	remove_tid_bad_from_list_ut();

	apr_thread_mutex_unlock(mutex_tid_ut);
	remove_tid_bad_list_ut();
}

void prcd_function_ut(apr_pool_t *p, double *old_tm, double new_tm) {
	if ((new_tm - *old_tm) > 0.1) {
		proceed_tid_pid_ut(get_memory_info_ut, p);
		*old_tm = new_tm;
	}
}

void prcd_function2_ut(apr_pool_t *p, double *old_tm1, double new_tm) {
	if ((new_tm - *old_tm1) > 1.0) {
		remove_counters_ut();
		*old_tm1 = new_tm;
	}
}

void remove_tid_bad_list_ut() {
	utarray_clear(bad_tids_ut);
}

void remove_tid_bad_from_list_ut() {
	bad_pid_tid_list_item *s = NULL;
	while ((s = (bad_pid_tid_list_item*) utarray_next(bad_tids_ut,s))) {
		if (s) {
			//remove_tid_pid_data(s->tid);
			add_item_to_removelist_tid_ut(s->tid);
		}
	}

}

void add_tid_to_bad_list_ut(pid_t pid, apr_pool_t *pool) {
	bad_pid_tid_list_item ptr;

	ptr.tid = pid;
	ptr.p = pool;
	utarray_push_back(bad_tids_ut, &ptr);

}

void init_tid_pid_ut(apr_pool_t *pool) {
	int rv = apr_thread_mutex_create(&mutex_tid_ut, APR_THREAD_MUTEX_DEFAULT,
			pool);
	if (rv != APR_SUCCESS)
		return;
	rv = apr_thread_mutex_create(&mutex_counters_ut, APR_THREAD_MUTEX_DEFAULT,
			pool);
	if (rv != APR_SUCCESS)
		return;
	rv = apr_thread_mutex_create(&mutex_list_ut, APR_THREAD_MUTEX_DEFAULT, pool);
	if (rv != APR_SUCCESS)
		return;
	rv = apr_thread_mutex_create(&mutex_remove_list_ut, APR_THREAD_MUTEX_DEFAULT,
			pool);
	if (rv != APR_SUCCESS)
		return;

	utarray_new(bad_tids_ut, &bad_pid_tid_list_item_icd);
	utarray_new(list_1_ut, &performance_module_send_req_list_icd);
	utarray_new(list_2_ut, &performance_module_send_req_list_icd);
	utarray_new(remove_list_1_ut, &remove_ietm_list_item_icd);
	utarray_new(remove_list_2_ut, &remove_ietm_list_item_icd);
}

void destroy_tid_pid_ut() {

	tids_tid_pid_list_item_hh *s1, *tmp1;
	counters_pid_tid_list_item_hh *s2, *tmp2;

	HASH_ITER(hh, tids_ut, s1, tmp1) {
		HASH_DEL(tids_ut, s1);
		free(s1);
	}

	HASH_ITER(hh, counters_ut, s2, tmp2) {
		HASH_DEL(counters_ut, s2);
		free(s2);
	}

	utarray_free(bad_tids_ut);
	utarray_free(list_1_ut);
	utarray_free(list_2_ut);
	utarray_free(remove_list_1_ut);
	utarray_free(remove_list_2_ut);
	apr_thread_mutex_destroy(mutex_tid_ut);
	apr_thread_mutex_destroy(mutex_counters_ut);
	apr_thread_mutex_destroy(mutex_list_ut);
	apr_thread_mutex_destroy(mutex_remove_list_ut);
}

void debug_tid_pid_ut() {
#ifdef DEBUG_MODULE_H
	write_debug_info("-----------------Head------------------------");
	apr_thread_mutex_lock(mutex_tid_ut);

	tids_tid_pid_list_item_hh *s = NULL;

	for (s = tids_ut; s != NULL;
			s = (tids_tid_pid_list_item_hh *) (s->hh.next)) {
		write_debug_info("TID %d URL %s Fd %d", s->tid_pid, s->data.hostname, s->fd);
	}

	apr_thread_mutex_unlock(mutex_tid_ut);
	write_debug_info("-----------------Tail------------------------");
#endif
}
