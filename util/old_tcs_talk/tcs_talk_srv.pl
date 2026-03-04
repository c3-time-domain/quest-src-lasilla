#!/usr/bin/perl -w

# Server for TCS_TALK-Yale communications
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
  my ( $cmd ) = @_;
  printf "tcs_talk_srv: command is: $cmd\n";
  my $ret = `$cmd`;
  printf "tcs_talk_srv:  return is: $ret\n";
  return $ret;
}

sub tcs_talk {
  my ( $cmd ) = @_;
  chomp ($cmd);
  my $ret = docmd "echo $cmd | /home/observer/bin/tcs_talk";
  return $ret;
}


######################################################################
# Get socket routines
######################################################################
sub get_config_info {
    
    my $cfgfile = "/home/observer/questlib/tcs_talk.cfg";
    my $tcs_talk_cfg = new FileHandle "<  $cfgfile"
	or die "Could not open $cfgfile: $!";
    my $nmatch = 0;

    while(<$tcs_talk_cfg>) {
	if(/^TCS_TALK_COMMAND_PORT\s+(\S+)/) {
	    $tcs_talk_command_port = $1;
	    $nmatch++;
        } 
	if(/^TCS_TALK_HOSTNAME\s+(\S+)/) {
	    $tcs_talk_hostname = $1;
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
    if (!defined($tcs_talk_command_port)) {
	printf STDERR "Couldn't find TCS_TALK_COMMAND_PORT in $cfgfile\n";
    }
    if (!defined($tcs_talk_hostname)) {
	printf STDERR "Couldn't find TCS_TALK_HOSTNAME in $cfgfile\n";
    }
    if (!defined($timeout)) {
	printf STDERR "Couldn't find TIMEOUT in $cfgfile\n";
    }
    if (!defined($max_write_time)) {
	printf STDERR "Couldn't find MAX_WRITE_TIME in $cfgfile\n";
    }
    $tcs_talk_cfg->close or die "Couldn't close tcs_talk_cfg:$!\n";
	
    if ($nmatch != 4) {
	die "Ports undefined or incomplete config file";
    }
    return $nmatch;
}

sub open_tcs_talk_command_socket {
    $comsock = new IO::Socket::INET (
				     LocalHost =>	$tcs_talk_hostname,
				     LocalPort =>	$tcs_talk_command_port,
				     Proto =>	'tcp',
				     Listen =>	1,
				     Reuse =>	1,
				     );
    die "Could not create tcs_talk_command socket to host $tcs_talk_hostname at port $tcs_talk_command_port : $!\n" unless $comsock;
    $tsel = new IO::Select();
}

sub connect_tcs_talk_command_socket {
    $new_comsock = $comsock->accept();
    if (defined($new_comsock)) {
	$tsel->add($new_comsock);
	return 0;
    } else {
	return 1;
    }
}
    

sub close_tcs_talk_command_socket {
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
    close_tcs_talk_command_socket;

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
    $tcs_talk_hostname = $ARGV[0];
    printf STDERR "Using host $tcs_talk_hostname\n";
}


# Perl magic variables setting input and output line separator to null:
$/ = "\0";
$\ = "\0";

# Run initialization sequence to make sure camera, disk, are ready


open_tcs_talk_command_socket;

while(1) {

#   printf STDERR "tcs_talk_srv: waiting for socket connection \n";
    if (!connect_tcs_talk_command_socket) {
#        printf STDERR "tcs_talk_srv: reading next command\n";

	if (defined($msg=sel_read($tsel))) {

	    if (($msg =~ /^x\0/) || ($msg =~ /^exit\0/) ) {
                printf STDERR "tcs_talk_srv: exit command received.\n";
		$ret = shutDown();
		sel_write($tsel,$ret);
	    } else {
		$ret = tcs_talk "$msg";
		sel_write($tsel,$ret);
	    }
	} else {
    
           printf STDERR "tcs_talk_srv: error reading next command\n";

        }

    } else {
    
        printf STDERR "tcs_talk_srv: error waiting for connection\n";
  
    }
    close_tcs_talk_command_socket;
}



