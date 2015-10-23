#!/bin/bash

#CentOS 5.4  - Ok
#FreeBSD 8.2 - FAILED
#Debian 6.0 - Ok
#openSUSE 11.4 - Ok

#simple curses library to create windows on terminal
#
#author: Patrice Ferlet metal3d@copix.org
#license: new BSD
#
#create_buffer patch by Laurent Bachelier

create_buffer(){

  [[ "$1" != "" ]] &&  buffername=$1 || buffername="bashsimplecurses"

  # Try to use mktemp before using the unsafe method
  if [ -x `which mktemp` ]; then
    mktemp ${buffername}.XXXXXXXXXX
  else
    echo "bashsimplecurses."$RANDOM
  fi
}

#Usefull variables
LASTCOLS=0
BUFFER=`create_buffer`
POSX=0
POSY=0
LASTWINPOS=0

#call on SIGINT and SIGKILL
#it removes buffer before to stop
on_kill(){
    echo "Exiting"
    rm -rf $BUFFER
    exit 0
}
trap on_kill SIGINT SIGTERM


#initialize terminal
term_init(){
    POSX=0
    POSY=0
    tput clear >> $BUFFER
}


#change line
_nl(){
    POSY=$((POSY+1))
    tput cup $POSY $POSX >> $BUFFER
    #echo 
}


move_up(){
    set_position $POSX 0
}

col_right(){
    left=$((LASTCOLS+POSX))
    set_position $left $LASTWINPOS
}

#put display coordinates
set_position(){
    POSX=$1
    POSY=$2
}

#initialize chars to use
_TL="\033(0l\033(B"
_TR="\033(0k\033(B"
_BL="\033(0m\033(B"
_BR="\033(0j\033(B"
_SEPL="\033(0t\033(B"
_SEPR="\033(0u\033(B"
_VLINE="\033(0x\033(B"
_HLINE="\033(0q\033(B"
init_chars(){    
    if [[ "$ASCIIMODE" != "" ]]; then
        if [[ "$ASCIIMODE" == "ascii" ]]; then
            _TL="+"
            _TR="+"
            _BL="+"
            _BR="+"
            _SEPL="+"
            _SEPR="+"
            _VLINE="|"
            _HLINE="-"
        fi
    fi
}

#Append a windo on POSX,POSY
window(){
    LASTWINPOS=$POSY
    title=$1
    color=$2      
    tput cup $POSY $POSX 
    cols=$(tput cols)
    cols=$((cols))
    if [[ "$3" != "" ]]; then
        cols=$3
        if [ $(echo $3 | grep "%") ];then
            cols=$(tput cols)
            cols=$((cols))
            w=$(echo $3 | sed 's/%//')
            cols=$((w*cols/100))
        fi
    fi
    len=$(echo "$1" | echo $(($(wc -c)-1)))
    left=$(((cols/2) - (len/2) -1))

    #draw up line
    clean_line
    echo -ne $_TL
    for i in `seq 3 $cols`; do echo -ne $_HLINE; done
    echo -ne $_TR
    #next line, draw title
    _nl

    tput sc
    clean_line
    echo -ne $_VLINE
    tput cuf $left
    #set title color
    case $color in
        green)
            echo -n -e "\E[01;32m"
            ;;
        red)
            echo -n -e "\E[01;31m"
            ;;
        blue)
            echo -n -e "\E[01;34m"
            ;;
        grey|*)
            echo -n -e "\E[01;37m"
            ;;
    esac
    
    
    echo $title
    tput rc
    tput cuf $((cols-1))
    echo -ne $_VLINE
    echo -n -e "\e[00m"
    _nl
    #then draw bottom line for title
    addsep
    
    LASTCOLS=$cols

}

#append a separator, new line
addsep (){
    clean_line
    echo -ne $_SEPL
    for i in `seq 3 $cols`; do echo -ne $_HLINE; done
    echo -ne $_SEPR
    _nl
}


#clean the current line
clean_line(){
    tput sc
    #tput el
    tput rc
    
}


#add text on current window
append_file(){
    [[ "$1" != "" ]] && align="left" || align=$1
    while read l;do
        l=`echo $l | sed 's/____SPACES____/ /g'`
        _append "$l" $align
    done < "$1"
}
append(){
    text=$(echo -e $1 | fold -w $((LASTCOLS-2)) -s)
    rbuffer=`create_buffer bashsimplecursesfilebuffer`
    echo  -e "$text" > $rbuffer
    while read a; do
        _append "$a" $2
    done < $rbuffer
    rm -f $rbuffer
}
_append(){
    clean_line
    tput sc
    echo -ne $_VLINE
    len=$(echo "$1" | wc -c )
    len=$((len-1))
    left=$((LASTCOLS/2 - len/2 -1))
    if [ $left -lt 0 ]; then 
	left=0
    fi
    [[ "$2" == "left" ]] && left=0

    tput cuf $left
    echo -e "$1"
    tput rc
    tput cuf $((LASTCOLS-1))
    echo -ne $_VLINE
    _nl
}

#add separated values on current window
append_tabbed(){
    [[ $2 == "" ]] && echo "append_tabbed: Second argument needed" >&2 && exit 1
    [[ "$3" != "" ]] && delim=$3 || delim=":"
    clean_line
    tput sc
    echo -ne $_VLINE
    len=$(echo "$1" | wc -c )
    len=$((len-1))
    left=$((LASTCOLS/$2)) 
    for i in `seq 0 $(($2))`; do
        tput rc
        tput cuf $((left*i+1))
        echo "`echo $1 | cut -f$((i+1)) -d"$delim"`" 
    done
    tput rc
    tput cuf $((LASTCOLS-1))
    echo -ne $_VLINE
    _nl
}

#append a command output
append_command(){
    buff=`create_buffer command`
    echo -e "`$1`" | sed 's/ /____SPACES____/g' > $buff 2>&1
    append_file $buff "left"
    rm -f $buff
}

#close the window display
endwin(){
    clean_line
    echo -ne $_BL
    for i in `seq 3 $LASTCOLS`; do echo -ne $_HLINE; done
    echo -ne $_BR
    _nl
}

#refresh display
refresh (){
    cat $BUFFER
    echo "" > $BUFFER
}



#main loop called
main_loop (){
    term_init
    init_chars
    [[ "$1" == "" ]] && time=1 || time=$1
    while [[ 1 ]];do
        tput cup 0 0 >> $BUFFER
        tput il $(tput lines) >>$BUFFER
        main >> $BUFFER 
        tput cup $(tput lines) $(tput cols) >> $BUFFER 
        refresh
        sleep $time
        POSX=0
        POSY=0
    done
}

