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
 * common.c
 *
 *  Created on: May 28, 2011
 *  Author: SKOREE
 *  E-mail: a@bayrepo.ru
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

#include "common.h"
#include "html-sources.h"

#include "custom_report.h"

extern char *pagehtml1[];
extern char *jquery162[];

char *
get_host_name(request_rec * r) {
	if (r->server && r->server->server_hostname)
		return r->server->server_hostname;
	return apr_pstrdup(r->pool, "localhost");
}

char *
performance_module_get_parameter_from_uri(request_rec * r, char *name) {
	char *query_p = r->parsed_uri.query;
	if (query_p == NULL)
		return NULL;
	char *query = apr_pstrdup(r->pool, query_p);
	char *p;
	for (p = query; *p; ++p)
		if (*p == '+')
			*p = ' ';
	ap_unescape_url(query);
	char *ctx;
	while ((query = apr_strtok(query, "&", &ctx)) != NULL) {
		char *sub_query = apr_pstrdup(r->pool, query);
		char *ctxi;
		sub_query = apr_strtok(sub_query, "=", &ctxi);
		if (!sub_query)
			return NULL;
		char *name_token = sub_query;
		char *value_token = apr_strtok(NULL, "=", &ctxi);
		if (!apr_strnatcasecmp(name, name_token)) {
			return value_token;
		}
		query = NULL;
	}
	return NULL;
}

#define CHECK_SELECTED(x,y) (x==y)?" selected":""

