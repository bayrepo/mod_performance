%global contentdir  /var/www
# API/ABI check
%global apiver      20100412
%global zendver     20100525
%global pdover      20080721
# Extension version
%global fileinfover 1.0.5
%global pharver     2.0.1
%global zipver      1.11.0
%global jsonver     1.2.1

%global httpd_mmn %(cat %{_includedir}/httpd/.mmn || echo missing-httpd-devel)
%global mysql_sock %(mysql_config --socket || echo /var/lib/mysql/mysql.sock)

# Regression tests take a long time, you can skip 'em with this
%{!?runselftest: %{expand: %%global runselftest 1}}

# Use the arch-specific mysql_config binary to avoid mismatch with the
# arch detection heuristic used by bindir/mysql_config.
%global mysql_config %{_libdir}/mysql/mysql_config

%ifarch %{ix86} x86_64
%global with_fpm 1
%else
%global with_fpm 0
%endif

%global with_zip       1
%global with_libzip    0
%global zipmod         %nil
%global with_litespeed 1

%global real_name php
%global name php54
%global base_ver 5.4

Summary: PHP scripting language for creating dynamic web sites
Name: %{name}
Version: 5.4.30
Release: 1.ius.1%{?dist}
Epoch: 1
License: PHP
Group: Development/Languages
URL: http://www.php.net/

Source0: http://www.php.net/distributions/%{real_name}-%{version}%{?rcver}.tar.bz2
Source1: php.conf
Source2: php.ini
Source3: macros.php
Source4: php-fpm.conf
Source5: php-fpm-www.conf
Source6: php-fpm.init
Source7: php-fpm.logrotate
Source8: php-fpm.sysconfig

# Build fixes
Patch5: php-5.2.0-includedir.patch
Patch6: php-5.2.4-embed.patch
Patch7: php-5.3.0-recode.patch

# Fixes for extension modules

# Functional changes
Patch40: php-5.4.0-dlopen.patch
Patch41: php-5.4.0-easter.patch
Patch42: php-5.3.1-systzdata-v7.patch
# See http://bugs.php.net/53436
Patch43: php-5.4.0-phpize.patch
# Use system libzip instead of bundled one
Patch44: php-5.4.0-system-libzip.patch

# https://bugs.php.net/bug.php?id=62172
# patched up stream
#Patch50: fpm-balancer.patch
Patch51: php-5.4.18-bison.patch

Patch100: php-5.4.30-fpm.patch

# Fixes for tests


BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: bzip2-devel, curl-devel >= 7.9, db4-devel, gmp-devel
BuildRequires: httpd-devel >= 2.0.46-1, pam-devel
BuildRequires: libstdc++-devel, openssl-devel
%if 0%{?fedora} >= 11 || 0%{?rhel} >= 6
# For Sqlite3 extension
BuildRequires: sqlite-devel >= 3.6.0
%else
BuildRequires: sqlite-devel >= 3.0.0
%endif
BuildRequires: zlib-devel, smtpdaemon, libedit-devel
BuildRequires: bzip2, perl, libtool >= 1.4.3, gcc-c++
BuildRequires: libtool-ltdl-devel
BuildRequires: bison
%if %{with_libzip}
BuildRequires: libzip-devel >= 0.10
%endif

Obsoletes: %{name}-dbg, php3, phpfi, stronghold-php, %{name}-zts < 5.3.7


Provides: %{name}-zts = %{epoch}:%{version}-%{release}
Provides: %{real_name}-zts = %{epoch}:%{version}-%{release}
Provides: %{name}-zts = %{epoch}:%{version}-%{release}
Provides: %{real_name}-zts = %{epoch}:%{version}-%{release}

Requires: httpd-mmn = %{httpd_mmn}
Provides: mod_%{real_name} = %{epoch}:%{version}-%{release}
Provides: mod_%{name} = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
# For backwards-compatibility, require php-cli for the time being:
Requires: %{name}-cli = %{epoch}:%{version}-%{release}
# To ensure correct /var/lib/php/session ownership:
Requires(pre): httpd

Provides: %{real_name} = %{epoch}:%{version}-%{release}
Provides: php54 = %{epoch}:%{version}-%{release}

Conflicts: %{real_name} < %{base_ver}
Conflicts: php51, php52, php53u


# Don't provides extensions, which are not shared library, as .so
# RPM 4.8
%{?filter_provides_in: %filter_provides_in %{_libdir}/php/modules/.*\.so$}
%{?filter_provides_in: %filter_provides_in %{_libdir}/php-zts/modules/.*\.so$}
%{?filter_setup}
# RPM 4.9
%global __provides_exclude_from %{?__provides_exclude_from:%__provides_exclude_from|}%{_libdir}/php/modules/.*\\.so$
%global __provides_exclude_from %{__provides_exclude_from}|%{_libdir}/php-zts/modules/.*\\.so$


%description
PHP is an HTML-embedded scripting language. PHP attempts to make it
easy for developers to write dynamically generated web pages. PHP also
offers built-in database integration for several commercial and
non-commercial database management systems, so writing a
database-enabled webpage with PHP is fairly simple. The most common
use of PHP coding is probably as a replacement for CGI scripts. 

The php package contains the module which adds support for the PHP
language to Apache HTTP Server.

%package cli
Group: Development/Languages
Summary: Command-line interface for PHP
Requires: %{real_name}-common = %{epoch}:%{version}-%{release}
Provides: %{name}-cgi = %{epoch}:%{version}-%{release}, php-cgi = %{epoch}:%{version}-%{release}
Provides: %{real_name}-cgi = %{epoch}:%{version}-%{release}, php-cgi = %{epoch}:%{version}-%{release}
Provides: %{name}-pcntl, php-pcntl
Provides: %{real_name}-pcntl, php-pcntl
Provides: %{name}-readline, php-readline
Provides: %{real_name}-readline, php-readline
Provides: %{real_name}-cli = %{epoch}:%{version}-%{release}

%description cli
The php-cli package contains the command-line interface 
executing PHP scripts, /usr/bin/php, and the CGI interface.


%if %{with_fpm}
%package fpm
Group: Development/Languages
Summary: PHP FastCGI Process Manager
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Requires(pre): /usr/sbin/useradd
Provides: %{name}-fpm = %{epoch}:%{version}-%{release}
Provides: %{real_name}-fpm = %{epoch}:%{version}-%{release}
BuildRequires: libevent-devel >= 1.4.11

%description fpm
PHP-FPM (FastCGI Process Manager) is an alternative PHP FastCGI
implementation with some additional features useful for sites of
any size, especially busier sites.
%endif