#main loop called
main_loop_ext (){
    if [ -n "$2" ]; then
	term_init
	init_chars
    fi
    tput cup 0 0 >> $BUFFER
    tput il $(tput lines) >>$BUFFER
    $1 >> $BUFFER 
    #tput cup $(tput lines) $(tput cols) >> $BUFFER 
    tput cup $(tput lines) 0 >> $BUFFER
    refresh
}
############################################################################################################################################################
CONFIG_TYPE="SQLITE"


#Функция определения типа операционной системы
# centos, debian, suse, freebsd etc
getOSName(){
    OS=`echo \`uname\`|tr "[:upper:]" "[:lower:]"`
    KERNEL=`uname -r|tr "[:upper:]" "[:lower:]"`
    MACH=`uname -m|tr "[:upper:]" "[:lower:]"`

    if [ "{$OS}" == "windowsnt" ]; then
	OS=windows
    elif [ "{$OS}" == "darwin" ]; then
	OS=mac
    else
	OS=`uname`
	if [ "${OS}" = "SunOS" ] ; then
    	    OS=Solaris
    	    ARCH=`uname -p`
    	    OSSTR="${OS} ${REV}(${ARCH} `uname -v`)"
	elif [ "${OS}" = "AIX" ] ; then
    	    OSSTR="${OS} `oslevel` (`oslevel -r`)"
	elif [ "${OS}" = "Linux" ] ; then
    	    if [ -f /etc/redhat-release ] ; then
        	DistroBasedOn='RedHat'
        	DIST=`cat /etc/redhat-release |sed s/\ release.*//|tr "[:upper:]" "[:lower:]"`
        	PSUEDONAME=`cat /etc/redhat-release | sed s/.*\(// | sed s/\)//|tr "[:upper:]" "[:lower:]"`
        	REV=`cat /etc/redhat-release | sed s/.*release\ // | sed s/\ .*//|tr "[:upper:]" "[:lower:]"`
    	    elif [ -f /etc/SuSE-release ] ; then
        	DistroBasedOn='SuSe'
        	PSUEDONAME=`cat /etc/SuSE-release | tr "\n" ' '| sed s/VERSION.*//|tr "[:upper:]" "[:lower:]"`
        	REV=`cat /etc/SuSE-release | tr "\n" ' ' | sed s/.*=\ //|tr "[:upper:]" "[:lower:]"`
    	    elif [ -f /etc/mandrake-release ] ; then
        	DistroBasedOn='Mandrake'
        	PSUEDONAME=`cat /etc/mandrake-release | sed s/.*\(// | sed s/\)//|tr "[:upper:]" "[:lower:]"`
        	REV=`cat /etc/mandrake-release | sed s/.*release\ // | sed s/\ .*//|tr "[:upper:]" "[:lower:]"`
    	    elif [ -f /etc/debian_version ] ; then
        	DistroBasedOn='Debian'
        	DIST=`cat /etc/lsb-release | grep '^DISTRIB_ID' | awk -F=  '{ print $2 }'|tr "[:upper:]" "[:lower:]"`
        	PSUEDONAME=`cat /etc/lsb-release | grep '^DISTRIB_CODENAME' | awk -F=  '{ print $2 }|tr "[:upper:]" "[:lower:]"'`
        	REV=`cat /etc/lsb-release | grep '^DISTRIB_RELEASE' | awk -F=  '{ print $2 }|tr "[:upper:]" "[:lower:]"'`
    	    fi
    	    if [ -f /etc/UnitedLinux-release ] ; then
        	DIST="${DIST}[`cat /etc/UnitedLinux-release | tr "\n" ' ' | sed s/VERSION.*//|tr \"[:upper:]\" \"[:lower:]\"`]"
    	    fi
    	    OS=`echo $OS|tr "[:upper:]" "[:lower:]"`
    	    DistroBasedOn=`echo $DistroBasedOn|tr "[:upper:]" "[:lower:]"`
    	    readonly OS
    	    readonly DIST
    	    readonly DistroBasedOn
    	    readonly PSUEDONAME
    	    readonly REV
    	    readonly KERNEL
    	    readonly MACH
	fi

    fi
}

#(YES/NO функция)Проверка наличия ncurses
checkIsPckg(){
    if [ "$DistroBasedOn" == "redhat" ]; then
	local __vr=`rpm -q "$1"`
	if [ -n "$__vr" ]; then
	    return 0
	fi
    fi
    if [ "$OS" == "FreeBSD" ]; then
	local __vr=`pkg_info | grep "$1"`
	if [ -n "$__vr" ]; then
	    return 0
	fi
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
	local __vr=`dpkg --list | grep "$1"`
	if [ -n "$__vr" ]; then
	    return 0
	fi
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
	local __vr=`rpm -q "$1"`
	if [ -n "$__vr" ]; then
	    return 0
	fi
    fi
    return 1
}

#(YES/NO функция)Функция пытается установить пакет
tryToInstallPckg(){
    if [ "$DistroBasedOn" == "redhat" ]; then
	yum install -y "$1" --nogpgcheck
	return $?
    fi
    if [ "$OS" == "FreeBSD" ]; then
	pkg_add -r "$1"
	return $?
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
	apt-get install -y -q "$1"
	return $?
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
	zypper install -y "$1"
	return $?
    fi
    return 1
}

#Проверка результата функции YES/NO функции с остановом
checkYesNoBash(){
    if [ $? == 1 ]; then
	_step="$2"
    fi
}

checkYesNoBashTxt(){
    if [ $? == 1 ]; then
	echo -e "\n$1"
	exit 1
    fi
}

#Чтение одного символа
readChar(){
    echo -n "$1:"
    read -n 1 GCHAR
}

#Проверка на числовую последовательность
checkNumbers(){
NMBS="$1"
CHR="$2"
if [ -z "$CHR" ]; then
    return 0;
fi

arr=`echo $NMBS|tr " " "\n"`                                                                                                                                    
                                                                                                                                                             
for x in $arr                                                                                                                                                
do                                                                                                                                                           
    if [ "$x" == "$CHR" ]; then
	return 0;
    fi
done  
return 1
}

#Чтение шаблона конфигурационного файла
getTemplate(){
  cat $0 | sed -n '/^#======TEMPLATE1========/,$p'
}

#Сборка модуля
makeModule(){
 if [ "$DistroBasedOn" == "redhat" ]; then
  make 2>&1
 fi
 if [ "$OS" == "FreeBSD" ]; then
  gmake 2>&1
 fi
 if [ "$DistroBasedOn" == "debian" ]; then
  make 2>&1
 fi
 if [ "$DistroBasedOn" == "suse" ]; then
  make 2>&1
 fi
}

