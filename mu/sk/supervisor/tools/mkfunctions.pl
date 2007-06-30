#! /usr/bin/env perl

use strict;
use warnings;

# We need the source path to find the function table.
die unless $ENV{'S'};

my $supervisor = $ENV{'S'} . '/supervisor';

# Paths to the input table, the stub file and the headers.
my $table = $supervisor . '/functions.table';
my $stub = $supervisor . '/functions.c';
my $header = $supervisor . '/functions.h';

# File handles.
my ($table_fh, $stub_fh, $header_fh);

open($table_fh, '<' . $table) || die "$!";

my %functions;

while (<$table_fh>) {
	chomp;

	next if /^$/ or /^#/;

	my ($function, $args) = m/^([A-Za-z_]+)\([^)]*\);/g;

	$functions{$function} = $args;
}

close($table_fh);

open($header_fh, '>' . $header) || die "$!";

print $header_fh '#ifndef _SUPERVISOR_FUNCTIONS_H_' . "\n";
print $header_fh '#define _SUPERVISOR_FUNCTIONS_H_' . "\n";
print $header_fh "\n";

print $header_fh 'enum FunctionNumber {' . "\n";
foreach my $function (sort keys %functions) {
	print $header_fh "\t" . $function . ',' . "\n";
}
print $header_fh '};' . "\n";

print $header_fh "\n";

# Function parameters;  XXX

print $header_fh "\n";
print $header_fh '#endif /* ! _SUPERVISOR_FUNCTIONS_H_ */' . "\n";

close($header_fh);

open($stub_fh, '>' . $stub) || die "$!";
close($stub_fh);
