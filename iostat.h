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
 * iostat.h
 *
 *  Created on: May 30, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#ifndef IOSTAT_H_
#define IOSTAT_H_

#include <stdint.h>

#define _PATH_SIZE 4096

enum TRY_FILE_TO_BUFFER_IOSTAT
{
  TRY_FILE_TO_BUFFER_OK_IOSTAT = 0,
  TRY_FILE_TO_BUFFER_OPEN_IOSTAT = -1,
  TRY_FILE_TO_BUFFER_READ_IOSTAT = -2
};

typedef struct _iostat
{
  long long rchar;
  long long wchar;
  long long read_bytes;
  long long write_bytes;
  long long cancelled_write_bytes;
} iostat;

typedef struct _glibtop_cpu_own
{
  uint64_t flags;
  uint64_t total;
  uint64_t user;
  uint64_t nice;
  uint64_t sys;
  uint64_t idle;
  uint64_t iowait;
  uint64_t irq;
  uint64_t softirq;
  uint64_t frequency;
} glibtop_cpu_own;

typedef struct _glibtop_proc_time_own
{
  uint64_t rtime;
  uint64_t utime;
  uint64_t stime;
  uint64_t cutime;
  uint64_t cstime;
  uint64_t timeout;
  uint64_t it_real_value;
  uint64_t frequency;
} glibtop_proc_time_own;

typedef struct _glibtop_mem_own
{
  uint64_t flags;
  uint64_t total;
  uint64_t used;
  uint64_t free;
  uint64_t shared;
  uint64_t buffer;
  uint64_t cached;
  uint64_t user;
  uint64_t locked;
} glibtop_mem_own;

typedef struct _glibtop_proc_mem_own
{
  uint64_t size;
  uint64_t vsize;
  uint64_t resident;
  uint64_t share;
  uint64_t rss;
  uint64_t rss_rlim;
} glibtop_proc_mem_own;

void get_io_stat (iostat * info, long pid, long tid);
void io_stat_reset (iostat * info);
int glibtop_init_own ();
void glibtop_get_cpu_own (glibtop_cpu_own * buf);
void glibtop_get_proc_time_own (glibtop_proc_time_own * buf, pid_t pid, pid_t tid);
void glibtop_get_mem_own (glibtop_mem_own * buf);
void glibtop_get_proc_mem_own (glibtop_proc_mem_own * buf, pid_t pid, pid_t tid);
int glibtop_get_mem_own_ret (glibtop_mem_own * buf);
int glibtop_get_proc_mem_own_ret (glibtop_proc_mem_own * buf, pid_t pid, pid_t tid);
long get_cpu_num ();
void get_io_stat_current(iostat * info, long pid, long tid);
#endif /* IOSTAT_H_ */