getMemoryInfo(){
 if [ "$DistroBasedOn" == "redhat" ]; then
  ( echo -n '1024*'; cat /proc/meminfo | grep MemTotal | awk '{print $2}' ) | bc
 fi
 if [ "$OS" == "FreeBSD" ]; then
  ( dmesg | grep "real memory" | awk '{print $4}' ) | bc
 fi
 if [ "$DistroBasedOn" == "debian" ]; then
  ( echo -n '1024*'; cat /proc/meminfo | grep MemTotal | awk '{print $2}' ) | bc
 fi
 if [ "$DistroBasedOn" == "suse" ]; then
  ( echo -n '1024*'; cat /proc/meminfo | grep MemTotal | awk '{print $2}' ) | bc
 fi
}

#PerformanceSocket - путь к сокету
#PerformanceEnabled - включить наблюдение
#PerformanceHostFilter - список отслеживаемых хостов
#PerformanceDB - путь к базе данных сведений
#PerformanceHistory - число дней хранения истории
#PerformanceWorkHandler - рабочий хандлер при котором выводится статистика
#PerformanceURI - регулярное выражение фильтра по URI
#PerformanceScript - регулярное выражение фильтра по имени скрипта
#PerformanceMaxThreads - максимальное число дополнительно запускаемых одновременно нитей(умноженное на 2)
#PerformanceStackSize - размер стека для создаваемого следящего потока
#PerformanceUserHandler - рабочий хандлер при котором выводится статистика для хоста
#PerformanceUseCanonical - использовать каноническое имя при логировании
#PerformanceExtended - использовать команду контроля выполняющихся потоков
#PerformanceLog - путь к файлу логов
#PerformanceLogFormat - формат выводимого лога (%DATE%, %CPU%, %MEM%, %URI%, %HOST%, %SCRIPT%, %EXCTIME%)
#PerformanceLogType - тип логирования информации (Log, SQLite, MySQL, Postgres)
#PerformanceDbUserName - пользователь для соединения с БД(MySQL,...)
#PerformanceDBPassword - пароль для соединения с БД(MySQL,...)
#PerformanceDBName - имя базы для соединения с БД(MySQL,...)
#PerformanceDBHost - хост БД(MySQL,...)
#PerformanceUseCPUTopMode - Irix/Solaris режим подсчета CPU % как в procps top (работает только в Linux) - Irix(считать #будто 1 процессор), Solaris(считать? что в системе N процессоров)
#PerformanceCheckDaemon - On/Off - включить режим слежения за демоном(CPU usage, Memory usage, IO usage)
#PerformanceCheckDaemonInterval - интервал отслеживания ресурсов, потребляемых деменом, в секундах
#PerformanceCheckDaemonMemory - размер доступной демону памяти в Мбайт, при достижении этого лимита - демон перезапускается
#PerformanceCheckDaemonTimeExec - время выполнения демона, после его истечения демон перезапускается в секундах или в #формате HH:MM:SS - т.е. время каждого дня, когда перезапускается демон
#PerformanceFragmentationTime - ежедневная дефрагментация базы данных. Только для MySQL
#PerformanceExternalScript - список скриптов, которые будут обрабатываться внешними модулями, например - mod_fcgid, #mod_cgid, suphp...
#PerformanceMinExecTime - два параметра 1) число(в 1/100 секунды), 2) HARD/SOFT - задает минимальное время выполения скрипта #и способ его сохранения HARD(не сохранять)/SOFT(сохранять с 0 %CPU)

initVars(){
if [ "$DistroBasedOn" == "redhat" ]; then
 TEMPLATE1_MODULE_PATH="modules/mod_performance.so"
fi
if [ "$OS" == "FreeBSD" ]; then
 TEMPLATE1_MODULE_PATH="libexec/apache22/mod_performance.so"
fi
if [ "$DistroBasedOn" == "debian" ]; then
 TEMPLATE1_MODULE_PATH="/usr/lib/apache2/modules/mod_performance.so"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 TEMPLATE1_MODULE_PATH="/usr/lib/apache2-prefork/mod_performance.so"
fi
TEMPLATE1_PerformanceSocket="/opt/performance/perfsock"
TEMPLATE1_PerformanceEnabled="On"
TEMPLATE1_PerformanceHostFilter=""
TEMPLATE1_PerformanceDB="/opt/performance/perfdb"
TEMPLATE1_PerformanceHistory="10"
TEMPLATE1_PerformanceWorkHandler=""
TEMPLATE1_PerformanceURI=""
TEMPLATE1_PerformanceScript=".php"
__initVars_tmp1=`getMemoryInfo` 
TEMPLATE1_PerformanceMaxThreads=`echo "$__initVars_tmp1/1024/1024/4" | bc`
TEMPLATE1_PerformanceStackSize="1"
TEMPLATE1_PerformanceUserHandler=""
TEMPLATE1_PerformanceUseCanonical="On"
TEMPLATE1_PerformanceExtended=""
TEMPLATE1_PerformanceLog=""
TEMPLATE1_PerformanceLogFormat=""
TEMPLATE1_PerformanceLogType="SQLite"
TEMPLATE1_PerformanceDbUserName=""
TEMPLATE1_PerformanceDBPassword=""
TEMPLATE1_PerformanceDBName=""
TEMPLATE1_PerformanceDBHost=""
if [ "$OS" == "FreeBSD" ]; then
 TEMPLATE1_PerformanceUseCPUTopMode=""
else
 TEMPLATE1_PerformanceUseCPUTopMode="Irix"
fi
TEMPLATE1_PerformanceCheckDaemon="Off"
TEMPLATE1_PerformanceCheckDaemonInterval=""
TEMPLATE1_PerformanceCheckDaemonMemory=""
TEMPLATE1_PerformanceCheckDaemonTimeExec="1:00:00"
TEMPLATE1_PerformanceFragmentationTime=""
TEMPLATE1_PerformanceExternalScript=".php"
TEMPLATE1_PerformanceMinExecTime=""
TEMPLATE1_PerformanceSilentMode=""
TEMPLATE1_PerformancePeriodicalWatch=""
}

