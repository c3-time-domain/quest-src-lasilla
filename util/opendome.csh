#!/bin/tcsh
#
# opendome.csh
#
# Allo the dome_daemon to open the dome shutter by writing "OPEN" to the dome-daemon 
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

set questctl_on = 0
set dome_daemon_on = 0

# if sun is up, return with error
if ( `sunup |& tail -n 1` ) then
   echo "ERROR sun is up. Open cancelled"
   exit
endif

echo "OPEN " `date -u ` " " `date +"%s"` >! $COMMAND_FILE

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

# check if other domes are open
set lasilla_domes_open = 0
set l = `weather`
if ( $#l == 19 ) then 
   if ( $l[19] == 1 ) then
      set lasilla_domes_open = $l[19]
   endif
endif


# if questctl or dome_daemon is not running, echo warning
#
if ( ! $questctl_on ) then
   echo "WARNING: questctl not running. Opening delayed"
   if ( ! $dome_daemon_on ) then
     echo "WARNING: dome_daemon not running. Dome will not open"
   endif
#
# Otherwise check if other domes are opened and issue warning if not
else
  if ( ! $lasilla_domes_open ) then
     echo "WARNING: No other domes opened. Opening delayed"
  else
     echo "Open command initiated"
  endif
endif
   

