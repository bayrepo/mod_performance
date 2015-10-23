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
 * mod_reports.c
 *
 *  Created on: May 28, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

/*
 * Если Apache 1.3 - прекратить сборку
 */
#ifdef APACHE1_3
#error "This source is for Apache version 2.0 or 2.2 only"
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

#include "reports.h"
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

#include <sys/syscall.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#include <gd.h>
#include <gdfonts.h>

#include "common.h"
#include "sql_adapter.h"
#include "chart.h"

#include "custom_report.h"

extern int log_type;

void show_row1_reports(void *r, char *id, char *dateadd, char *host, char *uri,
		char *script, char *cpu, char *memory, char *exc_time, char *cpu_sec,
		char *memory_mb, char *bytes_read, char *bytes_write) {
	request_rec *rr = (request_rec *) r;
	ap_rputs("<tr>", rr);
	ap_rvputs(r, "<td>", id, "</td>", NULL);
	ap_rvputs(r, "<td>", dateadd, "</td>", NULL);
	ap_rvputs(r, "<td>", host, "</td>", NULL);
	ap_rvputs(r, "<td>", uri, "</td>", NULL);
	ap_rvputs(r, "<td>", script, "</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(cpu)), "</td>",
			NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(memory)), "</td>",
			NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(exc_time)),
			"</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(cpu_sec)), "</td>",
			NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(memory_mb)),
			"</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(bytes_read)),
			"</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(bytes_write)),
			"</td>", NULL);
	ap_rputs("</tr>\n", rr);
}

static void common_report_part(request_rec * r, char **period, char **host,
		char **script, char **uri, char **period_begin, char **period_end,
		int *sorti, int *srt_tp, int admin) {
	r->content_type = apr_pstrdup(r->pool, "text/html");
	*period = performance_module_get_parameter_from_uri(r, "prd");
	int prd_i = (int) apr_atoi64(*period ? *period : "1");
	if ((prd_i < 1) || (prd_i > 10))
		prd_i = 1;
	*period = apr_psprintf(r->pool, "%d", prd_i);
	if (!admin)
		*host = performance_module_get_parameter_from_uri(r, "host");
	else
		*host = get_host_name(r);
	*script = performance_module_get_parameter_from_uri(r, "script");
	*uri = performance_module_get_parameter_from_uri(r, "uri");
	*period_begin = performance_module_get_parameter_from_uri(r, "prd_b");
	*period_end = performance_module_get_parameter_from_uri(r, "prd_e");
	char *sort = performance_module_get_parameter_from_uri(r, "sort");
	int srt_i = (int) apr_atoi64(sort ? sort : "1");
	if ((srt_i < 1) || (srt_i > 12))
		srt_i = 1;
	*sorti = srt_i;
	sort = performance_module_get_parameter_from_uri(r, "tp");
	srt_i = (int) apr_atoi64(sort ? sort : "0");
	if ((srt_i < 0) || (srt_i > 1))
		srt_i = 0;
	*srt_tp = srt_i;
	performance_module_show_common_part(r, admin);
}

static void common_report_part_no_show(request_rec * r, char **period,
		char **host, char **script, char **uri, char **period_begin,
		char **period_end, int *sorti, int *srt_tp, int admin) {
	*period = performance_module_get_parameter_from_uri(r, "prd");
	int prd_i = (int) apr_atoi64(*period ? *period : "1");
	if ((prd_i < 1) || (prd_i > 10))
		prd_i = 1;
	*period = apr_psprintf(r->pool, "%d", prd_i);
	if (!admin)
		*host = performance_module_get_parameter_from_uri(r, "host");
	else
		*host = get_host_name(r);
	*script = performance_module_get_parameter_from_uri(r, "script");
	*uri = performance_module_get_parameter_from_uri(r, "uri");
	*period_begin = performance_module_get_parameter_from_uri(r, "prd_b");
	*period_end = performance_module_get_parameter_from_uri(r, "prd_e");
	char *sort = performance_module_get_parameter_from_uri(r, "sort");
	int srt_i = (int) apr_atoi64(sort ? sort : "1");
	if ((srt_i < 1) || (srt_i > 12))
		srt_i = 1;
	*sorti = srt_i;
	sort = performance_module_get_parameter_from_uri(r, "tp");
	srt_i = (int) apr_atoi64(sort ? sort : "0");
	if ((srt_i < 0) || (srt_i > 1))
		srt_i = 0;
	*srt_tp = srt_i;
}