getsuphp(){
_cd=`pwd`
if [ "$DistroBasedOn" == "redhat" ]; then
wget -q http://www.suphp.org/download/suphp-0.7.1.tar.gz
fi
if [ "$OS" == "FreeBSD" ]; then
fetch -q http://www.suphp.org/download/suphp-0.7.1.tar.gz
fi
if [ "$DistroBasedOn" == "debian" ]; then
wget -q http://www.suphp.org/download/suphp-0.7.1.tar.gz
fi
if [ "$DistroBasedOn" == "suse" ]; then
checkIsPckg "patch"
if [ $? == 1 ]; then
    tryToInstallPckg "patch"
    checkYesNoBashTxt "Не могу поставить пакет patch. Установка модуля отменена"
fi
wget -q http://www.suphp.org/download/suphp-0.7.1.tar.gz
fi
if [ $? == 0 ];then
 tar zxvf suphp-0.7.1.tar.gz  2>&1 1>/dev/null
 cd suphp-0.7.1
 cp ../../patches/modperf_su_php0.7.1_2011_07_20.patch modperf_su_php0.7.1_2011_07_20.patch
 patch -p1 < modperf_su_php0.7.1_2011_07_20.patch 2>&1 1>/dev/null
 if [ "$DistroBasedOn" == "redhat" ]; then
 _result1=`./configure --with-apr=/usr/bin/apr-1-config --with-apxs=/usr/sbin/apxs 2>&1`
 fi
 if [ "$OS" == "FreeBSD" ]; then
 _result1=`./configure --with-apr=/usr/local/bin/apr-1-config --with-apxs=/usr/local/sbin/apxs 2>&1`
 fi
 if [ "$DistroBasedOn" == "debian" ]; then
 _result1=`./configure --with-apr=/usr/bin/apr-1-config --with-apxs=/usr/bin/apxs2 2>&1`
 fi
 if [ "$DistroBasedOn" == "suse" ]; then
 _result1=`./configure --with-apr=/usr/bin/apr-1-config --with-apxs=/usr/sbin/apxs2 2>&1`
 fi
 _result2=`make 2>&1`
 if [ $? == 0 ]; then
  if [ ! -d "../../.libs" ]; then
   mkdir -p ../../.libs
  fi
  cp -f src/apache2/.libs/mod_suphp.so.0.0.0 ../../.libs/mod_suphp.so
  cd "$_cd"
  return 0
 else
  _result = `echo $_result1 $_result2`
 fi
fi
cd "$_cd"
return 1
}

escape(){
 echo "$1" | sed 's/\//\\\//g'
}

parseTemplate(){
_tpl=`echo "$1"`
if [ -z "$TEMPLATE1_MODULE_PATH" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_MODULE_PATH/d`
else
 _tmp=`escape "$TEMPLATE1_MODULE_PATH"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_MODULE_PATH/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceSocket" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceSocket/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceSocket"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceSocket/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceEnabled" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceEnabled/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceEnabled"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceEnabled/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceHostFilter" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceHostFilter/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceHostFilter"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceHostFilter/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceDB" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceDB$/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceDB"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceDB$/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceHistory" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceHistory/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceHistory"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceHistory/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceWorkHandler" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceWorkHandler/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceWorkHandler"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceWorkHandler/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceURI" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceURI/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceURI"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceURI/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceScript" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceScript/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceScript"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceScript/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceMaxThreads" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceMaxThreads/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceMaxThreads"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceMaxThreads/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceStackSize" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceStackSize/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceStackSize"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceStackSize/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceUserHandler" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceUserHandler/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceUserHandler"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceUserHandler/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceUseCanonical" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceUseCanonical/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceUseCanonical"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceUseCanonical/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceExtended" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceExtended/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceExtended"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceExtended/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceLog" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceLog$/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceLog"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceLog$/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceLogFormat" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceLogFormat/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceLogFormat"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceLogFormat/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceLogType" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceLogType/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceLogType"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceLogType/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceDbUserName" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceDbUserName/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceDbUserName"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceDbUserName/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceDBPassword" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceDBPassword/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceDBPassword"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceDBPassword/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceDBName" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceDBName/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceDBName"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceDBName/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceDBHost" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceDBHost/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceDBHost"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceDBHost/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceUseCPUTopMode" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceUseCPUTopMode/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceUseCPUTopMode"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceUseCPUTopMode/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceCheckDaemon" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceCheckDaemon$/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceCheckDaemon"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceCheckDaemon$/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceCheckDaemonInterval" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceCheckDaemonInterval/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceCheckDaemonInterval"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceCheckDaemonInterval/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceCheckDaemonMemory" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceCheckDaemonMemory/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceCheckDaemonMemory"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceCheckDaemonMemory/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceCheckDaemonTimeExec" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceCheckDaemonTimeExec/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceCheckDaemonTimeExec"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceCheckDaemonTimeExec/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceFragmentationTime" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceFragmentationTime/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceFragmentationTime"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceFragmentationTime/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceExternalScript" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceExternalScript/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceExternalScript"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceExternalScript/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceMinExecTime" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceMinExecTime/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceMinExecTime"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceMinExecTime/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceSilentMode" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceSilentMode/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceSilentMode"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceSilentMode/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformanceSocketPerm" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformanceSocketPerm/d`
else
 _tmp=`escape "$TEMPLATE1_PerformanceSocketPerm"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformanceSocketPerm/$_tmp/"`
fi

if [ -z "$TEMPLATE1_PerformancePeriodicalWatch" ];then
 _tpl=`echo "$_tpl" | sed /TEMPLATE1_PerformancePeriodicalWatch/d`
else
 _tmp=`escape "$TEMPLATE1_PerformancePeriodicalWatch"`
 _tpl=`echo "$_tpl" | sed "s/TEMPLATE1_PerformancePeriodicalWatch/$_tmp/"`
fi

echo "$_tpl"
}

#########################################################################################################################

frame1(){

step1(){
    window "Шаг 1" "red"
    append "Добро пожаловать в систему интерактивной сборки модуля mod_performance"
    append "система позволяет корректно собрать модуль, а также подготовить файл конфигурации" "left"
    append "На текущий момент поддерживаются конфигурации:" "left"
    append "1) Apache 2.2 prefork + mod_php" "left"
    append "2) Apache 2.2 prefork + suphp" "left"
    append "3) Apache 2.2 itk + mod_php" "left"
    append "4) Apache 2.2 prefork + mod_ruid2 + mod_php" "left"
    append ""
    append "Не рекомендуется использовать модуль для режима Apache 2.2 worker или event" "left"
    append "Если после установки модуля веб-сервер работает нестабильно, то просто отключите модуль" "left"
    append "а об ошибке или проблемах можно написать по адресу: http://lexvit.dn.ua/helpdesk/" "left"
    endwin 

    window "Предположительная текущая конфигурация" "green"
    if [ "$DistroBasedOn" == "redhat" ]; then
    _apache=`cat /etc/sysconfig/httpd | grep ^HTTPD=`
    if [ -z "$_apache" ]; then
     _apache_bin="/usr/sbin/httpd"
    else
     _apache_bin=`echo "$_apache" | sed s/HTTPD=//`
    fi
    fi
    if [ "$OS" == "FreeBSD" ]; then
     _apache_bin="/usr/local/sbin/httpd"
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
     _apache_bin="/usr/sbin/apache2"
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
     _apache_bin="/usr/sbin/httpd2-prefork"
    fi
    _apache_type=`"$_apache_bin" -V | grep "Server MPM" | sed s/Server\ MPM://`
    _apache_ver=`"$_apache_bin" -V | grep "Server version" | sed s/Server\ version://`
    _modules=`"$_apache_bin" -M 2>&1`
    append "`echo Apache MPM: $_apache_type`"
    append "`echo Apache version: $_apache_ver`"
    _mod_php_y=`echo "$_modules" | grep php5_module`
    if [ -z "$_mod_php_y" ];then
	append "mod_php: disabled"
    else	
	append "mod_php: yes"
    fi
    _mod_ruid_y=`echo "$_modules" | grep mod_ruid2`
    if [ -z "$_mod_ruid_y" ];then
	append "mod_ruid2: disabled"
    else	
	append "mod_ruid2: yes"
    fi
    _mod_suphp_y=`echo "$_modules" | grep mod_suphp`
    if [ -z "$_mod_suphp_y" ];then
	append "mod_suphp: disabled"
    else	
	append "mod_suphp: yes"
    fi

    endwin

}
main_loop_ext step1
readChar "К следующему шагу [y/n] (нажмите y или n без нажатия Enter)"
if [ "$GCHAR" != "y" ]; then
    echo ""
    _step=""
else
    _step="2"
fi

}

frame2(){
step2(){
    window "Шаг 2" "red"
    append "Сборка модуля" 
    append "Будут проверены все зависимости и необходимые для сборки и работы пакеты, а также произведена сборка модуля." "left"
    append "Если какого-нибудь из пакетов не будет хватать - инсталлятор попытается поставить его самостоятельно" "left"
    append "Требование для корректной работы модуля - это наличие пакетов:" "left"
    append "1) минимальная конфигурация - httpd, apr, gd - данные собираемые модулем сохраняются в лог файл apache, отсутсвует web-интерфейс" "left"
    append "2) конфигурация SQLite - httpd, apr, gd, sqlite3 - данные собираемые модулем сохраняются в базе sqlite, режим легко настраивается, но при большом числе запросов к серверу создает большую нагрузку на дисковую подсистему" "left"
    append "3) конфигурация MySQL - httpd, apr, gd, mysql - данные собираемые модулем сохраняются в базе mysql. Тяжелее настройка, но намного меньше нагрузка на дисковую подсистему" "left"
    append "4) конфигурация PostgreSQL - httpd, apr, gd, postgres - аналогично режиму mysql, только в качестве БД используется PostgreSQL" "left"
    append "5) пропустить сборку - перейти к формированию файла конфигурации" "left"
    append "Режимы 2,3,4 предоставляют веб-интерфейс модуля с отчетами" "left"
    append "При нагрузке на сервер 3-5 запросов в секунду можно использовать режим 2 иначе 3 или 4" "left"
    endwin 

}

main_loop_ext step2 "sec"
readChar "Выберите режим конфигурации (по умолчанию - 2) [1/2/3/4/q - выход]"
checkNumbers "1 2 3 4 5 q" "$GCHAR"
if [ $? == 1 ]; then
checkYesNoBash "Неверный выбор. Выберите 1,2,3,4,5,q или просто Enter. Процесс установки будет завершен" "2"
else
if [ "$GCHAR" == "q" ]; then
 _step=""
else
 case "$GCHAR" in
 1)
 _step="3"
 ;;
 2)
 _step="4"
 ;;
 3)
 _step="5"
 ;;
 4)
 _step="6"
 ;;
 5)
 _step="7"
 ;;
 esac
