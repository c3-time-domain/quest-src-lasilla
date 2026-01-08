#!/bin/tcsh
#
# talk.csh
#
# read TCS commands from command line and send over serial port
# to the TCS
#
# Most commands are single Words. Only two 2-word commands are allowed.
# They are "NEXTRA" and "NEXTDEC", which take arguments as the second
# word in the command
#
set l = "NOP"
while ( $l != "STOP") 
  set l = $<
  if ( $l == "NEXTRA" || $l == "NEXTDEC" || $l == "TRK" ) then
     set l1 = $<
     echo "$l $l1" | tcs_talk
  else
     echo $l | tcs_talk
  endif
end

