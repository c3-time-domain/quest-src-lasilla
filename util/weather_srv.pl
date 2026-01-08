#!/usr/bin/perl -w
#
# weather_srv.pl
#
# Server for weather data on La Silla
#
# DLR 2009 Jun 12 


use Env;
use Cwd;
use FileHandle;
use IO::Socket;
use IO::Select;
use POSIX ":sys_wait_h";


sub docmd {
  my ($cmd) = @_;
  printf STDERR "weather_srv.pl cmd: $cmd\n";
  my $ret = `$cmd`;
#  my $ret = system($cmd);
  return $ret;

}
######################################################################
#Get socket routines
######################################################################
sub get_config_info {
    
    my $cfgfile = "$PALOMARDIR/config/weather_srv.cfg";
    my $weathercfg = new FileHandle "<  $cfgfile"
	or die "Could not open $cfgfile: $!";
    my $nmatch = 0;

    while(<$weathercfg>) {
	if(/^WEATHER_COMMAND_PORT\s+(\S+)/) {
	    $weather_command_port = $1;
	    $nmatch++;
        }
        if(/^WEATHER_HOSTNAME\s+(\S+)/) {
            $weather_hostname = $1;
            $nmatch++;
        }
	if(/^TIMEOUT\s+(\S+)/) {
	    $timeout = $1/1000.;
	    $nmatch++;
        }
    }
    if (!defined($weather_command_port)) {
	printf STDERR "Couldn't find WEATHER_COMMAND_PORT in $cfgfile\n";
    }
    if (!defined($weather_hostname)) {
	printf STDERR "Couldn't find WEATHER_HOSTNAME in $cfgfile\n";
    }
    if (!defined($timeout)) {
	printf STDERR "Couldn't find TIMEOUT in $cfgfile\n";
    }
    $weathercfg->close or die "Couldn't close weathercfg:$!\n";
	
    if ($nmatch != 3) {
	die "Ports undefined or incomplete config file";
    }
    return $nmatch;
}

sub open_weather_command_socket {
    $comsock = new IO::Socket::INET (
				     LocalHost =>	$weather_hostname,
				     LocalPort =>	$weather_command_port,
				     Proto =>	'tcp',
				     Listen =>	1,
				     Reuse =>	1,
				     );
    die "Could not create weather_command socket: $!\n" unless $comsock;
    $tsel = new IO::Select();
}

sub connect_weather_command_socket {
    $new_comsock = $comsock->accept();
    if (defined($new_comsock)) {
	$tsel->add($new_comsock);
	return 0;
    } else {
	return 1;
    }
}
    

sub close_weather_command_socket {
    $tsel->remove($new_comsock);
    close($new_comsock);
}

sub sel_write {
    my ($sel, $ret) = @_;
    if (@s=$sel->can_write($timeout)) {
	$w=$s[0];
	$ret_time = time();
	printf STDERR "RETURNING (%d) ==> $ret\n",$ret_time;
	my $diff = $ret_time - $cmd_time;
	printf STDERR "Time to return = $diff\n";
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
    close_weather_command_socket;

    exit(0);
}

##########################################################################
# Main program
#
# Takes commands: s (status)
#                 x (shutdown)
##########################################################################
##########################################################################

get_config_info;

# Perl magic variables setting input and output line separator to null:
$/ = "\0";
$\ = "\0";



open_weather_command_socket;

while(1) {
    $cmd_time=time();
    printf STDERR "weather_srv: waiting for socket connection: %d\n",$cmd_time;
    if (!connect_weather_command_socket) {
        $cmd_time=time();
         printf STDERR "weather_srv: reading next command: %d\n",$cmd_time;

	if (defined($msg=sel_read($tsel))) {

	    $cmd_time=time();
	    printf STDERR "COMMAND (%d)   ==> $msg\n",$cmd_time;
	    
	    if (($msg =~ /^s\0/) || ($msg =~ /^status\0/) ) {
		$ret = docmd("ntt_dome_status");
		sel_write($tsel,$ret);
	    } elsif (($msg =~ /^x\0/) || ($msg =~ /^shutdown\0/) ) {
		shutDown();
	    } else {
		printf STDERR "server: bad command: $msg\n";
		sel_write($tsel,-1);
	    }
	} else {
    
           $cmd_time=time();
           printf STDERR "weather_srv: error reading next command: %d\n",$cmd_time;

        }

    } else {
    
        $cmd_time=time();
        printf STDERR "weather_srv: error waiting for connection: %d\n",$cmd_time;
  
    }
    close_weather_command_socket;
}



