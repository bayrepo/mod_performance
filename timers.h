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
 * timers.h
 *
 *  Created on: Jul 7, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#ifndef TIMERS_H_
#define TIMERS_H_

typedef void (*handler) (int sig, siginfo_t * si, void *uc);

long gettimeinterval (char *timestring);
void reset_timer (int sig, char *new_time);
int inittimer ();
int addtimer (apr_pool_t * p, int sec, int repeat,
	      void (*handler) (int, siginfo_t *, void *));


#endif /* TIMERS_H_ */
