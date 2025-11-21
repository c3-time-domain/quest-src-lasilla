#!/usr/bin/perl -w
#
# status_srv.pl
#
# Server for status data 
#
# This server is intented to run continuously on public-facing
# host (ls4-nuc) where a local client willcontinuously
# send status information (telescope, weather, camera, etc)
#
# This server will copy this info to a publically available
# webpage
#
# DLR 2025 Jul 10 


use Env;
use Cwd;
use FileHandle;
use IO::Socket;
use IO::Select;
use POSIX ":sys_wait_h";


sub docmd {
  my ($cmd) = @_;
  printf STDERR "status_srv.pl cmd: $cmd\n";
  my $ret = `$cmd`;
#  my $ret = system($cmd);
  return $ret;

}
######################################################################
#Get socket routines
######################################################################
sub get_config_info {
    
    my $cfgfile = "$LS4_ROOT/questlib/status_srv.cfg";
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
        if(/^STATUS_DIRECTORY\s+(\S+)/) {
            $status_directory= $1;
            $nmatch++;
        }
        if(/^STATUS_FILE\s+(\S+)/) {
            $status_file= $1;
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
    if (!defined($status_directory)) {
	printf STDERR "Couldn't find STATUS_DIRECTORY in $cfgfile\n";
    }
    if (!defined($status_file)) {
	printf STDERR "Couldn't find STATUS_FILE in $cfgfile\n";
    }
    else{
        print "status_file = $status_file\n";
    }
    if (!defined($timeout)) {
	printf STDERR "Couldn't find TIMEOUT in $cfgfile\n";
    }
    $statuscfg->close or die "Couldn't close statuscfg:$!\n";
	
    if ($nmatch != 5) {
	die "incomplete config file";
    }
    return $nmatch;
}

sub open_status_command_socket {
    $comsock = new IO::Socket::INET (
				     LocalHost =>	$status_hostname,
				     LocalPort =>	$status_command_port,
				     Proto =>	'tcp',
				     Listen =>	1,
				     Reuse =>	1,
				     );
    die "Could not create status_command socket: $!\n" unless $comsock;
    $tsel = new IO::Select();
}

sub connect_status_command_socket {
    $new_comsock = $comsock->accept();
    if (defined($new_comsock)) {
	$tsel->add($new_comsock);
	return 0;
    } else {
	return 1;
    }
}
    

sub close_status_command_socket {
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
    close_status_command_socket;

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


open my_file, ">$status_directory/$status_file" or die $!;
print my_file "start<br>\n";
close my_file;
chmod (0666,"$status_directory/$status_file");

open_status_command_socket;

while(1) {
    $cmd_time=time();
    printf STDERR "status_srv: waiting for socket connection: %d\n",$cmd_time;
    if (!connect_status_command_socket) {
        $cmd_time=time();
         printf STDERR "status_srv: reading next command: %d\n",$cmd_time;

	if (defined($msg=sel_read($tsel))) {

	    $cmd_time=time();
	    printf STDERR "COMMAND (%d)   ==> $msg\n",$cmd_time;
            if($msg =~ /^restart\0/){
                open my_file, ">$status_directory/$status_file" or die $!;
                print my_file "";
                close my_file;
		chmod (0666,"$status_directory/$status_file");
            }

	    open my_file, ">>$status_directory/$status_file" or die $!;
	    chop $msg;
	    print my_file "\n$msg<br>\n";
	    close my_file;
	    my_file->autoflush(0);
	    
	    if (($msg =~ /^x\0/) || ($msg =~ /^shutown\0/) ) {
		shutDown();
	    }
	    else{
		sel_write($tsel,0);
            }
#           else {
#		printf STDERR "server: bad command: $msg\n";
#		sel_write($tsel,-1);
#	    }
	} else {
    
           $cmd_time=time();
           printf STDERR "status_srv: error reading next command: %d\n",$cmd_time;

        }

    } else {
    
        $cmd_time=time();
        printf STDERR "status_srv: error waiting for connection: %d\n",$cmd_time;
  
    }
    close_status_command_socket;
}



