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
 * mod_performance.c (Apache 2.x)
 *
 *  Created on: Mar 12, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 *
 *  Описание:
 *  mod_performance - модуль для вычисления нагрузки, создаваемой сайтом
 *  Управляющие параметры
 *  PerformanceSocket - путь к сокету
 *  PerformanceEnabled - включить наблюдение
 *  PerformanceHostFilter - список отслеживаемых хостов
 *  PerformanceDB - путь к базе данных сведений
 *  PerformanceHistory - число дней хранения истории
 *  PerformanceWorkHandler - рабочий хандлер при котором выводится статистика
 *  PerformanceURI - регулярное выражение фильтра по URI
 *  PerformanceScript - регулярное выражение фильтра по имени скрипта
 *  PerformanceUserHandler - рабочий хандлер при котором выводится статистика для хоста
 *  PerformanceUseCanonical - использовать каноническое имя при логировании
 *  PerformanceLog - путь к файлу логов
 *  PerformanceLogFormat - формат выводимого лога (%DATE%, %CPU%, %MEM%, %URI%, %HOST%, %SCRIPT%, %EXCTIME%)
 *  PerformanceLogType - тип логирования информации (Log, SQLite, MySQL, Postgres)
 *  PerformanceDbUserName - пользователь для соединения с БД(MySQL,...)
 *  PerformanceDBPassword - пароль для соединения с БД(MySQL,...)
 *  PerformanceDBName - имя базы для соединения с БД(MySQL,...)
 *  PerformanceDBHost - хост БД(MySQL,...)
 *  PerformanceUseCPUTopMode - Irix/Solaris режим подсчета CPU % как в procps top (работает только в Linux)
 *  PerformanceCheckDaemonTimeExec - время выполнения демона, после его истечения демон перезапускается в секундах или в формате HH:MM:SS - т.е. время каждого дня, когда перезапускается демон
 *  PerformanceFragmentationTime - ежедневная дефрагментация базы данных. Только для MySQL
 *  PerformanceExternalScript - список скриптов, которые будут обрабатываться внешними модулями, например - mod_fcgid, mod_cgid, suphp...
 *  PerformanceMinExecTime - два параметра 1) число(в 1/100 секунды), 2) HARD/SOFT - задает минимальное время выполения скрипта и способ его сохранения HARD(не сохранять)/SOFT(сохранять с 0 %CPU)
 *  PerformanceSilentMode - On/Off включить или выключить молчаливый режим
 *  PerformanceSocketPermType - права доступа к сокету в формате ddd, например, 755 или 600..., а также имя сокета будет создаваться с PID{PID/NOPID} процесса или без него (PerformanceSocketPermType 600 PID)
 *  PerformanceUseTid - On/Off считать статистику по TID/PID
 *  PerformanceHostId - имя сервера для которого пишутся данные (по умолчанию localhost)
 *  PerformanceCustomReports - путь к файлу, описывающему отчеты
 *  PerformanceBlockedSocket - использовать блокирующий сокет
 *  PerformanceCustomPool - использовать не APR распределитель памяти
 *
 *  Deprecated
 *  PerformanceMaxThreads
 *  PerformanceStackSize
 *  PerformanceExtended
 *  PerformancePeriodicalWatch
 *
 */

/*
 * Если Apache 1.3 - прекратить сборку
 */
#ifdef APACHE1_3
#error "This source is for Apache version 2.0 or 2.2 only"
#endif

#if defined(linux)
#define PERF_SYSTEM_CHECK_OK 1
#endif
#if defined(__FreeBSD__)
#define PERF_SYSTEM_CHECK_OK 2
#endif

#ifndef PERF_SYSTEM_CHECK_OK
#error "This source is for linux or freebsd systems only"
#endif

#define CORE_PRIVATE

#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#ifndef APACHE2_4
#include "mpm_default.h"
#endif
#include "unixd.h"
#include "ap_mpm.h"
#include "mod_log_config.h"
/*
 * Для Apache 2.0 этого заголовочного файла нет, поэтому regexp будем проверять
 * средствами системы.
 */
#ifndef APACHE2_0
#include "ap_regex.h"
#else
#include "regex.h"
#endif
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

#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#include <gdfontl.h>
#include <gdfontg.h>

#include "common.h"
#include "sql_adapter.h"
#include "reports.h"
#if defined(linux)
#include "iostat.h"
#endif
#if defined(__FreeBSD__)
#include "freebsd_getsysinfo.h"
#endif
#include "hertz-calcualtion.h"
#include "timers.h"
#include "chart.h"
#include "html-sources.h"
#include "tid_pid_list.h"
#include "custom_report.h"

#include "debug.h"

#include "tid_pid_list_ut.h"

#ifdef APACHE2_4
#define unixd_config ap_unixd_config
#define unixd_setup_child ap_unixd_setup_child
#endif

module AP_MODULE_DECLARE_DATA performance_module;

//Configuration structure
typedef struct performance_module_cfg {
	int performance_enabled;
	apr_array_header_t *performance_host_filter;
	const char *performance_work_handler;
	const char *performance_user_handler;
	apr_array_header_t *performance_uri;
	apr_array_header_t *performance_script;
	int performance_use_cononical_name;
	const char *performance_logname;
	apr_file_t *fd;
	const char *performance_log_format;
	apr_array_header_t *performance_log_format_parsed;
	apr_array_header_t *performance_external_handlers;
} performance_module_cfg;

#define DAEMON_STARTUP_ERROR 254
#define DEFAULT_SOCKET  DEFAULT_REL_RUNTIMEDIR "/perfsocket"
#define DEFAULT_WORK_HANDLER "performance-status"
#define DEFAULT_USER_HANDLER "user-status"
#define DEFAULT_HISTORY_TIME 30
#define DEFAULT_DB  DEFAULT_REL_RUNTIMEDIR "/perfdb"
#define DEFAULT_PERF_LISTENBACKLOG 200
#define MODULE_INITIALIZATION_POINTER 0xFFFFFFFF
#define WATCH_PERIOD 10000
#define DELETE_INTERVAL 60*60*3
#define FORMAT_DEFAULT "[%DATE%] from %HOST% (%URI%) script %SCRIPT%: cpu %CPU% (%CPUS%), memory %MEM% (%MEMMB%), execution time %EXCTIME%, IO: R - %BYTES_R% W - %BYTES_W%"
#define DEFAULT_USERNAME "perf_user"
#define DEFAULT_PASSWORD "perf_password"
#define DEFAULT_DB_NAME "perf_db"
#define DEFAULT_DB_HOST "localhost"
#define DEFAULT_HOST_NAME "localhost"

#ifndef DEFAULT_CONNECT_ATTEMPTS
#define DEFAULT_CONNECT_ATTEMPTS  2
#endif

char gHostName[32];

glibtop_mem_own global_mem;

static char *performance_socket;
static char *performance_socket_no_pid;
static char *performance_db;
int performance_history;
static pid_t parent_pid;
static apr_proc_t daemon_proc;
apr_array_header_t *default_performance_log_format_parsed;
int log_type;
static char *performance_username;
static char *performance_password;
static char *performance_dbname;
static char *performance_dbhost;
static int performance_top;
static char *performance_check_extm;
static char *performance_db_defrag;
static int performance_silent_mode;

static long performance_min_exec_time = 123456789;
static apr_fileperms_t performance_socket_perm = 0x0000;
static int performance_use_pid = 1;
static int use_tid = 0;
static int performance_blocksave = 0;
static int performance_usecustompool = 1;

static apr_threadkey_t *key;

volatile int thread_should_stop = 0;
static volatile int daemon_should_exit = 0;
static apr_pool_t *pperf = NULL;
server_rec *root_server = NULL;
static apr_pool_t *root_pool = NULL;

char *template_path = NULL;

static int server_limit, thread_limit;

static pid_t child_pid;
static int registered_filter = 0;

apr_thread_mutex_t *mutex_db = NULL;

static __thread intptr_t sd_global = 0;

unsigned long long glbHtz;

#define BEGIN_SUB_POOL_SECTION(main_pool, pool) apr_pool_t *pool; \
                                                apr_pool_create (&pool, main_pool)

#define END_SUB_POOL_SECTION(main_pool, pool)   apr_pool_destroy(pool)

char *performance_paremeters[] = { "DATE", "CPU", "MEM", "URI", "HOST",
		"SCRIPT", "EXCTIME", "CPUS", "MEMMB", "BYTES_W", "BYTES_R" };

void init_global_mem() {
	memset(&global_mem, 0, sizeof(glibtop_mem_own));
#if defined(linux)
	glibtop_get_mem_own(&global_mem);
#endif
}

glibtop_mem_own *get_global_mem() {
	return &global_mem;
}

int get_use_tid() {
	return use_tid;
}

char *get_host_id() {
	return gHostName;
}

int performance_get_all_access() {
	module *mod;
	mod = ap_find_linked_module("mod_ruid2.c");
	if (mod)
		return 1;
	mod = ap_find_linked_module("itk.c");
	if (mod)
		return 1;
	return 0;
}

char *
performance_get_current_time_as_string(apr_pool_t * p) {
	apr_time_t tm;
	apr_time_exp_t te;
	apr_size_t retsize;
	char *tm_str = apr_palloc(p, 1024);
	tm = apr_time_now();
	apr_time_exp_lt(&te, tm);
	apr_strftime(tm_str, &retsize, 1024, "%Y-%m-%d %H:%M:%S", &te);
	return tm_str;
}

char *
performance_log_format_parse_param(apr_pool_t * p, char *str) {
	char *tmp = apr_pstrndup(p, str + 1, strlen(str) - 2);
	int i;
	for (i = 0; i < 11; i++) {
		if (!apr_strnatcasecmp(tmp, performance_paremeters[i]))
			return str;
	}

	if (!apr_strnatcasecmp(tmp, ""))
		return apr_pstrdup(p, "%");
	return NULL;
}

