#!/usr/bin/perl -w

# Server for QUEST-Yale communications
# 
# modified Jun 14 2003 by DLR to eliminate cammgr
# modification of camctl required.
#
# modified by DLR to take out call to
# camBiasOff  12-15-03
#
# modified by DLR to put Finger Positions and temperatures in FITS
# header on initialization. Finger Temperatures also put in after
# each exposure command
# DLR - 01-12-04


use Env;
use Cwd;

printf STDERR "questsrv_daytime.pl cmd: $LS4_ROOT\n";
