mod_performance.la: mod_performance.slo send_info.slo tid_pid_list.slo tid_pid_list_ut.slo sql_adapter.slo reports.slo common.slo iostat.slo hertz-calculation.slo timers.slo chart.slo html-sources.slo debug.slo custom_report.slo freebsd_getsysinfo.slo 
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version mod_performance.lo common.lo send_info.lo tid_pid_list.lo tid_pid_list_ut.lo sql_adapter.lo reports.lo iostat.lo hertz-calculation.lo timers.lo chart.lo html-sources.lo debug.lo custom_report.lo freebsd_getsysinfo.lo $(LIBS)
DISTCLEAN_TARGETS = modules.mk
shared =  mod_performance.la