fi
fi

}


frame3_ok(){
step3_1(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "минимальная конфигурация - httpd, apr, gd - данные собираемые модулем сохраняются в лог файл apache, отсутсвует web-интерфейс" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append ""
    append ""
    _dir=`pwd`
    append "`echo \"Модуль собран и размещен в каталоге $_dir/.libs\"`" "left"
    append "Дальнейшая конфигурация - это формирование конфигурационного файла и дополнительные сборки, например сборка пропатченного suphp" "left"
    endwin 

}
main_loop_ext step3_1 "sec"
readChar "Перейти к формированию конфигурационного файла модуля[y/n]"
if [ "$GCHAR" != "y" ]; then
     echo ""
     _step=""
else
     _step="7"
fi
}

frame3_error(){
step3_1_err(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "минимальная конфигурация - httpd, apr, gd - данные собираемые модулем сохраняются в лог файл apache, отсутсвует web-интерфейс" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append ""
    append ""
    append "`echo $_result`" 
    endwin 

}
main_loop_ext step3_1_err "sec"
readChar "Для выхода нажмите любую клавишу"
_step=""
}

frame3_mysql_ok(){
step3_mysql_1(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "конфигурация mysql - httpd, apr, gd, mysql" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append "mysql - установлен" "left"
    append ""
    append ""
    _dir=`pwd`
    append "`echo \"Модуль собран и размещен в каталоге $_dir/.libs\"`" "left"
    append "Дальнейшая конфигурация - это формирование конфигурационного файла и дополнительные сборки, например сборка пропатченного suphp" "left"
    endwin 

}
main_loop_ext step3_mysql_1 "sec"
readChar "Перейти к формированию конфигурационного файла модуля[y/n]"
if [ "$GCHAR" != "y" ]; then
     echo ""
     _step=""
else
     _step="7"
fi
}

frame3_mysql_error(){
step3_1_mysql_err(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "конфигурация mysql - httpd, apr, gd, mysql" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append "mysql - установлен" "left"
    append ""
    append ""
    append "`echo $_result`" 
    endwin 

}
main_loop_ext step3_1_mysql_err "sec"
readChar "Для выхода нажмите любую клавишу"
_step=""
}

frame3_sqlite_ok(){
step3_sqlite_1(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "конфигурация sqlite - httpd, apr, gd, sqlite" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append "sqlite - установлен" "left"
    append ""
    append ""
    _dir=`pwd`
    append "`echo \"Модуль собран и размещен в каталоге $_dir/.libs\"`" "left"
    append "Дальнейшая конфигурация - это формирование конфигурационного файла и дополнительные сборки, например сборка пропатченного suphp" "left"
    endwin 

}
main_loop_ext step3_sqlite_1 "sec"
readChar "Перейти к формированию конфигурационного файла модуля[y/n]"
if [ "$GCHAR" != "y" ]; then
     echo ""
     _step=""
else
     _step="7"
fi
}

frame3_sqlite_error(){
step3_1_sqlite_err(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "конфигурация sqlite - httpd, apr, gd, sqlite" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append "sqlite - установлен" "left"
    append ""
    append ""
    append "`echo $_result`" 
    endwin 

}
main_loop_ext step3_1_sqlite_err "sec"
readChar "Для выхода нажмите любую клавишу"
_step=""
}

