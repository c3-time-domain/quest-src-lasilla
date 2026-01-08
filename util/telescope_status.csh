#!/bin/tcsh
#
# telescope_status.csh
#
# Get verbose output of TCS status
#
# calls executable telescope_status, which passively reads
# the TCS_FILE updated by questctl. Call domestatus.csh first to
# update the TCS_FILE
#
#
# DLR 2009 Aug 10
#
if ( ! $?LS4_ROOT ) then
   echo "LS4_ROOT is not a defined environment variable"
   exit -1
endif
source $LS4_ROOT/.login

set TEMP_FILE = "/tmp/telescope_status.tmp"
#
domestatus.csh >& $TEMP_FILE
telescope_status |& tail -n 1
#
