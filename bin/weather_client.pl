#!/usr/bin/perl -w

# Test client for weather_yale.pl

use Env;
use Cwd;
use FileHandle;
use IO::Socket;
use IO::Select;

sub get_config_info {
    
    my $cfgfile = "$LS4_ROOT/questlib/weather_srv.cfg";
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
    $weathercfg->close or die "Couldn't close cfgfile:$!";
	
    if ($nmatch != 3) {
	die "Ports undefined";
    }
    return $nmatch;
}
  

sub connect_weather_command_socket {

    $comsock = new IO::Socket::INET (
                                 PeerAddr =>	$weather_hostname,
                                 PeerPort =>	$weather_command_port,
                                 Proto =>	'tcp',
                                );
    die "Could not create weather_command_socket: $!\n" unless $comsock;
    $tsel = new IO::Select();
    $tsel->add($comsock);

}    

sub sel_write {
    my ($sel) = @_;
    if (@s=$sel->can_write($timeout)) {
	chomp($_);
	$w=$s[0];
	printf $w "$_\0";
    } else {
	printf STDERR "timeout on socket write\n";
    }
}

sub sel_read {
    my ($sel) = @_;
    if (@s=$sel->can_read($timeout)) {
	$w=$s[0];
	return <$w>;
    } else {
	printf STDERR "timeout on socket read\n";
	return -2;
    }
}

get_config_info;
if ($#ARGV == 0) {
    $weather_hostname = $ARGV[0];
    printf STDERR "Using host $weather_hostname\n";
}


while(<STDIN>) {
        connect_weather_command_socket;
	sel_write($tsel);
	$ret = sel_read($tsel);
	printf STDERR "client: $_\n";
	if (/^x$/ || /^shutdown/) {
	    last;
	}
	if (defined($ret)) {
	    printf STDERR "client: ret=$ret\n";
	} else {
	    printf STDERR "client: ret is undefined\n";
	}
       close($comsock);
}
printf STDERR "closing socket\n";


