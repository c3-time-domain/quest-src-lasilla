#!/bin/tcsh
#
# status_daemon.csh
#
# periodically check weather (currently just the open/close status of the NTT
# and the 2.2-m telescopes at La SIlla). If the status changes, report the
# change to the status server (running on blade16)
#
set UPDATE_INTERVAL = 10
set NO_STATUS = -2
set STATUS_LOG = "status.log"
set CLIENT = "status_client.pl"
set WEATHER_LOG = "weather.log"
set WEATHER_LOG1 = "weather.logs"
set t = `date +"%Y%m%dT%H:%M:%S"`
set ERROR_TIMEOUT = 120
set UPDATE_INTERVAL = 3600
set dome_status_index = 19
set t_prev = `date +"%s"`


if ( -e $STATUS_LOG) rm $STATUS_LOG
echo "$t starting status daemon" > $STATUS_LOG
echo "$t $NO_STATUS" >> $STATUS_LOG
echo "restart" | $CLIENT 
echo "$t starting status log" | $CLIENT 
#
while (1)
#
# get last status from STATUS_LOG
#
  set l = `tail -n 1 $STATUS_LOG`
  if ( $#l >= 2 )then
    set last_status = $l[2]
  else
    set last_status = $NO_STATUS
  endif
#
# get current status
#
  weather >& $WEATHER_LOG

# check if there was an ERROR

  set le = `grep ERROR $WEATHER_LOG`
  if ( $#le > 0 ) then
    set error_flag = 1
  else
    set error_flag = 0
  endif

# if there was an error, keep trying every 10 seconds
# until ERROR_TIMEOUT is exceeded
  set t1 = 0
  set iteration = 0
  while ( ( $error_flag > 0 ) && $t1 < $ERROR_TIMEOUT )
    date >> $STATUS_LOG
    echo $le >> $STATUS_LOG
    @ iteration = $iteration + 1
    echo "Error reading weather iteration $iteration. Waiting 10 seconds and checking again" >> $STATUS_LOG
    sleep 10
    @ t1 = $t1 + 10
    weather >& $WEATHER_LOG
    set le = `grep ERROR $WEATHER_LOG`
    if ( $#le > 0 ) then
      set error_flag = 1
    else
      set error_flag = 0
    endif
  end
  
  if ( $error_flag > 0 ) then
    date >> $STATUS_LOG
    echo "unable to read weather for $ERROR_TIMEOUT seconds" >> $STATUS_LOG
  endif

  set l = `tail -n 1 $WEATHER_LOG`

  if ( $#l == $dome_status_index ) then
    set current_status = $l[$dome_status_index]
  else
    set current_status = $NO_STATUS
  endif
#
# if status has changed, report it to the status server
# and append the new status to the STATUS_LOG
#
  set t_current = `date +"%s"`
  
  if ( ( $current_status != $last_status ) || ( $t_current - $t_prev > $UPDATE_INTERVAL) ) then
     set t = `date +"%Y%m%dT%H:%M:%S"`
     echo "$t $l" | $CLIENT 
     echo "TCS_STATE: " `get_tcs_status.csh`  | $CLIENT 
     echo "$t $current_status $l" >> $STATUS_LOG
     date >> $WEATHER_LOG1
     cat $WEATHER_LOG >> $WEATHER_LOG1
     csh ./net_test.csh >! net_test.tmp
     set l  = `grep -e "16384" net_test.tmp`
     if ( $#l == 10 ) then
        echo "TCP_TEST: to CTIO $l[5] ; to Yale $l[10] Mb/s" | $CLIENT
     endif
     set t_prev = $t_current
  endif
  sleep $UPDATE_INTERVAL
end