char *
performance_log_format_get_string(apr_pool_t * p, apr_array_header_t * arr,
		char *dateadd, char *uri, char *host, char *script, double cpu,
		double mem, double exc_time, double cpus, double memmb, double btrd,
		double btwr) {
	int i;
	char *str = apr_pstrdup(p, "");
	for (i = 0; i < arr->nelts; i++) {
		const char *s = ((const char **) arr->elts)[i];
		if ((*s == '%') && (strlen(s) > 1)) {
			if (!apr_strnatcasecmp(s, "%DATE%")) {
				str = apr_pstrcat(p, str, dateadd, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%CPU%")) {
				char *nmb = apr_psprintf(p, "%f", cpu);
				str = apr_pstrcat(p, str, nmb, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%MEM%")) {
				char *nmb = apr_psprintf(p, "%f", mem);
				str = apr_pstrcat(p, str, nmb, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%EXCTIME%")) {
				char *nmb = apr_psprintf(p, "%f", exc_time);
				str = apr_pstrcat(p, str, nmb, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%URI%")) {
				str = apr_pstrcat(p, str, uri, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%HOST%")) {
				str = apr_pstrcat(p, str, host, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%SCRIPT%")) {
				str = apr_pstrcat(p, str, script, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%CPUS%")) {
				char *nmb = apr_psprintf(p, "%f", cpus);
				str = apr_pstrcat(p, str, nmb, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%MEMMB%")) {
				char *nmb = apr_psprintf(p, "%f", memmb);
				str = apr_pstrcat(p, str, nmb, NULL);
				continue;
			}
			if (!apr_strnatcasecmp(s, "%BYTES_W%")) {
				char *nmb = apr_psprintf(p, "%f", btrd);
				str = apr_pstrcat(p, str, nmb, NULL);
				continue;
			}

			if (!apr_strnatcasecmp(s, "%BYTES_R%")) {
				char *nmb = apr_psprintf(p, "%f", btwr);
				str = apr_pstrcat(p, str, nmb, NULL);
				continue;
			}

		} else {
			str = apr_pstrcat(p, str, s, NULL);
		}
	}
	return str;
}

apr_array_header_t *
performance_log_format_parse(apr_pool_t * p, char *str) {
	apr_array_header_t *new_arr = apr_array_make(p, 1, sizeof(char *));

	char *beg_p = str;
	while (*str) {
		if (*str == '%') {
			if (beg_p != str) {
				if (*beg_p != '%') {
					*(char **) apr_array_push(new_arr) = apr_pstrndup(p, beg_p,
							str - beg_p);
					beg_p = str;
				} else {
					char *ptr = apr_pstrndup(p, beg_p, str - beg_p + 1);
					ptr = performance_log_format_parse_param(p, ptr);
					if (ptr) {
						*(char **) apr_array_push(new_arr) = ptr;
					}
					beg_p = str + 1;
				}
			}
		}
		str++;
	}

	if (beg_p != str) {
		if (*beg_p != '%') {
			*(char **) apr_array_push(new_arr) = apr_pstrndup(p, beg_p,
					str - beg_p);
			beg_p = str;
		} else {
			char *ptr = apr_pstrndup(p, beg_p, str - beg_p + 1);
			ptr = performance_log_format_parse_param(p, ptr);
			if (ptr) {
				*(char **) apr_array_push(new_arr) = ptr;
			}
			beg_p = str + 1;
		}
	}

	return new_arr;
}

static int performance_module_start(apr_pool_t * p, server_rec * main_server,
		apr_proc_t * procnew);

static int sqlite3_check_for_tables(apr_pool_t * p, server_rec * main_server) {

	char *err = sql_adapter_get_create_table(p, log_type, mutex_db);
	if (err) {
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "database table creation error: %s", err);
		return 0;
	}
	return 1;
}

static void sqlite3_save_request_info(apr_pool_t * p, server_rec * main_server,
		char *hostname, char *uri, char *script, double dtime, double dcpu,
		double dmemory, double dcpu_sec, double dmemory_mb, double dbr,
		double dbw) {

	apr_pool_t *inner_pool = NULL;
	if (apr_pool_create (&inner_pool, p) != APR_SUCCESS)
		return;
	char *err = sql_adapter_get_insert_table(inner_pool, log_type, hostname,
			uri, script, dtime, dcpu, dmemory, mutex_db, dcpu_sec, dmemory_mb,
			dbr, dbw);
	if (err) {
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "database insert error: %s", err);
	}

	//apr_pool_clear (inner_pool);
	apr_pool_destroy(inner_pool);
	return;

}

static void sqlite3_delete_request_info(apr_pool_t * p,
		server_rec * main_server, int keep_days) {

	apr_pool_t *inner_pool = NULL;
	if (apr_pool_create (&inner_pool, p) != APR_SUCCESS)
		return;
	char *err = sql_adapter_get_delete_table(inner_pool, log_type,
			performance_history, mutex_db);
	if (err) {
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "database deletion error: %s", err);
	}

	//apr_pool_clear (inner_pool);
	apr_pool_destroy(inner_pool);
	return;
}

static void daemon_signal_handler(int sig) {
	if (sig == SIGHUP) {
		++daemon_should_exit;
	}
}

static apr_status_t performance_read_data_from(int fd, void *vbuf,
		size_t buf_size) {
	char *buf = vbuf;
	int rc;
	size_t bytes_read = 0;

	do {
		do {
			errno = 0;
			rc = read(fd, buf + bytes_read, buf_size - bytes_read);
		} while (rc < 0 && errno == EINTR);
		switch (rc) {
		case -1:
			return errno;
		case 0: /* unexpected */
			return ECONNRESET;
		default:
			bytes_read += rc;
		}
	} while (bytes_read < buf_size);

	return APR_SUCCESS;
}

static apr_status_t performance_send_data_to(int fd, const void *buf,
		size_t buf_size) {
	int rc, __trys = 0;
	//int read_buffer = 0;

	//if(performance_blocksave){
	//	rc = read(fd, (void *)&read_buffer, sizeof(int));
	//}
	do {
		errno = 0;
		rc = write(fd, buf, buf_size);
		if (rc == buf_size)
			return APR_SUCCESS;
		__trys++;
		if (__trys > 10)
			return -1;
	} while (rc < 0 && (errno == EINTR || errno == EAGAIN));
	if (rc < 0) {
		return errno;
	}

	return APR_SUCCESS;
}

static performance_module_cfg *
performance_module_sconfig(const request_rec * r) {
	return (performance_module_cfg *) ap_get_module_config(
			r->server->module_config, &performance_module);
}

static void *
create_performance_module_config(apr_pool_t * p, server_rec * s) {
	performance_module_cfg *c = (performance_module_cfg *) apr_pcalloc(p,
			sizeof(performance_module_cfg));

	c->performance_enabled = 0;
	c->performance_host_filter = NULL;
	c->performance_work_handler = apr_pstrdup(p, DEFAULT_WORK_HANDLER);
	c->performance_uri = NULL;
	c->performance_script = NULL;
	c->performance_user_handler = apr_pstrdup(p, DEFAULT_USER_HANDLER);
	c->performance_use_cononical_name = 0;
	c->performance_log_format = NULL;
	c->performance_log_format_parsed = NULL;
	c->performance_logname = NULL;
	c->fd = 0;
	c->performance_external_handlers = NULL;
	return c;
}

static void *
merge_performance_module_config(apr_pool_t * p, void *basev, void *overridesv) {
	performance_module_cfg *base = (performance_module_cfg *) basev,
			*overrides = (performance_module_cfg *) overridesv;

	performance_module_cfg *new = apr_palloc(p, sizeof(performance_module_cfg));
	memcpy(new, overrides, sizeof(performance_module_cfg));
	if (!new->performance_enabled) {
		new->performance_enabled = base->performance_enabled;
	}
	if (!new->performance_host_filter) {
		new->performance_host_filter = base->performance_host_filter;
	}
	if (!new->performance_log_format) {
		new->performance_log_format = base->performance_log_format;
	}
	if (!new->performance_log_format_parsed) {
		new->performance_log_format_parsed =
				base->performance_log_format_parsed;
	}
	if (!new->performance_logname) {
		new->performance_logname = base->performance_logname;
	}
	if (!new->fd) {
		new->fd = base->fd;
	}
	if (!new->performance_script) {
		new->performance_script = base->performance_script;
	}
	if (!new->performance_uri) {
		new->performance_uri = base->performance_uri;
	}
	if (!new->performance_use_cononical_name) {
		new->performance_use_cononical_name =
				base->performance_use_cononical_name;
	}

	if (!new->performance_external_handlers) {
		new->performance_external_handlers =
				base->performance_external_handlers;
	}

	//return overrides->performance_work_handler ? overrides : base;
	return new;
}

static int performance_module_pre_config(apr_pool_t * pconf, apr_pool_t * plog,
		apr_pool_t * ptemp) {
	performance_socket = ap_append_pid(pconf, DEFAULT_SOCKET, ".");
	performance_socket_no_pid = apr_pstrdup(pconf, DEFAULT_SOCKET);
	performance_history = DEFAULT_HISTORY_TIME;
	performance_db = apr_pstrdup(pconf, DEFAULT_DB);
	default_performance_log_format_parsed = performance_log_format_parse(pconf,
	FORMAT_DEFAULT);
	log_type = 1;
	performance_username = apr_pstrdup(pconf, DEFAULT_USERNAME);
	performance_password = apr_pstrdup(pconf, DEFAULT_PASSWORD);
	performance_dbname = apr_pstrdup(pconf, DEFAULT_DB_NAME);
	performance_dbhost = apr_pstrdup(pconf, DEFAULT_DB_HOST);
#if defined(linux)
	performance_top = 1;
#endif
	performance_check_extm = NULL;
	performance_db_defrag = NULL;
	performance_min_exec_time = 123456789;
	performance_silent_mode = 1;
	performance_socket_perm = 0000;
	performance_use_pid = 1;
	apr_cpystrn(gHostName, DEFAULT_HOST_NAME, sizeof(gHostName));
	template_path = NULL;
	performance_usecustompool = 1;
	performance_blocksave = 0;

	return OK;
}

static const char *
set_performance_module_socket(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_socket = ap_append_pid(cmd->pool, arg, ".");
	performance_socket = ap_server_root_relative(cmd->pool, performance_socket);
	performance_socket_no_pid = apr_pstrdup(cmd->pool, arg);
	performance_socket_no_pid = ap_server_root_relative(cmd->pool,
			performance_socket_no_pid);

	if (!performance_socket) {
		return apr_pstrcat(cmd->pool, "Invalid PerformanceSocket path", arg,
		NULL);
	}

	return NULL;
}

static const char *
set_performance_module_tpl_path(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	template_path = apr_pstrdup(cmd->pool, arg);
	template_path = ap_server_root_relative(cmd->pool, template_path);

	if (!template_path) {
		return apr_pstrcat(cmd->pool, "Invalid path to template", arg, NULL);
	}

	return NULL;
}

static const char *
set_performance_module_host_id(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}
	if (arg) {
		apr_cpystrn(gHostName, arg, sizeof(gHostName));
	}

	return NULL;
}

static const char *
set_performance_module_username(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_username = (char *) arg;

	if (!performance_username) {
		return apr_pstrcat(cmd->pool, "Invalid PerformanceDBUser name ", arg,
		NULL);
	}

	return NULL;
}

static const char *
set_performance_module_password(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_password = (char *) arg;

	if (!performance_password) {
		return apr_pstrcat(cmd->pool, "Invalid PerformanceDBPassword name ",
				arg, NULL);
	}

	return NULL;
}

static const char *
set_performance_module_db_name(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_dbname = (char *) arg;

	if (!performance_dbname) {
		return apr_pstrcat(cmd->pool, "Invalid PerformanceDBName name ", arg,
		NULL);
	}

	return NULL;
}

static const char *
set_performance_module_db_host(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_dbhost = (char *) arg;

	if (!performance_dbhost) {
		return apr_pstrcat(cmd->pool, "Invalid PerformanceDBHost name ", arg,
		NULL);
	}

	return NULL;
}

static const char *
set_performance_module_enabled(cmd_parms * cmd, void *dummy, const char *arg) {
	server_rec *s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);

	if (!apr_strnatcmp(arg, "On"))
		conf->performance_enabled = 1;
	else
		conf->performance_enabled = 0;
	return NULL;
}

static const char *
set_performance_module_tid(cmd_parms * cmd, void *dummy, const char *arg) {
	//server_rec *s = cmd->server;
	//performance_module_cfg *conf = ap_get_module_config (s->module_config,
	//		&performance_module);
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	if (!apr_strnatcmp(arg, "On"))
		use_tid = 1;
	else
		use_tid = 0;
	return NULL;
}

static const char *
set_performance_blocked_save(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	if (!apr_strnatcmp(arg, "On"))
		performance_blocksave = 1;
	else
		performance_blocksave = 0;
	return NULL;
}

static const char *
set_performance_custom_pool(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	if (!apr_strnatcmp(arg, "On"))
		performance_usecustompool = 1;
	else
		performance_usecustompool = 0;
	return NULL;
}

static const char *
set_performance_module_enabled_canonical(cmd_parms * cmd, void *dummy,
		const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}
	server_rec *s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);

	if (!apr_strnatcmp(arg, "On"))
		conf->performance_use_cononical_name = 1;
	else
		conf->performance_use_cononical_name = 0;
	return NULL;
}

#if defined(linux)
static const char *
set_performance_module_cpu_top(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	if (!apr_strnatcmp(arg, "Irix"))
		performance_top = 1;
	else if (!apr_strnatcmp(arg, "Solaris"))
		performance_top = 2;
	else
		performance_top = 1;
	return NULL;
}
#endif

static const char *
set_performance_module_cpu_top_depr(cmd_parms * cmd, void *dummy,
		const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - use new PerformanceUseCPUTopMode");
	return NULL;
}

static const char *
set_performance_module_silent_mode(cmd_parms * cmd, void *dummy,
		const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	if (!apr_strnatcmp(arg, "On"))
		performance_silent_mode = 1;
	else
		performance_silent_mode = 0;
	return NULL;
}

static apr_fileperms_t performance_convert_number_to_perm(int number) {
	apr_fileperms_t perm = 0;
	if (number & 0400)
		perm = perm | APR_FPROT_UREAD;
	if (number & 0200)
		perm = perm | APR_FPROT_UWRITE;
	if (number & 0100)
		perm = perm | APR_FPROT_UEXECUTE;
	if (number & 0040)
		perm = perm | APR_FPROT_GREAD;
	if (number & 0020)
		perm = perm | APR_FPROT_GWRITE;
	if (number & 0010)
		perm = perm | APR_FPROT_GEXECUTE;
	if (number & 0004)
		perm = perm | APR_FPROT_WREAD;
	if (number & 0002)
		perm = perm | APR_FPROT_WWRITE;
	if (number & 0001)
		perm = perm | APR_FPROT_WEXECUTE;
	return perm;
}

static const char *
set_performance_module_socket_perm(cmd_parms * cmd, void *dummy,
		const char *arg1, const char *arg2) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	if (arg1) {
		int tmp_perm = (int) apr_atoi64(arg1);
		int tmp_other = ((tmp_perm % 10) > 7 ? 7 : (tmp_perm % 10));
		;
		tmp_perm = tmp_perm / 10;
		int tmp_group = ((tmp_perm % 10) > 7 ? 7 : (tmp_perm % 10));
		tmp_perm = tmp_perm / 10;
		int tmp_user = ((tmp_perm % 10) > 7 ? 7 : (tmp_perm % 10));
		int tmp_mask = tmp_other | (tmp_group << 3) | (tmp_user << 6);
		performance_socket_perm = performance_convert_number_to_perm(tmp_mask);
	}
	if (arg2) {
		if (!apr_strnatcmp(arg2, "NOPID")) {
			performance_use_pid = 0;
		}
	}

	return NULL;
}

