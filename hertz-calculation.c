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
 * hertz-calculation.c
 *
 *  Created on: Jun 13, 2011
 *  Author: SKOREE
 *  E-mail: a@bayrepo.ru
 */


/*
 * Выдрано из ps
 */

#if defined(linux)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

#include <unistd.h>
#include <fcntl.h>

#ifndef HZ
#include <netinet/in.h>		/* htons */
#endif

#include "hertz-calcualtion.h"

static char buf[2048];

#define FILE_TO_BUF(filename, fd) do{                           \
    static int local_n;                                         \
    if (fd == -1 && (fd = open(filename, O_RDONLY)) == -1) {    \
      if(stat_fd>0) close(stat_fd);                             \
      if(uptime_fd) close(uptime_fd);                           \
      return (unsigned long long)-1;                            \
    }                                                           \
    lseek(fd, 0L, SEEK_SET);                                    \
    if ((local_n = read(fd, buf, sizeof buf - 1)) < 0) {        \
      if(stat_fd>0) close(stat_fd);                             \
      if(uptime_fd) close(uptime_fd);                           \
      return (unsigned long long)-1;                            \
    }                                                           \
    buf[local_n] = '\0';                                        \
}while(0)

#define LINUX_VERSION(x,y,z)   (0x10000*(x) + 0x100*(y) + z)

#define STAT_FILE    "/proc/stat"
static int stat_fd = -1;
#define UPTIME_FILE  "/proc/uptime"
static int uptime_fd = -1;

static long smp_num_cpus;
static unsigned long long Hertz = (unsigned long long) -1;

static unsigned long long
old_Hertz_hack (void)
{
  unsigned long long user_j, nice_j, sys_j, other_j;	/* jiffies (clock ticks) */
  double up_1, up_2, seconds;
  unsigned long long jiffies;
  unsigned h;
  char *savelocale;

  savelocale = setlocale (LC_NUMERIC, NULL);
  setlocale (LC_NUMERIC, "C");
  do
    {
      FILE_TO_BUF (UPTIME_FILE, uptime_fd);
      sscanf (buf, "%lf", &up_1);
      /* uptime(&up_1, NULL); */
      FILE_TO_BUF (STAT_FILE, stat_fd);
      sscanf (buf, "cpu %Lu %Lu %Lu %Lu", &user_j, &nice_j, &sys_j, &other_j);
      FILE_TO_BUF (UPTIME_FILE, uptime_fd);
      sscanf (buf, "%lf", &up_2);
      /* uptime(&up_2, NULL); */
    }
  while ((long long) ((up_2 - up_1) * 1000.0 / up_1));	/* want under 0.1% error */
  setlocale (LC_NUMERIC, savelocale);
  jiffies = user_j + nice_j + sys_j + other_j;
  seconds = (up_1 + up_2) / 2;
  //h = (unsigned)( (double)jiffies/seconds/smp_num_cpus );
  h = (unsigned) ((double) jiffies / seconds);
  /* actual values used by 2.4 kernels: 32 64 100 128 1000 1024 1200 */
  switch (h)
    {
    case 9 ... 11:
      Hertz = 10;
      break;			/* S/390 (sometimes) */
    case 18 ... 22:
      Hertz = 20;
      break;			/* user-mode Linux */
    case 30 ... 34:
      Hertz = 32;
      break;			/* ia64 emulator */
    case 48 ... 52:
      Hertz = 50;
      break;
    case 58 ... 61:
      Hertz = 60;
      break;
    case 62 ... 65:
      Hertz = 64;
      break;			/* StrongARM /Shark */
    case 95 ... 105:
      Hertz = 100;
      break;			/* normal Linux */
    case 124 ... 132:
      Hertz = 128;
      break;			/* MIPS, ARM */
    case 195 ... 204:
      Hertz = 200;
      break;			/* normal << 1 */
    case 247 ... 252:
      Hertz = 250;
      break;
    case 253 ... 260:
      Hertz = 256;
      break;
    case 393 ... 408:
      Hertz = 400;
      break;			/* normal << 2 */
    case 790 ... 808:
      Hertz = 800;
      break;			/* normal << 3 */
    case 990 ... 1010:
      Hertz = 1000;
      break;			/* ARM */
    case 1015 ... 1035:
      Hertz = 1024;
      break;			/* Alpha, ia64 */
    case 1180 ... 1220:
      Hertz = 1200;
      break;			/* Alpha */
    default:
#ifdef HZ
      Hertz = (unsigned long long) HZ;	/* <asm/param.h> */
#else
      /* If 32-bit or big-endian (not Alpha or ia64), assume HZ is 100. */
      Hertz = (sizeof (long) == sizeof (int)
	       || htons (999) == 999) ? 100UL : 1024UL;
#endif
    }
  if (stat_fd > 0)
    close (stat_fd);
  if (uptime_fd)
    close (uptime_fd);
  return Hertz;
}

unsigned long long
init_libproc (void)
{
  smp_num_cpus = sysconf (_SC_NPROCESSORS_ONLN);
  if (smp_num_cpus < 1)
    smp_num_cpus = 1;		/* SPARC glibc is buggy */

  return old_Hertz_hack ();
}

#endif