%package common
Group: Development/Languages
Summary: Common files for PHP
Provides: %{name}-common = %{epoch}:%{version}-%{release}
Provides: %{real_name}-common = %{epoch}:%{version}-%{release}
Provides: %{name}(language) = %{epoch}:%{version}
Provides: %{name}(language)%{?_isa} = %{epoch}:%{version}
Provides: %{real_name}(language) = %{epoch}:%{version}
Provides: %{real_name}(language)%{?_isa} = %{epoch}:%{version}
# ABI/API check - Arch specific
Provides: %{name}-api = %{apiver}, %{name}-zend-abi = %{zendver}
Provides: %{name}(api) = %{apiver}, %{name}(zend-abi) = %{zendver}
# Provides for all builtin/shared modules:
Provides: %{name}-bz2
Provides: %{name}-calendar
Provides: %{name}-ctype
Provides: %{name}-curl
Provides: %{name}-date
Provides: %{name}-exif
Provides: %{name}-fileinfo
Provides: %{name}-pecl-Fileinfo = %{fileinfover}
Provides: %{name}-pecl(Fileinfo) = %{fileinfover}
Provides: %{name}-ftp
Provides: %{name}-gettext
Provides: %{name}-gmp
Provides: %{name}-hash
Provides: %{name}-mhash = %{epoch}:%{version}
Provides: %{name}-iconv
Provides: %{name}-json
Provides: %{name}-pecl-json = %{jsonver}
Provides: %{name}-pecl(json) = %{jsonver}
Provides: %{name}-libxml
Provides: %{name}-openssl
Provides: %{name}-pecl-phar = %{pharver}
Provides: %{name}-pecl(phar) = %{pharver}
Provides: %{name}-pcre
Provides: %{name}-reflection
Provides: %{name}-session
Provides: %{name}-shmop
Provides: %{name}-simplexml
Provides: %{name}-sockets
Provides: %{name}-spl
Provides: %{name}-tokenizer
Provides: %{name}-filter
Provides: %{name}-filter%{?_isa}
# ABI/API check - Arch specific
Provides: %{real_name}-api = %{apiver}, %{real_name}-zend-abi = %{zendver}
Provides: %{real_name}(api) = %{apiver}, %{real_name}(zend-abi) = %{zendver}
# Provides for all builtin/shared modules:
Provides: %{real_name}-bz2
Provides: %{real_name}-calendar
Provides: %{real_name}-ctype
Provides: %{real_name}-curl
Provides: %{real_name}-date
Provides: %{real_name}-exif
Provides: %{real_name}-fileinfo
Provides: %{real_name}-pecl-Fileinfo = %{fileinfover}
Provides: %{real_name}-pecl(Fileinfo) = %{fileinfover}
Provides: %{real_name}-ftp
Provides: %{real_name}-gettext
Provides: %{real_name}-gmp
Provides: %{real_name}-hash
Provides: %{real_name}-mhash = %{epoch}:%{version}
Provides: %{real_name}-iconv
Provides: %{real_name}-json
Provides: %{real_name}-pecl-json = %{jsonver}
Provides: %{real_name}-pecl(json) = %{jsonver}
Provides: %{real_name}-libxml
Provides: %{real_name}-openssl
Provides: %{real_name}-pecl-phar = %{pharver}
Provides: %{real_name}-pecl(phar) = %{pharver}
Provides: %{real_name}-pcre
Provides: %{real_name}-reflection
Provides: %{real_name}-session
Provides: %{real_name}-shmop
Provides: %{real_name}-simplexml
Provides: %{real_name}-sockets
Provides: %{real_name}-spl
Provides: %{real_name}-tokenizer
Provides: %{real_name}-filter
Provides: %{real_name}-filter%{?_isa}
%if %{with_zip}
Provides: %{name}-zip
Provides: %{real_name}-zip
Provides: %{name}-pecl-zip = %{zipver}
Provides: %{real_name}-pecl-zip = %{zipver}
Provides: %{name}-pecl(zip) = %{zipver}
Provides: %{real_name}-pecl(zip) = %{zipver}
Obsoletes: %{name}-pecl-zip
Obsoletes: %{real_name}-pecl-zip
%endif
Provides: %{name}-zlib, %{name}-zlib
Provides: %{real_name}-zlib, %{real_name}-zlib
Obsoletes: %{name}-openssl, %{name}-pecl-json, %{name}-json, %{name}-pecl-phar, %{name}-pecl-Fileinfo
Obsoletes: %{name}-mhash < 5.3.0

%description common
The php-common package contains files used by both the php
package and the php-cli package.

%package devel
Group: Development/Libraries
Summary: Files needed for building PHP extensions
Requires: %{name} = %{epoch}:%{version}-%{release}, autoconf, automake
Obsoletes: %{name}-pecl-pdo-devel
Provides: %{name}-devel = %{epoch}:%{version}-%{release}
Provides: %{real_name}-devel = %{epoch}:%{version}-%{release}
Provides: %{name}-zts-devel = %{epoch}:%{version}-%{release}
Provides: %{real_name}-zts-devel = %{epoch}:%{version}-%{release}
Provides: %{name}-zts-devel = %{epoch}:%{version}-%{release}
Provides: %{real_name}-zts-devel = %{epoch}:%{version}-%{release}

%description devel
The php-devel package contains the files needed for building PHP
extensions. If you need to compile your own PHP extensions, you will
need to install this package.

%package imap
Summary: A module for PHP applications that use IMAP
Group: Development/Languages
Provides: %{name}-imap = %{epoch}:%{version}-%{release}
Provides: %{real_name}-imap = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Obsoletes: mod_php3-imap, stronghold-php-imap
BuildRequires: krb5-devel, openssl-devel, libc-client-devel

%description imap
The php-imap package contains a dynamic shared object (DSO) for the
Apache Web server. When compiled into Apache, the php-imap module will
add IMAP (Internet Message Access Protocol) support to PHP. IMAP is a
protocol for retrieving and uploading e-mail messages on mail
servers. PHP is an HTML-embedded scripting language. If you need IMAP
support for PHP applications, you will need to install this package
and the php package.

%package ldap
Summary: A module for PHP applications that use LDAP
Group: Development/Languages
Provides: %{name}-ldap = %{epoch}:%{version}-%{release}
Provides: %{real_name}-ldap = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Obsoletes: mod_php3-ldap, stronghold-php-ldap
BuildRequires: cyrus-sasl-devel, openldap-devel, openssl-devel

%description ldap
The php-ldap package is a dynamic shared object (DSO) for the Apache
Web server that adds Lightweight Directory Access Protocol (LDAP)
support to PHP. LDAP is a set of protocols for accessing directory
services over the Internet. PHP is an HTML-embedded scripting
language. If you need LDAP support for PHP applications, you will
need to install this package in addition to the php package.

%package pdo
Summary: A database access abstraction module for PHP applications
Group: Development/Languages
Provides: %{name}-pdo = %{epoch}:%{version}-%{release}
Provides: %{real_name}-pdo = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Obsoletes: %{name}-pecl-pdo-sqlite, %{name}-pecl-pdo
# ABI/API check - Arch specific
Provides: %{name}-pdo-abi = %{pdover}
Provides: %{name}-sqlite3, %{name}-sqlite3
Provides: %{name}-pdo_sqlite, %{name}-pdo_sqlite
# ABI/API check - Arch specific
Provides: %{real_name}-pdo-abi = %{pdover}
Provides: %{real_name}-sqlite3, %{real_name}-sqlite3
Provides: %{real_name}-pdo_sqlite, %{real_name}-pdo_sqlite

%description pdo
The php-pdo package contains a dynamic shared object that will add
a database access abstraction layer to PHP.  This module provides
a common interface for accessing MySQL, PostgreSQL or other 
databases.