static const char *
set_performance_module_check_daemon_exctime(cmd_parms * cmd, void *dummy,
		const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_check_extm = (char *) arg;

	return NULL;
}

static const char *
set_performance_module_dboptimize(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_db_defrag = (char *) arg;

	return NULL;
}

static const char *
set_performance_module_cpu_exec_time(cmd_parms * cmd, void *dummy,
		const char *arg1, const char *arg2) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	int find_data = 0;

	performance_min_exec_time = apr_atoi64(arg1);
	if (!performance_min_exec_time) {
		performance_min_exec_time = apr_atoi64(arg2);
		if (!performance_min_exec_time) {
			return apr_pstrcat(cmd->pool,
					"Invalid PerformanceMinCPUExecTime directive. Wrong script execution length. Must be more then 0: ",
					arg2, NULL);
		}

		if (!apr_strnatcasecmp(arg1, "SOFT")) {
			performance_min_exec_time = performance_min_exec_time * 1;
			find_data++;
		}
		if (!apr_strnatcasecmp(arg1, "HARD")) {
			performance_min_exec_time = performance_min_exec_time * (-1);
			find_data++;
		}
		if (!find_data) {
			return apr_pstrcat(cmd->pool,
					"Invalid PerformanceMinCPUExecTime directive. Must be SOFT or HARD: ",
					arg1, NULL);
		}

	} else {
		if (!apr_strnatcasecmp(arg2, "SOFT")) {
			performance_min_exec_time = performance_min_exec_time * 1;
			find_data++;
		}
		if (!apr_strnatcasecmp(arg2, "HARD")) {
			performance_min_exec_time = performance_min_exec_time * (-1);
			find_data++;
		}
		if (!find_data) {
			return apr_pstrcat(cmd->pool,
					"Invalid PerformanceMinCPUExecTime directive. Must be SOFT or HARD: ",
					arg2, NULL);
		}

	}

	return NULL;
}

static const char *
set_performance_module_db(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_db = ap_server_root_relative(cmd->pool, arg);

	if (!performance_db) {
		return apr_pstrcat(cmd->pool, "Invalid PerformanceDB path ", arg, NULL);
	}

	return NULL;
}

static const char *
set_performance_module_history(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	performance_history = (int) apr_atoi64(arg);
	if (performance_history < 0) {
		return apr_pstrcat(cmd->pool,
				"Invalid PerformanceHistory value, less than 0", arg, NULL);
	}
	return NULL;
}

static const char *
set_performance_module_work_handler(cmd_parms * cmd, void *dummy,
		const char *arg) {
	server_rec *s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);

	conf->performance_work_handler = arg;

	if (!conf->performance_work_handler) {
		return apr_pstrcat(cmd->pool, "Invalid work handler ", arg, NULL);
	}
	return NULL;
}

static const char *
set_performance_module_log(cmd_parms * cmd, void *dummy, const char *arg) {
	performance_module_cfg *cfg = ap_get_module_config(
			cmd->server->module_config, &performance_module);
	cfg->performance_logname = arg;
	return NULL;
}

static const char *
set_performance_module_log_format(cmd_parms * cmd, void *dummy, const char *arg) {
	performance_module_cfg *cfg = ap_get_module_config(
			cmd->server->module_config, &performance_module);
	char *tmp_str = apr_pstrdup(cmd->pool, arg);
	cfg->performance_log_format_parsed = performance_log_format_parse(cmd->pool,
			tmp_str);

	return NULL;
}

static const char *
set_performance_module_log_type(cmd_parms * cmd, void *dummy, const char *arg) {
	const char *err = ap_check_cmd_context(cmd, GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}

	if (!apr_strnatcasecmp(arg, "Log")) {
		log_type = 0;
	}
	if (!apr_strnatcasecmp(arg, "SQLite")) {
		log_type = 1;
	}
	if (!apr_strnatcasecmp(arg, "MySQL")) {
		log_type = 2;
	}
	if (!apr_strnatcasecmp(arg, "Postgres")) {
		log_type = 3;
	}

	return NULL;
}

static const char *
set_performance_module_user_handler(cmd_parms * cmd, void *dummy,
		const char *arg) {
	server_rec *s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);

	conf->performance_user_handler = arg;

	if (!conf->performance_user_handler) {
		return apr_pstrcat(cmd->pool, "Invalid user handler ", arg, NULL);
	}
	return NULL;
}

static const char *
set_performance_module_host_filter(cmd_parms * cmd, void *dummy,
		const char *arg) {
	server_rec *s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);
	if (!conf->performance_host_filter) {
		conf->performance_host_filter = apr_array_make(cmd->pool, 2,
				sizeof(char *));
	}
	*(const char **) apr_array_push(conf->performance_host_filter) = arg;
	return NULL;
}

static const char *
set_performance_module_external_handler(cmd_parms * cmd, void *dummy,
		const char *arg) {

	server_rec * s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);
	if (!conf->performance_external_handlers) {
		conf->performance_external_handlers = apr_array_make(cmd->pool, 2,
				sizeof(char *));
	}
	*(const char **) apr_array_push(conf->performance_external_handlers) = arg;
	return NULL;
}

static const char *
set_performance_module_uri(cmd_parms * cmd, void *dummy, const char *arg) {
	server_rec *s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);
	if (!conf->performance_uri) {
		conf->performance_uri = apr_array_make(cmd->pool, 2, sizeof(char *));
	}
	*(const char **) apr_array_push(conf->performance_uri) = arg;
	return NULL;
}

static const char *
set_performance_module_script(cmd_parms * cmd, void *dummy, const char *arg) {
	server_rec *s = cmd->server;
	performance_module_cfg *conf = ap_get_module_config(s->module_config,
			&performance_module);
	if (!conf->performance_script) {
		conf->performance_script = apr_array_make(cmd->pool, 2, sizeof(char *));
	}
	*(const char **) apr_array_push(conf->performance_script) = arg;
	return NULL;
}

void math_get_pcpu(double *dcpu, glibtop_cpu_own * current_cpu_beg,
		glibtop_proc_time_own * current_cpu_pid_beg, double current_tm_val_beg,
		glibtop_cpu_own * current_cpu_end,
		glibtop_proc_time_own * current_cpu_pid_end, double current_tm_val_end) {
#if defined(linux)
	int maxPCPU = ((performance_top == 2) ? 1 : get_cpu_num()) * 100;
#else
	int maxPCPU = 100;
#endif

	double dmaxPCPU = maxPCPU;
#if defined(linux)
	if (!performance_top) {
		uint64_t dtotal, dpu, dps;
		dtotal = current_cpu_end->total - current_cpu_beg->total;
		dpu = current_cpu_pid_end->utime - current_cpu_pid_beg->utime;
		dps = current_cpu_pid_end->stime - current_cpu_pid_beg->stime;

		if ((dtotal) > 0) {
			*dcpu = ((double) (dpu + dps) * 100.0) / ((double) (dtotal) * 1.0);
		}
	} else {
		float et;
		et = current_tm_val_end - current_tm_val_beg;

		if (et > 0.0) {
			long long tics = (current_cpu_pid_end->stime
					+ current_cpu_pid_end->utime)
					- (current_cpu_pid_beg->stime + current_cpu_pid_beg->utime);
			float Frame_tscale =
					100.0f
							/ ((float) 100.0 * (float) et
									* (float) (
											(performance_top == 2) ?
													get_cpu_num() : 1));
			*dcpu = (double) ((float) tics * Frame_tscale);
		}
	}
#endif
#if defined(__FreeBSD__)
	uint64_t dtotal, dpu, dps;
	dtotal = current_cpu_end->total - current_cpu_beg->total;
	dpu = current_cpu_pid_end->process_cpu - current_cpu_pid_beg->process_cpu;
	dps = current_cpu_pid_end->system_cpu - current_cpu_pid_beg->system_cpu;

	if ((dtotal) > 0)
	{
		*dcpu = ((double) (dpu + dps) * 100.0) / ((double) (dtotal) * 1.0);
	}
#endif

	if (*dcpu > dmaxPCPU) {
		*dcpu = dmaxPCPU;
	}

}

