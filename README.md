# mod_performance

This is usual Apache module for Linux (CentOS, RedHat, Fedora, Debian, OpenSuse) which can: 1) accumulate requests resource usage; 2) gives ability to analize stored data. It takes such parameters of request as:

    requested virtualhost;
    requested file;
    URI;
    CPU usage %;
    Memory usage %;
    I/O usage;
    execution time;

Collected info can be stored in SQLite, MySQL, PostgreSQL database or in text file. Module’s filter provide an opportunity to save only especial requests (for example only php scripts).

Module can work with such apache configurations:

    Apache mod_php (prefork, worker, event);
    Apache itk + mod_php;
    Apache mod_ruid2 + mod_php
    Apache + suphp (patched by mod_performance patch)
    Apache + php-fpm (patched by mod_performance patch)

More info http://bayrepo.net/
