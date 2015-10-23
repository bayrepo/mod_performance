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
 * sql_adapter.h
 *
 *  Created on: May 21, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#ifndef SQL_ADAPTER_H_
#define SQL_ADAPTER_H_

int sql_adapter_load_library (apr_pool_t * p, int db_type);
int
sql_adapter_connect_db (apr_pool_t * p, int db_type, char *host,
			char *username, char *password, char *dbname,
			char *path);
char *sql_adapter_get_create_table (apr_pool_t * p, int db_type,
				    apr_thread_mutex_t * mutex_db);
char *sql_adapter_get_insert_table (apr_pool_t * p, int db_type,
				    char *hostname, char *uri, char *script,
				    double dtime, double dcpu, double dmemory,
				    apr_thread_mutex_t * mutex_db,
				    double dcpu_sec, double dmemory_mb,
				    double dbr, double dbw);
char *sql_adapter_get_delete_table (apr_pool_t * p, int db_type, int days,
				    apr_thread_mutex_t * mutex_db);
char *sql_adapter_get_date_period (apr_pool_t * p, char *period_begin,
				   char *period_end, char *period, int db_type, char *alias);
char *sql_adapter_optimize_table (apr_pool_t * p, int db_type,
				  apr_thread_mutex_t * mutex_db);

typedef void (*call_back_function1) (void *, char *, char *, char *, char *,
				     char *, char *, char *, char *, char *,
				     char *, char *, char *);

char *sql_adapter_get_limit (apr_pool_t * p, char *page_number, int per_page,
			     int db_type);
char *sql_adapter_get_full_text_info (apr_pool_t * p, int db_type, void *r,
				      char *period, char *host, char *script,
				      char *uri, char *period_begin,
				      char *period_end, int sort, int tp, char *page_number,
				      int per_page,
				      void (*call_back_function1) (void *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *,
								   char *));

typedef void (*call_back_function2) (void *, int, char *);
char *sql_adapter_get_full_text_info_count (apr_pool_t * p, int db_type,
					    void *r, char *period, char *host,
					    char *script, char *uri,
					    char *period_begin,
					    char *period_end,
					    char *page_number, int per_page,
					    void (*call_back_function2) (void
									 *,
									 int,
									 char
									 *));

typedef void (*call_back_function3) (void *, int, double, double, double, double, void *, int);
//void (*call_back_function3)(void *r, int dateadd, double max_cpu, double max_memory, void *ptr);

char *sql_adapter_get_full_text_info_picture (apr_pool_t * p, int db_type,
					      void *r, char *period,
					      char *host, char *script,
					      char *uri, char *period_begin,
					      char *period_end,
					      char *page_number, int per_page,
					      void (*call_back_function3)
					      (void *, int, double, double, double, double, void *,
					          int), void *ptr, int number);


//-------------------------Reports------------------------------------
char *sql_adapter_get_cpu_max_text_info (apr_pool_t * p, int db_type, void *r,
					 char *period, char *host,
					 char *script, char *uri,
					 char *period_begin, char *period_end,
					 char *page_number, int per_page,
					 void (*call_back_function1) (void *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char
								      *));
char *sql_adapter_get_cpu_max_text_info_no_hard (apr_pool_t * p, int db_type, void *r,
                                         char *period, char *host,
                                         char *script, char *uri,
                                         char *period_begin, char *period_end,
                                         char *page_number, int per_page,
                                         void (*call_back_function1) (void *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char
                                                                      *));
char *sql_adapter_get_mem_max_text_info (apr_pool_t * p, int db_type, void *r,
					 char *period, char *host,
					 char *script, char *uri,
					 char *period_begin, char *period_end,
					 char *page_number, int per_page,
					 void (*call_back_function1) (void *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char *,
								      char
								      *));
char *sql_adapter_get_mem_max_text_info_no_hard (apr_pool_t * p, int db_type, void *r,
                                         char *period, char *host,
                                         char *script, char *uri,
                                         char *period_begin, char *period_end,
                                         char *page_number, int per_page,
                                         void (*call_back_function1) (void *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char *,
                                                                      char
                                                                      *));
char *sql_adapter_get_time_max_text_info (apr_pool_t * p, int db_type,
					  void *r, char *period, char *host,
					  char *script, char *uri,
					  char *period_begin,
					  char *period_end, char *page_number,
					  int per_page,
					  void (*call_back_function1) (void *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char *,
								       char
								       *));
