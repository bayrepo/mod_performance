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
 * freebsd_getsysinfo.c
 *
 *  Created on: Jun 19, 2011
 *  Author: SKOREE
 *  E-mail: alexey_com@ukr.net
 *  Site: lexvit.dn.ua
 */

#if defined(__FreeBSD__)

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <apr_general.h>
#include <apr_pools.h>

#include <kvm.h>
#include <paths.h>
#include <limits.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>


#include "freebsd_getsysinfo.h"

#define tv2sec(tv)	(((uint64_t) tv.tv_sec * 1000000) + (uint64_t) tv.tv_usec)

fixpt_t ccpu;			/* kernel _ccpu variable */
unsigned long mempages = 1;	/* number of pages of phys. memory */
int fscale = 1;
int memorypagesize = 1;

long frglhz;

int
getstathz (void)
{
  int mib[2];
  size_t size;
  struct clockinfo clockrate;

  mib[0] = CTL_KERN;
  mib[1] = KERN_CLOCKRATE;
  size = sizeof clockrate;
  if (sysctl (mib, 2, &clockrate, &size, NULL, 0) == -1)
    return -1;
  return clockrate.stathz;
}

int
gethz (void)
{
  int mib[2];
  size_t size;
  struct clockinfo clockrate;

  mib[0] = CTL_KERN;
  mib[1] = KERN_CLOCKRATE;
  size = sizeof clockrate;
  if (sysctl (mib, 2, &clockrate, &size, NULL, 0) == -1)
    return -1;
  return clockrate.hz;
}

int
init_freebsd_info (void)
{
  size_t oldlen;
  size_t size;

  oldlen = sizeof (ccpu);
  if (sysctlbyname ("kern.ccpu", &ccpu, &oldlen, NULL, 0) == -1)
    return (1);
  oldlen = sizeof (fscale);
  if (sysctlbyname ("kern.fscale", &fscale, &oldlen, NULL, 0) == -1)
    return (1);
  oldlen = sizeof (mempages);
  if (sysctlbyname ("hw.availpages", &mempages, &oldlen, NULL, 0) == -1)
    return (1);
  memorypagesize = getpagesize ();
  frglhz = (long) (1.0 / (gethz () * 1.0) * 1000000.0);
  if (frglhz < 0)
    return (1);
  return (0);
}

void
get_freebsd_mem_info (void * p, pid_t pid, freebsd_get_mem * fgm)
{
	get_freebsd_mem_info_ret(fgm, pid, -1);
}

int
get_freebsd_mem_info_ret (freebsd_get_mem * fgm, pid_t pid, pid_t tid)
{
  const char *nlistf, *memf;
  memf = _PATH_DEVNULL;
  static kvm_t *kd;
  struct kinfo_proc *kp;
  char errbuf[_POSIX2_LINE_MAX];
  int flag = pid, what = KERN_PROC_PID, nentries, i;
  nlistf = NULL;
  fgm->memory_bytes = 0;
  fgm->memory_percents = 0;
  fgm->memory_total = 0;
  kd = kvm_openfiles (nlistf, memf, NULL, O_RDONLY, errbuf);
  if (kd == 0)
    {
      return -1;
    }
  else
    {
      kp = kvm_getprocs (kd, what, flag, &nentries);
      if ((kp == NULL && nentries > 0) || (kp != NULL && nentries < 0))
	{
	  kvm_close (kd);
	  return -1;
	}
      else
	{
	  if (nentries > 0)
	    {
	      for (i = nentries; --i >= 0; ++kp)
		{
		  fgm->memory_bytes = kp->ki_rssize * memorypagesize;
		  fgm->memory_percents = (double) (kp->ki_rssize) / mempages;
		  fgm->memory_total = mempages * memorypagesize;
		  break;
		}
	    } else {
	    	kvm_close (kd);
	    	return -1;
	    }
	}
      kvm_close (kd);
    }
  return 0;
}

void
get_freebsd_cpu_info (void * p, freebsd_glibtop_cpu * buf)
{
  int i;
  long cpts[CPUSTATES];
  for (i = 0; i < CPUSTATES; i++)
    cpts[i] = 0;
  size_t size = sizeof (cpts);
  memset (buf, 0, sizeof (freebsd_glibtop_cpu));

  if (!sysctlbyname ("kern.cp_time", &cpts, &size, NULL, 0))
    {
      buf->user = cpts[CP_USER] * frglhz;
      buf->nice = cpts[CP_NICE] * frglhz;
      buf->sys = cpts[CP_SYS] * frglhz;
      buf->idle = cpts[CP_IDLE] * frglhz;
      buf->iowait = cpts[CP_INTR] * frglhz;

      buf->total = cpts[CP_USER] * frglhz + cpts[CP_NICE] * frglhz
	+ cpts[CP_SYS] * frglhz + cpts[CP_IDLE] * frglhz;
    }

}

void
get_freebsd_cpukvm_info (void * p, pid_t pid, freebsd_get_cpu * fgc)
{
  const char *nlistf, *memf;
  memf = _PATH_DEVNULL;
  static kvm_t *kd;
  struct kinfo_proc *kp;
  int dummy;
  fgc->process_cpu = 0;
  fgc->system_cpu = 0;
  char errbuf[_POSIX2_LINE_MAX];
  int flag = pid, what = KERN_PROC_PID, nentries, i;
  nlistf = NULL;
  kd = kvm_openfiles (nlistf, memf, NULL, O_RDONLY, errbuf);
  if (kd == 0)
    {
      //fprintf(stderr, "%s\n", errbuf);
      dummy = 1;
    }
  else
    {
      kp = kvm_getprocs (kd, what, flag, &nentries);
      if ((kp == NULL && nentries > 0) || (kp != NULL && nentries < 0))
	{
	  //fprintf(stderr, "%s\n", kvm_geterr(kd));
	  dummy = 1;
	}
      else
	{
	  if (nentries > 0)
	    {
	      for (i = nentries; --i >= 0; ++kp)
		{
		  fgc->process_cpu =
		    kp->ki_runtime + tv2sec (kp->ki_childtime);
		  fgc->system_cpu = 0;
		  break;
		}
	    }
	}
      kvm_close (kd);
    }
}

void
get_freebsd_io_info (void * p, pid_t pid, freebsd_get_io * fgi)
{
  const char *nlistf, *memf;
  memf = _PATH_DEVNULL;
  static kvm_t *kd;
  struct kinfo_proc *kp;
  char errbuf[_POSIX2_LINE_MAX];
  int flag = pid, what = KERN_PROC_PID, nentries, i;
  nlistf = NULL;
  int dummy;
  fgi->io_write = 0;
  fgi->io_read = 0;
  kd = kvm_openfiles (nlistf, memf, NULL, O_RDONLY, errbuf);
  if (kd == 0)
    //fprintf(stderr, "%s\n", errbuf);
    dummy = 1;
  else
    {
      kp = kvm_getprocs (kd, what, flag, &nentries);
      if ((kp == NULL && nentries > 0) || (kp != NULL && nentries < 0))
	{
	  //fprintf(stderr, "%s\n", kvm_geterr(kd));
	  dummy = 1;
	}
      else
	{
	  if (nentries > 0)
	    {
	      for (i = nentries; --i >= 0; ++kp)
		{
		  fgi->io_read = kp->ki_rusage.ru_inblock;
		  fgi->io_write = kp->ki_rusage.ru_oublock;
		  //fgi->io_read += kp->ki_rusage_ch.ru_inblock;
		  //fgi->io_write += kp->ki_rusage_ch.ru_oublock;
		  break;
		}
	    }
	}
      kvm_close (kd);
    }
}

#endif
