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
 * reports.h
 *
 *  Created on: May 28, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#ifndef REPORTS_H_
#define REPORTS_H_

void performance_module_show_max_no_graph_page (request_rec * r, int admin);
void
performance_module_show_max_mem_no_graph_page (request_rec * r, int admin);
void
performance_module_show_max_time_no_graph_page (request_rec * r, int admin);

void performance_module_show_max_no_graph_page_no_hard (request_rec * r, int admin);
void
performance_module_show_max_mem_no_graph_page_no_hard (request_rec * r, int admin);
void
performance_module_show_max_time_no_graph_page_no_hard (request_rec * r, int admin);

void performance_module_show_host_no_graph_page (request_rec * r, int admin);

void
performance_module_show_host_average_no_graph_page (request_rec * r,
						    int admin);
void
performance_module_show_index_no_graph_page (request_rec * r, int admin);

void performance_module_show_graph_page (request_rec * r, int admin);

void performance_module_show_graph_page_memory (request_rec * r, int admin);
void performance_module_show_graph_page_ioread (request_rec * r, int admin);
void performance_module_show_graph_page_iowrite (request_rec * r, int admin);
void performance_module_show_host_graph_page (request_rec * r, int admin);
void performance_module_show_host_average_graph_cpu (request_rec * r, int admin);
void performance_module_show_host_average_graph_mem (request_rec * r, int admin);
void performance_module_show_host_average_graph_ioread (request_rec * r, int admin);
void performance_module_show_host_average_graph_iowrite (request_rec * r, int admin);
void performance_module_show_graph_page_cpu (request_rec * r, int admin);
void
performance_module_show_index_no_graph_page_no_data(request_rec * r, int admin);
void
performance_module_show_exec_range_no_graph_page(request_rec * r, int admin);

void
performance_module_show_exec_range_no_graph_page_common(request_rec * r, int admin);

void performance_module_custom_report_no_graph_page(request_rec * r, int admin,
		void *itm, int db_type);

#endif /* REPORTS_H_ */
