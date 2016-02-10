Summary: Apache module
Name: mod_performance04
Version: 0.4
Release: 9%{?dist}
Source0: %{name}-%{version}.tar.bz2
Group: System Environment/Daemons
License: ASL 2.0                                                                                                                       
URL: http://lexvit.dn.ua
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: httpd-devel
BuildRequires: apr-devel                                                                                                                                                                                                                                                                                                                                                                                                                                                 
BuildRequires: gd-devel 

Requires: httpd
Requires: apr
Requires: gd

%description
This package contains Apache module

%prep
%setup -q

%build
make

%install
mkdir -p $RPM_BUILD_ROOT/opt/performance
mkdir -p $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d/
mkdir -p $RPM_BUILD_ROOT%{_libdir}/
mkdir -p $RPM_BUILD_ROOT/opt/lexvit/mod_performance04
install -m 644 -D mod_performance.conf $RPM_BUILD_ROOT/%{_sysconfdir}/httpd/conf.d/
install -m 755 -D .libs/mod_performance.so  $RPM_BUILD_ROOT/opt/lexvit/mod_performance04/
install -m 644 -D custom.tpl $RPM_BUILD_ROOT/opt/performance/
install -D -m 755 libmodperformance.so.0.4 $RPM_BUILD_ROOT%{_libdir}/libmodperformance.so.0.4
cp libmodperformance.so $RPM_BUILD_ROOT%{_libdir}/libmodperformance.so

%posttrans
ldconfig
 

