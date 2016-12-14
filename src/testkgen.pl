#!/usr/bin/perl -w
#
# testkgen.pl TESTLOG ~/tmp/tmp/ ~/tmp/ishikawa/
if (@ARGV != 3) {
    die("testkgen.pl <log file> <out directory> <notify directory>\n");
}

$dir = $ARGV[1];
$notify = $ARGV[2];
open(INPUT, $ARGV[0]) or die("Can't open file: $ARGV[0]");
$age = 0;
while ($line = <INPUT>) {
    if ($line !~ /^[\d]/) { next; }
    ($_, $fname,$_) = split(',', $line);
    $fname =~ /\s+(.+).dat/;
    $wpath = $dir . $1 . ".dat";
    open(OUT, "> $wpath");
    printf OUT "$wpath";
    close(OUT);
    $age = ($age + 1) % 10;
    $agefile = $notify . $age;
    open(NOTIFY, "> $agefile");
    printf NOTIFY "$wpath";
    close(NOTIFY);
    printf("%s\n", $wpath);
    sleep(1);
}
