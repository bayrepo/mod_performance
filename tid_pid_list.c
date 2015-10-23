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
 * tid_pid_list.c
 *
 *  Created on: Oct 10, 2012
 *      Author: alexey
 */

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

#include "tid_pid_list.h"

#include "debug.h"

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

long global_id = 0;

#define my_alloc(x, pool, size) x = malloc(size); \
				apr_pool_cleanup_register(pool, x, apr_pool_cleanup_null,   apr_pool_cleanup_null)

#define my_free(x, pool) apr_pool_cleanup_run(pool, x, my_apr_free)

#define SAVE_CURRENT_TIME(x) do{ \
		struct timespec cur_tm; \
		clock_gettime(CLOCK_REALTIME, &cur_tm); \
		x = (double)cur_tm.tv_sec	+ (double) cur_tm.tv_nsec / (double) SEC2NANO; \
	} while(0)

apr_status_t my_apr_free(void *data) {
	free(data);
	return APR_SUCCESS;
}

apr_thread_mutex_t *mutex_tid = NULL;
apr_thread_mutex_t *mutex_counters = NULL;
apr_thread_mutex_t *mutex_list = NULL;
apr_thread_mutex_t *mutex_remove_list = NULL;

apr_hash_t *tids = NULL;
apr_hash_t *counters = NULL;
apr_array_header_t *bad_tids = NULL;

apr_array_header_t *list_1 = NULL;
apr_array_header_t *list_2 = NULL;

apr_array_header_t *remove_list_1 = NULL;
apr_array_header_t *remove_list_2 = NULL;

void add_item_to_list(performance_module_send_req *item, int fd) {
	apr_status_t rv;
	rv = apr_thread_mutex_trylock(mutex_list);
	if (APR_STATUS_IS_EBUSY(rv)) {

		performance_module_send_req_list *itm = apr_palloc(list_2->pool,
				sizeof(performance_module_send_req_list));
		if (itm) {
			itm->fd = fd;
			memcpy(&itm->req, item, sizeof(performance_module_send_req));
			*(performance_module_send_req_list **) apr_array_push(list_2) = itm;
		}
	} else {
		performance_module_send_req_list *itm = apr_palloc(list_1->pool,
				sizeof(performance_module_send_req_list));
		if (itm) {
			itm->fd = fd;
			memcpy(&itm->req, item, sizeof(performance_module_send_req));
			*(performance_module_send_req_list **) apr_array_push(list_1) = itm;
		}
		apr_thread_mutex_unlock(mutex_list);
	}
}

void add_item_to_removelist(int fd) {
	apr_status_t rv;
	rv = apr_thread_mutex_trylock(mutex_remove_list);
	if (APR_STATUS_IS_EBUSY(rv)) {

		remove_ietm_list_item *itm = apr_palloc(remove_list_2->pool,
				sizeof(remove_ietm_list_item));
		if (itm) {
			itm->fd = fd;
			*(remove_ietm_list_item **) apr_array_push(remove_list_2) = itm;
		}
	} else {
		remove_ietm_list_item *itm = apr_palloc(remove_list_1->pool,
				sizeof(remove_ietm_list_item));
		if (itm) {
			itm->fd = fd;
			*(remove_ietm_list_item **) apr_array_push(remove_list_1) = itm;
		}
		apr_thread_mutex_unlock(mutex_remove_list);
	}
}

void add_item_to_removelist_tid(int tid) {
	apr_status_t rv;
	tids_tid_pid_list_item *item = get_tid_pid_data(tid);
	if (item) {
		int fd = item->fd;
		rv = apr_thread_mutex_trylock(mutex_remove_list);
		if (APR_STATUS_IS_EBUSY(rv)) {

			remove_ietm_list_item *itm = apr_palloc(remove_list_2->pool,
					sizeof(remove_ietm_list_item));
			if (itm) {
				itm->fd = fd;
				*(remove_ietm_list_item **) apr_array_push(remove_list_2) = itm;
			}
		} else {
			remove_ietm_list_item *itm = apr_palloc(remove_list_1->pool,
					sizeof(remove_ietm_list_item));
			if (itm) {
				itm->fd = fd;
				*(remove_ietm_list_item **) apr_array_push(remove_list_1) = itm;
			}
			apr_thread_mutex_unlock(mutex_remove_list);
		}
	}
}

