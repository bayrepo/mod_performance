/*
 * Copyright 2012 Alexey Berezhok (bayrepo.info@gmail.com)
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
 * iostat.c
 *
 *  Created on: May 30, 2011
 *  Author: SKOREE
 *  E-mail: a@bayrepo.ru
 */

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/time.h>

#include "iostat.h"

#if defined(linux)

#define FILENAME        "/proc/stat"
#define LINUX_VERSION_CODE(x,y,z)   (0x10000*(x) + 0x100*(y) + z)

unsigned long os_version_code;
long smp_num_cpus = 1;

static int file_exists(char *fileName) {
	struct stat buf;
	int i = stat(fileName, &buf);
	if (i < 0)
		return 0;
	else {
		if (S_ISDIR (buf.st_mode)) {
			return 1;
		} else {
			return 0;
		}
	}
}

static inline char *
next_token(const char *p) {
	while (isspace (*p))
		p++;
	return (char *) p;
}

static char *
skip_token(const char *p) {
	p = next_token(p);
	while (*p && !isspace (*p))
		p++;
	p = next_token(p);
	return (char *) p;
}

static int has_sysfs(void) {
	static int init;
	static int sysfs;

	if (!init) {
		sysfs = file_exists("/sys");
		init = 1;
	}

	return sysfs;
}

static int set_linux_version() {
	struct utsname uts;
	unsigned x = 0, y = 0, z = 0;

	if (uname(&uts) == -1) /* failure most likely implies impending death */
		return -1;
	sscanf(uts.release, "%u.%u.%u", &x, &y, &z);
	if (LINUX_VERSION_CODE (x, y, z) >= LINUX_VERSION_CODE (2, 6, 0)
			&& !has_sysfs())
		return -1;

	os_version_code = LINUX_VERSION_CODE (x, y, z);
	return 0;
}

static int try_file_to_buffer_iostat(char *buffer, const char *format, ...) {
	char path[_PATH_SIZE];
	int fd;
	ssize_t len;
	va_list pa;

	va_start(pa, format);

	vsnprintf(path, sizeof path, format, pa);

	va_end(pa);

	buffer[0] = '\0';

	if ((fd = open(path, O_RDONLY)) < 0)
		return TRY_FILE_TO_BUFFER_OPEN_IOSTAT;

	len = read(fd, buffer, BUFSIZ - 1);
	close(fd);

	if (len < 0)
		return TRY_FILE_TO_BUFFER_READ_IOSTAT;

	buffer[len] = '\0';

	return TRY_FILE_TO_BUFFER_OK_IOSTAT;
}

static unsigned long long get_scaled_iostat(const char *buffer, const char *key) {
	const char *ptr;
	char *next;
	unsigned long long value = 0;
	//int dummy;

	ptr = strstr(buffer, key);
	if (ptr) {
		ptr += strlen(key);
		value = strtoull(ptr, &next, 0);
		if (strchr(next, 'k'))
			value *= 1024;
		else if (strchr(next, 'M'))
			value *= 1024 * 1024;
	} //else {
		//g_warning ("Could not read key '%s' in buffer '%s'", key, buffer);
	//	dummy = 1;
	//}

	return value;
}

void get_io_stat(iostat * info, long pid, long tid) {
	char buffer[BUFSIZ];
	io_stat_reset(info);
	int res;
	if (tid != (long) -1)
		res = try_file_to_buffer_iostat(buffer, "/proc/%d/task/%d/io", pid,
				tid);
	else
		res = try_file_to_buffer_iostat(buffer, "/proc/%d/io", pid);
	if (res == TRY_FILE_TO_BUFFER_OK_IOSTAT) {
		info->rchar = get_scaled_iostat(buffer, "rchar:");
		info->wchar = get_scaled_iostat(buffer, "wchar:");
		info->read_bytes = get_scaled_iostat(buffer, "read_bytes:");
		info->write_bytes = get_scaled_iostat(buffer, "write_bytes:");
		info->cancelled_write_bytes = get_scaled_iostat(buffer,
				"cancelled_write_bytes:");
	}
}

void get_io_stat_current(iostat * info, long pid, long tid) {
	char buffer[BUFSIZ];
	io_stat_reset(info);
	int res;
	if (tid != (long) -1)
		res = try_file_to_buffer_iostat(buffer, "/proc/self/task/%d/io", tid);
	else
		res = try_file_to_buffer_iostat(buffer, "/proc/self/io");
	if (res == TRY_FILE_TO_BUFFER_OK_IOSTAT) {
		info->rchar = get_scaled_iostat(buffer, "rchar:");
		info->wchar = get_scaled_iostat(buffer, "wchar:");
		info->read_bytes = get_scaled_iostat(buffer, "read_bytes:");
		info->write_bytes = get_scaled_iostat(buffer, "write_bytes:");
		info->cancelled_write_bytes = get_scaled_iostat(buffer,
				"cancelled_write_bytes:");
	}
}

void io_stat_reset(iostat * info) {
	memset(info, 0, sizeof(iostat));
}