void math_get_io(double *dwrite, double *dread, iostat * old, iostat * new) {
#if defined(linux)
	double f_write_end = new->wchar;
	double f_write_beg = old->wchar;
	double f_write =
			(f_write_end > f_write_beg) ? (f_write_end - f_write_beg) : 0.0;
	double f_read =
			(new->rchar > old->rchar) ?
					(double) (new->rchar - old->rchar) : 0.0;

	*dwrite = f_write / 1024.0;
	*dread = f_read / 1024.0;
#endif
#if defined(__FreeBSD__)
	double f_write_end = new->io_write;
	double f_write_beg = old->io_write;
	double f_write =
	(f_write_end > f_write_beg) ? (f_write_end - f_write_beg) : 0.0;
	double f_read =
	(new->io_read >
			old->io_read) ? (double) (new->io_read - old->io_read) : 0.0;

	*dwrite = f_write;
	*dread = f_read;

#endif
}

void math_get_mem(float *memrate, float *membytes, glibtop_mem_own * memory,
		glibtop_proc_mem_own * procmem) {
#if defined(linux)
	*memrate = 100 * (procmem->resident) / ((memory->total) * 1.0); // calculate the memory usage

	*memrate = isnan (*memrate) ? 0.0 : *memrate;
	*memrate = isinf (*memrate) ? 0.0 : *memrate;

	if (membytes) {
		*membytes = procmem->resident;
		*membytes = *membytes / (1024 * 1024 * 1.0);
		*membytes = isnan (*membytes) ? 0.0 : *membytes;
		*membytes = isinf (*membytes) ? 0.0 : *membytes;
	}
#endif
#if defined(__FreeBSD__)

	*memrate = 100.0 * procmem->memory_percents;

	*memrate = isnan (*memrate) ? 0.0 : *memrate;
	*memrate = isinf (*memrate) ? 0.0 : *memrate;

	if (membytes)
	{
		*membytes = procmem->memory_bytes;
		*membytes = *membytes / (1024 * 1024 * 1.0);
		*membytes = isnan (*membytes) ? 0.0 : *membytes;
		*membytes = isinf (*membytes) ? 0.0 : *membytes;
	}

#endif

}

void performance_module_save_to_db(double req_time, apr_pool_t *pool,
		server_rec *srv, server_rec *server,
		performance_module_send_req *req_begin, double dcpu, double dmem,
		double dmem_mb, double dwrite, double dread, double dt) {
#if defined(linux)
	int maxPCPU = ((performance_top == 2) ? 1 : get_cpu_num()) * 100;
#else
	int maxPCPU = 100;
#endif

	double dmaxPCPU = maxPCPU;

	double dcput = dt * dcpu / dmaxPCPU;
	server_rec *req_server;
	apr_status_t retval;
	req_server = srv;

	if (req_time < (1.0 / glbHtz)) {
		dcpu = 0.0;
	}

	if (dcpu < 0.0) {
		dcpu = 0.0;

	}

	performance_module_cfg *cfg = NULL;
	if (req_server) {
		cfg = ap_get_module_config(req_server->module_config,
				&performance_module);
	} else {
		cfg = ap_get_module_config(server->module_config, &performance_module);
	}

	float check_min_script;
	int need_to_write = 1;
	if (performance_min_exec_time != 123456789) {
		if (performance_min_exec_time < 0) {
			check_min_script = -performance_min_exec_time;
		} else {
			check_min_script = performance_min_exec_time;
		}
		check_min_script = check_min_script / (float) 100;
		if (req_time < check_min_script) {
			if (performance_min_exec_time < 0) {
				need_to_write = 0;
			} else {
				dcpu = 0.0;
			}
		}
	}

	if (need_to_write) {

		switch (log_type) {
		case 0: {
			BEGIN_SUB_POOL_SECTION(pool, _sub_pool);
			apr_size_t bytes_written;
			char *log_string;
			if (cfg && cfg->performance_logname && cfg->fd) {
				log_string = performance_log_format_get_string(_sub_pool,
						cfg->performance_log_format_parsed ?
								cfg->performance_log_format_parsed :
								default_performance_log_format_parsed,
						performance_get_current_time_as_string(_sub_pool),
						req_begin->uri, req_begin->hostname, req_begin->script,
						dcpu, dmem, dt, dcput, dmem_mb, dread, dwrite);
				log_string = apr_pstrcat(_sub_pool, log_string, "\n", NULL);

				bytes_written = strlen(log_string);
				retval = apr_file_write(cfg->fd, log_string, &bytes_written);
			}
			END_SUB_POOL_SECTION(pool, _sub_pool);
		}
			break;
		default: {
			sqlite3_save_request_info(pool, server, req_begin->hostname,
					req_begin->uri, req_begin->script, dt, dcpu, dmem, dcput,
					dmem_mb, dread, dwrite);
			break;
		}
		}
	}
}

static void performance_module_alarm(int sig) {
	if (sig == SIGALRM) {
		sqlite3_delete_request_info(root_pool, root_server,
				performance_history);
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, root_server, MODULE_PREFFIX
		"Deleting old performance items to trash was successfully");
		alarm(DELETE_INTERVAL);
	}
}

pid_t global_cur_pid = 0;

static void restar_daemon_interval(int sig, siginfo_t * si, void *uc) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, root_server, MODULE_PREFFIX
	" daemon will be restarted by scheduler. Next restart %s seconds",
			performance_check_extm ? performance_check_extm : "Ooops");
	daemon_should_exit++;
	;
}

static void restar_daemon_interval_val(int sig, siginfo_t * si, void *uc) {
	restar_daemon_interval(sig, si, uc);
	reset_timer(sig, performance_check_extm);
}

static void performance_db_defrag_action(int sig, siginfo_t * si, void *uc) {
	BEGIN_SUB_POOL_SECTION(pperf, _sub_pool);
	char *res = sql_adapter_optimize_table(_sub_pool, log_type, mutex_db);
	if (res) {
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, root_server,
		MODULE_PREFFIX "Data Base optimizing filed %s", res);
	} else {
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, root_server,
		MODULE_PREFFIX "Data Base optimizing successful");
	}
	END_SUB_POOL_SECTION(pperf, _sub_pool);
	reset_timer(sig, performance_db_defrag);
}

struct pollfd *fds = NULL;
nfds_t nfds;

void cleanup_sock(int shut, int s, int howmany) {
	if (shut) {
		shutdown(s, howmany);
	}
	close(s);
}

void performance_server_main_cycle(int l_sock, server_rec *main_server,
		apr_pool_t *pool) {
	int ret;
	int timeout = 1000;
	init_global_mem();
	struct sockaddr_un fsaun;
	socklen_t fromlen = (socklen_t) sizeof(fsaun);
	nfds = 1;
	fds = (struct pollfd *) calloc(1, nfds * sizeof(struct pollfd));
	fds->fd = l_sock;
	fds->events = POLLIN;

	write_debug_info("Thread listen connection started");

	while (!daemon_should_exit) {
		if (performance_usecustompool)
			debug_tid_pid_ut();
		else
			debug_tid_pid();
		static int iteration = 1;
		write_debug_info("Thread listen iteration %d", iteration++);
		int i;
		//Wait max 1 second
		ret = poll(fds, nfds, timeout);
		if (ret == -1) {
			write_debug_info("Poll error %d", ret);
			if (errno != EINTR) {
				ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
				MODULE_PREFFIX"Error polling on socket");
				return;
			}
			/*TODO подумать- а что если все-таки доберется до конца дескрипторов
			 * тогда жесть начинается. errno = EINTR и при SIGALRM*/
			//++daemon_should_exit;
			continue;
		}
		write_debug_info("Thread listen get nfds %d", nfds);
		for (i = 0; (i < nfds) && (ret); i++) {
			if (!(fds + i)->revents)
				continue;
			ret--;

			if (((fds + i)->fd == l_sock) && ((fds + i)->revents & POLLIN)) {
				/*
				 * Accept
				 */
				fds = (struct pollfd *) realloc(fds,
						(nfds + 1) * sizeof(struct pollfd));
				(fds + nfds)->fd = accept(l_sock, (struct sockaddr *) &fsaun,
						&fromlen);

				write_debug_info("Thread listen accept socket %d",
						(fds + nfds)->fd);

				if ((fds + nfds)->fd == -1) {
					ap_log_error(
					APLOG_MARK,
					APLOG_ERR,
					errno, main_server,
					MODULE_PREFFIX "Error on polling socket. Accepting error");

					cleanup_sock(0, (fds + nfds)->fd, 1);
					fds = (struct pollfd *) realloc(fds,
							nfds * sizeof(struct pollfd));
					continue;
				}
				(fds + nfds)->events = POLLIN;
				nfds++;
				continue;
			}

			//Read
			if ((fds + i)->revents & POLLIN) {
				write_debug_info("Thread listen accept socket %d - Read info",
						(fds + i)->fd);
				performance_module_send_req message;
				if ((fds + i)->revents & POLLHUP) {
					while (performance_read_data_from((fds + i)->fd,
							(void *) &message,
							sizeof(performance_module_send_req)) == APR_SUCCESS) {
						if (performance_usecustompool)
							add_item_to_list_ut(&message, (fds + i)->fd);
						else
							add_item_to_list(&message, (fds + i)->fd);
					}
				} else {
					if (performance_read_data_from((fds + i)->fd,
							(void *) &message,
							sizeof(performance_module_send_req)) == APR_SUCCESS) {
						if (performance_usecustompool)
							add_item_to_list_ut(&message, (fds + i)->fd);
						else
							add_item_to_list(&message, (fds + i)->fd);
					} else {
						write_debug_info(
								"Thread listen accept socket %d - Error",
								(fds + i)->fd);
						//remove_pid_tid_data_fd((fds + i)->fd);
						if (performance_usecustompool)
							add_item_to_removelist_ut((fds + i)->fd);
						else
							add_item_to_removelist((fds + i)->fd);
						cleanup_sock(1, (fds + i)->fd, 2);
						nfds--;
						memcpy(fds + i, fds + i + 1,
								(nfds - i) * sizeof(struct pollfd));
						fds = (struct pollfd *) realloc(fds,
								nfds * sizeof(struct pollfd));
						continue;
					}
				}

			}

			//Not open
			if ((fds + i)->revents & POLLNVAL) {
				write_debug_info(
						"Thread listen close socket %d - Descriptor is not open. Just remove it from array",
						(fds + i)->fd);
				//remove_pid_tid_data_fd((fds + i)->fd);
				if (performance_usecustompool)
					add_item_to_removelist_ut((fds + i)->fd);
				else
					add_item_to_removelist((fds + i)->fd);
				nfds--;
				memcpy(fds + i, fds + i + 1,
						(nfds - i) * sizeof(struct pollfd));
				fds = (struct pollfd *) realloc(fds,
						nfds * sizeof(struct pollfd));
				continue;
			}
			//Disconnect
			if ((fds + i)->revents & POLLHUP) {
				//remove_pid_tid_data_fd((fds + i)->fd);
				if (performance_usecustompool)
					add_item_to_removelist_ut((fds + i)->fd);
				else
					add_item_to_removelist((fds + i)->fd);
				write_debug_info("Thread listen accept socket %d - Disconnect",
						(fds + i)->fd);
				cleanup_sock(0, (fds + i)->fd, 2);
				nfds--;
				memcpy(fds + i, fds + i + 1,
						(nfds - i) * sizeof(struct pollfd));
				fds = (struct pollfd *) realloc(fds,
						nfds * sizeof(struct pollfd));
				continue;
			}
			//Error
			if ((fds + i)->revents & POLLERR) {
				write_debug_info("Thread listen accept socket %d - Error",
						(fds + i)->fd);
				//remove_pid_tid_data_fd((fds + i)->fd);
				if (performance_usecustompool)
					add_item_to_removelist_ut((fds + i)->fd);
				else
					add_item_to_removelist((fds + i)->fd);
				ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
				MODULE_PREFFIX "Error on polling socket. Error %d",
				errno);

				cleanup_sock(0, (fds + i)->fd, 2);
				nfds--;
				memcpy(fds + i, fds + i + 1,
						(nfds - i) * sizeof(struct pollfd));
				fds = (struct pollfd *) realloc(fds,
						nfds * sizeof(struct pollfd));
				continue;
			}

		}

	}
	if (performance_usecustompool)
		destroy_tid_pid_ut();
	else
		destroy_tid_pid();
	return;
}

