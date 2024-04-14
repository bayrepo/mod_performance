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
 * custom_report.c
 *
 *  Created on: Jan 11, 2013
 *      Author: Alexey Berezhok
 *		E-mail: a@bayrepo.ru
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
#include <apr_file_io.h>

#include "custom_report.h"

int global_custom_counter = 0;

apr_hash_t *custom_reports = NULL;

/*
 * Report example
 *
 * [REPORTNAME] <--- отображаемое имя
 * HEADER=T1|T2|... имен а загловочной секции таблиц
 * SSQL= <--селект для sqlite
 * MSQL= <--селект для mysql
 * PSQL= <--селект для pgsql
 * PAGINATION=yes/no нужно бить на страницы
 * SORT=1,2,3 <---номера полей, для которых нужно выводить в заголовке символ сортировки
 *
 * отчет отображается, если для текущей конфигурации БД есть запрос(SSQL,MSQL,PSQL)
 * поля
 * id :ITEMNUMBER:,
 * dateadd :DATEADD:,
 * host :FORHOST:,
 * uri :REQUESTURI:,
 * script :CALLEDSCRIPT:,
 * cpu :CPUUSAGE:,
 * memory :MEMUSAGEINPRCT:,
 * exc_time :EXECUTIONTIME:,
 * cpu_sec :CPUUSAGEINSEC:,
 * memory_mb :MEMUSAGEINMB:,
 * bytes_read :BYTESREAD:,
 * bytes_write :BYTESWRITE:
 * hostnm :HOSTNAME:
 * поля для фильтра
 * :FILTER: - учитывать поля фильтра
 * :PERIOD: - учитывать введенный период
 * :TBL: - performance
 */

char *custom_report_stripnlspc(char *str, apr_pool_t *pool) {
	int j = 0;
	for (j = 0; j < strlen(str); j++) {
		if ((*(str + j) == 0xD) || (*(str + j) == 0xA)) {
			*(str + j) = ' ';
		}
	}
	char *new_str = NULL;
	char *ptr = str;
	while (*ptr) {
		if (*ptr != ' ')
			break;
		ptr++;
	}
	if (*ptr) {
		new_str = apr_pstrdup(pool, ptr);
	}
	if (new_str) {
		ptr = strlen(new_str) + new_str - 1;
		while (ptr >= new_str) {
			if (*ptr != ' ')
				break;
			ptr--;
		}
		*(ptr + 1) = 0;
	} else {
		new_str = apr_pstrdup(pool, "");
	}
	return new_str;
}

char *custom_report_replace(char *str, char *what, char *to, apr_pool_t *pool) {
	char *pos = NULL, *pos_old = str;
	char *new_str = apr_pstrdup(pool, "");
	pos = strstr(str, what);
	while (pos) {
		char *tmp_str = apr_pstrndup(pool, pos_old, pos - pos_old);
		new_str = apr_pstrcat(pool, new_str, tmp_str, to, NULL);
		pos += strlen(what);
		pos_old = pos;
		pos = strstr(pos_old, what);
	}
	char *tmp_str = apr_pstrndup(pool, pos_old, str + strlen(str) - pos_old);
	new_str = apr_pstrcat(pool, new_str, tmp_str, NULL);
	return new_str;
}

char *custom_report_pasre_sql_filter(char *select, apr_pool_t *pool,
				char *period, char *filter){
	char *result = NULL;
	if(period){
		result = custom_report_replace(select, ":PERIOD:", period, pool);
	}
	if(filter && result){
		result = custom_report_replace(result, ":FILTER:", filter, pool);
	}
	return result;
}

char *custom_report_pasre_sql_flds(char *select, apr_pool_t *pool) {
	char *result = custom_report_replace(select, ":ITEMNUMBER:", "id", pool);
	result = custom_report_replace(result, ":DATEADD:", "dateadd", pool);
	result = custom_report_replace(result, ":FORHOST:", "host", pool);
	result = custom_report_replace(result, ":REQUESTURI:", "uri", pool);
	result = custom_report_replace(result, ":CALLEDSCRIPT:", "script", pool);
	result = custom_report_replace(result, ":CPUUSAGE:", "cpu", pool);
	result = custom_report_replace(result, ":MEMUSAGEINPRCT:", "memory", pool);
	result = custom_report_replace(result, ":EXECUTIONTIME:", "exc_time", pool);
	result = custom_report_replace(result, ":CPUUSAGEINSEC:", "cpu_sec", pool);
	result = custom_report_replace(result, ":MEMUSAGEINMB:", "memory_mb", pool);
	result = custom_report_replace(result, ":BYTESREAD:", "bytes_read", pool);
	result = custom_report_replace(result, ":BYTESWRITE:", "bytes_write", pool);
	result = custom_report_replace(result, ":HOSTNAME:", "hostnm", pool);
	result = custom_report_replace(result, ":TBL:", "performance", pool);
	return result;
}

