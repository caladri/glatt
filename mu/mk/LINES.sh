#! /bin/sh

sum=0

lines() {
	if [ -d $1 ]; then
		filelist=`find $1 -type f -name '*.[chS]' | grep -v build | grep -v framebuffer_font | grep -v core/queue.h | grep -v _as.h | xargs`
		wc -l $filelist | sort -n | sed 's;total$;'"$1"' - &;'
	else
		wc -l $1
	fi
}

if [ $# -eq 0 ]; then
	lines .
else
	for arg in "$@"; do
		lines $arg
	done
fi
