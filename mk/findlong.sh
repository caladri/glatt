#! /bin/sh

find "$@" -name '*.[chS]' | xargs -n1 perl -ne 'while (s/\t+/" " x (length($&) * 8 - length($`) % 8)/e) { }; if (length($_) > 81) { print $ARGV . ":\n" . $_ . "\n"; exit(0); }'
