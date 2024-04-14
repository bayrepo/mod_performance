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
 * custom_report.h
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


#ifndef CUSTOM_REPORT_H_
#define CUSTOM_REPORT_H_

typedef struct __custom_report_item {
	apr_array_header_t *fields_list;
	char *ssql;
	char *msql;
	char *psql;
	char *prp_sort;
	int pagination;
	apr_array_header_t *sorted_fields;
	int id;
	char *name;
} custom_report_item;

typedef struct __custom_report_item_lst {
	char *report_name;
	int id;
} custom_report_item_lst;

void custom_report_parse_reports(char *fname, apr_pool_t *pool);
apr_array_header_t *custom_report_get_repots_list(apr_pool_t *pool);
custom_report_item *custom_report_get_repot_item(int id);
int custom_report_is_sorted_field(custom_report_item *item, char *fld,
		apr_pool_t *pool);
char *custom_report_pasre_sql_filter(char *select, apr_pool_t *pool,
				char *period, char *filter);

#endif /* HUSTOM_REPORT_H_ */