%package mysql
Summary: A module for PHP applications that use MySQL databases
Group: Development/Languages
Provides: %{name}-mysql = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mysql = %{epoch}:%{version}-%{release}
Requires: %{name}-pdo = %{epoch}:%{version}-%{release}
Provides: %{name}_database
Provides: %{name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{name}-pdo_mysql, %{name}-pdo_mysql
Provides: %{real_name}_database
Provides: %{real_name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{real_name}-pdo_mysql, %{real_name}-pdo_mysql
Obsoletes: mod_php3-mysql, stronghold-php-mysql
BuildRequires: mysql-devel >= 4.1.0
Conflicts: %{name}-mysqlnd

%description mysql
The php-mysql package contains a dynamic shared object that will add
MySQL database support to PHP. MySQL is an object-relational database
management system. PHP is an HTML-embeddable scripting language. If
you need MySQL support for PHP applications, you will need to install
this package and the php package.

%package mysqlnd
Summary: A module for PHP applications that use MySQL databases
Group: Development/Languages
Provides: %{name}-mysqlnd = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mysqlnd = %{epoch}:%{version}-%{release}
Requires: %{name}-pdo = %{epoch}:%{version}-%{release}
Provides: %{name}_database
Provides: %{name}-mysql = %{epoch}:%{version}-%{release}
Provides: %{name}-mysql = %{epoch}:%{version}-%{release}
Provides: %{name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{name}-pdo_mysql, %{name}-pdo_mysql
Provides: %{real_name}_database
Provides: %{real_name}-mysql = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mysql = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mysqli = %{epoch}:%{version}-%{release}
Provides: %{real_name}-pdo_mysql, %{real_name}-pdo_mysql

%description mysqlnd
The php-mysqlnd package contains a dynamic shared object that will add
MySQL database support to PHP. MySQL is an object-relational database
management system. PHP is an HTML-embeddable scripting language. If
you need MySQL support for PHP applications, you will need to install
this package and the php package.

This package use the MySQL Native Driver

%package pgsql
Summary: A PostgreSQL database module for PHP
Group: Development/Languages
Provides: %{name}-pgsql = %{epoch}:%{version}-%{release}
Provides: %{real_name}-pgsql = %{epoch}:%{version}-%{release}
Requires: %{name}-pdo = %{epoch}:%{version}-%{release}
Provides: %{name}_database
Provides: %{name}-pdo_pgsql, %{name}-pdo_pgsql
Provides: %{real_name}_database
Provides: %{real_name}-pdo_pgsql, %{real_name}-pdo_pgsql
Obsoletes: mod_php3-pgsql, stronghold-php-pgsql
BuildRequires: krb5-devel, openssl-devel, postgresql-devel

%description pgsql
The php-pgsql package includes a dynamic shared object (DSO) that can
be compiled in to the Apache Web server to add PostgreSQL database
support to PHP. PostgreSQL is an object-relational database management
system that supports almost all SQL constructs. PHP is an
HTML-embedded scripting language. If you need back-end support for
PostgreSQL, you should install this package in addition to the main
php package.

%package process
Summary: Modules for PHP script using system process interfaces
Group: Development/Languages
Provides: %{name}-process = %{epoch}:%{version}-%{release}
Provides: %{real_name}-process = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Provides: %{name}-posix, %{name}-posix
Provides: %{name}-sysvsem, %{name}-sysvsem
Provides: %{name}-sysvshm, %{name}-sysvshm
Provides: %{name}-sysvmsg, %{name}-sysvmsg
Provides: %{real_name}-posix, %{real_name}-posix
Provides: %{real_name}-sysvsem, %{real_name}-sysvsem
Provides: %{real_name}-sysvshm, %{real_name}-sysvshm
Provides: %{real_name}-sysvmsg, %{real_name}-sysvmsg

%description process
The php-process package contains dynamic shared objects which add
support to PHP using system interfaces for inter-process
communication.

%package odbc
Group: Development/Languages
Provides: %{name}-odbc = %{epoch}:%{version}-%{release}
Provides: %{real_name}-odbc = %{epoch}:%{version}-%{release}
Requires: %{name}-pdo = %{epoch}:%{version}-%{release}
Summary: A module for PHP applications that use ODBC databases
Provides: %{name}_database
Provides: %{name}-pdo_odbc, %{name}-pdo_odbc
Provides: %{real_name}_database
Provides: %{real_name}-pdo_odbc, %{real_name}-pdo_odbc
Obsoletes: stronghold-php-odbc
BuildRequires: unixODBC-devel

%description odbc
The php-odbc package contains a dynamic shared object that will add
database support through ODBC to PHP. ODBC is an open specification
which provides a consistent API for developers to use for accessing
data sources (which are often, but not always, databases). PHP is an
HTML-embeddable scripting language. If you need ODBC support for PHP
applications, you will need to install this package and the php
package.

%package soap
Group: Development/Languages
Provides: %{name}-soap = %{epoch}:%{version}-%{release}
Provides: %{real_name}-soap = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Summary: A module for PHP applications that use the SOAP protocol
BuildRequires: libxml2-devel

%description soap
The php-soap package contains a dynamic shared object that will add
support to PHP for using the SOAP web services protocol.

%package interbase
Summary: 	A module for PHP applications that use Interbase/Firebird databases
Group: 		Development/Languages
BuildRequires:  firebird-devel
Provides: 	%{name}-interbase = %{epoch}:%{version}-%{release}
Provides: 	%{real_name}-interbase = %{epoch}:%{version}-%{release}
Requires: 	%{name}-pdo = %{epoch}:%{version}-%{release}
Provides: 	%{name}_database
Provides: 	%{name}-firebird, %{name}-firebird
Provides: 	%{name}-pdo_firebird, %{name}-pdo_firebird
Provides: 	%{real_name}_database
Provides: 	%{real_name}-firebird, %{real_name}-firebird
Provides: 	%{real_name}-pdo_firebird, %{real_name}-pdo_firebird

%description interbase
The php-interbase package contains a dynamic shared object that will add
database support through Interbase/Firebird to PHP.

InterBase is the name of the closed-source variant of this RDBMS that was
developed by Borland/Inprise. 

Firebird is a commercially independent project of C and C++ programmers, 
technical advisors and supporters developing and enhancing a multi-platform 
relational database management system based on the source code released by 
Inprise Corp (now known as Borland Software Corp) under the InterBase Public
License.

%package snmp
Summary: A module for PHP applications that query SNMP-managed devices
Group: Development/Languages
Provides: %{name}-snmp = %{epoch}:%{version}-%{release}
Provides: %{real_name}-snmp = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}, net-snmp
BuildRequires: net-snmp-devel

%description snmp
The php-snmp package contains a dynamic shared object that will add
support for querying SNMP devices to PHP.  PHP is an HTML-embeddable
scripting language. If you need SNMP support for PHP applications, you
will need to install this package and the php package.

%package xml
Summary: A module for PHP applications which use XML
Group: Development/Languages
Provides: %{name}-xml = %{epoch}:%{version}-%{release}
Provides: %{real_name}-xml = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Obsoletes: %{name}-domxml, %{name}-dom
Provides: %{name}-dom, %{name}-dom
Provides: %{name}-xsl, %{name}-xsl
Provides: %{name}-domxml, %{name}-domxml
Provides: %{name}-wddx, %{name}-wddx
Provides: %{name}-xmlreader, %{name}-xmlreader%{?_isa}
Provides: %{name}-xmlwriter, %{name}-xmlwriter%{?_isa} 
Provides: %{real_name}-dom, %{real_name}-dom
Provides: %{real_name}-xsl, %{real_name}-xsl
Provides: %{real_name}-domxml, %{real_name}-domxml
Provides: %{real_name}-wddx, %{real_name}-wddx
Provides: %{real_name}-xmlreader, %{real_name}-xmlreader%{?_isa}
Provides: %{real_name}-xmlwriter, %{real_name}-xmlwriter%{?_isa}
BuildRequires: libxslt-devel >= 1.0.18-1, libxml2-devel >= 2.4.14-1

%description xml
The php-xml package contains dynamic shared objects which add support
to PHP for manipulating XML documents using the DOM tree,
and performing XSL transformations on XML documents.

%package xmlrpc
Summary: A module for PHP applications which use the XML-RPC protocol
Group: Development/Languages
Provides: %{name}-xmlrpc = %{epoch}:%{version}-%{release}
Provides: %{real_name}-xmlrpc = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}

%description xmlrpc
The php-xmlrpc package contains a dynamic shared object that will add
support for the XML-RPC protocol to PHP.

%package mbstring
Summary: A module for PHP applications which need multi-byte string handling
Group: Development/Languages
Provides: %{name}-mbstring = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mbstring = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}

%description mbstring
The php-mbstring package contains a dynamic shared object that will add
support for multi-byte string handling to PHP.

%package gd
Summary: A module for PHP applications for using the gd graphics library
Group: Development/Languages
Provides: %{name}-gd = %{epoch}:%{version}-%{release}
Provides: %{real_name}-gd = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
# Required to build the bundled GD library
BuildRequires: libjpeg-devel, libpng-devel, freetype-devel
BuildRequires: libXpm-devel, t1lib-devel

%description gd
The php-gd package contains a dynamic shared object that will add
support for using the gd graphics library to PHP.

%package bcmath
Summary: A module for PHP applications for using the bcmath library
Group: Development/Languages
Provides: %{name}-bcmath = %{epoch}:%{version}-%{release}
Provides: %{real_name}-bcmath = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}

%description bcmath
The php-bcmath package contains a dynamic shared object that will add
support for using the bcmath library to PHP.

%package dba
Summary: A database abstraction layer module for PHP applications
Group: Development/Languages
Provides: %{name}-dba = %{epoch}:%{version}-%{release}
Provides: %{real_name}-dba = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}