void custom_report_parse_value(char *key, char *value, custom_report_item *item,
		apr_pool_t *pool) {
	if (value && key && item) {
		if (!apr_strnatcasecmp(key, "header")) {
			char *last = NULL;
			char *token = apr_strtok(value, "|", &last);
			if (token) {
				*(const char **) apr_array_push(item->fields_list) =
						(const char *) token;
			}
			while ((token = apr_strtok(NULL, "|", &last)) != NULL) {
				if (token) {
					*(const char **) apr_array_push(item->fields_list) =
							(const char *) token;
				}
			}
		} else if (!apr_strnatcasecmp(key, "ssql")) {
			item->ssql = custom_report_pasre_sql_flds(value, pool);
		} else if (!apr_strnatcasecmp(key, "msql")) {
			item->msql = custom_report_pasre_sql_flds(value, pool);
		} else if (!apr_strnatcasecmp(key, "psql")) {
			item->psql = custom_report_pasre_sql_flds(value, pool);
		} else if (!apr_strnatcasecmp(key, "pagination")) {
			if (!apr_strnatcasecmp(value, "yes")) {
				item->pagination = 1;
			} else {
				item->pagination = 0;
			}
		} else if (!apr_strnatcasecmp(key, "sort")) {
			item->prp_sort = value;
		}
	}

}

void custom_report_post_prcess() {
	if (custom_reports) {
		apr_hash_index_t *hi;
		for (hi = apr_hash_first(NULL, custom_reports); hi;
				hi = apr_hash_next(hi)) {
			const char *k;
			custom_report_item *v;

			apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
			if (v->prp_sort) {
				char *last = NULL;
				char *token = apr_strtok(v->prp_sort, ",", &last);
				if (token) {
					int nmb = (int) apr_atoi64(token) - 1;
					if (v->fields_list->nelts > nmb) {
						*(const char **) apr_array_push(v->sorted_fields) =
								(const char *) ((const char **) v->fields_list->elts)[nmb];
					}

				}
				while ((token = apr_strtok(NULL, ",", &last)) != NULL) {
					int nmb = (int) apr_atoi64(token) - 1;
					if (v->fields_list->nelts > nmb) {
						*(const char **) apr_array_push(v->sorted_fields) =
								(const char *) ((const char **) v->fields_list->elts)[nmb];
					}
				}
			}
		}
	}
}

void custom_report_parse_reports(char *fname, apr_pool_t *pool) {
	apr_status_t rv;
	apr_file_t *fp;
	apr_finfo_t finfo;
	if ((rv = apr_file_open(&fp, fname, APR_READ, APR_OS_DEFAULT, pool))
			!= APR_SUCCESS) {
		return;
	}
	rv = apr_file_info_get(&finfo, APR_FINFO_NORM, fp);
	if (rv == APR_SUCCESS) {
		char *buf = apr_palloc(pool, finfo.size * sizeof(char));
		if (buf) {
			rv = apr_file_read(fp, (void *) buf, (apr_size_t *) (&finfo.size));
			if (rv == APR_SUCCESS) {
				custom_reports = apr_hash_make(pool);
				if (!custom_reports) {
					apr_file_close(fp);
					return;
				}
				long i = 0;
				int fnd_section_name = 0, fnd_key = 0, fnd_val = 0;
				char *current_sec_name = NULL;
				custom_report_item *current_item = NULL;
				char *current_key = NULL;
				while (i < finfo.size) {
					if (*(buf + i) == '[') {
						fnd_section_name++;
						fnd_key = 0;
						fnd_val = 0;
					} else {
						if (*(buf + i) == ']') {
							if (fnd_section_name) {
								current_sec_name = apr_pstrmemdup(pool,
										buf + i - fnd_section_name + 2,
										fnd_section_name);
								*(current_sec_name + fnd_section_name - 2) = 0;
								current_sec_name = custom_report_stripnlspc(
										current_sec_name, pool);
								fnd_section_name = 0;
								fnd_key = 1;
								current_item =
										(custom_report_item *) apr_hash_get(
												custom_reports,
												current_sec_name,
												APR_HASH_KEY_STRING);
								if (!current_item) {
									current_item = apr_palloc(pool,
											sizeof(custom_report_item));
									memset(current_item, 0,
											sizeof(custom_report_item));
									current_item->fields_list = apr_array_make(
											pool, 1, sizeof(const char*));
									current_item->sorted_fields =
											apr_array_make(pool, 1,
													sizeof(const char*));
									current_item->id = global_custom_counter++;
									current_item->name = apr_pstrdup(pool,
											current_sec_name);
									apr_hash_set(custom_reports,
											current_sec_name,
											APR_HASH_KEY_STRING, current_item);
								}
							}
						}
					}
					if (fnd_section_name)
						fnd_section_name++;
					else {
						if (fnd_key) {
							if (*(buf + i) == '=') {
								current_key = apr_pstrmemdup(pool,
										buf + i - fnd_key + 2, fnd_key);
								*(current_key + fnd_key - 2) = 0;
								current_key = custom_report_stripnlspc(
										current_key, pool);
								fnd_key = 0;
								fnd_val = 1;
							} else {
								fnd_key++;
							}
						} else {
							if (*(buf + i) == ';') {
								fnd_key = 1;
								if (current_key && current_sec_name) {
									if (fnd_val) {
										char *value = apr_pstrmemdup(pool,
												buf + i - fnd_val + 2, fnd_val);
										*(value + fnd_val - 2) = 0;
										value = custom_report_stripnlspc(value,
												pool);
										custom_report_parse_value(current_key,
												value, current_item, pool);

									}
								}
								fnd_val = 0;
							}
						}
						if (fnd_val) {
							fnd_val++;
						}
					}
					i++;
				}

				if (current_key && current_sec_name) {
					if (fnd_val) {
						char *value = apr_pstrmemdup(pool,
								buf + i - fnd_val + 2, fnd_val);
						*(value + fnd_val - 2) = 0;
						value = custom_report_stripnlspc(value, pool);
						custom_report_parse_value(current_key, value,
								current_item, pool);

					}
				}
				fnd_val = 0;

			}
		}
	}
	apr_file_close(fp);
	custom_report_post_prcess();
}

