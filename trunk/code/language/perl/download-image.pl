#!/usr/bin/perl -w
use strict;

my $list_file = "list.txt";
my $ftp_path = 'ftp://test:test@172.16.100.71/FortiBalancer/v100/';
my $file_name = "";

open (LIST_FILE, "$list_file") || die ("couldn't open file $list_file");

my @lines = <LIST_FILE>;
chop(@lines);

my $i = 0;
while ($i < @lines) {
	if ($lines[$i] =~ /^#/ || $lines[$i] =~ /^[\s]*$/) {
		$i++;
		next;
	}

	$file_name = $ftp_path.$lines[$i];

	system("proz $file_name");

	$i++;
}

close (LIST_FILE);

