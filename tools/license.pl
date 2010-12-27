#!/usr/bin/env perl
$|=1;	# Flush writes as soon as print finishes.

use strict;
use warnings;

use File::Basename;

my $scriptpath = dirname($0);
my $infile = $ARGV[0];
my $outfile = $ARGV[1];
# NOTE: ToFix: This breaks if the full path to the file contains a space character.

local $/=undef;
open LICENSE, $infile || die ("Can't open $infile:$!");
my $license_text = <LICENSE>;
close LICENSE;
mkdir dirname($outfile);

$license_text =~ s/\n/\\n/g;
$license_text =~ s/"/\\"/g;

my $prefix   = "CPUID";
my $smprefix = "cpuid";

open OUT, ">", "$outfile.tmp" or die $!;
print OUT <<__eof__;
#ifndef __included_${smprefix}_license_h
#define __included_${smprefix}_license_h

#define ${prefix}_LICENSE "${license_text}"

#endif

__eof__
close OUT or die $!;

use Digest::MD5;

my $ctx = Digest::MD5->new;
my $md5old = ""; my $md5new = "";

if (-e $outfile) {
	open OUT, "$outfile" or die $!;
	$ctx->addfile(*OUT);
	$md5old = $ctx->hexdigest;
	close OUT
}

open OUT, "$outfile.tmp" or die $!;
$ctx->addfile(*OUT);
$md5new = $ctx->hexdigest;
close OUT;

use File::Copy;

if ($md5old ne $md5new) {
	if (-e $outfile) {
		unlink($outfile) or die $!;
	}
	move "$outfile.tmp", $outfile or die $!;
} else {
	unlink ("$outfile.tmp");
}
