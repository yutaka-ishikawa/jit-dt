#!/usr/bin/perl -w
#
# An example:
# 20160919153918.818, kobe_20160919153800_A08_pawr_vr.dat, 1.960876
# perl -MCPAN -e shell
# install DateTime.pm
use DateTime;

sub date2sec()
{
    my $ival = $_[0];
    my $sec = $ival%100;
    my $min = ($ival/100)%100;
    my $hour= ($ival/10000)%100;
    my $day= ($ival/1000000)%100;
    my $mon = ($ival/100000000)%100;
    my $year= int $ival/10000000000;
    my $dt = DateTime->new(
	year => $year, month => $mon, day => $day,
	hour => $hour, minute=> $min, second => $sec);
    return $dt;
}

print("#procs, generated time, sent time, completion time, delay in server, delay in network, size\n");
open(SERVER, $ARGV[0]) or die("Can't open file: $ARGV[0]");
open(CLIENT, $ARGV[1]) or die("Can't open file: $ARGV[0]");
while ($cline = <CLIENT>) {
    my ($cfname, $sfaname, $elapsed, $size);
    my ($date1, $date2, $dt1, $dt2, $dt3, $fdelay, $netdelay, $fsec, $netsec);

    if ($cline =~ /nprocs/) {
	if ($cline =~ /nprocs\((.+)\)/) {
	    $nprocs= $1;
	}
	next;
    }
    if ($cline !~ /kobe_/) { next; }
    ($cfname, $compl, $size) = split(',', $cline);
    if ($cfname =~ /kobe_(\d+)_A08_pawr_(.+).dat/) {
	$gentime = $1; # $type = $2;
	#printf("%d, %s, %s, %d\n", $date1, $elapsed, $type, $size);
	$dt1 = &date2sec($gentime);
	($year, $month, $day, $hour, $min, $sec, $msec) = split(':', $compl);
	$year = int($year);
	$nano = $msec*1000000;
	$dt2 = DateTime->new(year => $year, month => $month, day => $day,
			     hour => $hour, minute=> $min, second => $sec,
			     nanosecond => $nano);
	# $dt1: generated time, $dt2: completion time at client
	while($sline = <SERVER>) {
	    if ($sline !~ /kobe_/) { next; }
	    ($sent, $sfname, $elapsed) = split(',', $sline);
	    if ($sfname =~ /kobe_(\d+)_A08_pawr_(.+).dat/) {
		if ($gentime == $1) {
		    # $dt3: sent time from the server
		    $dt3 = &date2sec($sent);
		    $fdelay = $dt3 - $dt1; 
		    $netdelay = $dt2 - $dt3;
		    $fsec = $fdelay->minutes()*60 + $fdelay->seconds();
		    $netsec = $netdelay->minutes()*60 + $netdelay->seconds();
		    printf("%d,%s,%s,%s,%d,%d,%d\n",
			   $nprocs, $gentime, $sent, $compl,
			   $fsec, $netsec, $size);
		    last;
		}
	    }
	}
    }
}