static void *APR_THREAD_FUNC proceed_data_every_second(apr_thread_t * thd,
		void *data) {
	apr_pool_t *pool = (apr_pool_t *) data;
	double old_tm = 0.0, new_tm = 0.0;
	struct timespec cur_tm;
	while (!daemon_should_exit) {
		if (performance_usecustompool)
			clean_item_list_ut(root_server, pool);
		else
			clean_item_list(root_server, pool);
		usleep(100000);
		//Check all table of tids
		clock_gettime(CLOCK_REALTIME, &cur_tm);
		new_tm = cur_tm.tv_sec + (double) cur_tm.tv_nsec / (double) SEC2NANO;
		if (old_tm == 0.0)
			old_tm = new_tm;
		if (performance_usecustompool)
			prcd_function_ut(pool, &old_tm, new_tm);
		else
			prcd_function(pool, &old_tm, new_tm);

	}
	return NULL;
}

static void *APR_THREAD_FUNC proceed_data_every_second2(apr_thread_t * thd,
		void *data) {
	apr_pool_t *pool = (apr_pool_t *) data;
	double old_tm = 0.0, new_tm = 0.0;
	struct timespec cur_tm;
	while (!daemon_should_exit) {
		usleep(1000000);
		//Check all table of tids
		clock_gettime(CLOCK_REALTIME, &cur_tm);
		new_tm = cur_tm.tv_sec + (double) cur_tm.tv_nsec / (double) SEC2NANO;
		if (old_tm == 0.0)
			old_tm = new_tm;

		if (performance_usecustompool)
			prcd_function2_ut(pool, &old_tm, new_tm);
		else
			prcd_function2(pool, &old_tm, new_tm);

	}
	return NULL;
}

static int performance_module_server(void *data) {

	struct sockaddr_un unix_addr;
	int sd, rc;
	mode_t omask;
	//apr_socklen_t len;
	server_rec *main_server = data;
	apr_status_t rv;

	apr_signal(SIGCHLD, SIG_IGN);
	apr_signal(SIGHUP, daemon_signal_handler);

	ap_close_listeners();

	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "Couldn't create unix domain socket");
		return errno;
	}

	memset(&unix_addr, 0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	if (!performance_use_pid)
		performance_socket = performance_socket_no_pid;
	apr_cpystrn(unix_addr.sun_path, performance_socket,
			sizeof unix_addr.sun_path);

	omask = umask(0077);
	rc = bind(sd, (struct sockaddr *) &unix_addr, sizeof(unix_addr));
	umask(omask);

	if (performance_get_all_access())
		rv = apr_file_perms_set(performance_socket,
				APR_FPROT_UREAD | APR_FPROT_UWRITE | APR_FPROT_UEXECUTE
						| APR_FPROT_GREAD | APR_FPROT_GWRITE | APR_FPROT_WREAD
						| APR_FPROT_WWRITE);
	else
		rv = apr_file_perms_set(performance_socket,
		APR_FPROT_UREAD | APR_FPROT_UWRITE | APR_FPROT_UEXECUTE);
	if (performance_socket_perm) {
		rv = apr_file_perms_set(performance_socket, performance_socket_perm);
	}
	if (rv != APR_SUCCESS) {
		ap_log_error(APLOG_MARK, APLOG_CRIT, rv, main_server, MODULE_PREFFIX
		"Couldn't set permissions on unix domain socket %s",
				performance_socket);
		close(sd);
		return rv;
	}

	if (listen(sd, DEFAULT_PERF_LISTENBACKLOG) < 0) {
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "Couldn't listen on unix domain socket");
		close(sd);
		return errno;
	}

	if (!geteuid()) {
		if (chown(performance_socket, unixd_config.user_id, -1) < 0) {
			ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
			MODULE_PREFFIX
			"Couldn't change owner of unix domain socket %s",
					performance_socket);
			close(sd);
			return errno;
		}
	}

	unixd_setup_child();

	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
	MODULE_PREFFIX "Performance daemon started successfully");
	if (log_type) {
		apr_signal(SIGALRM, performance_module_alarm);
		alarm(DELETE_INTERVAL);
		sqlite3_delete_request_info(pperf, main_server, performance_history);
	}

	//Устанавливаем таймер на перезапуск демона? если он установлен
	if (performance_check_extm) {
		long get_interval = gettimeinterval(performance_check_extm);
		if (!get_interval) {
			get_interval = apr_atoi64(performance_check_extm);
			if (get_interval) {
				//Задан интревал перезапуска
				addtimer(pperf, get_interval, 0, &restar_daemon_interval);
			}
		} else {
			//Задано время ежедневного запуска
			addtimer(pperf, get_interval, 1, &restar_daemon_interval_val);
		}

	}

	if (performance_db_defrag && (log_type == 2)) {
		long get_interval = gettimeinterval(performance_db_defrag);
		if (get_interval) {
			ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
			MODULE_PREFFIX "Optimize mode enabled");
			addtimer(pperf, get_interval, 1, &performance_db_defrag_action);
		}
	}

	apr_threadattr_t *thd_attr = NULL;
	apr_thread_t *thd_arr = NULL;
	apr_threadattr_create(&thd_attr, pperf);
	apr_threadattr_t *thd_attr2 = NULL;
	apr_thread_t *thd_arr2 = NULL;
	apr_threadattr_create(&thd_attr2, pperf);

	if (performance_usecustompool)
		init_tid_pid_ut(pperf);
	else
		init_tid_pid(pperf);

	rv = apr_thread_create(&thd_arr, thd_attr, proceed_data_every_second, pperf,
			pperf);
	if (rv != APR_SUCCESS) {
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
		MODULE_PREFFIX "Can't start memory watcher");
	} else {
		rv = apr_thread_create(&thd_arr2, thd_attr2, proceed_data_every_second2,
				pperf, pperf);
		if (rv != APR_SUCCESS) {
			ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
			MODULE_PREFFIX "Can't start counters saver");
			daemon_should_exit = 1;
			rv = apr_thread_join(&rv, thd_arr);
		} else {
			performance_server_main_cycle(sd, main_server, pperf);
			daemon_should_exit = 1;
			rv = apr_thread_join(&rv, thd_arr);
			rv = apr_thread_join(&rv, thd_arr2);
		}
	}

	apr_thread_mutex_destroy(mutex_db);
	close(sd);

	return -1;
}

#if APR_HAS_OTHER_CHILD
static void performance_module_maint(int reason, void *data, apr_wait_t status) {
	apr_proc_t *proc = data;
	int mpm_state;
	int stopping;

	switch (reason) {
	case APR_OC_REASON_DEATH:
		apr_proc_other_child_unregister(data);
		stopping = 1;
		if (ap_mpm_query(AP_MPMQ_MPM_STATE, &mpm_state) == APR_SUCCESS
				&& mpm_state != AP_MPMQ_STOPPING) {
			stopping = 0;
		}
		if (!stopping) {
			if (status == DAEMON_STARTUP_ERROR) {
				ap_log_error(APLOG_MARK, APLOG_CRIT, errno, NULL,
				MODULE_PREFFIX "daemon failed to initialize");
			} else {
				ap_log_error(APLOG_MARK, APLOG_ERR, errno, NULL,
				MODULE_PREFFIX "daemon process died, restarting");
				performance_module_start(root_pool, root_server, proc);
			}
		}
		break;
	case APR_OC_REASON_RESTART:
		apr_proc_other_child_unregister(data);
		break;
	case APR_OC_REASON_LOST:
		apr_proc_other_child_unregister(data);
		performance_module_start(root_pool, root_server, proc);
		break;
	case APR_OC_REASON_UNREGISTER:
		kill(proc->pid, SIGHUP);
		if (unlink(
				performance_use_pid ?
						performance_socket : performance_socket_no_pid)
				< 0&& errno != ENOENT) {
			ap_log_error(
			APLOG_MARK,
			APLOG_ERR,
			errno,
			NULL,
			MODULE_PREFFIX
			"Couldn't unlink unix domain socket %s",
					performance_use_pid ?
							performance_socket : performance_socket_no_pid);
		}
		break;
	}
}
#endif

static int performance_module_start(apr_pool_t * p, server_rec * main_server,
		apr_proc_t * procnew) {
	apr_status_t rv;
	daemon_should_exit = 0;
	rv = apr_proc_fork(&daemon_proc, main_server->process->pool);
	if (rv == APR_INCHILD) { /* child */
		if (pperf == NULL) {
			apr_pool_create(&pperf, p);
			//ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
			//		MODULE_PREFFIX "Created pool %pp", pperf);
		}
		exit(performance_module_server(main_server) > 0 ?
		DAEMON_STARTUP_ERROR :
															-1);
	} else if (rv == APR_INPARENT) {
#if APR_HAS_OTHER_CHILD
		if (!procnew) {
			procnew = apr_pcalloc(main_server->process->pool, sizeof(*procnew));
		}
		procnew->pid = daemon_proc.pid;
		procnew->err = procnew->in = procnew->out = NULL;
		apr_pool_note_subprocess(p, procnew, APR_KILL_AFTER_TIMEOUT);
		apr_proc_other_child_register(procnew, performance_module_maint,
				procnew, NULL, p);
#endif
	} else {
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "couldn't spawn daemon process");
		return DECLINED;
	}

	return OK;
}

static int performance_module_init(apr_pool_t * p, apr_pool_t * plog,
		apr_pool_t * ptemp, server_rec * main_server) {
	int return_value = OK, rc;
	const char *userdata_key = "performance_module_init";
	int first_time = 0;
	int *pDummy;
	apr_status_t rv;

	root_server = main_server;
	root_pool = p;

	performance_module_cfg *cfg = ap_get_module_config(
			main_server->module_config, &performance_module);
	performance_db = ap_server_root_relative(p, performance_db);

	if (log_type) {

		if (sql_adapter_load_library(p, log_type) < 0) {
			rc = 1;
			ap_log_error(APLOG_MARK, APLOG_WARNING, errno, main_server,
			MODULE_PREFFIX
			"Can't load db library");
		} else {

			if (sql_adapter_connect_db(p, log_type, performance_dbhost,
					performance_username, performance_password,
					performance_dbname, performance_db) < 0) {
				rc = 1;
				ap_log_error(APLOG_MARK, APLOG_WARNING, errno, main_server,
				MODULE_PREFFIX
				"Can't connect to db");
			} else
				rc = OK;
		}
	} else {
		rc = OK;
	}
	if (rc) {
		cfg->performance_enabled = 0;
		ap_log_error(APLOG_MARK, APLOG_WARNING, errno, main_server,
		MODULE_PREFFIX
		"Internal error, performance module will be disabled :(");
	} else {

		if (log_type == 1) {
			rv = apr_file_perms_set(performance_db,
			APR_FPROT_UREAD | APR_FPROT_UWRITE | APR_FPROT_UEXECUTE);
			if (rv != APR_SUCCESS) {
				ap_log_error(APLOG_MARK, APLOG_CRIT, rv, main_server,
				MODULE_PREFFIX
				"Couldn't set permissions on database file %s", performance_db);
				cfg->performance_enabled = 0;
			}
			if (!geteuid()) {
				if (chown(performance_db, unixd_config.user_id, -1) < 0) {
					ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
					MODULE_PREFFIX
					"Couldn't change owner of database file %s",
							performance_db);
					cfg->performance_enabled = 0;
				}
			}
		}

		apr_status_t rv_tmp = apr_thread_mutex_create(&mutex_db, APR_THREAD_MUTEX_DEFAULT,
				main_server->process->pool);
		if (rv_tmp != APR_SUCCESS){
			cfg->performance_enabled = 0;
			ap_log_error(APLOG_MARK, APLOG_WARNING, errno, main_server,
				MODULE_PREFFIX
				"Internal error(can't create db mutex), performance module will be disabled :(");
		}

		if (log_type) {
			if (!sqlite3_check_for_tables(p, main_server)) {
				cfg->performance_enabled = 0;
			}
		}
	}