void performance_module_show_common_part(request_rec * r, int admin) {
	int i = 0;
	while (pagehtml1[i]) {
		ap_rputs(pagehtml1[i], r);
		i++;
	}

	ap_rprintf(
			r,
			"<div class=\"infoBlockHeadMain\">Input parameters<span>[Hide]</span> Version %s.</div>\n",
			PERFORMANCE_MODULE_VERSION);

	ap_rputs("<div class=\"infoBlock\">\n", r);
	ap_rputs(
			"<table width=\"100%\" border=\"0\" cellspacing=\"10\" cellpadding=\"0\">\n",
			r);
	ap_rputs("<tr>\n", r);
	ap_rputs("<td colspan=\"5\">\n", r);
	ap_rputs("<div class=\"infoBlockName\">Report</div>\n", r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	ap_rputs("<select name=\"info\">\n", r);
	char *info = performance_module_get_parameter_from_uri(r, "info");
	int info_i = (int) apr_atoi64(info ? info : "0");
	if (info_i < 0)
		info_i = 0;
	int iinfo = info_i;
	ap_rvputs(r, "<option value=\"0\"", CHECK_SELECTED(info_i,0)
	, ">Show output without analytics</option>\n", NULL);
	ap_rvputs(r, "<option value=\"8\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,8),">-----------graph CPU</option>\n",NULL);
	ap_rvputs(r, "<option value=\"9\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,9),">-----------graph Memory</option>\n",NULL);
	ap_rvputs(r, "<option value=\"10\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,10),">-----------graph I/O read</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"17\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,17),">-----------graph I/O write</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"1\"", CHECK_SELECTED(info_i,1)
	, ">Maximal %CPU per site (hard request be careful)</option>\n", NULL);
	ap_rvputs(r, "<option value=\"30\"", CHECK_SELECTED(info_i,30)
	, ">Maximal %CPU (one from all)</option>\n", NULL);
	ap_rvputs(r, "<option value=\"3\"", CHECK_SELECTED(info_i,3)
	, ">Maximal memory %(hard request be careful)</option>\n", NULL);
	ap_rvputs(r, "<option value=\"31\"", CHECK_SELECTED(info_i,31)
	, ">Maximal memory % (one from all)</option>\n", NULL);
	ap_rvputs(r, "<option value=\"4\"", CHECK_SELECTED(info_i,4)
	, ">Maximal execution request time(hard request be careful)</option>\n",
			NULL);
	ap_rvputs(r, "<option value=\"32\"", CHECK_SELECTED(info_i,32)
	, ">Maximal execution request time (one from all)</option>\n", NULL);
	ap_rvputs(r, "<option value=\"2\"", CHECK_SELECTED(info_i,2)
	, ">Host requests statistics</option>\n", NULL);
	ap_rvputs(r, "<option value=\"11\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,11),">-----------graph requsts</option>\n",NULL);
	ap_rvputs(r, "<option value=\"7\"", CHECK_SELECTED(info_i,7)
	, ">Average usage per host</option>\n", NULL);
	ap_rvputs(r, "<option value=\"13\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,13),">-----------graph avg CPU</option>\n",NULL);
	ap_rvputs(r, "<option value=\"14\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,14),">-----------graph avg Mem</option>\n",NULL);
	ap_rvputs(r, "<option value=\"15\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,15)
			, ">-----------graph avg I/O read</option>\n", NULL);
	ap_rvputs(r, "<option value=\"16\" class=\"graphReport\"",
			CHECK_SELECTED(info_i,16)
			, ">-----------graph avg I/O write</option>\n", NULL);
	ap_rvputs(r, "<option value=\"20\"", CHECK_SELECTED(info_i,20)
	, ">Time execution range (per host)</option>\n", NULL);
	ap_rvputs(r, "<option value=\"33\"", CHECK_SELECTED(info_i,33)
	, ">Time execution range (common)</option>\n", NULL);

	apr_array_header_t *cstm = custom_report_get_repots_list(r->pool);

	int ind_c = 0;
	for (ind_c = 0; ind_c < cstm->nelts; ind_c++) {
		ap_rvputs(
				r,
				"<option value=\"",
				apr_itoa(
						r->pool,
						((const custom_report_item_lst **) cstm->elts)[ind_c]->id
								+ 500),
				"\"",
				CHECK_SELECTED(info_i,(((const custom_report_item_lst **) cstm->elts)[ind_c]->id + 500))
				, ">",
				((const custom_report_item_lst **) cstm->elts)[ind_c]->report_name,
				"</option>\n", NULL);
	}

	ap_rputs("</select>\n", r);
	ap_rputs("</div>\n", r);
	ap_rputs("</td></tr>\n", r);
	ap_rputs("<td>\n", r);
	ap_rputs("<div class=\"infoBlockName\">Sort field</div>\n", r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	info = performance_module_get_parameter_from_uri(r, "sort");
	info_i = (int) apr_atoi64(info ? info : "0");
	if ((info_i < 1) || (info_i > 12))
		info_i = 1;
	ap_rputs("<select name=\"sort\">\n", r);
	ap_rvputs(r, "<option value=\"1\"", CHECK_SELECTED(info_i,1),">1</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"2\"", CHECK_SELECTED(info_i,2),">2</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"3\"", CHECK_SELECTED(info_i,3),">3</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"4\"", CHECK_SELECTED(info_i,4),">4</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"5\"", CHECK_SELECTED(info_i,5),">5</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"6\"", CHECK_SELECTED(info_i,6),">6</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"7\"", CHECK_SELECTED(info_i,7),">7</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"8\"", CHECK_SELECTED(info_i,8),">8</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"9\"", CHECK_SELECTED(info_i,9),">9</option>\n"
,			NULL);
	ap_rvputs(r, "<option value=\"10\"", CHECK_SELECTED(info_i,10)
	, ">10</option>\n", NULL);
	ap_rvputs(r, "<option value=\"11\"", CHECK_SELECTED(info_i,11)
	, ">11</option>\n", NULL);
	ap_rvputs(r, "<option value=\"12\"", CHECK_SELECTED(info_i,12)
	, ">12</option>\n", NULL);
	ap_rputs("</select>\n", r);
	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);

	ap_rputs("<td>\n", r);
	ap_rputs("<div class=\"infoBlockName\">Sort type</div>\n", r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	char *tp = performance_module_get_parameter_from_uri(r, "tp");
	int tp_i = (int) apr_atoi64(tp ? tp : "0");
	if ((tp_i < 1) || (tp_i > 1))
		tp_i = 0;
	ap_rputs("<select name=\"tp\">\n", r);
	ap_rvputs(r, "<option value=\"0\"", CHECK_SELECTED(tp_i,0)
	, ">DESC</option>\n", NULL);
	ap_rvputs(r, "<option value=\"1\"", CHECK_SELECTED(tp_i,1),">ASC</option>\n"
,			NULL);
	ap_rputs("</select>\n", r);
	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);

	ap_rputs("<td>\n", r);
	ap_rputs("<div class=\"infoBlockName\">Period (days)</div>\n", r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	char *prd = performance_module_get_parameter_from_uri(r, "prd");
	int prd_i = (int) apr_atoi64(prd ? prd : "0");
	if ((prd_i < 1) || (prd_i > 10)) {
		prd = apr_pstrdup(r->pool, "1");
	}
	ap_rvputs(r, "<input type=\"text\" value=\"", prd,
			"\" name=\"prd\" id=\"period\" maxlength=\"2\" size=\"5\"/>\n",
			NULL);
	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);
	ap_rputs("<td>\n", r);
	ap_rputs(
			"<div class=\"infoBlockName\">Period begin (YYYY-MM-DD hh:mm:ss)</div>\n",
			r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	char *period_begin = performance_module_get_parameter_from_uri(r, "prd_b");
	ap_rvputs(
			r,
			"<input type=\"text\" value=\"",
			(period_begin ? period_begin : ""),
			"\" name=\"prd_b\" id=\"period_begin\" maxlength=\"20\" size=\"20\"/>\n",
			NULL);
	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);
	ap_rputs("<td>\n", r);
	ap_rputs(
			"<div class=\"infoBlockName\">Period end (YYYY-MM-DD hh:mm:ss)</div>\n",
			r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	char *period_end = performance_module_get_parameter_from_uri(r, "prd_e");
	ap_rvputs(
			r,
			"<input type=\"text\" value=\"",
			(period_end ? period_end : ""),
			"\" name=\"prd_e\" id=\"period_end\" maxlength=\"20\" size=\"20\"/>\n",
			NULL);
	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);
	ap_rputs("</td>\n", r);
	ap_rputs("</tr>\n", r);
	ap_rputs("<tr>\n", r);
	ap_rputs("<td colspan=\"2\">\n", r);
	ap_rputs(
			"<div class=\"infoBlockName\">Hostname(test.com or %test%)</div>\n",
			r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	char *host;
	if (!admin) {
		host = performance_module_get_parameter_from_uri(r, "host");
		ap_rvputs(r, "<input type=\"text\" value=\"", (host ? host : ""),
				"\" name=\"host\" maxlength=\"30\" size=\"20\"/>\n", NULL);
	} else {
		host = get_host_name(r);
		ap_rvputs(r, "<input type=\"text\" value=\"", (host ? host : ""),
				"\" name=\"host\" maxlength=\"30\" size=\"20\" readonly/>\n",
				NULL);
	}

	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);
	ap_rputs("<td>\n", r);
	ap_rputs(
			"<div class=\"infoBlockName\">Script name(index.php or %index%)</div>\n",
			r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	char *script = performance_module_get_parameter_from_uri(r, "script");
	ap_rvputs(r, "<input type=\"text\" value=\"", (script ? script : ""),
			"\" name=\"script\" maxlength=\"80\" size=\"40\"/>\n", NULL);
	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);
	ap_rputs("<td>\n", r);
	ap_rputs("<div class=\"infoBlockName\">URI(index.php or %index%)</div>\n",
			r);
	ap_rputs("<div class=\"infoBlockItem\">\n", r);
	char *uri = performance_module_get_parameter_from_uri(r, "uri");
	ap_rvputs(r, "<input type=\"text\" value=\"", (uri ? uri : ""),
			"\" name=\"uri\" maxlength=\"80\" size=\"40\"/>\n", NULL);
	ap_rputs("</div>\n", r);
	ap_rputs("</td>\n", r);
	ap_rputs("<td style=\"text-align: right\">\n", r);
	ap_rputs(
			"<a href=\"" FRESH_VERSION_URL "\" target=\"_blank\">&gt;&gt;Download newest version of module</a>&nbsp;&nbsp;&nbsp;\n",
			r);
	ap_rputs(
			"<a href=\"" DOCUMENTATION_URL "\" target=\"_blank\">&gt;&gt;Available documentation</a>\n",
			r);
	ap_rputs("</td>\n", r);
	ap_rputs("</tr>\n", r);
	ap_rputs("</table>\n", r);
	ap_rputs("<div class=\"infoBlockName\">\n", r);
	ap_rputs("<input type=\"hidden\" value=\"0\" name=\"mode\"/>\n", r);
	ap_rputs("<center><input type=\"submit\" value=\"Request\"/></center>\n",
			r);
	ap_rputs("</div>\n", r);
	ap_rputs("</div>\n", r);
	ap_rputs("</form>\n", r);
	ap_rputs("</div>\n", r);
	ap_rputs("<div id=\"rightDiv\">\n", r);

	info = performance_module_get_parameter_from_uri(r, "info");
	info_i = (int) apr_atoi64(info ? info : "-1");
	if (info_i < 0) {
		iinfo = -1;
	}

	switch (iinfo) {
	case 0:
		ap_rvputs(
				r,
				"<h1>Report: Show output without analytics (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 1:
		ap_rvputs(
				r,
				"<h1>Report: Maximal %CPU (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 2:
		ap_rvputs(
				r,
				"<h1>Report: Host requests statistics (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 3:
		ap_rvputs(
				r,
				"<h1>Report: Maximal memory % (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 4:
		ap_rvputs(
				r,
				"<h1>Report: Maximal execution request time (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 7:
		ap_rvputs(
				r,
				"<h1>Report: Average usage per host (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 8:
		ap_rvputs(r,
				"<h1>Report: Show output without analytics graph CPU</h1>\n",
				NULL);
		break;
	case 9:
		ap_rvputs(r,
				"<h1>Report: Show output without analytics graph Memory</h1>\n",
				NULL);
		break;
	case 10:
		ap_rvputs(
				r,
				"<h1>Report: Show output without analytics graph I/O read</h1>\n",
				NULL);
		break;
	case 11:
		ap_rvputs(r,
				"<h1>Report: Host requests statistics graph requsts</h1>\n",
				NULL);
		break;
	case 13:
		ap_rvputs(r, "<h1>Report: Average usage per host graph avg CPU</h1>\n",
				NULL);
		break;
	case 14:
		ap_rvputs(r, "<h1>Report: Average usage per host graph avg Mem</h1>\n",
				NULL);
		break;
	case 15:
		ap_rvputs(r,
				"<h1>Report: Average usage per host graph avg I/O read</h1>\n",
				NULL);
		break;
	case 16:
		ap_rvputs(r,
				"<h1>Report: Average usage per host graph avg I/O write</h1>\n",
				NULL);
		break;
	case 17:
		ap_rvputs(
				r,
				"<h1>Report: Show output without analytics graph I/O write</h1>\n",
				NULL);
		break;
	case 20:
		ap_rvputs(r, "<h1>Report: Time execution range</h1>\n", NULL);
		break;
	case 30:
		ap_rvputs(
				r,
				"<h1>Report: Maximal %CPU (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 31:
		ap_rvputs(
				r,
				"<h1>Report: Maximal memory % (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 32:
		ap_rvputs(
				r,
				"<h1>Report: Maximal execution request time (click \"v|^\" for local table sorting)</h1>\n",
				NULL);
		break;
	case 33:
		ap_rvputs(r, "<h1>Report: Time execution range (common)</h1>\n", NULL);
		break;
	case 100:
		ap_rvputs(r, "<h1>Report: Show current daemon threads</h1>\n", NULL);
		break;
	default:
		if (iinfo >= 500) {
			custom_report_item * itm = custom_report_get_repot_item(
					iinfo - 500);
			if (itm) {
				ap_rvputs(r, "<h1>Report: ", itm->name, "</h1>\n", NULL);
			}
		}
		break;
	}

}

void performance_module_show_footer_part(request_rec * r) {

	ap_rputs("</div>\n", r);
	ap_rputs("<div id=\"footer\">&copy; Alexey Berezhok(Skoree)</div>\n", r);
	ap_rputs("</body>\n", r);
	ap_rputs("</html>", r);

}

void print_js_content(request_rec *r) {
	r->content_type = apr_pstrdup(r->pool, "text/html");
	int i = 0;
	while (jquery162[i]) {
		ap_rputs(jquery162[i], r);
		i++;
	}
}