frame3_postgres_ok(){
step3_postgres_1(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "конфигурация postgres - httpd, apr, gd, postgres" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append "postgres - установлен" "left"
    append ""
    append ""
    _dir=`pwd`
    append "`echo \"Модуль собран и размещен в каталоге $_dir/.libs\"`" "left"
    append "Дальнейшая конфигурация - это формирование конфигурационного файла и дополнительные сборки, например сборка пропатченного suphp" "left"
    endwin 

}
main_loop_ext step3_postgres_1 "sec"
readChar "Перейти к формированию конфигурационного файла модуля[y/n]"
if [ "$GCHAR" != "y" ]; then
     echo ""
     _step=""
else
     _step="7"
fi
}

frame3_postgres_error(){
step3_1_postgres_err(){
    window "Шаг 3" "red"
    append "Сборка модуля" 
    append "конфигурация sqlite - httpd, apr, gd, sqlite" "left"
    append ""
    append ""
    append "httpd - установлен" "left"
    append "apr - установлен" "left"
    append "gd - установлен" "left"
    append "sqlite - установлен" "left"
    append ""
    append ""
    append "`echo $_result`" 
    endwin 

}
main_loop_ext step3_1_postgres_err "sec"
readChar "Для выхода нажмите любую клавишу"
_step=""
}

frame4(){
step4(){
    window "Шаг 4" "red"
    append "Конфигурирование модуля" 
    append "Добро пожаловать в мастер конфигурации" "left"
    append "1) Создать конфигурацию поумолчанию - SQLite (возможна дальнейшая правка конфигурационного файла руками)" "left"
    append "2) Создать конфигурацию поумолчанию - MySQL (возможна дальнейшая правка конфигурационного файла руками)" "left"
    append "3) Создать конфигурацию поумолчанию - PostgreSQL (возможна дальнейшая правка конфигурационного файла руками)" "left"
    append "4) Создать конфигурацию поумолчанию - Log (возможна дальнейшая правка конфигурационного файла руками)" "left"
    endwin 

}

main_loop_ext step4 "sec"
readChar "Выберите режим конфигурации (по умолчанию - 1) [1/2/3/4/q - выход]"
checkNumbers "1 2 3 4 q" "$GCHAR"
if [ $? == 1 ]; then
checkYesNoBash "Неверный выбор. Выберите 1,2,3,4,q или просто Enter. Процесс установки будет завершен" "1"
else
if [ "$GCHAR" == "q" ]; then
 _step=""
else
 case "$GCHAR" in
 1)
 CONFIG_TYPE="SQLITE"
 _step="8"
 ;;
 2)
 CONFIG_TYPE="MYSQL"
 _step="8"
 ;;
 3)
 CONFIG_TYPE="POSTGRESQL"
 _step="8"
 ;;
 4)
 CONFIG_TYPE="LOG"
 _step="8"
 ;;
 esac
fi
fi

}

frame5_8(){
__tmp58="$1"
step5_8(){
    window "Шаг 5" "red"
    append "Конфигурирование модуля" "left"
    append "Файл конфигурации по умолчанию создан и помещен в каталог .libs/modperfauto.conf" "left"
    append "Перепроверьте конфигурацию файла, пути к модулю и сокету" "left"
    append "Инструкции и рекомендации по использованию модуля можно прочитать по адресу http://lexvit.dn.ua/modperf" "left"
    append "Если у вас установлен и активирован suPHP, в этом случае интерактивный установщик позволяет пропатчить его для работы с модулем. Если желаете его пропатчить, нажмите y, иначе n или q" "left"
    endwin 

    window "Рекомендации" "green"
    case "$__tmp58" in
    SQLITE)
    append "Работа с SQLite. Самый простой вариант, база данных создается автоматически, таблица создается автоматически, нет необходимости в пользователях и прочее. Только одно очень важное замечание для тех кто уже использовал модуль версии 0.1: в старой базе и новой различаются структуры таблиц, поэтому для успешной работы старую базу лучше удалить. Т.к. cам модуль не пересоздает существующую таблицу." "left"
    append "" "left"
    append "Для работы с SQLite необходимо:" "left"
    append "PerformanceDB /opt/performance/perfdb" "left"
    append "PerformanceLogType SQLite" "left"
    append "" "left"
    append "" "left"
    append "создайте папку /opt/performance/, установите на нее права 755 и установите пользователя и группу под которой выполняется Apache" "left"
    ;;
    MYSQL)
    append "Работа с MySQL. Более сложный вариант." "left"
    append "Создайте базу данных, например, perf и пользователя perf с правами на эту базу:" "left"
    append "mysql> create database perf;" "left"
    append "mysql> CREATE USER 'perf'@'localhost' IDENTIFIED BY 'perf';" "left"
    append "mysql> GRANT ALL PRIVILEGES ON *.* TO 'perf'@'localhost' WITH GRANT OPTION;" "left"
    append "" "left"
    append "таблицу модуль создаст сам. И теперь в настройках модуля:" "left"
    append "PerformanceLogType MySQL" "left"
    append "PerformanceDbUserName perf" "left"
    append "PerformanceDBPassword perf" "left"
    append "PerformanceDBName perf" "left"
    append "" "left"
    append "" "left"
    append "создайте папку /opt/performance/, установите на нее права 755 и установите пользователя и группу под которой выполняется Apache" "left"
    ;;
    POSTGRESQL)
    append "Работа с PostgreSQL. Более сложный вариант." "left"
    append "Так же необходимо создать базу и пользователя для доступа:" "left"
    append "postgres=# CREATE USER perf WITH PASSWORD 'perf';" "left"
    append "postgres=# CREATE DATABASE perf;" "left"
    append "postgres=# GRANT ALL PRIVILEGES ON DATABASE perf to perf;" "left"
    append "" "left"
    append "в файле /var/lib/pgsql/data/pg_hba.conf" "left"
    append "" "left"
    append "local all all trust" "left"
    append "host all all 0.0.0.0/0 trust" "left"
    append "host all all : : : 1/128 trust" "left"
    append "" "left"
    append "и наконец настройки модуля:" "left"
    append "PerformanceLogType Postgres" "left"
    append "PerformanceDbUserName perf" "left"
    append "PerformanceDBPassword perf" "left"
    append "PerformanceDBName perf" "left"
    append "создайте папку /opt/performance/, установите на нее права 755 и установите пользователя и группу под которой выполняется Apache" "left"
    ;;
    LOG)
    append "Работа с текстовым логом." "left"
    append "В данном режиме не требуется никаких дополнительных библиотек. Достаточно прописать файл, куда будет консолидироваться статистика." "left"
    append "" "left"
    append "PerformanceLogType Log" "left"
    append "PerformanceLog /opt/performance/perf.log" "left"
    append "" "left"
    append "По умолчанию данные в этот файл попадают в таком формате:" "left"
    append "" "left"
    append "[%DATE%] from %HOST% (%URI%) script %SCRIPT%: cpu %CPU% (%CPUS%), memory %MEM% (%MEMMB%), execution time %EXCTIME%, IO: R - %BYTES_R% W - %BYTES_W%" "left"
    append "" "left"
    append "создайте папку /opt/performance/, установите на нее права 755 и установите пользователя и группу под которой выполняется Apache" "left"
    ;;
    esac
    endwin
}

