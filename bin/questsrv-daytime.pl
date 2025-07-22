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
use FileHandle;
use IO::Socket;
use IO::Select;
use POSIX ":sys_wait_h";


sub docmd {
  my ($cmd) = @_;
#  printf STDERR "questsrv_daytime.pl cmd: $cmd\n";
  my $ret = `$cmd`;
#  my $ret = system($cmd);
  return $ret;

}
######################################################################
# Routines for interacting with ccp
######################################################################

sub domestatus {
    # Check dome status
    my $ret = docmd "$LS4_ROOT/bin/domestatus.csh";
    return $ret;
}

sub domeinit {
    # set current dome position to stow position
    my $ret = docmd "$LS4_ROOT/bin/domeinit.csh";
    return $ret;
}

sub auto_dome_on {
    # turn dome following on
    my $ret = docmd "$LS4_ROOT/bin/auto_dome_on.csh";
    return $ret;
}

sub auto_dome_off {
    # turn dome following off
    my $ret = docmd "$LS4_ROOT/bin/auto_dome_off.csh";
    return $ret;
}

sub move_dome {
    my ($dome_pos) = @_;
    printf STDERR "moving telescope to position $dome_pos\n";
    # move dome to specified position
    my $ret = `$LS4_ROOT/bin/move_dome.csh $dome_pos`;
    return $ret;
}

sub closedome {
    # Close Dome
    my $ret = docmd "$LS4_ROOT/bin/closedome.csh";
    return $ret;
#    printf STDERR "closedome not yet implemented\n";
#    my $ret = "ERROR";
}

sub opendome {
    # Open Dome
    my $ret = docmd "$LS4_ROOT/bin/opendome.csh"; 
    return $ret;
#    printf STDERR "opendome not yet implemented\n";
#    my $ret = "ERROR";
}


sub telescope_status {
    # Check telescope status
    my $ret = docmd "$LS4_ROOT/bin/telescope_status.csh";
    return $ret;
}

sub weather {
    my $ret = docmd "$LS4_ROOT/bin/weather";
    return $ret;
}

sub filter {
    my $ret = docmd "$LS4_ROOT/getfilter";
    return $ret;
}


######################################################################
# Get socket routines
######################################################################
sub get_config_info {
    
    my $cfgfile = "$LS4_ROOT/questlib/questsrv-daytime.cfg";
    my $questcfg = new FileHandle "<  $cfgfile"
	or die "Could not open $cfgfile: $!";
    my $nmatch = 0;

    while(<$questcfg>) {
	if(/^QUEST_COMMAND_PORT\s+(\S+)/) {
	    $quest_command_port = $1;
	    $nmatch++;
        } 
	if(/^QUEST_HOSTNAME\s+(\S+)/) {
	    $quest_hostname = $1;
	    $nmatch++;
        }
	if(/^TIMEOUT\s+(\S+)/) {
	    $timeout = $1/1000.;
	    $nmatch++;
        }
	if(/^MAX_WRITE_TIME\s+(\S+)/) {
	    $max_write_time= $1;
	    $nmatch++;
        }
    }
    if (!defined($quest_command_port)) {
	printf STDERR "Couldn't find QUEST_COMMAND_PORT in $cfgfile\n";
    }
    if (!defined($quest_hostname)) {
	printf STDERR "Couldn't find QUEST_HOSTNAME in $cfgfile\n";
    }
    if (!defined($timeout)) {
	printf STDERR "Couldn't find TIMEOUT in $cfgfile\n";
    }
    if (!defined($max_write_time)) {
	printf STDERR "Couldn't find MAX_WRITE_TIME in $cfgfile\n";
    }
    $questcfg->close or die "Couldn't close questcfg:$!\n";
	
    if ($nmatch != 4) {
	die "Ports undefined or incomplete config file";
    }
    return $nmatch;
}

sub open_quest_command_socket {
    $comsock = new IO::Socket::INET (
				     LocalHost =>	$quest_hostname,
				     LocalPort =>	$quest_command_port,
				     Proto =>	'tcp',
				     Listen =>	1,
				     Reuse =>	1,
				     );
    die "Could not create quest_command socket to host $quest_hostname at port $quest_command_port : $!\n" unless $comsock;
    $tsel = new IO::Select();
}

sub connect_quest_command_socket {
    $new_comsock = $comsock->accept();
    if (defined($new_comsock)) {
	$tsel->add($new_comsock);
	return 0;
    } else {
	return 1;
    }
}
    

sub close_quest_command_socket {
    $tsel->remove($new_comsock);
    close($new_comsock);
}

