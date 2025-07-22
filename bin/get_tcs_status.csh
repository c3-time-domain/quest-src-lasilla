#!/bin/tcsh
#
# get_tcs_status.csh
#
# if questctl is running, just print the contents of TCS_FILE to stdout.
#
# Otherwise, run domestatus and then write the TCS_FILE
#
if ( ! $?LS4_ROOT ) then
   echo "LS4_ROOT is not a defined environment variable"
   exit -1
endif
source $LS4_ROOT/.login

set TCS_FILE = "$LS4_ROOT/quest-src-lasilla/tcs.status"
set PID_FILE = "$LS4_ROOT/logs/questctl.pid"
set TEMP_FILE = "/tmp/check_telescope_status.tmp"
alias check_questctl 'ps -aef | grep -e "`cat $PID_FILE`" | grep -ve "grep" | wc -l'

if (! `check_questctl`) then
     $LS4_ROOT/bin/domestatus >&  $TEMP_FILE
endif
cat $TCS_FILE | cut -c 1-125