void proceed_list_need(apr_array_header_t *lst, server_rec *srv,
		apr_pool_t *pool) {
	int i = 0;

	for (i = 0; i < lst->nelts; i++) {
		performance_module_send_req_list *s =
				((performance_module_send_req_list**) lst->elts)[i];
		if (s) {
			if (s->req.command == 0) {
				write_debug_info(
						"Thread listen accept socket %d - Read command begin TID %d CPU %lld %s",
						s->fd, s->req.current_tid,
						s->req.cpu2.utime + s->req.cpu2.stime, s->req.uri);
				add_new_pid_tid_data(&s->req, s->fd, pool, srv);
			} else {
				write_debug_info(
						"Thread listen accept socket %d - Read command end TID %d",
						s->fd, s->req.current_tid);
				tids_tid_pid_list_item *tbl = get_tid_pid_data(
						s->req.current_tid);
				if (tbl) {
					write_debug_info(
							"Thread listen accept socket %d - Read command end TID %d, get tid ok, CPU %lld",
							s->fd, s->req.current_tid,
							s->req.cpu2.utime + s->req.cpu2.stime);
					save_counters(tbl, &s->req, pool);
					remove_tid_pid_data(s->req.current_tid);
				}
			}
		}
		apr_array_pop(lst);
	}
}

void proceed_remove_list_need(apr_array_header_t *lst) {
	int i = 0;

	for (i = 0; i < lst->nelts; i++) {
		remove_ietm_list_item *s = ((remove_ietm_list_item**) lst->elts)[i];
		if (s) {
			remove_pid_tid_data_fd(s->fd);
		}
		apr_array_pop(lst);
	}
}

void clean_item_list(server_rec *srv, apr_pool_t *pool) {
	apr_thread_mutex_lock(mutex_list);
	proceed_list_need(list_1, srv, pool);
	apr_thread_mutex_unlock(mutex_list);
	proceed_list_need(list_2, srv, pool);
	apr_thread_mutex_lock(mutex_remove_list);
	proceed_remove_list_need(remove_list_1);
	apr_thread_mutex_unlock(mutex_remove_list);
	proceed_remove_list_need(remove_list_2);
}

void remove_pid_tid_data_fd(int fd) {
	apr_hash_index_t *hi;
	apr_thread_mutex_lock(mutex_tid);
	for (hi = apr_hash_first(NULL, tids); hi; hi = apr_hash_next(hi)) {
		pid_t *k;
		tids_tid_pid_list_item *v;

		apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
		if (v->fd == fd) {
			apr_hash_set(tids, (void *) k, sizeof(pid_t), NULL);
			//my_free(k, v->p);
			//my_free(v, v->p);
			break;
		}
	}
	apr_thread_mutex_unlock(mutex_tid);
}

void add_new_pid_tid_data(performance_module_send_req *data, int fd,
		apr_pool_t *pool, server_rec *srv) {
	pid_t tmp_key = data->current_tid;
	tids_tid_pid_list_item *tmp = NULL;
	apr_thread_mutex_lock(mutex_tid);
	tmp = (tids_tid_pid_list_item *) apr_hash_get(tids, &tmp_key,
			sizeof(pid_t));
	if (tmp == NULL) {
		my_alloc(tmp, pool, sizeof(tids_tid_pid_list_item));
		memcpy(&tmp->data, data, sizeof(performance_module_send_req));
		pid_t *kkey = NULL;
		my_alloc(kkey, pool, sizeof(pid_t));
		*kkey = data->current_tid;
		tmp->fd = fd;
		tmp->p = pool;
		tmp->max_mem = 0;
		tmp->max_mem_mb = 0;
		tmp->srv = srv;
		apr_hash_set(tids, kkey, sizeof(pid_t), tmp);
	} else {
		if (tmp->fd)
			close(tmp->fd);
		memcpy(&tmp->data, data, sizeof(performance_module_send_req));
		tmp->fd = fd;
		tmp->max_mem = 0;
		tmp->max_mem_mb = 0;
	}
	apr_thread_mutex_unlock(mutex_tid);
}