void performance_module_show_max_no_graph_page(request_rec * r, int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
#if defined(linux)
	ap_rputs(
			"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Kb)</th><th class=\"localsort\">IO WRITE(Kb)</th></tr>\n",
			r);
#endif
#if defined(__FreeBSD__)
    ap_rputs
            (
            "<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Blocks)</th><th class=\"localsort\">IO WRITE(Blocks)</th></tr>\n",
      r);
#endif

	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td><td class=\"nmb\">9</td><td class=\"nmb\">10</td><td class=\"nmb\">11</td><td class=\"nmb\">12</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_cpu_max_text_info(r->pool, log_type, (void *) r,
			period, host, script, uri, period_begin, period_end, NULL, 0,
			&show_row1_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_show_max_no_graph_page_no_hard(request_rec * r,
		int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
#if defined(linux)
	ap_rputs(
			"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Kb)</th><th class=\"localsort\">IO WRITE(Kb)</th></tr>\n",
			r);
#endif
#if defined(__FreeBSD__)
    ap_rputs(
    		"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Blocks)</th><th class=\"localsort\">IO WRITE(Blocks)</th></tr>\n",
      r);
#endif

	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td><td class=\"nmb\">9</td><td class=\"nmb\">10</td><td class=\"nmb\">11</td><td class=\"nmb\">12</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_cpu_max_text_info_no_hard(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, 0, &show_row1_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_show_max_mem_no_graph_page(request_rec * r, int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
#if defined(linux)
	ap_rputs(
			"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Kb)</th><th class=\"localsort\">IO WRITE(Kb)</th></tr>\n",
			r);
#endif
#if defined(__FreeBSD__)
    ap_rputs(
    		"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Blocks)</th><th class=\"localsort\">IO WRITE(Blocks)</th></tr>\n",
      r);
#endif
	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td><td class=\"nmb\">9</td><td class=\"nmb\">10</td><td class=\"nmb\">11</td><td class=\"nmb\">12</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_mem_max_text_info(r->pool, log_type, (void *) r,
			period, host, script, uri, period_begin, period_end, NULL, 0,
			&show_row1_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_show_max_mem_no_graph_page_no_hard(request_rec * r,
		int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
#if defined(linux)
	ap_rputs(
			"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Kb)</th><th class=\"localsort\">IO WRITE(Kb)</th></tr>\n",
			r);
#endif
#if defined(__FreeBSD__)
    ap_rputs(
    		"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Blocks)</th><th class=\"localsort\">IO WRITE(Blocks)</th></tr>\n",
      r);
#endif

	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td><td class=\"nmb\">9</td><td class=\"nmb\">10</td><td class=\"nmb\">11</td><td class=\"nmb\">12</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_mem_max_text_info_no_hard(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, 0, &show_row1_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_show_max_time_no_graph_page(request_rec * r, int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
#if defined(linux)
	ap_rputs(
			"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Kb)</th><th class=\"localsort\">IO WRITE(Kb)</th></tr>\n",
			r);
#endif
#if defined(__FreeBSD__)
    ap_rputs(
    		"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Blocks)</th><th class=\"localsort\">IO WRITE(Blocks)</th></tr>\n",
      r);
#endif

	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td><td class=\"nmb\">9</td><td class=\"nmb\">10</td><td class=\"nmb\">11</td><td class=\"nmb\">12</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_time_max_text_info(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, 0, &show_row1_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_show_max_time_no_graph_page_no_hard(request_rec * r,
		int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
#if defined(linux)
	ap_rputs(
			"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Kb)</th><th class=\"localsort\">IO WRITE(Kb)</th></tr>\n",
			r);
#endif
#if defined(__FreeBSD__)
    ap_rputs(
    		"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Blocks)</th><th class=\"localsort\">IO WRITE(Blocks)</th></tr>\n",
      r);
#endif
	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td><td class=\"nmb\">9</td><td class=\"nmb\">10</td><td class=\"nmb\">11</td><td class=\"nmb\">12</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_time_max_text_info_no_hard(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, 0, &show_row1_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void show_row4_reports(void *r, char *field1, char *field2, char *field3) {
	request_rec *rr = (request_rec *) r;
	ap_rputs("<tr>", rr);
	ap_rvputs(r, "<td>", field1, "</td>", NULL);
	ap_rvputs(r, "<td>", field2, "</td>", NULL);
	ap_rvputs(r, "<td>", field3, "</td>", NULL);
	ap_rputs("</tr>\n", rr);
}

void performance_module_show_host_no_graph_page(request_rec * r, int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
	ap_rputs(
			"<tr class=\"hd_class\"><th>HOST</th><th class=\"localsort\">% host requests</th><th class=\"localsort\">number requests</th></tr>\n",
			r);
	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_host_text_info(r->pool, log_type, (void *) r,
			period, host, script, uri, period_begin, period_end, sorti, tp,
			NULL, 0, &show_row4_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//  MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void show_row5_reports(void *r, char *field1, char *field2, char *field3,
		char *field4, char *field5, char *field6) {
	request_rec *rr = (request_rec *) r;
	ap_rputs("<tr>", rr);
	ap_rvputs(r, "<td>", field1, "</td>", NULL);

	ap_rvputs(r, "<td>", field2, "</td>", NULL);
	ap_rvputs(r, "<td>", field3, "</td>", NULL);
	ap_rvputs(r, "<td>", field4, "</td>", NULL);
	ap_rvputs(r, "<td>", field5, "</td>", NULL);
	ap_rvputs(r, "<td>", field6, "</td>", NULL);
	ap_rputs("</tr>\n", rr);
}

void performance_module_show_host_average_no_graph_page(request_rec * r,
		int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
	ap_rputs(
			"<tr class=\"hd_class\"><th class=\"localsort\">HOST</th><th class=\"localsort\">Avg. cpu %</th><th class=\"localsort\">Avg. memory %</th><th class=\"localsort\">Avg. exec</th><th class=\"localsort\">Avg. read</th><th class=\"localsort\">Avg. write</th></tr>\n",
			r);
	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_avg_text_info(r->pool, log_type, (void *) r,
			period, host, script, uri, period_begin, period_end, sorti, tp,
			NULL, 0, &show_row5_reports);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);
	performance_module_show_footer_part(r);
}

void show_row2(void *r, int count, char *page) {
	request_rec *rr = (request_rec *) r;
	int val = count;

	int pg_num = 0;
	if (page) {
		pg_num = (int) apr_atoi64(page) - 1;
		if (pg_num < 0)
			pg_num = 0;
	}

	if (val % ITEMS_PER_PAGE == 0)
		val = val / ITEMS_PER_PAGE;
	else
		val = val / ITEMS_PER_PAGE + 1;
	int ival = 0;
	ap_rvputs(rr, "<div style=\"font-size:0.9em; padding:5px;\">", NULL);
	for (ival = 0; ival < val; ival++) {
		char *ss = apr_pstrdup(rr->pool, rr->args);
		if (strstr(ss, "page")) {
			char *p = strstr(ss, "page");
			char *nss = apr_pstrndup(rr->pool, ss,
					((p - 1) >= ss) ? (p - ss - 1) : 0);
			char *p2 = strstr(p, "&");
			if (p2) {
				nss = apr_pstrcat(rr->pool, nss, p2, NULL);
			}
			ss = nss;
		}
		if (pg_num != ival)
			ap_rvputs(rr, "<a href=\"?", ss, "&page=", apr_itoa(rr->pool, ival + 1),
					"\">", apr_itoa(rr->pool, ival + 1), "</a>", "  ",
					NULL);
		else
			ap_rvputs(rr, apr_itoa(rr->pool, ival + 1), "  ", NULL);

	}
	ap_rvputs(rr, "</div>", NULL);
}

void show_row1(void *r, char *id, char *dateadd, char *host, char *uri,
		char *script, char *cpu, char *memory, char *exc_time, char *cpu_sec,
		char *memory_mb, char *bytes_read, char *bytes_write) {
	request_rec *rr = (request_rec *) r;
	ap_rputs("<tr>", rr);
	ap_rvputs(r, "<td>", id, "</td>", NULL);
	ap_rvputs(r, "<td>", dateadd, "</td>", NULL);
	ap_rvputs(r, "<td>", host, "</td>", NULL);
	ap_rvputs(r, "<td>", uri, "</td>", NULL);
	ap_rvputs(r, "<td>", script, "</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(cpu)), "</td>",
			NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(memory)), "</td>",
			NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(exc_time)),
			"</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(cpu_sec)), "</td>",
			NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(memory_mb)),
			"</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(bytes_read)),
			"</td>", NULL);
	ap_rvputs(r, "<td>", apr_psprintf(rr->pool, "%.5f", atof(bytes_write)),
			"</td>", NULL);
	ap_rputs("</tr>\n", rr);

}

