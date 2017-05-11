#!/usr/bin/perl -w
#
# Ex.
#    ./ktestgen.pl ../testdata/JITDTLOG.20170510 ../testdata/data.tar \
#		    /tmp/bell/ /tmp/ 2
#   In this case,
#     something like the following commands are executed each interval time:
#	cd /tmp/;
#	tar --to-stdout -x -f /home/ishikawa/jit-dt/src/../testdata/data.tar \
#	       kobe_20170510170000_A08_pawr_vr.dat.gz \
#	    | gunzip > kobe_20170510170000_A08_pawr_vr.dat
#
use Cwd;
use Time::HiRes qw(setitimer ITIMER_REAL);

if (@ARGV < 3) {
    die("ktestgen.pl <log file> <tar file> " .
		    "<watching directory> <out directory>" .
	            "[inteval] [count] \n");
}
open(INPUT, $ARGV[0]) or die("Can't open file: $ARGV[0]");
$cwd = Cwd::getcwd();
$targzfile = $ARGV[1];
if ($targzfile !~ "^/" && $targzfile !~ "^~") {
    $targzfile = $cwd . "/" . $targzfile;
}
$bellpath = $ARGV[2] . "/bell";
$dir = $ARGV[3];
if (@ARGV >= 5) {
    $watch_interval = int($ARGV[4]);
} else {
    $watch_interval = 30;
}
$first_sleep = 1;
$count = -1;
if (@ARGV == 6) {
    $count = int($ARGV[5]);
}
$retval = 0;
printf("Interval: %d sec, Count: %d\n", $watch_interval, $count);

$SIG{ALRM} = sub {
#    printf("ALRAM: %s\n", $cmd);
    if (eof(INPUT)) {
	printf("Finish.\n");
	exit 0;
    }
    $nfiles = 2;
    while ($nfiles-- > 0) {
	do {
	    $line = <INPUT>;
	} while ($line !~ /^[\d]/);
	($_, $fname,$_) = split(',', $line);
	$fname =~ /\s+(.+).dat/;
	$fgzname = $fname . ".gz";
	$cmd = "cd " . $dir . "; tar --to-stdout -x -f " . $targzfile . $fgzname
	    . " | gunzip >" . $fname;
	$retval = system $cmd;
	$time = localtime(time);
	if ($retval == 0) {
	    printf("[%s] %s\n", $time, $cmd);
	} else {
	    printf("[%s] exit code(%d) %s\n", $time, $retval, $cmd);
	    exit 0;
	}
	# belling to kwatcher
	open(OUT, "> $bellpath");
	printf OUT "$fname\n";
	close(OUT);
    }
    if ($count > 0) {
	$count--;
	if ($count == 0) {
	    printf("Finish.\n");
	    exit 0;
	}
    }
};

setitimer(ITIMER_REAL, $first_sleep, $watch_interval);

while (<>) {
    sleep(1000);
}
