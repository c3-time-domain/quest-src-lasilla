#!/usr/bin/perl -w


# Test client for tcs_talk_yale.pl

use Env;
use Cwd;
use FileHandle;
use IO::Socket;
use IO::Select;

sub get_config_info {
    
    my $cfgfile = "/home/observer/questlib/tcs_talk.cfg";
    my $tcs_talkcfg = new FileHandle "<  $cfgfile"
	or die "Could not open $cfgfile: $!";
    my $nmatch = 0;
    while(<$tcs_talkcfg>) {
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
    $tcs_talkcfg->close or die "Couldn't close cfgfile:$!";
	
    if ($nmatch != 3) {
	die "Ports undefined";
    }
    return $nmatch;
}
  

sub connect_tcs_talk_command_socket {

    $comsock = new IO::Socket::INET (
                                 PeerAddr =>	$tcs_talk_hostname,
                                 PeerPort =>	$tcs_talk_command_port,
                                 Proto =>	'tcp',
                                );
    die "Could not create tcs_talk_command_socket: $!\n" unless $comsock;
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
    $tcs_talk_hostname = $ARGV[0];
    printf STDERR "Using host $tcs_talk_hostname\n";
}


while(<STDIN>) {
        connect_tcs_talk_command_socket;
	#printf STDERR "client: $_\n";
	sel_write($tsel);
	$ret = sel_read($tsel);
	#printf STDERR "client: $_\n";
	if (defined($ret)) {
	    printf "$ret\n";
	} else {
	    printf STDERR "client: ret is undefined\n";
	}
    close($comsock);
}