void performance_module_show_index_no_graph_page(request_rec * r, int admin) {

	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	char *page_number = performance_module_get_parameter_from_uri(r, "page");

	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
#if defined(linux)
	ap_rputs(
			"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Kb)</th><th class=\"localsort\">IO WRITE(Kb)</th></tr>\n",
			r);
#endif
#if defined(__FreeBSD__)
    ap_rputs(
    		"<tr class=\"hd_class\"><th>ID</th><th>DATE ADD</th><th>HOSTNAME</th><th>URI</th><th>SCRIPT</th><th class=\"localsort\">CPU(%)</th><th class=\"localsort\">MEM(%)</th><th class=\"localsort\">TIME EXEC(sec)</th><th class=\"localsort\">CPU TM(sec)</th><th class=\"localsort\">MEM USE(Mb)</th><th class=\"localsort\">IO READ(Blocks)</th><th class=\"localsort\">IO WRITE(Blocks)</th></tr>\n",
      r);
#endif

	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td><td class=\"nmb\">9</td><td class=\"nmb\">10</td><td class=\"nmb\">11</td><td class=\"nmb\">12</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_full_text_info(r->pool, log_type, (void *) r,
			period, host, script, uri, period_begin, period_end, sorti, tp,
			page_number, ITEMS_PER_PAGE, &show_row1);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r),err);
	}

	ap_rputs("</tbody></table>\n", r);

	err = sql_adapter_get_full_text_info_count(r->pool, log_type, (void *) r,
			period, host, script, uri, period_begin, period_end, page_number,
			ITEMS_PER_PAGE, &show_row2);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	performance_module_show_footer_part(r);
}

