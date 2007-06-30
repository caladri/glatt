#! /usr/bin/env perl

use strict;
use warnings;

# We need the source path to find the function table.
die unless $ENV{'S'};

my $supervisor = $ENV{'S'} . '/supervisor';

# Paths to the input table, the stub file and the headers.
my $table = $supervisor . '/functions.table';
my $stub = $supervisor . '/supervisor_functions.c';
my $header = $supervisor . '/functions.h';

# File handles.
my ($table_fh, $stub_fh, $header_fh);

open($table_fh, '<' . $table) || die "$!";

my %functions;

while (<$table_fh>) {
	chomp;

	next if /^$/ or /^#/;

	my ($function, $args) = m/^([A-Za-z_]+)\(([^)]*)\);/g;

	$functions{$function} = $args;
}

close($table_fh);

sub Blurb($) {
	my ($fh) = @_;
	my ($base, $src) = ($ENV{'S'}, $table);

	$src =~ s@^${base}/@@g;

	print $fh <<_EOF_
/*
 * This file is auto-generated from ${src}.  Do not edit it directly.
 */

_EOF_
}

open($header_fh, '>' . $header) || die "$!";

Blurb($header_fh);

print $header_fh '#ifndef _SUPERVISOR_FUNCTIONS_H_' . "\n";
print $header_fh '#define _SUPERVISOR_FUNCTIONS_H_' . "\n";
print $header_fh "\n";

print $header_fh 'enum FunctionIndex {' . "\n";
foreach my $function (sort keys %functions) {
	print $header_fh "\t" . $function . '_Index' . ',' . "\n";
}
print $header_fh '};' . "\n";

foreach my $function (sort keys %functions) {
	print $header_fh "\n";

	print $header_fh 'struct ' . $function . '_Parameters' . ' {' . "\n";
	if ($functions{$function}) {
		my @args = split /,/, $functions{$function};
		my $acnt = 0;

		foreach my $arg (@args) {
			print $header_fh "\t" . $arg . 'arg' . $acnt++ . ';' . "\n";
		}
	}
	print $header_fh '};' . "\n";

	print $header_fh '#ifdef	SUPERVISOR_FUNCTIONS' . "\n";
	print $header_fh 'void ' . $function . '(';
	if ($functions{$function}) {
		print $header_fh $functions{$function};
	} else {
		print $header_fh 'void';
	}
	print $header_fh ');' . "\n";
	print $header_fh '#endif /* SUPERVISOR_FUNCTIONS */' . "\n";
}

print $header_fh "\n";
print $header_fh '#endif /* !_SUPERVISOR_FUNCTIONS_H_ */' . "\n";

close($header_fh);

open($stub_fh, '>' . $stub) || die "$!";

Blurb($stub_fh);

print $stub_fh '#include <sk/types.h>' . "\n";
print $stub_fh '#include <sk/supervisor.h>' . "\n";

print $stub_fh "\n";

foreach my $function (sort keys %functions) {
	print $stub_fh 'static void' . "\n";
	print $stub_fh $function . '_Stub(void *argp)' . "\n";
	print $stub_fh '{' . "\n";
	print $stub_fh "\t" . 'struct ' . $function . '_Parameters *args = argp;' . "\n";
	print $stub_fh "\n";
	if ($functions{$function}) {
		my @args = split /,/, $functions{$function};
		my $acnt = 0;

		print $stub_fh "\t" . $function . '(';
		foreach my $arg (@args) {
			print $stub_fh ',' unless $acnt == 0;
			print $stub_fh 'args->' . 'arg' . $acnt++;
		}
		print $stub_fh ');' . "\n";
	} else {
		print $stub_fh "\t" . '(void)args;' . "\n";
		print $stub_fh "\t" . $function . '();' . "\n";
	}

	print $stub_fh '}' . "\n";

	print $stub_fh "\n";
}

print $stub_fh 'bool' . "\n";
print $stub_fh 'supervisor_invoke(enum FunctionIndex f, void *arg, size_t argsize)' . "\n";
print $stub_fh '{' . "\n";
print $stub_fh "\t" . 'switch (f) {' . "\n";
foreach my $function (sort keys %functions) {
	print $stub_fh "\t" . 'case ' . $function . '_Index' . ':' . "\n";
	print $stub_fh "\t\t" . 'if (sizeof (struct ' . $function . '_Parameters) != argsize)' . "\n";
	print $stub_fh "\t\t\t" . 'return (false);' . "\n";
	print $stub_fh "\t\t" . $function . '_Stub' . '(arg);' . "\n";
	print $stub_fh "\t\t" . 'return (true);' . "\n";
}
print $stub_fh "\t" . 'default:' . "\n";
print $stub_fh "\t\t" . 'return (false);' . "\n";
print $stub_fh "\t" . '}' . "\n";
print $stub_fh '}' . "\n";

close($stub_fh);