main_loop_ext step5_8 "sec"
readChar "Пропатчить suPHP [y/n/q - выход]"
checkNumbers "y n q" "$GCHAR"
if [ $? == 1 ]; then
checkYesNoBash "Неверный выбор. Выберите y,n,q или просто Enter. Процесс установки будет завершен" "y"
else
if [ "$GCHAR" == "q" ]; then
 _step=""
else
 case "$GCHAR" in
 y)
 _step="9"
 ;;
 n)
 _step="1000"
 ;;
 esac
fi
fi

}

frame6(){
step6(){
    window "Шаг 6" "red"
    append "Cборка suPHP" "left"
    append "Собрана типовая конфигурация suPHP." "left"
    append "При необходимости можно собрать suPHP самостоятельно. http://lexvit.dn.ua/modperf" "left"
    endwin 
}

main_loop_ext step6 "sec"
readChar "Закончить [y/q - выход]"
checkNumbers "y q" "$GCHAR"
if [ $? == 1 ]; then
checkYesNoBash "Неверный выбор. Выберите y,q или просто Enter. Процесс установки будет завершен" "y"
else
if [ "$GCHAR" == "q" ]; then
 _step=""
else
 case "$GCHAR" in
 n)
 _step="1000"
 ;;
 esac
fi
fi

}

frame6_error(){
step6_err(){
    window "Шаг 6" "red"
    append "Cборка suPHP" "left"
    append "Собрана типовая конфигурация suPHP."
    append "При необходимости можно собрать suPHP самостоятельно. http://lexvit.dn.ua/modperf" "left"
    append ""
    append ""
    append "`echo $_result`" 
    endwin 

}
main_loop_ext step6_err "sec"
readChar "Для выхода нажмите любую клавишу"
_step=""
}

frame1000(){
step1000(){
    window "Финал" "red"
    append "Cборка и конфигурация завершена" "left"
    append "Информация о модуле на сайте - http://lexvit.dn.ua/modperf" "left"
    append "Поддержка модуля - http://lexvit.dn.ua/helpdesk" "left"
    append "Новости модуля - http://lexvit.dn.ua/event" "left"
    append "Планируемые изменения - http://lexvit.dn.ua/bugtraq" "left"
    endwin 

}
main_loop_ext step1000 "sec"
readChar "Для выхода нажмите любую клавишу"
_step=""
}

common_install(){
if [ "$DistroBasedOn" == "redhat" ]; then
 checkIsPckg "httpd-devel"
fi
if [ "$OS" == "FreeBSD" ]; then
 checkIsPckg "apache"
fi
if [ "$DistroBasedOn" == "debian" ]; then
 checkIsPckg "apache2-prefork-dev"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 checkIsPckg "apache2-devel"
fi
if [ $? == 1 ]; then
    if [ "$DistroBasedOn" == "redhat" ]; then
      tryToInstallPckg "httpd-devel"
    fi
    if [ "$OS" == "FreeBSD" ]; then
      tryToInstallPckg "apache22"
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
      tryToInstallPckg "apache2-prefork-dev"
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
      tryToInstallPckg "apache2-devel"
    fi
    checkYesNoBashTxt "Не могу поставить пакет httpd-devel. Установка модуля отменена"
fi
if [ "$DistroBasedOn" == "redhat" ]; then
 checkIsPckg "apr-devel"
fi
if [ "$OS" == "FreeBSD" ]; then
 checkIsPckg "apr-ipv6-devrandom-gdbm-db42"
fi
if [ "$DistroBasedOn" == "debian" ]; then
 checkIsPckg "libapr1-dev"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 checkIsPckg "libapr1-devel"
fi
if [ $? == 1 ]; then
    if [ "$DistroBasedOn" == "redhat" ]; then
     tryToInstallPckg "apr-devel"
    fi
    if [ "$OS" == "FreeBSD" ]; then
     tryToInstallPckg "apr-ipv6-devrandom-gdbm-db42"
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
     tryToInstallPckg "libapr1-dev"
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
     tryToInstallPckg "libapr1-devel"
    fi
    checkYesNoBashTxt "Не могу поставить пакет apr-devel. Установка модуля отменена"
fi
if [ "$DistroBasedOn" == "redhat" ]; then
 checkIsPckg "gd-devel"
fi
if [ "$OS" == "FreeBSD" ]; then
 checkIsPckg "gd"
fi
if [ "$DistroBasedOn" == "debian" ]; then
 checkIsPckg "libgd2-xpm-dev"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 checkIsPckg "gd-devel"
fi
if [ $? == 1 ]; then
    if [ "$DistroBasedOn" == "redhat" ]; then
     tryToInstallPckg "gd-devel"
    fi
    if [ "$OS" == "FreeBSD" ]; then
     tryToInstallPckg "gd"
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
     tryToInstallPckg "libgd2-xpm-dev"
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
     tryToInstallPckg "gd-devel"
    fi
    checkYesNoBashTxt "Не могу поставить пакет gd-devel. Установка модуля отменена"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 checkIsPckg "make"
 if [ $? == 1 ]; then
   tryToInstallPckg "make"
 fi
fi
}

############################################################################################################################################################
getOSName
initVars
checkIsPckg "ncurses"
if [ $? == 1 ]; then
    tryToInstallPckg "ncurses"
    checkYesNoBashTxt "Не могу поставить пакет ncurses. Установка модуля отменена"
fi

_step="1"
until [ -z $_step ]; do
case $_step in
1)
frame1
;;
2)
frame2
;;
3)
#httpd, apr, gd
common_install
_result=`makeModule`
if [ $? == 0 ]; then
frame3_ok
else
frame3_error
fi
;;
4)
#httpd, apr, gd, sqlite3
common_install
if [ "$DistroBasedOn" == "redhat" ]; then
 checkIsPckg "sqlite3"
fi
if [ "$OS" == "FreeBSD" ]; then
 checkIsPckg "sqlite3"
fi
if [ "$DistroBasedOn" == "debian" ]; then
 checkIsPckg "sqlite3"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 checkIsPckg "sqlite3"
fi
if [ $? == 1 ]; then
    if [ "$DistroBasedOn" == "redhat" ]; then
     tryToInstallPckg "sqlite3"
    fi
    if [ "$OS" == "FreeBSD" ]; then
     tryToInstallPckg "sqlite3"
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
     tryToInstallPckg "sqlite3"
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
     tryToInstallPckg "sqlite3"
    fi
    checkYesNoBashTxt "Не могу поставить пакет sqlite3. Установка модуля отменена"