%description dba
The php-dba package contains a dynamic shared object that will add
support for using the DBA database abstraction layer to PHP.

%if 0%{?with_litespeed}
%package litespeed
Summary: API for the Litespeed web server
Group: Development/Languages
Requires: %{name}-common = %{epoch}:%{version}-%{release}
Provides: %{real_name}-litespeed = %{epoch}:%{version}-%{release}
Provides: %{name}-litespeed = %{epoch}:%{version}-%{release}

%description litespeed
The php-litespeed package contains the binary used by the Litespeed web server.
%endif

%package mcrypt
Summary: Standard PHP module provides mcrypt library support
Group: Development/Languages
Provides: %{name}-mcrypt = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mcrypt = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
BuildRequires: libmcrypt-devel

%description mcrypt
The php-mcrypt package contains a dynamic shared object that will add
support for using the mcrypt library to PHP.

%package tidy
Summary: Standard PHP module provides tidy library support
Group: Development/Languages
Provides: %{name}-tidy = %{epoch}:%{version}-%{release}
Provides: %{real_name}-tidy = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
BuildRequires: libtidy-devel

%description tidy
The php-tidy package contains a dynamic shared object that will add
support for using the tidy library to PHP.

%package mssql
Summary: MSSQL database module for PHP
Group: Development/Languages
Provides: %{name}-mssql = %{epoch}:%{version}-%{release}
Provides: %{real_name}-mssql = %{epoch}:%{version}-%{release}
Requires: %{name}-pdo = %{epoch}:%{version}-%{release}
BuildRequires: freetds-devel
Provides: %{name}-pdo_dblib, %{name}-pdo_dblib
Provides: %{real_name}-pdo_dblib, %{real_name}-pdo_dblib

%description mssql
The php-mssql package contains a dynamic shared object that will
add MSSQL database support to PHP.  It uses the TDS (Tabular
DataStream) protocol through the freetds library, hence any
database server which supports TDS can be accessed.

%package embedded
Summary: PHP library for embedding in applications
Group: System Environment/Libraries
Provides: %{name}-embedded = %{epoch}:%{version}-%{release}
Provides: %{real_name}-embedded = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
# doing a real -devel package for just the .so symlink is a bit overkill
Provides: %{name}-embedded-devel = %{epoch}:%{version}-%{release}
Provides: %{name}-embedded-devel = %{epoch}:%{version}-%{release}
Provides: %{real_name}-embedded-devel = %{epoch}:%{version}-%{release}
Provides: %{real_name}-embedded-devel = %{epoch}:%{version}-%{release}

%description embedded
The php-embedded package contains a library which can be embedded
into applications to provide PHP scripting language support.

%package pspell
Summary: A module for PHP applications for using pspell interfaces
Group: System Environment/Libraries
Provides: %{name}-pspell = %{epoch}:%{version}-%{release}
Provides: %{real_name}-pspell = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
BuildRequires: aspell-devel >= 0.50.0

%description pspell
The php-pspell package contains a dynamic shared object that will add
support for using the pspell library to PHP.

%package recode
Summary: A module for PHP applications for using the recode library
Group: System Environment/Libraries
Provides: %{name}-recode = %{epoch}:%{version}-%{release}
Provides: %{real_name}-recode = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
BuildRequires: recode-devel

%description recode
The php-recode package contains a dynamic shared object that will add
support for using the recode library to PHP.

%package intl
Summary: Internationalization extension for PHP applications
Group: System Environment/Libraries
Provides: %{name}-intl = %{epoch}:%{version}-%{release}
Provides: %{real_name}-intl = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
BuildRequires: libicu-devel >= 3.6

%description intl
The php-intl package contains a dynamic shared object that will add
support for using the ICU library to PHP.

%package enchant
Summary: Human Language and Character Encoding Support
Group: System Environment/Libraries
Provides: %{name}-enchant = %{epoch}:%{version}-%{release}
Provides: %{real_name}-enchant = %{epoch}:%{version}-%{release}
Requires: %{name}-common = %{epoch}:%{version}-%{release}
BuildRequires: enchant-devel >= 1.2.4

%description enchant
The php-intl package contains a dynamic shared object that will add
support for using the enchant library to PHP.


%prep
%setup -q -n php-%{version}%{?rcver}

%patch5 -p1 -b .includedir
%patch6 -p1 -b .embed
%patch7 -p1 -b .recode

%patch40 -p1 -b .dlopen
%patch41 -p1 -b .easter
%patch42 -p1 -b .systzdata
%patch43 -p1 -b .headers
%if %{with_libzip}
%patch44 -p1 -b .systzip
%endif

#%if %{with_fpm}
#%patch50 -p1 -b .fpm_balancer
#%endif

%if 0%{?rhel} < 6
%patch51 -p1
%endif


%patch100 -p1
# Prevent %%doc confusion over LICENSE files
cp Zend/LICENSE Zend/ZEND_LICENSE
cp TSRM/LICENSE TSRM_LICENSE
cp ext/ereg/regex/COPYRIGHT regex_COPYRIGHT
cp ext/gd/libgd/README gd_README

# Multiple builds for multiple SAPIs
mkdir build-cgi build-apache build-embedded build-zts build-ztscli build-litespeed \
%if %{with_fpm}
    build-fpm
%endif

# Remove bogus test; position of read position after fopen(, "a+")
# is not defined by C standard, so don't presume anything.
rm -f ext/standard/tests/file/bug21131.phpt
# php_egg_logo_guid() removed by patch41
rm -f tests/basic/php_egg_logo_guid.phpt

# Tests that fail.
rm -f ext/standard/tests/file/bug22414.phpt \
      ext/iconv/tests/bug16069.phpt

# Safety check for API version change.
pver=$(awk '$2=="PHP_VERSION"{gsub(/\"/,"",$3);print$3}' main/php_version.h)
if test "x${pver}" != "x%{version}%{?rcver}"; then
   : Error: Upstream PHP version is now ${pver}, expecting %{version}%{?rcver}.
   : Update the version/rcver macros and rebuild.
   exit 1
fi

vapi=$(awk '$2=="PHP_API_VERSION"{gsub(/\"/,"",$3);print$3}' main/php.h)
if test "x${vapi}" != "x%{apiver}"; then
   : Error: Upstream API version is now ${vapi}, expecting %{apiver}.
   : Update the apiver macro and rebuild.
   exit 1
fi

vzend=$(awk '$2=="ZEND_MODULE_API_NO"{gsub(/\"/,"",$3);print$3}' Zend/zend_modules.h)
if test "x${vzend}" != "x%{zendver}"; then
   : Error: Upstream Zend ABI version is now ${vzend}, expecting %{zendver}.
   : Update the zendver macro and rebuild.
   exit 1
fi

# Safety check for PDO ABI version change
vpdo=$(awk '$2=="PDO_DRIVER_API"{gsub(/\"/,"",$3);print$3}' ext/pdo/php_pdo_driver.h)
if test "x${vpdo}" != "x%{pdover}"; then
   : Error: Upstream PDO ABI version is now ${vpdo}, expecting %{pdover}.
   : Update the pdover macro and rebuild.
   exit 1
fi

# Check for some extension version
ver=$(awk '$2=="PHP_FILEINFO_VERSION"{gsub(/\"/,"",$3);print$3}' ext/fileinfo/php_fileinfo.h)
if test "$ver" != "%{fileinfover}"; then
   : Error: Upstream FILEINFO version is now ${ver}, expecting %{fileinfover}.
   : Update the fileinfover macro and rebuild.
   exit 1
fi
ver=$(awk '$2=="PHP_PHAR_VERSION"{gsub(/\"/,"",$3);print$3}' ext/phar/php_phar.h)
if test "$ver" != "%{pharver}"; then
   : Error: Upstream PHAR version is now ${ver}, expecting %{pharver}.
   : Update the pharver macro and rebuild.
   exit 1
fi
ver=$(awk '$2=="PHP_ZIP_VERSION_STRING"{gsub(/\"/,"",$3);print$3}' ext/zip/php_zip.h)
if test "$ver" != "%{zipver}"; then
   : Error: Upstream ZIP version is now ${ver}, expecting %{zipver}.
   : Update the zipver macro and rebuild.
   exit 1
