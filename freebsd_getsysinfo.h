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
 * freebsd_getsysinfo.h
 *
 *  Created on: Jun 19, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#ifndef FREEBSD_GETSYSINFO_H_
#define FREEBSD_GETSYSINFO_H_

#include <stdint.h>

typedef struct _freebsd_get_mem
{
  long long memory_bytes;
  double memory_percents;
  long long memory_total;
} freebsd_get_mem;

typedef struct _freebsd_get_cpu
{
  long long system_cpu;
  long long process_cpu;
} freebsd_get_cpu;

typedef struct _freebsd_get_io
{
  long long io_write;
  long long io_read;
} freebsd_get_io;

typedef struct _freebsd_glibtop_cpu
{
  uint64_t total;
  uint64_t user;
  uint64_t nice;
  uint64_t sys;
  uint64_t idle;
  uint64_t iowait;
  uint64_t irq;
  uint64_t softirq;
  uint64_t frequency;
} freebsd_glibtop_cpu;

int init_freebsd_info (void);
void get_freebsd_mem_info (void * p, pid_t pid, freebsd_get_mem * fgm);
void get_freebsd_cpu_info (void * p, freebsd_glibtop_cpu * buf);
void get_freebsd_cpukvm_info (void * p, pid_t pid,
			      freebsd_get_cpu * fgc);
void get_freebsd_io_info (void * p, pid_t pid, freebsd_get_io * fgi);
int getstathz (void);
int gethz (void);

int get_freebsd_mem_info_ret (freebsd_get_mem * fgm, pid_t pid, pid_t tid);

#define glibtop_cpu_own freebsd_glibtop_cpu
#define glibtop_proc_time_own freebsd_get_cpu
#define iostat freebsd_get_io
#define glibtop_mem_own freebsd_get_mem
#define glibtop_proc_mem_own freebsd_get_mem
#define glibtop_get_proc_mem_own_ret get_freebsd_mem_info_ret

#endif /* FREEBSD_GETSYSINFO_H_ */