%clean 
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
/opt/lexvit/mod_performance04/*
%config(noreplace) %{_sysconfdir}/httpd/conf.d/*
%attr(777,root,root) /opt/performance
%config(noreplace) /opt/performance/custom.tpl
%{_libdir}/libmodperformance.so
%{_libdir}/libmodperformance.so.0.4

%changelog
* Thu Feb 11 2016 Alexey Berezhok <alexey_com@ukr.net> 0.4-9
- Bug fixes

* Tue Oct 27 2015 Alexey Berezhok <alexey_com@ukr.net> 0.4-8
- Bug fixes

* Wed Oct 29 2014 Alexey Berezhok <alexey_com@ukr.net> 0.4-7
- Cosmetic fixes

* Tue Oct 28 2014 Alexey Berezhok <alexey_com@ukr.net> 0.4-6
- Added PerformanceWorkMode

* Mon Aug 04 2014 Alexey Berezhok <alexey_com@ukr.net> 0.4-5
- Added libmodperformance building

* Sun Aug 03 2014 Alexey Berezhok <alexey_com@ukr.net> 0.4-4
- Added renewed documentation

* Sat Mar 23 2013 Alexey Berezhok <alexey_com@ukr.net> 0.4-3
- Added custom reports
- Rebuild module main algorithm
- Fixed some bugs
- Added global data storage
- Added data buferization

* Sun Nov 13 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-12
- Added custom port for postgresql connection

* Sat Oct 08 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-11
- Added patch for FASTCGI PHP
- Added mysql_ping
- Added PerformanceSocketType
- Added PerformancePeriodicalWatch
- Added patch mod_cgi

* Tue Sep 24 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-10
- Support openSUSE building
- Support openSUSE install

* Tue Sep 21 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-9
- Restored old Max-reports
- Added Time execution range report per host
- Added local table sorting 
- Added silent mode
- Removed pool cleaning

* Sun Sep 04 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-8
- Combined reports "Host request statistic" and "Host request number"
- Added local per page sort
- Changed select algorithm of Max-reports and "Time execution range" reports
- Added order type

* Mon Aug 22 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-7
- Added new built-in report - Time execution range

* Sat Aug 20 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-6
- Added PerformanceMinExecTime parameter
- Added new graphic drawer
- Added more graphical pages
- Rebuild web-interface
- Removes errors from virtual host to main log
- Added free sort by filed number
- Added Ubuntu build
- Added support mysql 5.0, mysql 5.1

* Thu Aug 04 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-5
- Minor fixes

* Wed Aug 03 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-4
- Removed action of cpuinfo dumping from apache module part to daemon
- Added PerformanceUseCPUTopMode - Irix/Solaris mode
- Removed PerformanceUseCPUUsageLikeTop

* Tue Jul 27 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-3
- Added mod_fcgid patch for mod_performance cooperation
- Added filter for external handlers (PerformanceExternalHandler)

* Wed Jul 20 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-2
- Added suphp patch for mod_performance cooperation

* Tue Jul 19 2011 Alexey Berezhok <alexey_com@ukr.net> 0.3-1
- Added registering external function for suphp, mod_fcgid and other external modules

* Tue Jul 12 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-22
- Added Debian support

* Sun Jul 10 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-21
- Added parameter PerformanceFragmentationTime
- Removed self check thread, instead it added timer

* Thu Jul 07 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-20
- Added timer for daemon restarting(interval or daily time in format HH:MM:SS)

* Sat Jul 02 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-19
- Fixed error of daemon restarting every time, when items deleted from base. Added memory limitation

* Sat Jul 02 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-18
- Added self check daemon mode with output statistics into error_log

* Sat Jul 02 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-17
- Added algorithm of CPU usage like top (for linux only)

* Tue Jun 28 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-16
- Fixed algorithm of CPU usage

* Thu Jun 23 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-15
- Removed dependency from lingtop2 and glib2
- Fixed algorithm of CPU usage calculating under Linux and FreeBSD

* Sun Jun 19 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-14
- Added FreeBSD 8 support

* Mon Jun 13 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-13
- Fixed algorithm of HZ calculation
- Changed algorithm of CPU time calculation

* Sun Jun 12 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-12
- Fixed all warning that outputs until build of module

* Sat Jun 04 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-11
- Added PostgreSQL support (http://lexvit.dn.ua/bugtraq/modperformance/ticket-13)

* Wed Jun 01 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-10
- Fixed bug 503 error (http://lexvit.dn.ua/bugtraq/modperformance/ticket-28)
- Fixed bug with report (http://lexvit.dn.ua/bugtraq/modperformance/ticket-26)
- Added support IO into avg. report, Kb/s into Kb (http://lexvit.dn.ua/bugtraq/modperformance/ticket-21)

* Wed Jun 01 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-9
- Fix bug with log

* Tue May 31 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-8
- Added io into reports output(http://lexvit.dn.ua/bugtraq/modperformance/ticket-21)
- Fix hostname garbage bug (http://lexvit.dn.ua/bugtraq/modperformance/ticket-23)
- Divide db pointer (http://lexvit.dn.ua/bugtraq/modperformance/ticket-22)

* Tue May 31 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-7
- Added calculation average io

* Sun May 29 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-6
- Added old reports(rest of them)

* Sun May 29 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-5
- Added support mod_ruid2 (http://lexvit.dn.ua/bugtraq/modperformance/ticket-20)
- Added old reports(part of them)

* Mon May 23 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-4
- Added support mod_ruid2 (http://lexvit.dn.ua/bugtraq/modperformance/ticket-18)

* Sun May 22 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-3
- Added support MySQL (http://lexvit.dn.ua/bugtraq/modperformance/ticket-9)

* Sat May 21 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-2
- Fixed bug with X incorrect time zone(http://lexvit.dn.ua/bugtraq/modperformance/ticket-14)

* Fri May 20 2011 Alexey Berezhok <alexey_com@ukr.net> 0.2-1
- Added PerformanceLog, PerformanceLogFormat, PerformanceLogType(http://lexvit.dn.ua/bugtraq/modperformance/ticket-10)

* Thu May 12 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-19
- Change owner of database file to Apache user and 600 permission
- Disable Glib warning and messages

* Tue May 03 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-18
- Fix some small mistakes 

* Sat Apr 30 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-17
- Add Average usage mode
- Add version display

* Fri Apr 29 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-16
- Add PerformanceExtended mode

* Fri Apr 29 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-15
- Modify history screen output
- Add output requests per host

* Fri Apr 29 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-14
- Add history screen mode

* Fri Apr 29 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-13
- Add period_begin, period_end

* Tue Apr 28 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-12
- Add PerformanceUseCanonical - use canonical script name
- Ignore redirect-handler in PerformanceUseCanonical mode

* Tue Apr 26 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-11
- Fix bug double free
- Add output modes: max CPU, max Mem, max exec. time, host request stat.
- Change calculation max cpu(mem, time) per time interval instead sum

* Sun Apr 10 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-10
- Restore saving filename instead canonical :(

* Sun Apr 10 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-9
- Add saving canonical_filename instead filename

* Sun Apr 10 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-8
- Set default thread stack size 10M
- Fix error PerformanceStackSize parameter(it set in Kb instead Mb)
- Remove PerformanceMaxThreads value multiplying

* Sat Apr 09 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-7
- Fix errors accessing information between hosts and admin

* Fri Apr 08 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-6
- Add PerformanceStackSize
- Add PerformanceUserHandler
- Optimize source code
- Remove errno from NOTICE messages

* Wed Apr 06 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-5
- Restore stack size to 10M. Add threads pool
- Add PerformanceMaxThreads
- Fix error with PerformanceURI

* Sun Apr 03 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-4
- Change stack size of thread to 1024 blocks. Add missing JOIN functions.

* Fri Apr 01 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-3
- Change condition checking. Added default config file

* Wed Mar 30 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-2
- Removed some memory leaks

* Sun Mar 28 2011 Alexey Berezhok <alexey_com@ukr.net> 0.1-1
- Initial package. Beta version

