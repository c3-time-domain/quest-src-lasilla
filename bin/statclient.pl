#!/usr/bin/perl -w

# Test client for questsrv.pl

use Env;
use Cwd;
use FileHandle;
use IO::Socket;
use IO::Select;

sub get_config_info {
    
#   my $cfgfile = "$PALOMARDIR/config/quest.cfg";
    my $cfgfile = "./quest.cfg";
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
    $questcfg->close or die "Couldn't close cfgfile:$!";
	
    if ($nmatch != 3) {
	die "Ports undefined";
    }
    return $nmatch;
}
  

sub connect_quest_command_socket {

    $comsock = new IO::Socket::INET (
                                 PeerAddr =>	$quest_hostname,
                                 PeerPort =>	$quest_command_port,
                                 Proto =>	'tcp',
                                );
    die "Could not create quest_command_socket: $!\n" unless $comsock;
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
    $quest_hostname = $ARGV[0];
    printf STDERR "Using host $quest_hostname\n";
}


while(<STDIN>) {
connect_quest_command_socket;
    if ($_ =~ /^q/) {
	    printf STDERR "closing socket\n";
	    close($comsock);
	    exit(0);
    } else {
	sel_write($tsel);
        $ret  = sel_read($tsel);
	printf STDERR "client: $_\n";
	if (/^x$/ || /^shutdown/) {
	    last;
	}
	if (defined($ret)) {
	    $mytime = time();
	    printf STDERR "(%d) client: ret=$ret\n",$mytime;
	} else {
	    printf STDERR "client: ret is undefined\n";
	}
    }
close($comsock);
}