tids_tid_pid_list_item *get_tid_pid_data(pid_t tid) {
	pid_t tmp_key = tid;
	tids_tid_pid_list_item *tmp = NULL;
	apr_thread_mutex_lock(mutex_tid);
	tmp = (tids_tid_pid_list_item *) apr_hash_get(tids, &tmp_key,
			sizeof(pid_t));
	apr_thread_mutex_unlock(mutex_tid);
	return tmp;
}

#define DIFF_FIELD(x,y,z,t) x.t = y->t - z->data.t; \
							if (x.t<0) x.t = 0

#define ADD_FIELD(x,y,z) x->z+=y->z
#define GET_TRIPLE_MAX(x,y,z) (((x>y)?x:y)>z)?((x>y)?x:y):z

void save_counters(tids_tid_pid_list_item *old,
		performance_module_send_req *new, apr_pool_t *pool) {
	apr_thread_mutex_lock(mutex_counters);
	global_id++;
	double dcpu = 0.0;
	math_get_pcpu(&dcpu, &old->data.cpu1, &old->data.cpu2, old->data.time_start,
			&new->cpu1, &new->cpu2, new->time_start);
	double dwrite = 0.0, dread = 0.0;
	math_get_io(&dwrite, &dread, &old->data.io, &new->io);
	float max_mem1, max_mem11;
	float max_mem1_mb, max_mem11_mb;
	math_get_mem(&max_mem1, &max_mem1_mb, get_global_mem(), &old->data.mem2);
	math_get_mem(&max_mem11, &max_mem11_mb, get_global_mem(), &new->mem2);
	counters_pid_tid_list_item *tmp = NULL;
	my_alloc(tmp, pool, sizeof(counters_pid_tid_list_item));
	long *tkey = NULL;
	my_alloc(tkey, pool, sizeof(long));
	*tkey = global_id;
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
	apr_hash_set(counters, tkey, sizeof(long), tmp);
	apr_thread_mutex_unlock(mutex_counters);
}

void remove_counters() {
	apr_hash_index_t *hi;
	write_debug_info("Remove counters begin-------------------------");
	apr_thread_mutex_lock(mutex_counters);
	for (hi = apr_hash_first(NULL, counters); hi; hi = apr_hash_next(hi)) {
		long *k;
		counters_pid_tid_list_item *v;

		apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
		write_debug_info("Counter %ld Tid %d Req %f Cpu %f", *k,
				v->data.current_tid, v->req_time, v->cpu_usage);

		//save counter
		performance_module_save_to_db(v->req_time, v->p, v->data.srv, v->srv,
				&v->data, v->cpu_usage, v->mem_usage, v->mem_usage_mb,
				v->io_usagew, v->io_usager, v->req_time);

		apr_hash_set(counters, (void *) k, sizeof(long), NULL);
		//my_free(k, v->p);
		//my_free(v, v->p);

	}
	apr_thread_mutex_unlock(mutex_counters);
	write_debug_info("Remove counters end  -------------------------");
}

void remove_tid_pid_data(pid_t tid) {
	apr_hash_index_t *hi;
	apr_thread_mutex_lock(mutex_tid);
	for (hi = apr_hash_first(NULL, tids); hi; hi = apr_hash_next(hi)) {
		pid_t *k;
		tids_tid_pid_list_item *v;

		apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
		if (v->data.current_tid == tid) {
			apr_hash_set(tids, (void *) k, sizeof(pid_t), NULL);
			//my_free(k, v->p);
			//my_free(v, v->p);
			break;
		}
	}
	apr_thread_mutex_unlock(mutex_tid);
}

