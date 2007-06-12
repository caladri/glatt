#! /bin/sh

mips64-gxemul-elf-nm -B -g *.o | sed 's/  */ /g' | cut -d ' ' -f 2,3 > foo.dat
grep '^T ' foo.dat | cut -d ' ' -f 2 | sh -c 'while read sym; do grep -q ^U.$sym foo.dat || echo $sym; done'
rm foo.dat