fi
ver=$(awk '$2=="PHP_JSON_VERSION"{gsub(/\"/,"",$3);print$3}' ext/json/php_json.h)
if test "$ver" != "%{jsonver}"; then
   : Error: Upstream JSON version is now ${ver}, expecting %{jsonver}.
   : Update the jsonver macro and rebuild.
   exit 1
fi

# Fix some bogus permissions
find . -name \*.[ch] -exec chmod 644 {} \;
chmod 644 README.*

# php-fpm configuration files for tmpfiles.d
echo "d %{_localstatedir}/run/php-fpm 755 root root" >php-fpm.tmpfiles


%build
#Fix for zts see, https://bugs.php.net/bug.php?id=65460
rm Zend/zend_{language,ini}_parser.[ch]
./genfiles

%if 0%{?fedora} >= 11 || 0%{?rhel} >= 6
# aclocal workaround - to be improved
cat `aclocal --print-ac-dir`/{libtool,ltoptions,ltsugar,ltversion,lt~obsolete}.m4 >>aclocal.m4
%endif

# Force use of system libtool:
libtoolize --force --copy
%if 0%{?fedora} >= 11 || 0%{?rhel} >= 6
cat `aclocal --print-ac-dir`/{libtool,ltoptions,ltsugar,ltversion,lt~obsolete}.m4 >build/libtool.m4
%else
cat `aclocal --print-ac-dir`/libtool.m4 > build/libtool.m4
%endif

# Regenerate configure scripts (patches change config.m4's)
touch configure.in
./buildconf --force

CFLAGS="$RPM_OPT_FLAGS -fno-strict-aliasing -Wno-pointer-sign"
export CFLAGS

# Install extension modules in %{_libdir}/php/modules.
EXTENSION_DIR=%{_libdir}/php/modules; export EXTENSION_DIR

# Set PEAR_INSTALLDIR to ensure that the hard-coded include_path
# includes the PEAR directory even though pear is packaged
# separately.
PEAR_INSTALLDIR=%{_datadir}/pear; export PEAR_INSTALLDIR

# Shell function to configure and build a PHP tree.
build() {
# bison-1.875-2 seems to produce a broken parser; workaround.
# mkdir Zend && cp ../Zend/zend_{language,ini}_{parser,scanner}.[ch] Zend
ln -sf ../configure
%configure \
	--cache-file=../config.cache \
        --with-libdir=%{_lib} \
	--with-config-file-path=%{_sysconfdir} \
	--with-config-file-scan-dir=%{_sysconfdir}/php.d \
	--disable-debug \
	--with-pic \
	--disable-rpath \
	--without-pear \
	--with-bz2 \
	--with-exec-dir=%{_bindir} \
	--with-freetype-dir=%{_prefix} \
	--with-png-dir=%{_prefix} \
	--with-xpm-dir=%{_prefix} \
	--enable-gd-native-ttf \
	--with-t1lib=%{_prefix} \
	--without-gdbm \
	--with-gettext \
	--with-gmp \
	--with-iconv \
	--with-jpeg-dir=%{_prefix} \
	--with-openssl \
        --with-pcre-regex \
	--with-zlib \
	--with-layout=GNU \
	--enable-exif \
	--enable-ftp \
	--enable-magic-quotes \
	--enable-sockets \
	--with-kerberos \
	--enable-ucd-snmp-hack \
	--enable-shmop \
	--enable-calendar \
        --with-libxml-dir=%{_prefix} \
	--enable-xml \
        --with-system-tzdata \
	--with-mhash \
	$* 
if test $? != 0; then 
  tail -500 config.log
  : configure failed
  exit 1
fi

make %{?_smp_mflags}
}

# Build /usr/bin/php-cgi with the CGI SAPI, and all the shared extensions
pushd build-cgi

build --enable-force-cgi-redirect \
      --libdir=%{_libdir}/php \
      --enable-pcntl \
      --with-imap=shared --with-imap-ssl \
      --enable-mbstring=shared \
      --enable-mbregex \
      --with-gd=shared \
      --enable-bcmath=shared \
      --enable-dba=shared --with-db4=%{_prefix} \
      --with-xmlrpc=shared \
      --with-ldap=shared --with-ldap-sasl \
      --enable-mysqlnd=shared \
      --with-mysql=shared,mysqlnd \
      --with-mysqli=shared,mysqlnd \
      --with-mysql-sock=%{mysql_sock} \
      --with-interbase=shared,%{_libdir}/firebird \
      --with-pdo-firebird=shared,%{_libdir}/firebird \
      --enable-dom=shared \
      --with-pgsql=shared \
      --enable-wddx=shared \
      --with-snmp=shared,%{_prefix} \
      --enable-soap=shared \
      --with-xsl=shared,%{_prefix} \
      --enable-xmlreader=shared --enable-xmlwriter=shared \
      --with-curl=shared,%{_prefix} \
      --enable-fastcgi \
      --enable-pdo=shared \
      --with-pdo-odbc=shared,unixODBC,%{_prefix} \
      --with-pdo-mysql=shared,mysqlnd \
      --with-pdo-pgsql=shared,%{_prefix} \
      --with-pdo-sqlite=shared,%{_prefix} \
      --with-pdo-dblib=shared,%{_prefix} \
%if 0%{?fedora} >= 11 || 0%{?rhel} >= 6
      --with-sqlite3=shared,%{_prefix} \
%else
      --without-sqlite3 \
%endif
      --enable-json=shared \
%if %{with_zip}
      --enable-zip=shared \
%endif
%if %{with_libzip}
      --with-libzip \
%endif
      --without-readline \
      --with-libedit \
      --with-pspell=shared \
      --enable-phar=shared \
      --with-mcrypt=shared,%{_prefix} \
      --with-tidy=shared,%{_prefix} \
      --with-mssql=shared,%{_prefix} \
      --enable-sysvmsg=shared --enable-sysvshm=shared --enable-sysvsem=shared \
      --enable-posix=shared \
      --with-unixODBC=shared,%{_prefix} \
      --enable-fileinfo=shared \
      --enable-intl=shared \
      --with-icu-dir=%{_prefix} \
      --with-enchant=shared,%{_prefix} \
      --with-recode=shared,%{_prefix}
popd

without_shared="--without-gd \
      --disable-dom --disable-dba --without-unixODBC \
      --disable-xmlreader --disable-xmlwriter \
      --without-sqlite3 --disable-phar --disable-fileinfo \
      --disable-json --without-pspell --disable-wddx \
      --without-curl --disable-posix \
      --disable-sysvmsg --disable-sysvshm --disable-sysvsem"

# Build Apache module, and the CLI SAPI, /usr/bin/php
pushd build-apache
build \
%if 0%{?rhel}  == 7
      --with-apxs2=%{_bindir}/apxs \
%else
      --with-apxs2=%{_sbindir}/apxs \
%endif
      --libdir=%{_libdir}/php \
      --enable-pdo=shared \
      --with-mysql=shared,%{_prefix} \
      --with-mysqli=shared,%{mysql_config} \
      --with-pdo-mysql=shared,%{mysql_config} \
      --with-pdo-sqlite=shared,%{_prefix} \
      ${without_shared}
popd

%if 0%{?with_litespeed}
# Build litespeed module
pushd build-litespeed
build --libdir=%{_libdir}/php \
 --with-litespeed \
 --enable-pdo=shared \
 --with-mysql=shared,%{_prefix} \
 --with-mysqli=shared,%{mysql_config} \
 --with-pdo-mysql=shared,%{mysql_config} \
 --with-pdo-sqlite=shared,%{_prefix} \
 ${without_shared}
popd
%endif

%if %{with_fpm}
# Build php-fpm
pushd build-fpm
build --enable-fpm \
      --libdir=%{_libdir}/php \
      --without-mysql --disable-pdo \
      ${without_shared}
popd
%endif

