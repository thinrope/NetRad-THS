#!/usr/bin/perl
use strict;
use warnings;
 

use Math::Business::SMA;
use Math::Business::WMA;
use Math::Business::EMA;
use Math::Business::HMA;

my $column = $ARGV[0] ? $ARGV[0] : 10;
my $days = $ARGV[1] ? $ARGV[1] : 1;
my %data;
$data{SMA} = new Math::Business::SMA($days);
$data{WMA} = new Math::Business::WMA($days);
$data{EMA} = new Math::Business::EMA($days);
$data{HMA} = new Math::Business::HMA($days);

my @R_header = split/\s+/, (<STDIN>);
print join("\t",@R_header,"");
my $H;
for my $k (sort keys %data)
{
	$H .= join("_", $R_header[$column], $k, $days). "\t";
}
chop $H;
print $H, "\n";

while (<STDIN>)
{
	my %q;
	my @R = split/\s+/;
	for my $k ( keys %data)
	{
		$data{$k}->insert($R[$column]);	# d_counts
		$q{$k} = $data{$k}->query;
		$q{$k} = defined($q{$k}) ? $q{$k} : "-";
	}

	print join("\t", @R, "");
	my $H;
	for my $k (sort keys %data)
	{
		$H .= "$q{$k}\t"
	}
	chop $H;
	print $H, "\n";
}
