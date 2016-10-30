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

print("#date, type, delay of rsync (sec), transfer time (sec)\n");
open(INPUT, $ARGV[0]) or die("Can't open file: $ARGV[0]");
$maxtime{'vr'} = 0; $mintime{'vr'} = 1000;
$maxtime{'ze'} = 0; $mintime{'ze'} = 1000;
while ($line = <INPUT>) {
    my ($datesec, $fname, $elapsed);
    my ($date1, $sec1);

    if ($line !~ /^[\d]/) { next; }
    ($datesec, $fname, $elapsed) = split(',', $line);
    ($date1, $msec1) = (split('\.', $datesec))[0..1];
    if ($fname =~ /kobe_(\d+)_A08_pawr_(.+).dat/) {
	$date2 = $1; $type = $2;
	$dt1 = &date2sec($date1);
	$dt2 = &date2sec($date2);
	$dl = $dt1 - $dt2;
	$delay = $dl->minutes*60 + $dl->seconds + $msec1/1000;
	printf("%d, %s, %6.3f, %6.3f\n", $date2, $type, $delay, $elapsed);
#	print("$dt1, $dt2, $type, $delay, $elapse\n");
	$tdl =  $delay + $elapsed;
	$ttime{$type} += $tdl;
	$maxtime{$type} = $maxtime{$type} > $tdl ? $maxtime{$type}: $tdl;
	$mintime{$type} = $mintime{$type} < $tdl ? $mintime{$type}: $tdl;
	$count{$type} += 1;
    }
}
print("# type, min, max, avg\n");
printf("vr, %6.3f, %6.3f, %6.3f\n",
       $ttime{'vr'}/$count{'vr'}, $maxtime{'vr'}, $mintime{'vr'} );
printf("ze, %6.3f, %6.3f, %6.3f\n",
       $ttime{'ze'}/$count{'ze'}, $maxtime{'ze'}, $mintime{'ze'} );