# Build for inclusion as embedded script language into applications,
# /usr/lib[64]/libphp5.so
pushd build-embedded
build --enable-embed \
      --without-mysql --disable-pdo \
      ${without_shared}
popd

# Build a special thread-safe (mainly for modules)
pushd build-ztscli

EXTENSION_DIR=%{_libdir}/php-zts/modules
build --enable-force-cgi-redirect \
      --includedir=%{_includedir}/php-zts \
      --libdir=%{_libdir}/php-zts \
      --enable-maintainer-zts \
      --with-config-file-scan-dir=%{_sysconfdir}/php-zts.d \
      --enable-pcntl \
      --with-imap=shared --with-imap-ssl \
      --enable-mbstring=shared \
      --enable-mbregex \
      --with-gd=shared \
      --enable-bcmath=shared \
      --enable-dba=shared --with-db4=%{_prefix} \
      --with-xmlrpc=shared \
      --with-ldap=shared --with-ldap-sasl \
      --enable-mysqlnd=shared \
      --with-mysql=shared,mysqlnd \
      --with-mysqli=shared,mysqlnd \
      --with-mysql-sock=%{mysql_sock} \
      --enable-mysqlnd-threading \
      --with-interbase=shared,%{_libdir}/firebird \
      --with-pdo-firebird=shared,%{_libdir}/firebird \
      --enable-dom=shared \
      --with-pgsql=shared \
      --enable-wddx=shared \
      --with-snmp=shared,%{_prefix} \
      --enable-soap=shared \
      --with-xsl=shared,%{_prefix} \
      --enable-xmlreader=shared --enable-xmlwriter=shared \
      --with-curl=shared,%{_prefix} \
      --enable-fastcgi \
      --enable-pdo=shared \
      --with-pdo-odbc=shared,unixODBC,%{_prefix} \
      --with-pdo-mysql=shared,mysqlnd \
      --with-pdo-pgsql=shared,%{_prefix} \
      --with-pdo-sqlite=shared,%{_prefix} \
      --with-pdo-dblib=shared,%{_prefix} \
%if 0%{?rhel} >= 6
      --with-sqlite3=shared,%{_prefix} \
%else
      --without-sqlite3 \
%endif
      --enable-json=shared \
%if %{with_zip}
      --enable-zip=shared \
%endif
%if %{with_libzip}
      --with-libzip \
%endif
      --without-readline \
      --with-libedit \
      --with-pspell=shared \
      --enable-phar=shared \
      --with-mcrypt=shared,%{_prefix} \
      --with-tidy=shared,%{_prefix} \
      --with-mssql=shared,%{_prefix} \
      --enable-sysvmsg=shared --enable-sysvshm=shared --enable-sysvsem=shared \
      --enable-posix=shared \
      --with-unixODBC=shared,%{_prefix} \
      --enable-fileinfo=shared \
      --enable-intl=shared \
      --with-icu-dir=%{_prefix} \
      --with-enchant=shared,%{_prefix} \
      --with-recode=shared,%{_prefix}
popd

# Build a special thread-safe Apache SAPI
pushd build-zts
build \
%if 0%{?rhel}  == 7
      --with-apxs2=%{_bindir}/apxs \
%else
      --with-apxs2=%{_sbindir}/apxs \
%endif
      --includedir=%{_includedir}/php-zts \
      --libdir=%{_libdir}/php-zts \
      --enable-maintainer-zts \
      --with-config-file-scan-dir=%{_sysconfdir}/php-zts.d \
      --enable-pdo=shared \
      --with-mysql=shared,%{_prefix} \
      --with-mysqli=shared,%{mysql_config} \
      --with-pdo-mysql=shared,%{mysql_config} \
      --with-pdo-sqlite=shared,%{_prefix} \
      ${without_shared}
popd

### NOTE!!! EXTENSION_DIR was changed for the -zts build, so it must remain
### the last SAPI to be built.

%check
%if %runselftest
cd build-apache
# Run tests, using the CLI SAPI
export NO_INTERACTION=1 REPORT_EXIT_STATUS=1 MALLOC_CHECK_=2
unset TZ LANG LC_ALL
if ! make test; then
  set +x
  for f in `find .. -name \*.diff -type f -print`; do
    echo "TEST FAILURE: $f --"
    cat "$f"
    echo "-- $f result ends."
  done
  set -x
  #exit 1
fi
unset NO_INTERACTION REPORT_EXIT_STATUS MALLOC_CHECK_
%endif

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

# Install the extensions for the ZTS version
make -C build-ztscli install \
     INSTALL_ROOT=$RPM_BUILD_ROOT

# rename extensions build with mysqlnd
mv $RPM_BUILD_ROOT%{_libdir}/php-zts/modules/mysql.so \
   $RPM_BUILD_ROOT%{_libdir}/php-zts/modules/mysqlnd_mysql.so
mv $RPM_BUILD_ROOT%{_libdir}/php-zts/modules/mysqli.so \
   $RPM_BUILD_ROOT%{_libdir}/php-zts/modules/mysqlnd_mysqli.so
mv $RPM_BUILD_ROOT%{_libdir}/php-zts/modules/pdo_mysql.so \
   $RPM_BUILD_ROOT%{_libdir}/php-zts/modules/pdo_mysqlnd.so

# Install the extensions for the ZTS version modules for libmysql
make -C build-zts install-modules \
     INSTALL_ROOT=$RPM_BUILD_ROOT

# rename ZTS binary
mv $RPM_BUILD_ROOT%{_bindir}/php        $RPM_BUILD_ROOT%{_bindir}/zts-php
mv $RPM_BUILD_ROOT%{_bindir}/phpize     $RPM_BUILD_ROOT%{_bindir}/zts-phpize
mv $RPM_BUILD_ROOT%{_bindir}/php-config $RPM_BUILD_ROOT%{_bindir}/zts-php-config

# Install the version for embedded script language in applications + php_embed.h
make -C build-embedded install-sapi install-headers \
     INSTALL_ROOT=$RPM_BUILD_ROOT

%if %{with_fpm}
# Install the php-fpm binary
make -C build-fpm install-fpm \
     INSTALL_ROOT=$RPM_BUILD_ROOT
%endif

# Install everything from the CGI SAPI build
make -C build-cgi install \
     INSTALL_ROOT=$RPM_BUILD_ROOT

# rename extensions build with mysqlnd
mv $RPM_BUILD_ROOT%{_libdir}/php/modules/mysql.so \
   $RPM_BUILD_ROOT%{_libdir}/php/modules/mysqlnd_mysql.so
mv $RPM_BUILD_ROOT%{_libdir}/php/modules/mysqli.so \
   $RPM_BUILD_ROOT%{_libdir}/php/modules/mysqlnd_mysqli.so
mv $RPM_BUILD_ROOT%{_libdir}/php/modules/pdo_mysql.so \
   $RPM_BUILD_ROOT%{_libdir}/php/modules/pdo_mysqlnd.so

# Install the mysql extension build with libmysql
make -C build-apache install-modules \
     INSTALL_ROOT=$RPM_BUILD_ROOT

# Install the default configuration file and icons
install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/
install -m 644 %{SOURCE2} $RPM_BUILD_ROOT%{_sysconfdir}/php.ini
install -m 755 -d $RPM_BUILD_ROOT%{contentdir}/icons
install -m 644 php.gif $RPM_BUILD_ROOT%{contentdir}/icons/php.gif

# For third-party packaging:
install -m 755 -d $RPM_BUILD_ROOT%{_datadir}/php

# install the DSO
install -m 755 -d $RPM_BUILD_ROOT%{_libdir}/httpd/modules
install -m 755 build-apache/libs/libphp5.so $RPM_BUILD_ROOT%{_libdir}/httpd/modules

# install the ZTS DSO
install -m 755 build-zts/libs/libphp5.so $RPM_BUILD_ROOT%{_libdir}/httpd/modules/libphp5-zts.so

%if 0%{?with_litespeed}
# install the php litespeed binary
install -m 755 build-litespeed/sapi/litespeed/php %{buildroot}%{_bindir}/php-ls
%endif