#if defined(__FreeBSD__)
	if (init_freebsd_info ())
	{
		cfg->performance_enabled = 0;
		ap_log_error (APLOG_MARK, APLOG_ERR, errno, main_server,
				MODULE_PREFFIX "Init FreeBSD info failed");
	}
	glbHtz = (unsigned long long) getstathz ();

#endif

#if defined(linux)
	glbHtz = init_libproc();
#endif

	if (glbHtz == (unsigned long long) -1) {
		cfg->performance_enabled = 0;
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "CPU HZ identification failed");
	}

#if defined(linux)
	if (glibtop_init_own() < 0) {
		cfg->performance_enabled = 0;
		ap_log_error(APLOG_MARK, APLOG_ERR, errno, main_server,
		MODULE_PREFFIX "pseudo glib_top_init failed");
	}
#endif

	apr_pool_userdata_get((void **) &pDummy, userdata_key,
			main_server->process->pool);
	if (!pDummy) {
		pDummy = apr_palloc(main_server->process->pool, sizeof(int));
		*pDummy = 1;
		apr_pool_userdata_set((void *) pDummy, userdata_key,
				apr_pool_cleanup_null, main_server->process->pool);
		first_time = 1;
	}

	inittimer();

	if (!first_time) {
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
		MODULE_PREFFIX "version " PERFORMANCE_MODULE_VERSION
		" loaded");
	}

	if (cfg->performance_enabled) {
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
		MODULE_PREFFIX "module enabled :)");

		if (!first_time) {
			parent_pid = getpid();
			performance_socket = ap_server_root_relative(p, performance_socket);
			performance_socket_no_pid = ap_server_root_relative(p,
					performance_socket_no_pid);
			return_value = performance_module_start(p, main_server, NULL);
			if (return_value != OK) {
				return return_value;
			}
			ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
			MODULE_PREFFIX "server started :)");
		}
	} else {
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
		MODULE_PREFFIX "module disabled :(");
	}

	ap_mpm_query(AP_MPMQ_HARD_LIMIT_THREADS, &thread_limit);
	ap_mpm_query(AP_MPMQ_HARD_LIMIT_DAEMONS, &server_limit);

	if (template_path) {
		custom_report_parse_reports(template_path, p);
	}

	if (registered_filter) {
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
				MODULE_PREFFIX " use filter");
	} else {
		ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, main_server,
				MODULE_PREFFIX " use log hook");
	}

	return return_value;
}

static int match_hostname(apr_array_header_t * allowed_hosts,
		const char *hostname) {
	int num_names;
	char **names_ptr;
	int accept = 0;
	if (!hostname)
		return accept;
	if (allowed_hosts) {
		names_ptr = (char **) allowed_hosts->elts;
		num_names = allowed_hosts->nelts;
	} else {
		return accept;
	}
	// match handle
	for (; num_names; ++names_ptr, --num_names) {
		if (strstr(hostname, *names_ptr)) {
			accept = 1;
			break;
		}
	}
	return accept;
}

// returns 1 if found, 0 if not
static int match_script(apr_array_header_t * scripts_list,
		const char *script_name) {
	int num_names;
	char **names_ptr;

#ifndef APACHE2_0
	ap_regex_t compiled_regex;
#else
	regex_t compiled_regex;
#endif

	if (scripts_list && script_name) {
		names_ptr = (char **) scripts_list->elts;
		num_names = scripts_list->nelts;

		for (; num_names; ++names_ptr, --num_names) {
			if (!strcmp("*", *names_ptr))
				return 1;

#ifndef APACHE2_0
			if (ap_regcomp(&compiled_regex, *names_ptr,
			AP_REG_EXTENDED | AP_REG_NOSUB))
				continue;

			if (!ap_regexec(&compiled_regex, script_name, 0, NULL, 0)) {
				ap_regfree(&compiled_regex);
				return 1;
			} else {
				ap_regfree(&compiled_regex);
				continue;
			}
#else

			if (regcomp (&compiled_regex, *names_ptr, REG_EXTENDED | REG_NOSUB))
			continue;

			if (!regexec (&compiled_regex, script_name, 0, NULL, 0))
			{
				regfree (&compiled_regex);
				return 1;
			}
			else
			{
				regfree (&compiled_regex);
				continue;
			}

#endif

		}
	}

	return 0;
}

static int own_connect(int socket, struct sockaddr *addr, socklen_t length) {
	errno = 0;
	if (!performance_blocksave) {
		int rt_code, rt_code2;
		rt_code = fcntl(socket, F_GETFL, 0);
		rt_code2 = fcntl(socket, F_SETFL, rt_code | O_NONBLOCK);

		fd_set set;
		int ret = 0;
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 100000;
		FD_ZERO(&set);
		FD_SET(socket, &set);

		if ((ret = connect(socket, (struct sockaddr *) addr, length)) == -1) {
			if ( errno != EINPROGRESS)
				return -1;

		} else {
			return ret;
		}
		ret = select(socket + 1, NULL, &set, NULL, &timeout);
		return (ret > 0) ? ret : -1;
	} else {
		return connect(socket, (struct sockaddr *) addr, length);
	}

}

static int connect_to_daemon(intptr_t * sdptr, request_rec * r) {
	struct sockaddr_un unix_addr;
	intptr_t sd;
	int connect_tries;
	apr_interval_time_t sliding_timer;

	memset(&unix_addr, 0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	apr_cpystrn(unix_addr.sun_path,
			performance_use_pid ?
					performance_socket : performance_socket_no_pid,
			sizeof unix_addr.sun_path);

	connect_tries = 0;
	sliding_timer = 100000; /* 100 milliseconds */
	while (1) {
		++connect_tries;
		if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
			//ap_log_rerror (APLOG_MARK, APLOG_ERR, errno, r,
			//		 MODULE_PREFFIX "unable to create socket to daemon");
			if (!performance_silent_mode)
				ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
				MODULE_PREFFIX"(host %s) unable to create socket to daemon",
						get_host_name(r));
			//return HTTP_INTERNAL_SERVER_ERROR;
			return DECLINED;
		}
		if (own_connect(sd, (struct sockaddr *) &unix_addr, sizeof(unix_addr))
				< 0) {
			if (errno == ECONNREFUSED
					&& connect_tries < DEFAULT_CONNECT_ATTEMPTS) {
				//ap_log_rerror (APLOG_MARK, APLOG_ERR, errno, r,
				//	     MODULE_PREFFIX
				//	     "connect #%d to daemon failed, sleeping before retry",
				//             connect_tries);
				if (!performance_silent_mode)
					ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
							MODULE_PREFFIX
							"(host %s) connect #%d to daemon failed, sleeping before retry",
							get_host_name(r), connect_tries);
				close(sd);
				/*apr_sleep (sliding_timer);
				 if (sliding_timer < apr_time_from_sec (2))
				 {
				 sliding_timer *= 2;
				 }*/
			} else {
				close(sd);
				//ap_log_rerror (APLOG_MARK, APLOG_ERR, errno, r,
				//	     MODULE_PREFFIX
				//	     "unable to connect to daemon after multiple tries");
				if (!performance_silent_mode)
					ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
							MODULE_PREFFIX
							"(host %s) unable to connect to daemon after multiple tries",
							get_host_name(r));
				//return HTTP_SERVICE_UNAVAILABLE;
				return DECLINED;
			}
		} else {
			break;
		}
		if (kill(daemon_proc.pid, 0) != 0) {
			//ap_log_rerror (APLOG_MARK, APLOG_ERR, errno, r,
			//		 MODULE_PREFFIX
			//		 "daemon is gone; is Apache terminating?");
			if (!performance_silent_mode)
				ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
				MODULE_PREFFIX
				"(host %s) daemon is gone; is Apache terminating?",
						get_host_name(r));
			//return HTTP_SERVICE_UNAVAILABLE;
			return DECLINED;
		}
	}
	*sdptr = sd;
	return OK;
}

void form_page_content(request_rec * r, int admin) {
	char *js = performance_module_get_parameter_from_uri(r, "js");
	if (js) {
//Выведем код ява-скрипта
//никаких больше параметров
		print_js_content(r);
		return;
	}
	char *pic = performance_module_get_parameter_from_uri(r, "pic");
	if (pic) {
//Это картинка, проверим другие параметры
		char *info = performance_module_get_parameter_from_uri(r, "info");
		int info_number = (int) apr_atoi64(info ? info : "0");
		switch (info_number) {
		case 9:
			performance_module_show_graph_page_memory(r, admin);
			break;
		case 10:
			performance_module_show_graph_page_ioread(r, admin);
			break;
		case 17:
			performance_module_show_graph_page_iowrite(r, admin);
			break;
		case 11:
			performance_module_show_host_graph_page(r, admin);
			break;
		case 13:
			performance_module_show_host_average_graph_cpu(r, admin);
			break;
		case 14:
			performance_module_show_host_average_graph_mem(r, admin);
			break;
		case 15:
			performance_module_show_host_average_graph_ioread(r, admin);
			break;
		case 16:
			performance_module_show_host_average_graph_iowrite(r, admin);
			break;
		default:
			performance_module_show_graph_page_cpu(r, admin);
		}
		return;
	}
	char *info_page = performance_module_get_parameter_from_uri(r, "info");
	int info_number_page = (int) apr_atoi64(info_page ? info_page : "-1");
	switch (info_number_page) {
	case 0:
		performance_module_show_index_no_graph_page(r, admin);
		break;
	case 1:
		performance_module_show_max_no_graph_page(r, admin);
		break;
	case 2:
		performance_module_show_host_no_graph_page(r, admin);
		break;
	case 3:
		performance_module_show_max_mem_no_graph_page(r, admin);
		break;
	case 4:
		performance_module_show_max_time_no_graph_page(r, admin);
		break;
	case 7:
		performance_module_show_host_average_no_graph_page(r, admin);
		break;
	case 8 ... 17:
		performance_module_show_graph_page(r, admin);
		break;
	case 20:
		performance_module_show_exec_range_no_graph_page(r, admin);
		break;
	case 30:
		performance_module_show_max_no_graph_page_no_hard(r, admin);
		break;
	case 31:
		performance_module_show_max_mem_no_graph_page_no_hard(r, admin);
		break;
	case 32:
		performance_module_show_max_time_no_graph_page_no_hard(r, admin);
		break;
	case 33:
		performance_module_show_exec_range_no_graph_page_common(r, admin);
		break;
	case 100:
		performance_module_show_index_no_graph_page(r, admin);
		break;
	default:
		if (info_number_page >= 500) {
			custom_report_item *itm = custom_report_get_repot_item(
					info_number_page - 500);
			if (itm)
				performance_module_custom_report_no_graph_page(r, admin, itm,
						log_type);
			else
				performance_module_show_index_no_graph_page_no_data(r, admin);
		} else
			performance_module_show_index_no_graph_page_no_data(r, admin);
	}

}