void performance_module_show_index_no_graph_page_no_data(request_rec * r,
		int admin) {

	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	ap_rputs("<center>No data available</center>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_show_graph_page(request_rec * r, int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	ap_rvputs(r, "<center><img src=\"?pic=1&", r->args, "\"></center>", NULL);

	performance_module_show_footer_part(r);
}

void show_row3(void *r, int dateadd, double max_cpu, double max_memory,
		double ioread, double iowrite, void *ptr, int number) {
	request_rec *rr = (request_rec *) r;
	apr_array_header_t *data = (apr_array_header_t *) ptr;
	chart_data_percent *ptr_i = apr_palloc(rr->pool,
			sizeof(chart_data_percent));
	ptr_i->date = dateadd;
	switch (number) {
	case 0:
		ptr_i->value = max_cpu;
		break;
	case 1:
		ptr_i->value = max_memory;
		break;
	case 2:
		ptr_i->value = ioread;
		break;
	case 3:
		ptr_i->value = iowrite;
		break;
	}
	*(chart_data_percent **) apr_array_push(data) = ptr_i;
}

void show_row4_data(void *r, char *hostname, double max_cpu, double max_memory,
		double tm, double ioread, double iowrite, void *ptr, int number) {
	request_rec *rr = (request_rec *) r;
	apr_array_header_t *data = (apr_array_header_t *) ptr;
	chart_data_pie *ptr_i = apr_palloc(rr->pool, sizeof(chart_data_pie));
	ptr_i->name = apr_pstrdup(rr->pool, hostname);
	switch (number) {
	case 0:
		ptr_i->value = max_cpu;
		break;
	case 1:
		ptr_i->value = max_memory;
		break;
	case 2:
		ptr_i->value = ioread;
		break;
	case 3:
		ptr_i->value = iowrite;
		break;
	}
	*(chart_data_pie **) apr_array_push(data) = ptr_i;
}