void get_memory_info(pid_t *pid, tids_tid_pid_list_item *item, apr_pool_t *pool) {
	float mem11, mem21;
	glibtop_proc_mem_own mem2;

	if ((glibtop_get_proc_mem_own_ret(&mem2, item->data.current_pid,
			get_use_tid() ? item->data.current_tid : (long) -1) < 0)) {
		add_tid_to_bad_list(*pid, pool);
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

void proceed_tid_pid(func_T func, apr_pool_t *pool) {
	apr_thread_mutex_lock(mutex_tid);
	apr_hash_index_t *hi;
	for (hi = apr_hash_first(NULL, tids); hi; hi = apr_hash_next(hi)) {
		pid_t *k;
		tids_tid_pid_list_item *v;

		apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
		func(k, v, pool);
	}

	apr_thread_mutex_unlock(mutex_tid);
	remove_tid_bad_from_list();
	remove_tid_bad_list();
}

void prcd_function(apr_pool_t *p, double *old_tm, double new_tm) {
	if ((new_tm - *old_tm) > 0.1) {
		proceed_tid_pid(get_memory_info, p);
		*old_tm = new_tm;
	}
}

void prcd_function2(apr_pool_t *p, double *old_tm1, double new_tm) {
	if ((new_tm - *old_tm1) > 1.0) {
		remove_counters();
		*old_tm1 = new_tm;
	}
}

void remove_tid_bad_list() {
	int i;
	int nxt = bad_tids->nelts;
	for (i = 0; i < nxt; i++) {
		/*bad_pid_tid_list_item *s = (bad_pid_tid_list_item *) */apr_array_pop(
				bad_tids);
		//if(s) my_free(s, s->p);
	}
}

void remove_tid_bad_from_list() {
	int i;
	for (i = 0; i < bad_tids->nelts; i++) {
		bad_pid_tid_list_item *s = ((bad_pid_tid_list_item**) bad_tids->elts)[i];
		if (s) {
			//remove_tid_pid_data(s->tid);
			add_item_to_removelist_tid(s->tid);
		}
	}

}

void add_tid_to_bad_list(pid_t pid, apr_pool_t *pool) {
	bad_pid_tid_list_item *ptr = NULL;
	my_alloc(ptr, bad_tids->pool, sizeof(bad_pid_tid_list_item));
	if (ptr) {
		ptr->tid = pid;
		ptr->p = pool;
		*(bad_pid_tid_list_item **) apr_array_push(bad_tids) = ptr;
	}
}

void init_tid_pid(apr_pool_t *pool) {
	int rv = apr_thread_mutex_create(&mutex_tid, APR_THREAD_MUTEX_DEFAULT,
			pool);
	if (rv != APR_SUCCESS)
		return;
	rv = apr_thread_mutex_create(&mutex_counters, APR_THREAD_MUTEX_DEFAULT,
			pool);
	if (rv != APR_SUCCESS)
		return;
	rv = apr_thread_mutex_create(&mutex_list, APR_THREAD_MUTEX_DEFAULT, pool);
	if (rv != APR_SUCCESS)
		return;
	rv = apr_thread_mutex_create(&mutex_remove_list, APR_THREAD_MUTEX_DEFAULT,
			pool);
	if (rv != APR_SUCCESS)
		return;
	tids = apr_hash_make(pool);
	counters = apr_hash_make(pool);
	bad_tids = apr_array_make(pool, 500, sizeof(bad_pid_tid_list_item *));
	list_1 = apr_array_make(pool, 500,
			sizeof(performance_module_send_req_list *));
	list_2 = apr_array_make(pool, 500,
			sizeof(performance_module_send_req_list *));
	remove_list_1 = apr_array_make(pool, 500, sizeof(remove_ietm_list_item *));
	remove_list_2 = apr_array_make(pool, 500, sizeof(remove_ietm_list_item *));
}

void destroy_tid_pid() {
	apr_thread_mutex_destroy(mutex_tid);
	apr_thread_mutex_destroy(mutex_counters);
	apr_thread_mutex_destroy(mutex_list);
	apr_thread_mutex_destroy(mutex_remove_list);
}

void debug_tid_pid() {
#ifdef DEBUG_MODULE_H
	write_debug_info("-----------------Head------------------------");
	apr_thread_mutex_lock(mutex_tid);
	apr_hash_index_t *hi;
	for (hi = apr_hash_first(NULL, tids); hi; hi = apr_hash_next(hi)) {
		pid_t *k;
		tids_tid_pid_list_item *v;

		apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
		write_debug_info("TID %d URL %s Fd %d", *k, v->data.hostname, v->fd);

	}

	apr_thread_mutex_unlock(mutex_tid);
	write_debug_info("-----------------Tail------------------------");
#endif
}