# Apache config fragment
install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d
install -m 644 %{SOURCE1} $RPM_BUILD_ROOT%{_sysconfdir}/httpd/conf.d

install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/php.d
install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/php-zts.d
install -m 755 -d $RPM_BUILD_ROOT%{_localstatedir}/lib/php
install -m 700 -d $RPM_BUILD_ROOT%{_localstatedir}/lib/php/session

%if %{with_fpm}
# PHP-FPM stuff
# Log
install -m 755 -d $RPM_BUILD_ROOT%{_localstatedir}/log/php-fpm
install -m 755 -d $RPM_BUILD_ROOT%{_localstatedir}/run/php-fpm
# Config
install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/php-fpm.d
install -m 644 %{SOURCE4} $RPM_BUILD_ROOT%{_sysconfdir}/php-fpm.conf
install -m 644 %{SOURCE5} $RPM_BUILD_ROOT%{_sysconfdir}/php-fpm.d/www.conf
mv $RPM_BUILD_ROOT%{_sysconfdir}/php-fpm.conf.default .
# tmpfiles.d
install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/tmpfiles.d
install -m 644 php-fpm.tmpfiles $RPM_BUILD_ROOT%{_sysconfdir}/tmpfiles.d/php-fpm.conf
# service
install -m 755 -d $RPM_BUILD_ROOT%{_initrddir}
install -m 755 %{SOURCE6} $RPM_BUILD_ROOT%{_initrddir}/php-fpm
# LogRotate
install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d
install -m 644 %{SOURCE7} $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/php-fpm
# Environment file
install -m 755 -d $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig
install -m 644 %{SOURCE8} $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/php-fpm
%endif
# Fix the link
(cd $RPM_BUILD_ROOT%{_bindir}; ln -sfn phar.phar phar)

# Generate files lists and stub .ini files for each subpackage
for mod in pgsql mysql mysqli odbc ldap snmp xmlrpc imap \
    mysqlnd mysqlnd_mysql mysqlnd_mysqli pdo_mysqlnd \
    mbstring gd dom xsl soap bcmath dba xmlreader xmlwriter \
    pdo pdo_mysql pdo_pgsql pdo_odbc pdo_sqlite json %{zipmod} \
%if 0%{?fedora} >= 11 || 0%{?rhel} >= 6
    sqlite3  \
%endif
%if %{with_zip}
    zip \
%endif
    interbase pdo_firebird \
    enchant phar fileinfo intl \
    mcrypt tidy pdo_dblib mssql pspell curl wddx \
    posix sysvshm sysvsem sysvmsg recode; do
    cat > $RPM_BUILD_ROOT%{_sysconfdir}/php.d/${mod}.ini <<EOF
; Enable ${mod} extension module
extension=${mod}.so
EOF
    cat > $RPM_BUILD_ROOT%{_sysconfdir}/php-zts.d/${mod}.ini <<EOF
; Enable ${mod} extension module
extension=${mod}.so
EOF
    cat > files.${mod} <<EOF
%attr(755,root,root) %{_libdir}/php/modules/${mod}.so
%attr(755,root,root) %{_libdir}/php-zts/modules/${mod}.so
%config(noreplace) %attr(644,root,root) %{_sysconfdir}/php.d/${mod}.ini
%config(noreplace) %attr(644,root,root) %{_sysconfdir}/php-zts.d/${mod}.ini
EOF
done

# The dom, xsl and xml* modules are all packaged in php-xml
cat files.dom files.xsl files.xml{reader,writer} files.wddx > files.xml

# The mysql and mysqli modules are both packaged in php-mysql
cat files.mysqli >> files.mysql
# mysqlnd
cat files.mysqlnd_mysql \
    files.mysqlnd_mysqli \
    files.pdo_mysqlnd \
    >> files.mysqlnd

# Split out the PDO modules
cat files.pdo_dblib >> files.mssql
cat files.pdo_mysql >> files.mysql
cat files.pdo_pgsql >> files.pgsql
cat files.pdo_odbc >> files.odbc
cat files.pdo_firebird >> files.interbase

# sysv* and posix in packaged in php-process
cat files.sysv* files.posix > files.process

# Package sqlite3 and pdo_sqlite with pdo; isolating the sqlite dependency
# isn't useful at this time since rpm itself requires sqlite.
cat files.pdo_sqlite >> files.pdo
%if 0%{?fedora} >= 11 || 0%{?rhel} >= 6
cat files.sqlite3 >> files.pdo
%endif

# Package json, zip, curl, phar and fileinfo in -common.
cat files.json files.curl files.phar files.fileinfo > files.common
%if %{with_zip}
cat files.zip >> files.common
%endif

# Install the macros file:
install -d $RPM_BUILD_ROOT%{_sysconfdir}/rpm
sed -e "s/@PHP_APIVER@/%{apiver}/" \
    -e "s/@PHP_ZENDVER@/%{zendver}/" \
    -e "s/@PHP_PDOVER@/%{pdover}/" \
    < %{SOURCE3} > macros.php
install -m 644 -c macros.php \
           $RPM_BUILD_ROOT%{_sysconfdir}/rpm/macros.php

