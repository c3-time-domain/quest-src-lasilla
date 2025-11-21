#!/bin/tcsh
if ( ! $?LS4_ROOT ) then
   echo "LS4_ROOT is not a defined environment variable"
   exit -1
endif

`date` >& test.log
alias domestatus 'echo "domestatus" | $LS4_ROOT/quest-src-lasilla/util/questclient.pl'
set i = 1
while ( $i <= 10 )
domestatus >>& test.log
sleep 3
end
