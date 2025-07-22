#!/usr/bin/perl -w

# client for status_srv.pl
#
# This client passes standard input to the status server.
# The status server appends that information to the status webpage
#

use Env;
use Cwd;
use FileHandle;
use IO::Socket;
use IO::Select;

sub get_config_info {
    
    my $cfgfile = "$LS4_ROOT/questlib/status_client.cfg";
    my $statuscfg = new FileHandle "<  $cfgfile"
	or die "Could not open $cfgfile: $!";
    my $nmatch = 0;
    while(<$statuscfg>) {
	if(/^STATUS_COMMAND_PORT\s+(\S+)/) {
	    $status_command_port = $1;
	    $nmatch++;
        } 
	if(/^STATUS_HOSTNAME\s+(\S+)/) {
	    $status_hostname = $1;
	    $nmatch++;
        }
	if(/^TIMEOUT\s+(\S+)/) {
	    $timeout = $1/1000.;
	    $nmatch++;
        }
    }
    if (!defined($status_command_port)) {
	printf STDERR "Couldn't find STATUS_COMMAND_PORT in $cfgfile\n";
    }
    if (!defined($status_hostname)) {
	printf STDERR "Couldn't find STATUS_HOSTNAME in $cfgfile\n";
    }
    if (!defined($timeout)) {
	printf STDERR "Couldn't find TIMEOUT in $cfgfile\n";
    }
    $statuscfg->close or die "Couldn't close cfgfile:$!";
	
    if ($nmatch != 3) {
	die "Ports undefined";
    }
    return $nmatch;
}
  

sub connect_status_command_socket {

    $comsock = new IO::Socket::INET (
                                 PeerAddr =>	$status_hostname,
                                 PeerPort =>	$status_command_port,
                                 Proto =>	'tcp',
                                );
    die "Could not create status_command_socket: $!\n" unless $comsock;
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
    $status_hostname = $ARGV[0];
    printf STDERR "Using host $status_hostname\n";
}


while(<STDIN>) {
        connect_status_command_socket;
	sel_write($tsel);
	$ret = sel_read($tsel);
#	printf STDERR "client: $_\n";
	if (/^x$/ || /^shutdown/) {
	    last;
	}
	if (defined($ret)) {
#	    printf STDERR "client: ret=$ret\n";
	    printf STDERR "$ret";
	} else {
	    printf STDERR "client: ret is undefined\n";
	}
       close($comsock);
}
#printf STDERR "closing socket\n";