apr_array_header_t *custom_report_get_repots_list(apr_pool_t *pool) {
	apr_array_header_t * list = apr_array_make(pool, 1, sizeof(const char*));
	if (custom_reports) {
		apr_hash_index_t *hi;
		for (hi = apr_hash_first(NULL, custom_reports); hi;
				hi = apr_hash_next(hi)) {
			const char *k;
			custom_report_item *v;

			apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
			custom_report_item_lst *lst = apr_palloc(pool,
					sizeof(custom_report_item_lst));
			if (lst) {
				lst->report_name = (char *) k;
				lst->id = v->id;
				*(const custom_report_item_lst **) apr_array_push(list) =
						(const custom_report_item_lst *) lst;
			}
		}
	}
	return list;
}
custom_report_item *custom_report_get_repot_item(int id) {
	if (custom_reports) {
		apr_hash_index_t *hi;
		for (hi = apr_hash_first(NULL, custom_reports); hi;
				hi = apr_hash_next(hi)) {
			const char *k;
			custom_report_item *v;

			apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
			if (v->id == id)
				return v;
		}
	}
	return NULL;
}

int custom_report_is_sorted_field(custom_report_item *item, char *fld,
		apr_pool_t *pool) {
	int i = 0;
	for (i = 0; i < item->sorted_fields->nelts; i++) {
		const char *fld2 = ((const char **) item->sorted_fields->elts)[i];
		if (!apr_strnatcasecmp(fld, fld2))
			return 1;
	}
	return 0;
}

#ifdef MAKE_RUN
int main() {
	apr_status_t rv;
	apr_pool_t *mp;

	/* per-process initialization */
	rv = apr_initialize();
	if (rv != APR_SUCCESS) {
		return -1;
	}

	/* create a memory pool. */
	apr_pool_create(&mp, NULL);

	custom_report_parse_reports("tst.txt", mp);

	if (custom_reports) {
		apr_hash_index_t *hi;
		for (hi = apr_hash_first(NULL, custom_reports); hi;
				hi = apr_hash_next(hi)) {
			const char *k;
			custom_report_item *v;

			apr_hash_this(hi, (const void**) &k, NULL, (void**) &v);
			printf("Item %s\n", k);
			printf("  Header\n");
			int i;
			for (i = 0; i < v->fields_list->nelts; i++) {
				printf("    %s\n", ((const char **) v->fields_list->elts)[i]);
			}
			printf("  ssql %s\n", v->ssql);
			printf("  msql %s\n", v->msql);
			printf("  psql %s\n", v->psql);
			printf("  pagination %s\n", v->pagination ? "yes" : "no");
			printf("  Sorted\n");
			for (i = 0; i < v->sorted_fields->nelts; i++) {
				printf("    %s\n", ((const char **) v->sorted_fields->elts)[i]);
			}
		}
	}

	apr_array_header_t *t = custom_report_get_repots_list(mp);
	int i = 0;
	for (i = 0; i < t->nelts; i++) {
		printf("Report %s\n", ((const custom_report_item_lst **) t->elts)[i]->report_name);
	}

	/* destroy the memory pool. These chunks above are freed by this */
	apr_pool_destroy(mp);

	apr_terminate();
	return 0;
}
#endif