void show_row3_data(void *r, char *hostname, double value, void *ptr) {
	request_rec *rr = (request_rec *) r;
	apr_array_header_t *data = (apr_array_header_t *) ptr;
	chart_data_pie *ptr_i = apr_palloc(rr->pool, sizeof(chart_data_pie));
	ptr_i->name = apr_pstrdup(rr->pool, hostname);
	ptr_i->value = value;
	*(chart_data_pie **) apr_array_push(data) = ptr_i;
}

void performance_module_show_graph_page_memory(request_rec * r, int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_percent *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_full_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, ITEMS_PER_PAGE, &show_row3, (void *) data, 1);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_bars(im, data, "Memory Usage %", CHART_DATE, width,
			height, CHART_COLOR_RED);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_graph_page_ioread(request_rec * r, int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_percent *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_full_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, ITEMS_PER_PAGE, &show_row3, (void *) data, 2);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_bars(im, data, "Read operations (Kb|Blocks)",
			CHART_DATE | CHART_SHOW_UNITS, width, height, CHART_COLOR_YELLOW);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_graph_page_iowrite(request_rec * r, int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_percent *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_full_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, ITEMS_PER_PAGE, &show_row3, (void *) data, 3);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//   MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_bars(im, data, "Write operations (Kb|Blocks)",
			CHART_DATE | CHART_SHOW_UNITS, width, height, CHART_COLOR_BLUE);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_host_graph_page(request_rec * r, int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_pie *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_host_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end, 2,
			NULL, 0, &show_row3_data, data);

	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//  MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_pie(im, data, "Host statistics %", width, height);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_host_average_graph_cpu(request_rec * r, int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_pie *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_avg_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end, 2,
			NULL, 0, &show_row4_data, data, 0);

	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_pie(im, data, "Average CPU", width, height);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_host_average_graph_mem(request_rec * r, int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_pie *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_avg_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end, 3,
			NULL, 0, &show_row4_data, data, 0);

	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_pie(im, data, "Average memory", width, height);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_host_average_graph_ioread(request_rec * r,
		int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_pie *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_avg_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end, 5,
			NULL, 0, &show_row4_data, data, 0);

	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_pie(im, data, "Average read operations", width, height);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_host_average_graph_iowrite(request_rec * r,
		int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_pie *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_avg_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end, 6,
			NULL, 0, &show_row4_data, data, 0);

	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//  MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_pie(im, data, "Average write operations", width, height);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void performance_module_show_graph_page_cpu(request_rec * r, int admin) {
	apr_array_header_t *data;
	data = apr_array_make(r->pool, 1, sizeof(chart_data_percent *));
	r->content_type = apr_pstrdup(r->pool, "image/gif");
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part_no_show(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	char *err = sql_adapter_get_full_text_info_picture(r->pool, log_type,
			(void *) r, period, host, script, uri, period_begin, period_end,
			NULL, ITEMS_PER_PAGE, &show_row3, (void *) data, 0);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
		return;
	}
	int width = 800, height = 480;
	gdImagePtr im = NULL;
	im = chart_create_bars(im, data, "CPU Usage %", CHART_DATE, width, height,
			CHART_COLOR_GREEN);
	int length;
	char *dataptr = gdImageGifPtr(im, &length);
	ap_rwrite(dataptr, length, r);
	gdImageDestroy(im);
}

void show_row10(void *r, char *host, char *condition, char *count,
		char *min_cpu, char *max_cpu, char *avg_cpu, char *min_mem,
		char *max_mem, char *avg_mem) {
	request_rec *rr = (request_rec *) r;
	ap_rputs("<tr>", rr);
	ap_rvputs(r, "<td>", condition, "</td>", NULL);
	ap_rvputs(r, "<td>", count, "</td>", NULL);
	ap_rvputs(r, "<td>", min_cpu, "</td>", NULL);
	ap_rvputs(r, "<td>", max_cpu, "</td>", NULL);
	ap_rvputs(r, "<td>", avg_cpu, "</td>", NULL);
	ap_rvputs(r, "<td>", min_mem, "</td>", NULL);
	ap_rvputs(r, "<td>", max_mem, "</td>", NULL);
	ap_rvputs(r, "<td>", avg_mem, "</td>", NULL);
	ap_rputs("</tr>\n", rr);

}