static int performance_module_handler(request_rec * r) {

	performance_module_cfg *cfg = performance_module_sconfig(r);
	intptr_t sd = 0;

	write_debug_info("Proceed handler %s - PID %d, TID %d",
			r->handler ? r->handler : "NULL", getpid(), gettid());

	int need_proceed = 0;

	if (r->header_only) {
		return DECLINED;
	}

	if (log_type && cfg->performance_enabled) {

		if (!apr_strnatcmp(r->handler, cfg->performance_work_handler)) {
			form_page_content(r, 0);

			return OK;
		}

		if (!apr_strnatcmp(r->handler, cfg->performance_user_handler)) {

			form_page_content(r, 1);
			return OK;
		}
	}

	if (!r->hostname)
		return DECLINED;

	if (cfg->performance_enabled) {

		if (cfg->performance_host_filter || cfg->performance_uri
				|| cfg->performance_script) {
			if (cfg->performance_host_filter) {
				if (match_hostname(cfg->performance_host_filter, r->hostname))
					need_proceed = 1;
				else
					need_proceed = -1;
			}

			if (cfg->performance_uri) {
				if (need_proceed != -1) {
					if (match_script(cfg->performance_uri, r->uri))
						need_proceed = 1;
					else
						need_proceed = -1;
				}
			}

			if (cfg->performance_script) {
				if (need_proceed != -1) {
					if (match_script(cfg->performance_script, r->filename))
						need_proceed = 1;
					else
						need_proceed = -1;
				}
			}

		}

		if (need_proceed == -1) {
			need_proceed = 0;
		}

	}

	if ((cfg->performance_use_cononical_name)
			&& (!apr_strnatcmp(r->handler, "redirect-handler"))
			&& (need_proceed)) {
		need_proceed = 0;
	}

	write_debug_info("Proceed handler %s - PID %d, TID %d need prcd %d",
			r->handler ? r->handler : "NULL", getpid(), gettid(), need_proceed);
	if (need_proceed) {
		if(registered_filter) {
			ap_add_output_filter("MODPERF_RESULT_FILTER", NULL, r, r->connection);
		}
		int retval;

		if ((retval = connect_to_daemon(&sd, r)) != OK) {
			write_debug_info(
					"Proceed handler %s - PID %d, TID %d Error daemon connection",
					r->handler ? r->handler : "NULL", getpid(), gettid());
			return retval;
		}
		apr_threadkey_private_set((void *) sd, key);
		sd_global = sd;

		performance_module_send_req *req = apr_palloc(r->pool,
				sizeof(performance_module_send_req));

		modperformance_sendbegin_info_send_info(req, r->uri, r->filename,
				r->server ? r->server->server_hostname : "", (char *) r->method,
				r->args, r->canonical_filename, use_tid, r->server, r->pool,
				cfg->performance_use_cononical_name);

		write_debug_info(
				"Proceed handler %s - PID %d, TID %d Sending data %s FD %d",
				r->handler ? r->handler : "NULL", getpid(), gettid(), req->uri,
				sd);
		if (performance_send_data_to(sd, (const void *) req,
				sizeof(performance_module_send_req)) != APR_SUCCESS) {
			if (!performance_silent_mode)
				ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
						MODULE_PREFFIX
						"(host %s) can't send begin info for daemon ERRNO %d, HOSTNAME %s, URI %s",
						get_host_name(r),
						errno, r->hostname, r->uri);
			write_debug_info(
					"Proceed handler %s - PID %d, TID %d Sending data %s error FD %d",
					r->handler ? r->handler : "NULL", getpid(), gettid(),
					req->uri, sd);
		}

	}

	return DECLINED;
}

static apr_status_t ap_modperf_out_filter(ap_filter_t *f,
		apr_bucket_brigade *in) {
	request_rec * r = f->r;
	intptr_t sd = sd_global;

	write_debug_info("Proceed handler %s - PID %d, TID %d End cicle FD %d",
			r->handler ? r->handler : "NULL", getpid(), gettid(), sd);
	if (sd) {

		performance_module_send_req *req = apr_palloc(r->pool,
				sizeof(performance_module_send_req));

		modperformance_sendend_info_send_info(req, "", "", (char *) "", "",
				use_tid, r->server, r->pool);

		write_debug_info(
				"Proceed handler %s - PID %d, TID %d End cicle FD %d %s",
				r->handler ? r->handler : "NULL", getpid(), gettid(), sd,
				req->uri);

		if (performance_send_data_to(sd, (const void *) req,
				sizeof(performance_module_send_req)) != APR_SUCCESS) {

			if (!performance_silent_mode)
				ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
				MODULE_PREFFIX
				"(host %s) can't send end info for daemon ERRNO %d",
						get_host_name(r),
						errno);
			write_debug_info(
					"Proceed handler %s - PID %d, TID %d End error FD %d %s",
					r->handler ? r->handler : "NULL", getpid(), gettid(), sd,
					req->uri );
		}
		write_debug_info(
				"Proceed handler %s - PID %d, TID %d End cicle ok FD %d %s",
				r->handler ? r->handler : "NULL", getpid(), gettid(), sd,
				req->uri);

		shutdown(sd, SHUT_RDWR);
		close(sd);
		sd = 0;

	}

	ap_remove_output_filter(f);
	return ap_pass_brigade(f->next, in);

}

static int performance_module_leave_handler(request_rec * r) {
	intptr_t sd;

	if(registered_filter) {
	      return DECLINED;
	}


	apr_threadkey_private_get((void *) &sd, key);
	write_debug_info("Proceed handler %s - PID %d, TID %d End cicle FD %d",
			r->handler ? r->handler : "NULL", getpid(), gettid(), sd);
	if (sd) {

		performance_module_send_req *req = apr_palloc(r->pool,
				sizeof(performance_module_send_req));

		modperformance_sendend_info_send_info(req, r->uri,
				r->server ? r->server->server_hostname : "", (char *) r->method,
				r->args, use_tid, r->server, r->pool);

		write_debug_info(
				"Proceed handler %s - PID %d, TID %d End cicle FD %d %s",
				r->handler ? r->handler : "NULL", getpid(), gettid(), sd,
				req->uri);

		if (performance_send_data_to(sd, (const void *) req,
				sizeof(performance_module_send_req)) != APR_SUCCESS) {
			//ap_log_rerror (APLOG_MARK, APLOG_ERR, errno, r,
			//	 MODULE_PREFFIX
			//	 "can't send end info for daemon ERRNO %d", errno);
			if (!performance_silent_mode)
				ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
				MODULE_PREFFIX
				"(host %s) can't send end info for daemon ERRNO %d",
						get_host_name(r),
						errno);
			write_debug_info(
					"Proceed handler %s - PID %d, TID %d End error FD %d %s",
					r->handler ? r->handler : "NULL", getpid(), gettid(), sd,
					req->uri);
		}
		write_debug_info(
				"Proceed handler %s - PID %d, TID %d End cicle ok FD %d %s",
				r->handler ? r->handler : "NULL", getpid(), gettid(), sd,
				req->uri);

		shutdown(sd, SHUT_RDWR);
		close(sd);

	}
	apr_threadkey_private_set(NULL, key);
	return DECLINED;
}

static apr_status_t performance_module_destroy_key_pool(void *dummy) {
	apr_threadkey_private_delete(key);
	return APR_SUCCESS;
}

static void performance_module_child_init(apr_pool_t * p, server_rec * s) {
	apr_pool_cleanup_register(p, 0, performance_module_destroy_key_pool,
			performance_module_destroy_key_pool);
	apr_threadkey_private_create(&key, NULL, p);

	child_pid = getpid();
}

static int performance_open_log(server_rec * s, apr_pool_t * p) {
	performance_module_cfg *cfg = ap_get_module_config(s->module_config,
			&performance_module);

	if (!cfg->performance_logname)
		return 1;

	if (*cfg->performance_logname == '|') {
		piped_log *pl;
		const char *pname = ap_server_root_relative(p,
				cfg->performance_logname + 1);

		pl = ap_open_piped_log(p, pname);
		if (pl == NULL) {
			ap_log_error(APLOG_MARK, APLOG_ERR, 0, s, MODULE_PREFFIX
			" couldn't spawn Performance log pipe %s",
					cfg->performance_logname);
			return 0;
		}
		cfg->fd = ap_piped_log_write_fd(pl);
	} else {
		const char *fname = ap_server_root_relative(p,
				cfg->performance_logname);
		apr_status_t rv;

		if ((rv = apr_file_open(&cfg->fd, fname,
		APR_WRITE | APR_APPEND | APR_CREATE, APR_OS_DEFAULT, p)) != APR_SUCCESS) {
			ap_log_error(APLOG_MARK, APLOG_ERR, rv, s, MODULE_PREFFIX
			" could not open Performance log file %s.", fname);
			return 0;
		}
	}

	return 1;
}

static int performance_log_init(apr_pool_t * pc, apr_pool_t * p,
		apr_pool_t * pt, server_rec * s) {
	for (; s; s = s->next) {
		if (!performance_open_log(s, p)) {
			return HTTP_INTERNAL_SERVER_ERROR;
		}
	}

	return OK;
}

//--------Список экспортируемых функций----------------------

APR_DECLARE_OPTIONAL_FN(void, send_begininfo_to_daemon,
		(request_rec * r, pid_t pid, int *sd));
APR_DECLARE_OPTIONAL_FN(void, send_endinfo_to_daemon,
		(request_rec * r, pid_t pid, int *sd));
APR_DECLARE_OPTIONAL_FN(int, match_external_handlers, (request_rec * r));

void send_begininfo_to_daemon(request_rec * r, pid_t pid, int *sd) {

	if(!r || !r->server) return;

	performance_module_cfg *cfg = performance_module_sconfig(r);

	if (!*sd) {
		if (connect_to_daemon((intptr_t *) sd, r) != OK) {
			*sd = 0;
		}
	}

	performance_module_send_req *req = apr_palloc(r->pool,
			sizeof(performance_module_send_req));

	modperformance_sendbegin_info_send_info(req, r->uri, r->filename,
			r->server->server_hostname, (char *) r->method,
			r->args, r->canonical_filename, 0, r->server, r->pool,
			cfg->performance_use_cononical_name);

	if (performance_send_data_to(*sd, (const void *) req,
			sizeof(performance_module_send_req)) != APR_SUCCESS) {
//ap_log_rerror (APLOG_MARK, APLOG_ERR, errno, r,
//	     MODULE_PREFFIX
//	     "can't send begin info for daemon ERRNO %d, HOSTNAME %s, URI %s from exported function",
//	     errno, r->hostname, r->uri);
		if (!performance_silent_mode)
			ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
					MODULE_PREFFIX
					"(host %s) can't send begin info for daemon ERRNO %d, HOSTNAME %s, URI %s from exported function",
					get_host_name(r),
					errno, r->hostname, r->uri);
	}
}

