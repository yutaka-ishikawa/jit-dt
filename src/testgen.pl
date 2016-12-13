#!/usr/bin/perl -w
#
if (@ARGV != 2) { die("testgen.pl <log file> <out directory>\n"); }

$dir = $ARGV[1];
open(INPUT, $ARGV[0]) or die("Can't open file: $ARGV[0]");
while ($line = <INPUT>) {
    if ($line !~ /^[\d]/) { next; }
    ($_, $fname,$_) = split(',', $line);
    $fname =~ /\s+(.+).dat/;
    $wpath = $dir . $1 . ".dat";
    printf("%s\n", $wpath);
    open(OUT, "> $wpath");
    printf OUT "$wpath\n";
    close(OUT);
    sleep(1);
}
