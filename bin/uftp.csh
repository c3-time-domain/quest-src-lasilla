#!/bin/csh
#set bit_rate = 12000
set bit_rate = 18000
echo "using bit rate of $bit_rate"
/usr/local/bin/uftp -U -R $bit_rate -l 4 -H 130.132.48.116 $argv[1]