void send_endinfo_to_daemon(request_rec * r, pid_t pid, int *sd) {

	if (*sd) {

		performance_module_send_req *req = apr_palloc(r->pool,
				sizeof(performance_module_send_req));

		modperformance_sendend_info_send_info(req, r->uri,
				r->server ? r->server->server_hostname : "", (char *) r->method,
				r->args, 0, r->server, r->pool);

		if (performance_send_data_to(*sd, (const void *) req,
				sizeof(performance_module_send_req)) != APR_SUCCESS) {
			//ap_log_rerror (APLOG_MARK, APLOG_ERR, errno, r,
			//	 MODULE_PREFFIX
			//	 "can't send end info for daemon ERRNO %d from exported function",
			//	 errno);
			if (!performance_silent_mode)
				ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool,
						MODULE_PREFFIX
						"(host %s) can't send end info for daemon ERRNO %d from exported function",
						get_host_name(r),
						errno);
		}

		shutdown(*sd, SHUT_RDWR);
		close(*sd);

	}
}

int match_external_handlers(request_rec * r) {
	if (r->filename) {
		performance_module_cfg *cfg = performance_module_sconfig(r);
		if (cfg->performance_external_handlers) {
			return match_script(cfg->performance_external_handlers, r->filename);
		}
	}
	return 0;
}

//--------Список экспортируемых функций----------------------

static void register_hook(apr_pool_t * p) {
	static const char * const aszPre[] = { "mod_include.c", "mod_php.c",
			"mod_cgi.c", "mod_fcgid.c", NULL };

	ap_hook_pre_config(performance_module_pre_config, NULL, NULL,
	APR_HOOK_MIDDLE);
	ap_hook_post_config(performance_module_init, aszPre, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(performance_module_handler, NULL, aszPre,
	APR_HOOK_REALLY_FIRST);
	ap_hook_child_init(performance_module_child_init, NULL, NULL,
	APR_HOOK_MIDDLE);
	ap_hook_log_transaction(performance_module_leave_handler, NULL, NULL,
	APR_HOOK_LAST);
	ap_hook_open_logs(performance_log_init, NULL, NULL, APR_HOOK_MIDDLE);

	ap_register_output_filter("MODPERF_RESULT_FILTER", ap_modperf_out_filter,
	NULL, AP_FTYPE_CONTENT_SET);

	APR_REGISTER_OPTIONAL_FN(send_begininfo_to_daemon);
	APR_REGISTER_OPTIONAL_FN(send_endinfo_to_daemon);
	APR_REGISTER_OPTIONAL_FN(match_external_handlers);
}

static const char *
set_performance_module_enabled_extended(cmd_parms * cmd, void *dummy,
		const char *arg) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - PerformanceExtended");
	return NULL;
}

static const char *
set_performance_module_check_daemon(cmd_parms * cmd, void *dummy,
		const char *arg) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - PerformanceCheckDaemon");
	return NULL;
}

static const char *
set_performance_module_periodical_watch(cmd_parms * cmd, void *dummy,
		const char *arg) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - PerformancePeriodicalWatch");
	return NULL;
}

static const char *
set_performance_module_check_daemon_interval(cmd_parms * cmd, void *dummy,
		const char *arg) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - PerformanceCheckDaemonInterval");
	return NULL;
}

static const char *
set_performance_module_check_daemon_memory(cmd_parms * cmd, void *dummy,
		const char *arg) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - PerformanceCheckDaemonMemory");
	return NULL;
}

static const char *
set_performance_module_stacksize(cmd_parms * cmd, void *dummy, const char *arg) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - PerformanceStackSize");
	return NULL;
}

static const char *
set_performance_module_maxthreads(cmd_parms * cmd, void *dummy, const char *arg) {
	ap_log_error(APLOG_MARK, APLOG_NOTICE, 0, cmd->server, MODULE_PREFFIX
	"Deprecated parameter - PerformanceMaxThreads");
	return NULL;
}

static const char *
set_perf_work_mode(cmd_parms * cmd, void *mcfg, const char *arg) {
	const char *err = ap_check_cmd_context(cmd,
	GLOBAL_ONLY);
	if (err != NULL) {
		return err;
	}
	if (!apr_strnatcasecmp(arg, "Filter")) {
		registered_filter = 1;
	}

	return NULL;
}

static const command_rec performance_module_cmds[] =
		{
		AP_INIT_TAKE1 ("PerformanceSocket", set_performance_module_socket, NULL,
				RSRC_CONF,
				"the name of the socket to use for communication with "
				"the daemon."),
		AP_INIT_TAKE1("PerformanceEnabled",
				set_performance_module_enabled, NULL, RSRC_CONF,
				"Performance mode enable"),
		AP_INIT_TAKE1("PerformanceDB", set_performance_module_db, NULL,
				RSRC_CONF, "the name of the db file"),
		AP_INIT_TAKE1("PerformanceHistory",
				set_performance_module_history, NULL, RSRC_CONF,
				"history keeping time"),
		AP_INIT_TAKE1 ("PerformanceWorkHandler",
				set_performance_module_work_handler, NULL, RSRC_CONF,
				"Statistics output handler"),
		AP_INIT_TAKE1 ("PerformanceUserHandler",
				set_performance_module_user_handler, NULL, RSRC_CONF,
				"Statistics output handler for users"),
		AP_INIT_ITERATE ("PerformanceHostFilter",
				set_performance_module_host_filter, NULL, RSRC_CONF,
				"hostnames list"),
		AP_INIT_ITERATE ("PerformanceURI",
				set_performance_module_uri, NULL, RSRC_CONF,
				"URIs regexp list"),
		AP_INIT_ITERATE ("PerformanceScript",
				set_performance_module_script, NULL, RSRC_CONF,
				"scripts regexp list"),
		AP_INIT_TAKE1 ("PerformanceUseCanonical",
				set_performance_module_enabled_canonical, NULL,
				RSRC_CONF,
				"Performance log canonical file name instead standard"),
		AP_INIT_TAKE1 ("PerformanceLog",
				set_performance_module_log, NULL,
				RSRC_CONF,
				"Use single log file for statistic"),
		AP_INIT_TAKE1 ("PerformanceLogFormat",
				set_performance_module_log_format, NULL,
				RSRC_CONF,
				"Set log format"),
		AP_INIT_TAKE1 ("PerformanceLogType",
				set_performance_module_log_type, NULL,
				RSRC_CONF,
				"Set log type"),
		AP_INIT_TAKE1 ("PerformanceDbUserName",
				set_performance_module_username, NULL,
				RSRC_CONF,
				"Set db user name"),
		AP_INIT_TAKE1 ("PerformanceDBPassword",
				set_performance_module_password, NULL,
				RSRC_CONF,
				"Set db password"),
		AP_INIT_TAKE1 ("PerformanceDBName",
				set_performance_module_db_name, NULL,
				RSRC_CONF,
				"Set db name"),
		AP_INIT_TAKE1 ("PerformanceDBHost",
				set_performance_module_db_host, NULL,
				RSRC_CONF,
				"Set db host"),
		AP_INIT_TAKE1 ("PerformanceWorkMode", set_perf_work_mode, NULL,
				RSRC_CONF,
				"Use filter for LVE out"),

#if defined(linux)
				AP_INIT_TAKE1 ("PerformanceUseCPUTopMode",
						set_performance_module_cpu_top, NULL,
						RSRC_CONF,
						"Use CPU Usage algorithm like top"),
#endif
				AP_INIT_TAKE1 ("PerformanceCheckDaemonTimeExec",
						set_performance_module_check_daemon_exctime, NULL,
						RSRC_CONF,
						"Daemon execution time. Restart after repairing"),
						AP_INIT_TAKE1("PerformanceFragmentationTime",
								set_performance_module_dboptimize, NULL,
								RSRC_CONF,
								"Experimental. DB optimize - defragmentation. MySQL only"),
				AP_INIT_ITERATE("PerformanceExternalScript",
						set_performance_module_external_handler, NULL,
						RSRC_CONF,
						"set handlers filter for mod_fcgid, suphp, etc..."),
						AP_INIT_TAKE2("PerformanceMinExecTime",
								set_performance_module_cpu_exec_time, NULL,
								RSRC_CONF,
								"Set minimum script execution time. Less script wil save as 0, or will not save at all"),
				AP_INIT_TAKE1 ("PerformanceSilentMode",
						set_performance_module_silent_mode, NULL,
						RSRC_CONF,
						"Enable/Disable silent mode"),
				AP_INIT_TAKE1 ("PerformanceUseTid",
						set_performance_module_tid, NULL,
						RSRC_CONF,
						"Use tid"),
				AP_INIT_TAKE1 ("PerformanceBlockedSocket",
						set_performance_blocked_save, NULL,
						RSRC_CONF,
						"Blocked save"),
				AP_INIT_TAKE1 ("PerformanceCustomPool",
						set_performance_custom_pool, NULL,
						RSRC_CONF,
						"Use custom pool"),
				AP_INIT_TAKE2 ("PerformanceSocketPermType",
						set_performance_module_socket_perm, NULL,
						RSRC_CONF,
						"Set socket permissions"),
				AP_INIT_TAKE1("PerformanceHostId",
						set_performance_module_host_id, NULL, RSRC_CONF,
						"Set host ID"),
				AP_INIT_TAKE1("PerformanceCustomReports",
						set_performance_module_tpl_path, NULL,
						RSRC_CONF, "Path to report templates"),
//Deprecated
				AP_INIT_TAKE1("PerformanceMaxThreads",
						set_performance_module_maxthreads, NULL,
						RSRC_CONF,
						"Deprecated parameter - max threads count"),
				AP_INIT_TAKE1("PerformanceStackSize",
						set_performance_module_stacksize, NULL,
						RSRC_CONF,
						"Deprecated parameter - thread stack size"),
				AP_INIT_TAKE1 ("PerformanceExtended",
						set_performance_module_enabled_extended, NULL,
						RSRC_CONF,
						"Deprecated parameter - Use extended mode"),
						AP_INIT_TAKE1("PerformanceCheckDaemon",
								set_performance_module_check_daemon, NULL,
								RSRC_CONF,
								"Deprecated parameter - Check for daemon resource usage"),
						AP_INIT_TAKE1("PerformanceCheckDaemonInterval",
								set_performance_module_check_daemon_interval,
								NULL, RSRC_CONF,
								"Deprecated parameter - Interval of daemon usage resources in seconds"),
						AP_INIT_TAKE1("PerformanceCheckDaemonMemory",
								set_performance_module_check_daemon_memory,
								NULL, RSRC_CONF,
								"Deprecated parameter - Allowed memory for daemon. Restart if used more then allowed"),
						AP_INIT_TAKE1("PerformancePeriodicalWatch",
								set_performance_module_periodical_watch, NULL,
								RSRC_CONF,
								"Deprecated parameter - Enable/Disable periodical cpu and io usage during process watching"),
						AP_INIT_TAKE1("PerformanceUseCPUUsageLikeTop",
								set_performance_module_cpu_top_depr, NULL,
								RSRC_CONF,
								"Deprecated parameter - use new PerformanceUseCPUTopMode"),

				{ NULL } };

module AP_MODULE_DECLARE_DATA performance_module = { STANDARD20_MODULE_STUFF,
NULL, /* dir config creater */
NULL, /* dir merger --- default is to override */
create_performance_module_config, /* server config */
merge_performance_module_config, /* merge server config */
performance_module_cmds, /* command table */
register_hook /* register_handlers */
};