void performance_module_show_exec_range_no_graph_page(request_rec * r,
		int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);

	ap_rputs(
			"<tr class=\"hd_class\"><th>EXECUTION TIME</th><th>COUNT</th><th>MIN(CPU%)</th><th>MAX(CPU%)</th><th>AVG(CPU%)</th><th>MIN(MEM%)</th><th>MAX(MEM%)</th><th>AVG(MEM%)</th></tr>\n",
			r);

	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_exec_tm(r->pool, log_type, (void *) r, period,
			host, script, uri, period_begin, period_end, NULL, ITEMS_PER_PAGE,
			&show_row10);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r),err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_show_exec_range_no_graph_page_common(request_rec * r,
		int admin) {
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);

	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);

	ap_rputs(
			"<tr class=\"hd_class\"><th>EXECUTION TIME</th><th>COUNT</th><th>MIN(CPU%)</th><th>MAX(CPU%)</th><th>AVG(CPU%)</th><th>MIN(MEM%)</th><th>MAX(MEM%)</th><th>AVG(MEM%)</th></tr>\n",
			r);

	ap_rputs(
			"<tr><td class=\"nmb\">1</td><td class=\"nmb\">2</td><td class=\"nmb\">3</td><td class=\"nmb\">4</td><td class=\"nmb\">5</td><td class=\"nmb\">6</td><td class=\"nmb\">7</td><td class=\"nmb\">8</td></tr></thead><tbody>",
			r);

	char *err = sql_adapter_get_exec_tm_common(r->pool, log_type, (void *) r,
			period, host, script, uri, period_begin, period_end, NULL,
			ITEMS_PER_PAGE, &show_row10);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//    MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r),err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}

void performance_module_custom_report_no_graph_page(request_rec * r, int admin,
		void *itm, int db_type) {
	custom_report_item *item = (custom_report_item *) itm;
	char *period = NULL, *host = NULL, *script = NULL, *uri = NULL,
			*period_begin = NULL, *period_end = NULL;
	int sorti = 1, tp = 0;
	common_report_part(r, &period, &host, &script, &uri, &period_begin,
			&period_end, &sorti, &tp, admin);
	ap_rputs(
			"<table border=\"0\" cellspacing=\"1\" cellpadding=\"0\" class=\"rightData\"><thead>\n",
			r);
	ap_rputs("<tr class=\"hd_class\">\n", r);
	int i;
	for (i = 0; i < item->fields_list->nelts; i++) {
		if (custom_report_is_sorted_field(item,
				(char *) ((const char **) item->fields_list->elts)[i],
				r->pool)) {
			ap_rvputs(r, "<th class=\"localsort\">",
					((const char **) item->fields_list->elts)[i], "</th>",
					NULL);
		} else {
			ap_rvputs(r, "<th>", ((const char **) item->fields_list->elts)[i],
					"</th>", NULL);
		}
	}
	ap_rputs("</tr>\n", r);
	ap_rputs("<tr>\n", r);
	for (i = 0; i < item->fields_list->nelts; i++) {
		ap_rprintf(r, "<td class=\"nmb\">%d</td>", i+1);
	}

	ap_rputs("</tr></thead><tbody>\n", r);

	char *err = sql_adapter_get_custom_text_info(r->pool, db_type, (void *) r,
			period, host, script, uri, period_begin, period_end, sorti, tp,
			NULL, 0, item);
	if (err) {
		//ap_log_rerror(APLOG_MARK, APLOG_ERR, errno, r,
		//  MODULE_PREFFIX "DB request error. Request %s", err);
		ap_log_perror(APLOG_MARK, APLOG_ERR, errno, r->pool, MODULE_PREFFIX"(host %s) DB request error. Request %s", get_host_name(r), err);
	}

	ap_rputs("</tbody></table>\n", r);

	performance_module_show_footer_part(r);
}
