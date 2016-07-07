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
 * common.h
 *
 *  Created on: May 28, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#ifndef COMMON_H_
#define COMMON_H_

#define PERFORMANCE_MODULE_VERSION "0.4-17"

#define MODULE_PREFFIX "mod_performance: "
#define ITEMS_PER_PAGE 100
#define FRESH_VERSION_URL "https://github.com/bayrepo/mod_performance/archive/master.zip"
#define DOCUMENTATION_URL "http://wiki.bayrepo.net/docs"

char *performance_module_get_parameter_from_uri (request_rec * r, char *name);
void
performance_module_show_common_part (request_rec * r, int admin);
void performance_module_show_footer_part (request_rec * r);
void print_js_content(request_rec *r);
char *
get_host_name (request_rec * r);

#endif /* COMMON_H_ */
