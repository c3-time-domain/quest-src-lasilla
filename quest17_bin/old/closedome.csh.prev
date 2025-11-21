#!/bin/tcsh
#
# closedome.csh
#
# Allow the dome_daemon to open the dome shutter by writing "CLOSE" to the dome-daemon 
# command file (COMMAND_FILE),
#
# DLR 2009 Aug 10
#
if ( ! $?LS4_ROOT ) then
   echo "LS4_ROOT is not a defined environment variable"
   exit -1
endif
source $LS4_ROOT/.login

set COMMAND_FILE = "$LS4_ROOT/status_srv/dome_daemon.command"
set PID_FILE = "$LS4_ROOT/logs/questctl.pid"
set VERBOSE = 0
set CLOSEDOME_SIGNAL = 10
alias closedome_direct  '$LS4_ROOT/bin/closedome'

set questctl_on = 0
set dome_daemon_on = 0

# check if questctl is running
if ( -e $PID_FILE ) then
   set questctl_pid = `cat $PID_FILE`
   alias check_questctl 'ps -aef | grep -e "`cat $PID_FILE`" | grep -ve "grep" | wc -l'
   if ( `check_questctl` ) then
      set questctl_on = 1
   endif
endif

# check if dome_daemon is running. If not, return with warning
set l = `ps -aef | grep dome_daemon.csh | grep -ve "grep" | wc -l`
if ( $l == 1 ) then
   set dome_daemon_on = 1
endif

echo "CLOSE " `date -u ` >! $COMMAND_FILE

if ( ! $questctl_on ) then
   echo "WARNING: questctl not running. Sending direct commands to TCS"
   $LS4_ROOT/bin/closedome |& tail -n 1
   echo "done"
else 
   if ( ! $dome_daemon_on ) then
     echo "WARNING: dome_daemon not running. Sending signal to questctl "
     kill -$CLOSEDOME_SIGNAL $questctl_pid
   endif
   echo "close sequence initiated. May take a few minutes"
endif

