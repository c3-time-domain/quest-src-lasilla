#!/bin/tcsh
#
# dome_status.csh
#
# Determine if dome shutter is opened, closed, or neither
# If questctl is running, read TCS file to determine status.
# Otherwise, use domestatus command.
#
#
# DLR 2009 Aug 10
#
set TCS_FILE = "/home/observer/quest-src-lasilla/tcs.status"
set PID_FILE = "/home/observer/logs/questctl.pid"
set VERBOSE = 0

if ( ! -e $PID_FILE ) then
   echo "$PID_FILE does not exist. Exiting" 
endif
if ( ! -e $TCS_FILE ) then
   echo "$TCS_FILE does not exist. Exiting" 
endif

set questctl_pid = `cat $PID_FILE`
alias check_questctl 'ps -aef | grep -e " `cat $PID_FILE` " | grep -ve "grep" | wc -l'
alias domestatus_direct '/home/observer/bin/domestatus'
alias domestatus 'set l0 = `cat $TCS_FILE | cut -c 1-125`; if ( $#l0 >= 15 ) echo $l0[$#l0] ; if ($#l0 < 15) echo -1'
set temp_file = "/tmp/dome_status.tmp"


# see if schmidt dome is open
#
#
  if ($VERBOSE == 1 ) then
    echo `date` " checking dome status" >> $dome_log
  endif

  if (`check_questctl`) then
     set l1 = `domestatus`
     set i = 1
     while ( $l1 == -1 && $i < 10)
        @ i = $i + 1
        sleep 1
        set l1 = `domestatus`
     end
  else
     domestatus_direct >& $temp_file
     set l2 = `cat $temp_file | tail -n 1 | grep -e "dome open" | wc -l`
     set l3 = `cat $temp_file | tail -n 1 | grep -e "failed" | wc -l`
     set l1 = 0
     @ l1 = $l2 - $l3
  endif

  if ( $l1 == 1 ) then
     echo "open"
  else if ( $l1 == 0 ) then
     echo "closed"
  else
     echo "unknown"
  endif
#
