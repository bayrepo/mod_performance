##
##  Makefile -- Build procedure for sample performance Apache module
##  Autogenerated via ``apxs -n performance -g''.
##

##Get system info
UNAME := $(shell uname)
ARCH=$(shell uname -m)

ifeq "$(ARCH)" "x86_64"
BLIB=lib64
CFLAGS=-O2 -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic -fno-strict-aliasing
else
BLIB=lib
CFLAGS=-O2 -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -mtune=generic -fno-strict-aliasing
endif

##Check for Linux
ifeq "$(UNAME)" "Linux"

#Check for Debian
LINNAME := $(shell cat /etc/issue | cut -f1 -d' ' | tr "\n" " " | cut -f1 -d' ')
LINNAME2 := $(shell cat /etc/issue | cut -f3 -d' ' | tr "\n" " " | cut -f1 -d' ')
ifeq "$(LINNAME)" "Debian"
builddir=.
top_builddir=/usr/share/apache2
top_srcdir=/usr/share/apache2
include /usr/share/apache2/build/special.mk
else
#Check for Ubuntu
ifeq "$(LINNAME)" "Ubuntu"
builddir=.
top_builddir=/usr/share/apache2
top_srcdir=/usr/share/apache2
include /usr/share/apache2/build/special.mk
else

ifeq "$(LINNAME2)" "openSUSE"
builddir=.
top_builddir=/usr/share/apache2
top_srcdir=/usr/share/apache2
include /usr/share/apache2/build/special.mk
else

builddir=.
top_srcdir=/etc/httpd
top_builddir=/usr/$(BLIB)/httpd
include /usr/$(BLIB)/httpd/build/special.mk

endif
endif
endif

#   the used tools
APXS=apxs
APACHECTL=apachectl

#   additional defines, includes and libraries
DEFS=$(APVER)
#get apache version
ifeq "$(APVER)" ""
APVER20 := $(shell apachectl -v | grep apache/2.0 -i)
APVER24 := $(shell apachectl -v | grep apache/2.4 -i)
ifneq "$(APVER20)" ""
DEFS=-DAPACHE2_0
endif
ifneq "$(APVER24)" ""
DEFS=-DAPACHE2_4
endif
endif

DEFS := $(DEFS) -Wall

ifeq "$(LINNAME2)" "openSUSE"
INCLUDES=-I/usr/include/apache2-prefork
else
INCLUDES=
endif
LIBS=-L/usr/$(BLIB) -lgd -lrt

endif

##Check for FreeBSD
ifeq "$(UNAME)" "FreeBSD"

#   additional defines, includes and libraries
DEFS=$(APVER)
#get apache version
ifeq "$(APVER)" ""
APVER20 := $(shell apachectl -v | grep apache/2.0 -i)
APVER24 := $(shell apachectl -v | grep apache/2.4 -i)
ifneq "$(APVER20)" ""
DEFS=-DAPACHE2_0
endif
ifneq "$(APVER24)" ""
DEFS=-DAPACHE2_4
endif
endif

DEFS := $(DEFS) -Wall


builddir=.
ifneq "$(APVER20)" ""
top_srcdir=/usr/local/share/apache22
top_builddir=/usr/local/share/apache22
include /usr/local/share/apache22/build/special.mk
endif

ifneq "$(APVER24)" ""
top_srcdir=/usr/local/share/apache24
top_builddir=/usr/local/share/apache24
include /usr/local/share/apache24/build/special.mk
endif

#   the used tools
APXS=apxs
APACHECTL=apachectl

#   additional defines, includes and libraries
#DEFS=-Dmy_define=my_value
#INCLUDES=-Imy/include/dir
#LIBS=-Lmy/lib/dir -lmylib
LIBS=-L/usr/$(BLIB) -L/usr/local/$(BLIB) -lgd -lkvm -lm -lrt


endif

ifneq "$(APVER24)" ""
DEFS=-DAPACHE2_4
endif

#   the default target
all: local-shared-build libr

libr:
ifeq "$(UNAME)" "FreeBSD"
        cc -c send_info.c -Wall -Werror -fpic -o send_info.cc.o -D_LIBBUILD
        cc -c iostat.c -Wall -Werror -fpic -o iostat.cc.o -D_LIBBUILD
        cc -c freebsd_getsysinfo.c -Wall -Werror -fpic -o freebsd_getsysinfo.cc.o -D_LIBBUILD
        cc -c lib-functions.c -Wall -Werror -fpic -D_LIBBUILD
        cc -shared -o libmodperformance.so.0.4 lib-functions.o send_info.cc.o iostat.cc.o freebsd_getsysinfo.cc.o
else
        gcc -c send_info.c -Wall -Werror -fpic -o send_info.cc.o -D_LIBBUILD
        gcc -c iostat.c -Wall -Werror -fpic -o iostat.cc.o -D_LIBBUILD
        gcc -c freebsd_getsysinfo.c -Wall -Werror -fpic -o freebsd_getsysinfo.cc.o -D_LIBBUILD
        gcc -c lib-functions.c -Wall -Werror -fpic -D_LIBBUILD
        gcc -shared -o libmodperformance.so.0.4 lib-functions.o send_info.cc.o iostat.cc.o freebsd_getsysinfo.cc.o
endif

#   install the shared object file into Apache 
install: install-modules-yes

#   cleanup
clean:
	rm -f mod_performance.o mod_performance.lo mod_performance.slo mod_performance.la 

#   simple test
test: reload
	lynx -mime_header http://localhost/performance

#   install and activate shared object by reloading Apache to
#   force a reload of the shared object file
reload: install restart

#   the general Apache start/restart/stop
#   procedures
start:
	$(APACHECTL) start
restart:
	$(APACHECTL) restart
stop:
	$(APACHECTL) stop

