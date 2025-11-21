#!/bin/tcsh
#
socat pty,link=/dev/ttyV0,raw tcp-listen:6011
