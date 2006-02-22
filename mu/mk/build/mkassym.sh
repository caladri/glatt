#!/bin/sh
# From:
# $FreeBSD: src/sys/kern/genassym.sh,v 1.4 2002/02/11 03:54:30 obrien Exp $

${NM:='nm'} "$1" | ${AWK:='awk'} '
/ C .*sign$/ {
	sign = substr($1, length($1) - 3, 4)
	sub("^0*", "", sign)
	if (sign != "")
		sign = "-"
}
/ C .*w0$/ {
	w0 = substr($1, length($1) - 3, 4)
}
/ C .*w1$/ {
	w1 = substr($1, length($1) - 3, 4)
}
/ C .*w2$/ {
	w2 = substr($1, length($1) - 3, 4)
}
/ C .*w3$/ {
	w3 = substr($1, length($1) - 3, 4)
	w = w3 w2 w1 w0
	sub("^0*", "", w)
	if (w == "")
		w = "0"
	sub("w3$", "", $3)
	# This still has minor problems representing INT_MIN, etc.  E.g.,
	# with 32-bit 2''s complement ints, this prints -0x80000000, which 
	# has the wrong type (unsigned int).
	printf("#define\t%s\t%s0x%s\n", $3, sign, w)
}
'