char *sql_adapter_get_time_max_text_info_no_hard (apr_pool_t * p, int db_type,
                                          void *r, char *period, char *host,
                                          char *script, char *uri,
                                          char *period_begin,
                                          char *period_end, char *page_number,
                                          int per_page,
                                          void (*call_back_function1) (void *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char *,
                                                                       char
                                                                       *));

typedef void (*call_back_function4) (void *, char *, char *, char *);
//void (*call_back_function1)(void *r, char *host, char *prct);

char *sql_adapter_get_host_text_info (apr_pool_t * p, int db_type, void *r,
				      char *period, char *host, char *script,
				      char *uri, char *period_begin,
				      char *period_end, int sort, int tp, char *page_number,
				      int per_page,
				      void (*call_back_function4) (void *,
								   char *,
								   char *,
								   char *));
char *sql_adapter_get_host_click_text_info (apr_pool_t * p, int db_type,
					    void *r, char *period, char *host,
					    char *script, char *uri,
					    char *period_begin,
					    char *period_end, int sort,
					    char *page_number, int per_page,
					    void (*call_back_function4) (void
									 *,
									 char
									 *,
									 char
									 *));

typedef void (*call_back_function5) (void *, char *, char *, char *, char *,
				     char *, char *, char *, char *, char *,
				     char *);
//void (*call_back_function5)(void *r, char *field1, char *field2, char *field3, char *field4, char *field5, char *field6, char *field7, char *field8);

char *sql_adapter_get_avg_text_info (apr_pool_t * p, int db_type, void *r,
				     char *period, char *host, char *script,
				     char *uri, char *period_begin,
				     char *period_end, int sort, int tp,char *page_number,
				     int per_page,
				     void (*call_back_function5) (void *,
								  char *,
								  char *,
								  char *,
								  char *,
								  char *,
								  char *
								  ));
typedef void (*call_back_function4_data) (void *, char *,
    double, void *);

char *
sql_adapter_get_host_text_info_picture (apr_pool_t * p, int db_type, void *r,
                                char *period, char *host, char *script,
                                char *uri, char *period_begin,
                                char *period_end, int sort, char *page_number,
                                int per_page,
                                void (*call_back_function4_data) (void *, char *,
                                                             double, void *), void *ptr);

typedef void (*call_back_function4_1) (void *, char *, double, void *);

char *
sql_adapter_get_host_click_text_info_picture(apr_pool_t * p, int db_type, void *r,
    char *period, char *host, char *script, char *uri, char *period_begin,
    char *period_end, int sort, char *page_number, int per_page, void
    (*call_back_function4_1)(void *, char *, double, void *), void *ptr);

typedef void
    (*call_back_function5_1)(void *, char *, double, double, double, double,
        double, void *, int);

char *
sql_adapter_get_avg_text_info_picture(apr_pool_t * p, int db_type, void *r,
    char *period, char *host, char *script, char *uri, char *period_begin,
    char *period_end, int sort, char *page_number, int per_page, void
    (*call_back_function5_1)(void *, char *, double, double, double, double,
        double, void *, int), void * ptr, int number);

typedef void
    (*call_back_function6)(void *, char *, char *, char *, char *, char *,
        char *, char *, char *, char *);

char *
sql_adapter_get_exec_tm(apr_pool_t * p, int db_type, void *r,
    char *period, char *host, char *script, char *uri, char *period_begin,
    char *period_end, char *page_number, int per_page, void
    (*call_back_function6)(void *, char *, char *, char *, char *, char *,
        char *, char *, char *, char *));

char *
sql_adapter_get_exec_tm_common(apr_pool_t * p, int db_type, void *r,
    char *period, char *host, char *script, char *uri, char *period_begin,
    char *period_end, char *page_number, int per_page, void
    (*call_back_function6)(void *, char *, char *, char *, char *, char *,
        char *, char *, char *, char *));

char *
sql_adapter_get_custom_text_info(apr_pool_t * p, int db_type, void *r,
		char *period, char *host, char *script, char *uri, char *period_begin,
		char *period_end, int sort, int tp, char *page_number, int per_page,
		void * itm);

#endif /* SQL_ADAPTER_H_ */