fi
_result=`makeModule`
if [ $? == 0 ]; then
frame3_sqlite_ok
else
frame3_sqlite_error
fi
;;
5)
#httpd, apr, gd, mysql
common_install
if [ "$DistroBasedOn" == "redhat" ]; then
 checkIsPckg "mysql"
fi
if [ "$OS" == "FreeBSD" ]; then
 checkIsPckg "mysql"
fi
if [ "$DistroBasedOn" == "debian" ]; then
 checkIsPckg "libmysqlclient16"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 checkIsPckg "mysql-community-server-client"
fi
if [ $? == 1 ]; then
    if [ "$DistroBasedOn" == "redhat" ]; then
     tryToInstallPckg "mysql"
    fi
    if [ "$OS" == "FreeBSD" ]; then
     tryToInstallPckg "mysql51-client"
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
     tryToInstallPckg "libmysqlclient16"
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
     tryToInstallPckg "mysql-community-server-client"
    fi
    checkYesNoBashTxt "Не могу поставить пакет mysql. Установка модуля отменена"
fi
_result=`makeModule`
if [ $? == 0 ]; then
frame3_mysql_ok
else
frame3_mysql_error
fi
;;
6)
#httpd, apr, gd, postgres
common_install
if [ "$DistroBasedOn" == "redhat" ]; then
 checkIsPckg "potgresql"
fi
if [ "$OS" == "FreeBSD" ]; then
echo ""
fi
if [ "$DistroBasedOn" == "debian" ]; then
 checkIsPckg "potgresql"
fi
if [ "$DistroBasedOn" == "suse" ]; then
 checkIsPckg "suse"
fi
if [ $? == 1 ]; then
    if [ "$DistroBasedOn" == "redhat" ]; then
     tryToInstallPckg "potgresql"
    fi
    if [ "$OS" == "FreeBSD" ]; then
    echo ""
    fi
    if [ "$DistroBasedOn" == "debian" ]; then
     tryToInstallPckg "potgresql"
    fi
    if [ "$DistroBasedOn" == "suse" ]; then
     tryToInstallPckg "potgresql"
    fi
    checkYesNoBashTxt "Не могу поставить пакет potgresql. Установка модуля отменена"
fi
_result=`makeModule`
if [ $? == 0 ]; then
frame3_postgres_ok
else
frame3_postgres_error
fi
;;
7)
frame4
;;
8)
case "$CONFIG_TYPE" in
SQLITE)
TEMPLATE1_PerformanceDB="/opt/performance/perfdb"
TEMPLATE1_PerformanceLogType="SQLite"
;;
MYSQL)
TEMPLATE1_PerformanceDB=""
TEMPLATE1_PerformanceLogType="MySQL"
TEMPLATE1_PerformanceDbUserName="perf"
TEMPLATE1_PerformanceDBPassword="perf"
TEMPLATE1_PerformanceDBName="perf"
;;
POSTGRESQL)
TEMPLATE1_PerformanceDB=""
TEMPLATE1_PerformanceLogType="Postgres"
TEMPLATE1_PerformanceDbUserName="perf"
TEMPLATE1_PerformanceDBPassword="perf"
TEMPLATE1_PerformanceDBName="perf"
;;
LOG)
TEMPLATE1_PerformanceDB=""
TEMPLATE1_PerformanceLog="/opt/performance/perf.log"
TEMPLATE1_PerformanceLogType="Log"
;;
esac
_modconfig=`getTemplate`
_modconfig=`parseTemplate "$_modconfig"`
if [ -d ".libs" ]; then
  echo "$_modconfig" > .libs/modperfauto.conf
else
  mkdir .libs
  echo "$_modconfig" > .libs/modperfauto.conf
fi
frame5_8 "$CONFIG_TYPE"
;;
9)
mkdir suphp
cd suphp
getsuphp
if [ $? == 1 ]; then
 cd ..
 rm -fr suphp
 frame6_error
else
 cd ..
 rm -fr suphp
 frame6
fi
;;
1000)
frame1000
;;
*)
_step=""
;;
esac
done

echo ""

exit 0

#======TEMPLATE1========
LoadModule performance_module TEMPLATE1_MODULE_PATH

<IfModule mod_performance.c>
 #On module
 PerformanceEnabled TEMPLATE1_PerformanceEnabled

 #Filters
 PerformanceHostFilter TEMPLATE1_PerformanceHostFilter
 PerformanceURI TEMPLATE1_PerformanceURI
 PerformanceScript TEMPLATE1_PerformanceScript
 PerformanceExternalScript TEMPLATE1_PerformanceExternalScript

 #Pathes
 PerformanceSocket TEMPLATE1_PerformanceSocket
 PerformanceDB TEMPLATE1_PerformanceDB
 PerformanceLog TEMPLATE1_PerformanceLog

 #Handlers
 PerformanceWorkHandler TEMPLATE1_PerformanceWorkHandler
 PerformanceUserHandler TEMPLATE1_PerformanceUserHandler

 #Stack info
 PerformanceStackSize TEMPLATE1_PerformanceStackSize
 PerformanceMaxThreads TEMPLATE1_PerformanceMaxThreads
 PerformanceExtended TEMPLATE1_PerformanceExtended

 #Log type
 PerformanceLogType TEMPLATE1_PerformanceLogType
 PerformanceLogFormat TEMPLATE1_PerformanceLogFormat
 PerformanceDbUserName TEMPLATE1_PerformanceDbUserName
 PerformanceDBPassword TEMPLATE1_PerformanceDBPassword
 PerformanceDBName TEMPLATE1_PerformanceDBName
 PerformanceDBHost TEMPLATE1_PerformanceDBHost

 #Self control
 PerformanceCheckDaemon TEMPLATE1_PerformanceCheckDaemon
 PerformanceCheckDaemonInterval TEMPLATE1_PerformanceCheckDaemonInterval
 PerformanceCheckDaemonMemory TEMPLATE1_PerformanceCheckDaemonMemory
 PerformanceCheckDaemonTimeExec TEMPLATE1_PerformanceCheckDaemonTimeExec

 #Add
 PerformanceHistory TEMPLATE1_PerformanceHistory
 PerformanceUseCanonical TEMPLATE1_PerformanceUseCanonical
 PerformanceUseCPUTopMode TEMPLATE1_PerformanceUseCPUTopMode
 PerformanceFragmentationTime TEMPLATE1_PerformanceFragmentationTime
 PerformanceMinExecTime TEMPLATE1_PerformanceMinExecTime
 PerformanceSilentMode TEMPLATE1_PerformanceSilentMode
 PerformanceSocketPerm TEMPLATE1_PerformanceSocketPerm
 PerformancePeriodicalWatch TEMPLATE1_PerformancePeriodicalWatch

 <Location /performance-status>
    SetHandler performance-status
    #Add password or allowed IP here
 </Location>
</IfModule>