void glibtop_get_cpu_own(glibtop_cpu_own * buf) {
	char buffer[BUFSIZ], *p;

	memset(buf, 0, sizeof(glibtop_cpu_own));

	int res = try_file_to_buffer_iostat(buffer, FILENAME);
	if (res == TRY_FILE_TO_BUFFER_OK_IOSTAT) {
		p = skip_token(buffer);
		buf->user = strtoull(p, &p, 0);
		buf->nice = strtoull(p, &p, 0);
		buf->sys = strtoull(p, &p, 0);
		buf->idle = strtoull(p, &p, 0);
		buf->total = buf->user + buf->nice + buf->sys + buf->idle;

		/* 2.6 kernel */
		if (os_version_code >= LINUX_VERSION_CODE (2, 6, 0)) {
			buf->iowait = strtoull(p, &p, 0);
			buf->irq = strtoull(p, &p, 0);
			buf->softirq = strtoull(p, &p, 0);

			buf->total += buf->iowait + buf->irq + buf->softirq;
		}

		buf->frequency = 100;
	}
}

int glibtop_init_own() {
	smp_num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (smp_num_cpus < 1)
		smp_num_cpus = 1;
	return set_linux_version();
}

static inline char *
proc_stat_after_cmd(char *p) {
	p = strrchr(p, ')');
	if (p)
		*p++ = '\0';
	return p;
}

static inline char *
skip_multiple_token(const char *p, size_t count) {
	while (count--)
		p = skip_token(p);

	return (char *) p;
}

void glibtop_get_proc_time_own(glibtop_proc_time_own * buf, pid_t pid,
		pid_t tid) {
	char buffer[BUFSIZ], *p;
	memset(buf, 0, sizeof(glibtop_proc_time_own));

	int res;
	if (tid != (long) -1)
		res = try_file_to_buffer_iostat(buffer, "/proc/%d/task/%d/stat", pid,
				tid);
	else
		res = try_file_to_buffer_iostat(buffer, "/proc/%d/stat", pid);
	if (res == TRY_FILE_TO_BUFFER_OK_IOSTAT) {
		p = proc_stat_after_cmd(buffer);
		if (!p)
			return;

		p = skip_multiple_token(p, 11);

		buf->utime = strtoull(p, &p, 0);
		buf->stime = strtoull(p, &p, 0);
		buf->rtime = buf->utime + buf->stime;
		buf->cutime = strtoull(p, &p, 0);
		buf->cstime = strtoull(p, &p, 0);
		p = skip_multiple_token(p, 3);
		buf->it_real_value = strtoull(p, &p, 0);
		buf->frequency = 100;

	}
}

#define FILENAMEMEM        "/proc/meminfo"

void glibtop_get_mem_own(glibtop_mem_own * buf) {
	glibtop_get_mem_own_ret(buf);

}

int glibtop_get_mem_own_ret(glibtop_mem_own * buf) {
	char buffer[BUFSIZ];
	memset(buf, 0, sizeof *buf);

	int res = try_file_to_buffer_iostat(buffer, FILENAMEMEM);
	if (res == TRY_FILE_TO_BUFFER_OK_IOSTAT) {
		buf->total = get_scaled_iostat(buffer, "MemTotal:");
		buf->free = get_scaled_iostat(buffer, "MemFree:");
		buf->used = buf->total - buf->free;
		buf->shared = 0;
		buf->buffer = get_scaled_iostat(buffer, "Buffers:");
		buf->cached = get_scaled_iostat(buffer, "Cached:");

		buf->user = buf->total - buf->free - buf->cached - buf->buffer;
		return 0;
	}
	return -1;

}

static size_t get_page_size(void) {
	static size_t pagesize = 0;

	if (!pagesize) {
		pagesize = getpagesize();
	}

	return pagesize;
}

void glibtop_get_proc_mem_own(glibtop_proc_mem_own * buf, pid_t pid, pid_t tid) {
	glibtop_get_proc_mem_own_ret(buf, pid, tid);
}

int glibtop_get_proc_mem_own_ret(glibtop_proc_mem_own * buf, pid_t pid,
		pid_t tid) {
	char buffer[BUFSIZ], *p;
	const size_t pagesize = get_page_size();
	memset(buf, 0, sizeof(glibtop_proc_mem_own));

	int res;
	if (tid != (long) -1)
		res = try_file_to_buffer_iostat(buffer, "/proc/%d/task/%d/stat", pid,
				tid);
	else
		res = try_file_to_buffer_iostat(buffer, "/proc/%d/stat", pid);
	if (res == TRY_FILE_TO_BUFFER_OK_IOSTAT) {
		p = proc_stat_after_cmd(buffer);
		if (!p)
			return -1;

		p = skip_multiple_token(p, 20);

		buf->vsize = strtoull(p, &p, 0);
		buf->rss = strtoull(p, &p, 0);
		buf->rss_rlim = strtoull(p, &p, 0);

		if (tid != (long) -1)
			res = try_file_to_buffer_iostat(buffer, "/proc/%d/task/%d/statm",
				pid, tid);
		else
			res = try_file_to_buffer_iostat(buffer, "/proc/%d/statm",
							pid);
		if (res == TRY_FILE_TO_BUFFER_OK_IOSTAT) {
			buf->size = strtoull(buffer, &p, 0);
			buf->resident = strtoull(p, &p, 0);
			buf->share = strtoull(p, &p, 0);
		} else
			return -1;
		buf->size *= pagesize;
		buf->resident *= pagesize;
		buf->share *= pagesize;
		buf->rss *= pagesize;
		return 0;
	}
	return -1;
}

long get_cpu_num() {
	return smp_num_cpus;
}

#endif
