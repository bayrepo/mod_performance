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
 * timers.c
 *
 *  Created on: Jul 7, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include <apr_general.h>
#include <apr_pools.h>

#include <regex.h>

#include "timers.h"

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define MAX_REGISTERED_SIGNALS 10

static int global_signal;
static int number_of_signals;
static timer_t timerid[MAX_REGISTERED_SIGNALS];
static struct sigaction sa[MAX_REGISTERED_SIGNALS];
static struct sigevent sev[MAX_REGISTERED_SIGNALS];
static struct itimerspec its[MAX_REGISTERED_SIGNALS];


static apr_status_t
deletetimer (void *number)
{
  intptr_t nm = (intptr_t) number;
  int nm_t = (int) nm;
  timer_delete (timerid[nm_t]);
  return APR_SUCCESS;
}

long
gettimeinterval (char *timestring)
{
  regex_t compiled_regex;
  regex_t compiled_regex_all;
  regmatch_t regexp_result;
  //size_t matches = 1;
  char buffer[512];
  time_t rawtime;
  struct tm *timeinfo;
  int error;
  int hours = 0, minutes = 0, seconds = 0;
  if (regcomp
      (&compiled_regex_all,
       "^[0-9]\\{1,2\\}[:-][0-9]\\{1,2\\}[:-]\\{0,1\\}[0-9]\\{0,2\\}$", 0))
    {
      return 0;
    }
  error = regexec (&compiled_regex_all, timestring, 1, &regexp_result, 0);
  if (error)
    {
      regfree (&compiled_regex_all);
      return 0;
    }
  regfree (&compiled_regex_all);
  if (regcomp (&compiled_regex, "[0-9]\\{1,2\\}", 0))
    {
      return 0;
    }
  char *p = timestring;
  error = regexec (&compiled_regex, p, 1, &regexp_result, 0);
  int counter = 0;
  while (error == 0)
    {				/* while matches found */
      strncpy (buffer, p + regexp_result.rm_so,
	       regexp_result.rm_eo - regexp_result.rm_so);
      buffer[regexp_result.rm_eo - regexp_result.rm_so] = 0;
      switch (counter)
	{
	case 0:
	  hours = atoi (buffer);
	  if ((hours < 0) || (hours > 23))
	    {
	      regfree (&compiled_regex);
	      return 0;
	    }
	  break;
	case 1:
	  minutes = atoi (buffer);
	  if ((minutes < 0) || (minutes > 59))
	    {
	      regfree (&compiled_regex);
	      return 0;
	    }
	  break;
	case 2:
	  seconds = atoi (buffer);
	  if ((seconds < 0) || (seconds > 59))
	    {
	      regfree (&compiled_regex);
	      return 0;
	    }
	  break;
	default:
	  regfree (&compiled_regex);
	  return 0;
	}
      p = p + regexp_result.rm_eo;
      error = regexec (&compiled_regex, p, 1, &regexp_result, REG_NOTBOL);
      counter++;
    }

  regfree (&compiled_regex);
  time (&rawtime);
  timeinfo = localtime (&rawtime);

  long current_tm =
    timeinfo->tm_hour * 3600 + timeinfo->tm_min * 60 + timeinfo->tm_sec;
  long need_tm = hours * 3600 + minutes * 60 + seconds;

  if (need_tm > current_tm)
    return need_tm - current_tm;
  else
    {
      return 24 * 3600 - current_tm + need_tm;
    }

}

void
reset_timer (int sig, char *new_time)
{
  int number_of_signals = sig - global_signal;
  its[number_of_signals].it_value.tv_sec = gettimeinterval (new_time);
  its[number_of_signals].it_value.tv_nsec = 0;
  timer_settime (timerid[number_of_signals], 0, &its[number_of_signals],
		 NULL);
}

int
inittimer ()
{
  global_signal = SIG;
  number_of_signals = 0;
  return 0;
}

int
addtimer (apr_pool_t * p, int sec, int repeat, void
	  (*handler) (int, siginfo_t *, void *))
{
  if (number_of_signals < MAX_REGISTERED_SIGNALS)
    {
      sa[number_of_signals].sa_flags = SA_SIGINFO;
      sa[number_of_signals].sa_sigaction = *handler;
      sigemptyset (&sa[number_of_signals].sa_mask);
      if (sigaction
	  (global_signal + number_of_signals, &sa[number_of_signals],
	   NULL) == -1)
	{
	  return -1;
	}
      else
	{
	  sev[number_of_signals].sigev_notify = SIGEV_SIGNAL;
	  sev[number_of_signals].sigev_signo = global_signal
	    + number_of_signals;
	  sev[number_of_signals].sigev_value.sival_ptr
	    = &timerid[number_of_signals];
	  if (timer_create (CLOCKID, &sev[number_of_signals],
			    &timerid[number_of_signals]) == -1)
	    {
	      return -1;
	    }
	  else
	    {
	      its[number_of_signals].it_value.tv_sec = sec;
	      its[number_of_signals].it_value.tv_nsec = 0;
	      switch (repeat)
		{
		case 0:
		  its[number_of_signals].it_interval.tv_sec =
		    its[number_of_signals].it_value.tv_sec;
		  its[number_of_signals].it_interval.tv_nsec =
		    its[number_of_signals].it_value.tv_nsec;
		  break;
		case 1:
		  its[number_of_signals].it_interval.tv_sec = 0;
		  its[number_of_signals].it_interval.tv_nsec = 0;
		  break;
		case 2:
		  its[number_of_signals].it_interval.tv_sec = 24 * 60 * 60;
		  its[number_of_signals].it_interval.tv_nsec = 0;
		  break;
		}
	      if (timer_settime
		  (timerid[number_of_signals], 0, &its[number_of_signals],
		   NULL) == -1)
		{
		  timer_delete (timerid[number_of_signals]);
		  return -1;
		}
	      else
		{
		  intptr_t t = (intptr_t) number_of_signals;
		  apr_pool_cleanup_register (p, (void *) t, deletetimer,
					     apr_pool_cleanup_null);
		  number_of_signals++;
		  return global_signal + number_of_signals;
		}
	    }
	}
    }
  else
    {
      return -1;
    }
}