sub sel_write {
    my ($sel, $ret) = @_;
    if (@s=$sel->can_write($timeout)) {
	$w=$s[0];
	print $w "$ret\n";
	
	return 0;
    } else {
	printf STDERR "timeout on socket write\n";
	return -1;
    }
}

sub sel_read {
    my ($sel) = @_;
    if (@s=$sel->can_read) {
	$w=$s[0];
    } else {
	printf STDERR "timeout on socket read\n";
    }
    return <$w>;
}

sub shutDown {
    close_quest_command_socket;

    exit(0);
}

######################################################################
##########################################################################
##########################################################################
# Main program
#
# Takes commands: d (domestatus)
#                 w (weather)     
#                 f (filter)
#                 x ( exit )
##########################################################################
##########################################################################

get_config_info;
if ($#ARGV == 0) {
    $quest_hostname = $ARGV[0];
    printf STDERR "Using host $quest_hostname\n";
}


# Perl magic variables setting input and output line separator to null:
$/ = "\0";
$\ = "\0";

# Run initialization sequence to make sure camera, disk, are ready


open_quest_command_socket;

while(1) {

#   printf STDERR "questsrv_daytime: waiting for socket connection \n";
    if (!connect_quest_command_socket) {
#        printf STDERR "questsrv_daytime: reading next command\n";

	if (defined($msg=sel_read($tsel))) {

	    
	    if (($msg =~ /^d\0/) || ($msg =~ /^domestatus\0/) ) {
#               printf STDERR "questsrv_daytime: checking dome status\n";
		$ret = domestatus();
#               printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
	    elsif (($msg =~ /^c\0/) || ($msg =~ /^closedome\0/) ) {
                printf STDERR "questsrv_daytime: closing dome\n";
		$ret = closedome();
                printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
	    elsif (($msg =~ /^o\0/) || ($msg =~ /^opendome\0/) ) {
                printf STDERR "questsrv_daytime: opening dome\n";
		$ret = opendome();
                printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
	    elsif ($msg =~ /^auto_dome_on\0/ ) {
                printf STDERR "questsrv_daytime: turnon on autodome\n";
		$ret = auto_dome_on();
                printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
	    elsif ($msg =~ /^auto_dome_off\0/ ) {
                printf STDERR "questsrv_daytime: turnon off autodome\n";
		$ret = auto_dome_off();
                printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
	    elsif ($msg =~ /^dome_init\0/ ) {
                printf STDERR "questsrv_daytime: init dome stow position\n";
		$ret = dome_init();
                printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
#	    elsif (($msg =~ /^m\0/) || ($msg =~ /^move_dome\0/) ) {
#           } elsif ($msg =~ /^e\s+(\S+)\s+(\S+)\s+(\S+)\0/) {
            elsif ( ($msg =~ /^m\s+(\S+)\0/) || ($msg =~ /^move_dome\s+(\S+)\0/)) {
                $dome_pos = $1;
#                printf STDERR "questsrv_daytime: moving dome to position $dome_pos\n";
#		$ret = move_dome();
                $ret = `$LS4_ROOT/bin/move_dome.csh $dome_pos`;
                printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
	    elsif (($msg =~ /^t\0/) || ($msg =~ /^telstatus\0/) ) {
#               printf STDERR "questsrv_daytime: checking telescope status\n";
		$ret = telescope_status();
#               printf STDERR "questsrv_daytime: ret is $ret\n";
		sel_write($tsel,$ret);
            }
	    elsif (($msg =~ /^w\0/) || ($msg =~ /^weather\0/) ) {
#               printf STDERR "questsrv_daytime: checking weather\n";
		$ret = weather();
		sel_write($tsel,$ret);
            }
	    elsif (($msg =~ /^f\0/) || ($msg =~ /^filter\0/) ) {
#               printf STDERR "questsrv_daytime: checking filter\n";
		$ret = filter();
		sel_write($tsel,$ret);
            }
	    elsif (($msg =~ /^x\0/) || ($msg =~ /^exit\0/) ) {
                printf STDERR "questsrv_daytime: exit command received.\n";
		$ret = shutDown();
		sel_write($tsel,$ret);
	    } else {
		printf STDERR "server: bad command: $msg\n";
		sel_write($tsel,-1);
	    }
	} else {
    
           printf STDERR "questsrv_daytime: error reading next command\n";

        }

    } else {
    
        printf STDERR "questsrv_daytime: error waiting for connection\n";
  
    }
    close_quest_command_socket;
}