# Remove unpackaged files
rm -rf $RPM_BUILD_ROOT%{_libdir}/php/modules/*.a \
       $RPM_BUILD_ROOT%{_libdir}/php-zts/modules/*.a \
       $RPM_BUILD_ROOT%{_bindir}/{phptar} \
       $RPM_BUILD_ROOT%{_datadir}/pear \
       $RPM_BUILD_ROOT%{_libdir}/libphp5.la

# Remove irrelevant docs
rm -f README.{Zeus,QNX,CVS-RULES}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
rm files.* macros.php

%if %{with_fpm}
%pre fpm
# Add the "apache" user as we don't require httpd
getent group  apache >/dev/null || \
  groupadd -g 48 -r apache
getent passwd apache >/dev/null || \
  useradd -r -u 48 -g apache -s /sbin/nologin \
    -d %{_httpd_contentdir} -c "Apache" apache
exit 0

%post fpm
/sbin/chkconfig --add php-fpm

%preun fpm
if [ "$1" = 0 ] ; then
    /sbin/service php-fpm stop >/dev/null 2>&1
    /sbin/chkconfig --del php-fpm
fi
%endif

%post embedded -p /sbin/ldconfig
%postun embedded -p /sbin/ldconfig

%files
%defattr(-,root,root)
%{_libdir}/httpd/modules/libphp5.so
%{_libdir}/httpd/modules/libphp5-zts.so
%attr(0770,root,apache) %dir %{_localstatedir}/lib/php/session
%config(noreplace) %{_sysconfdir}/httpd/conf.d/php.conf
%{contentdir}/icons/php.gif

%files common -f files.common
%defattr(-,root,root)
%doc CODING_STANDARDS CREDITS EXTENSIONS LICENSE NEWS README*
%doc Zend/ZEND_* TSRM_LICENSE regex_COPYRIGHT
%doc php.ini-*
%config(noreplace) %{_sysconfdir}/php.ini
%dir %{_sysconfdir}/php.d
%dir %{_sysconfdir}/php-zts.d
%dir %{_libdir}/php
%dir %{_libdir}/php/modules
%dir %{_libdir}/php-zts
%dir %{_libdir}/php-zts/modules
%dir %{_localstatedir}/lib/php
%dir %{_datadir}/php

%files cli
%defattr(-,root,root)
%{_bindir}/php
%{_bindir}/php-cgi
%{_mandir}/man1/php-cgi.1*
%{_bindir}/phar.phar
%{_bindir}/phar
%{_mandir}/man1/phar.1*
%{_mandir}/man1/phar.phar.1*
# provides phpize here (not in -devel) for pecl command
%{_bindir}/phpize
%{_mandir}/man1/php.1*
%{_mandir}/man1/phpize.1*
%doc sapi/cgi/README* sapi/cli/README

%if %{with_fpm}
%files fpm
%defattr(-,root,root)
%doc php-fpm.conf.default
%config(noreplace) %{_sysconfdir}/php-fpm.conf
%config(noreplace) %{_sysconfdir}/php-fpm.d/www.conf
%config(noreplace) %{_sysconfdir}/logrotate.d/php-fpm
%config(noreplace) %{_sysconfdir}/sysconfig/php-fpm
%config(noreplace) %{_sysconfdir}/tmpfiles.d/php-fpm.conf
%{_initrddir}/php-fpm
%{_sbindir}/php-fpm
%dir %{_sysconfdir}/php-fpm.d
# log owned by apache for log
%attr(770,apache,root) %dir %{_localstatedir}/log/php-fpm
%dir %{_localstatedir}/run/php-fpm
%{_mandir}/man8/php-fpm.8*
%{_datadir}/fpm/status.html
%endif

%files devel
%defattr(-,root,root)
%{_bindir}/php-config
%{_bindir}/zts-php-config
%{_bindir}/zts-phpize
# usefull only to test other module during build
%{_bindir}/zts-php
%{_includedir}/php
%{_includedir}/php-zts
%{_libdir}/php/build
%{_libdir}/php-zts/build
%{_mandir}/man1/php-config.1*
%config %{_sysconfdir}/rpm/macros.php

%files embedded
%defattr(-,root,root,-)
%{_libdir}/libphp5.so
%{_libdir}/libphp5-%{version}%{?rcver}.so

%if 0%{?with_litespeed}
%files litespeed
%defattr(-,root,root)
%{_bindir}/php-ls
%endif

%files pgsql -f files.pgsql
%files mysql -f files.mysql
%files odbc -f files.odbc
%files imap -f files.imap
%files ldap -f files.ldap
%files snmp -f files.snmp
%files xml -f files.xml
%files xmlrpc -f files.xmlrpc
%files mbstring -f files.mbstring
%files gd -f files.gd
%defattr(-,root,root,-)
%doc gd_README
%files soap -f files.soap
%files bcmath -f files.bcmath
%files dba -f files.dba
%files pdo -f files.pdo
%files mcrypt -f files.mcrypt
%files tidy -f files.tidy
%files mssql -f files.mssql
%files pspell -f files.pspell
%files intl -f files.intl
%files process -f files.process
%files recode -f files.recode
%files interbase -f files.interbase
%files enchant -f files.enchant
%files mysqlnd -f files.mysqlnd


%changelog
* Fri Jun 27 2014 Carl George <carl.george@rackspace.com> - 5.4.30-1.ius
- Latest upstream source
- Patch53 removed (fixed upstream)

* Mon Jun 09 2014 Carl George <carl.george@rackspace.com> - 5.4.29-3.ius
- Add %pre fpm section to add apache user
- Update requires for fpm
- Correct bc issue in unserialize function

* Wed Jun 04 2014 Carl George <carl.george@rackspace.com> - 5.4.29-2.ius
- Rebuild for updated gnutls (RHSA-2014-0595)

* Fri May 30 2014 Carl George <carl.george@rackspace.com> - 5.4.29-1.ius
- latest sources from upstream

* Fri May 09 2014 Carl George <carl.george@rackspace.com> - 5.4.28-2.ius
- update php-fpm-www.conf

* Fri May 02 2014 Carl George <carl.george@rackspace.com> - 5.4.28-1.ius
- latest sources from upstream
- optimize version checks

* Fri Apr 04 2014 Ben Harper <ben.harper@rackspace.com> - 5.4.27-1.ius
- Latest sources from upstream

* Fri Mar 07 2014 Ben Harper <ben.harper@rackspace.com> - 5.4.26-1.ius
- Latest sources from upstream

* Fri Feb 07 2014 Ben Harper <ben.harper@rackspace.com> - 5.4.25-1.ius
- Latest sources from upstream

* Fri Jan 10 2014 Ben Harper <ben.harper@rackspace.com> - 5.4.24-1.ius
- Latest sources from upstream

* Fri Dec 13 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.23-1.ius
- Latest sources from upstream
- Source9, Source10 and Patch52 removed as cve-2013-6420 patched upstream

* Wed Dec 11 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.22-2.ius
- Source9, Source10 and Patch52 add to address cve-2013-6420

* Fri Nov 15 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.22-1.ius
- Latest sources from upstream

* Wed Nov 06 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.21-2.ius
- adding provides from LB bugs 1248288, 1248294 and 1248299

* Fri Oct 18 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.21-1.ius
- Latest sources from upstream

* Fri Sep 20 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.20-1.ius
- Latest sources from upstream

* Fri Aug 23 2013 Ben Harper <ben.harper@rackspace.com> - 5.8.19-1.ius
- Latest sources from upstream
- updated php-5.4.18-bison.patch, as it got patch upstream for RHEL 6 but not 5

* Tue Aug 20 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.18-2.ius
- added Provides for cli, see LP bug#1214603

* Fri Aug 16 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.18-1.ius
- Latest sources from upstream
- added php-5.4.18-bison.patch, see LP bug #1213017

* Mon Jul 08 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.17-1.ius
- Latest sources from upstream

* Fri Jun 07 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.16-1.ius
- Latest sources from upstream
- disable Patch50

* Thu May 09 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.15-1.ius
- Latest source from upstream

* Fri Apr 12 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.14-1.ius
- Latest source from upstream

* Fri Mar 15 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.13-1.ius
- Latest source from upstream

* Fri Feb 22 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.12-1.ius
- Latest source from upstream

* Thu Jan 17 2013 Ben Harper <ben.harper@rackspace.com> - 5.4.11-1.ius
- Latest source from upstream

* Thu Dec 20 2012 Ben Harper <ben.harper@rackspace.com> - 5.4.10.-1.ius
- Latest source from upstream

* Mon Nov 26 2012 Ben Harper <ben.harper@rackspace.com> - 5.4.9-1.ius
- Latest source from upstream

* Fri Oct 19 2012 Ben Harper <ben.harper@rackspace.com> - 5.4.8-1.ius
- Latest source from upstream
- Patch for FPM issue removed, addressed upstream
* Thu Sep 27 2012 Ben Harper <ben.harper@rackspace.com> - 5.4.7-2.ius
- Patch for FPM issue see: https://bugs.php.net/bug.php?id=62886

* Mon Sep 17 2012 Ben Harper <ben.harper@rackspace.com> - 5.4.7-1.ius
- Latest source from upstream

* Tue Aug 21 2012 Mark McKinstry <mmckinst@nexcess.net> - 5.4.6-2.ius
- compile with bundled PCRE instead of system PCRE

* Mon Aug 20 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.6-1
- Latest source from upstream

* Mon Jul 23 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.5-1
- Latest sources from upstream
- Fixes CVE-2012-2688
- Updating zipver macro to 1.11.0

* Thu Jun 14 2012 Dustin Henry Offutt <dustin.offutt@rackspace.com> 5.4.4-1
- Latest sources from upstream

* Tue May 29 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.3-2
- Removing duplicate Provides in common package

* Tue May 08 2012 Dustin Henry Offutt <dustin.offutt@rackspace.com> 5.4.3-1
- Latest sources from upstream

* Tue May 08 2012 Dustin Henry Offutt <dustin.offutt@rackspace.com> 5.4.2-3
- Add litespeed support by reconciling and applying diff supplied by
  Mark McKinstry (diff was against spec version 5.4.0-6)

* Tue May 08 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.2-2
- Building shared zip module

* Mon May 07 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.2-1
- Latest sources from upstream

* Wed May 02 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.1-2
- Fixing missing Provides on sub-packages

* Wed May 02 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.1-1
- Latest sources from upstream
- Fixing missing Provides on sub-packages

* Mon Apr 23 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.0-6
- Removing %%{isasuffix}

* Tue Apr 17 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.0-5
- Adding support for EL5 by removing sqlite3 from el5 builds

* Mon Apr 02 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.0-4
- Not working with systemd fpm service in EL

* Wed Mar 14 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.0-3
- removing trailing %

* Tue Mar 13 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.0-2
- Removing _isa tag from Requires

* Mon Mar 12 2012 Jeffrey Ness <jeffrey.ness@rackspace.com> 5.4.0-1
- Initial php54 built based off Fedora Rawhide
