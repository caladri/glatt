#! /bin/sh

filterfilelist() {
	grep -vE '(framebuffer_font|core/queue.h)'
}

lines() {
	filelist=$1
	if [ -d "${filelist}" ]; then
		filelist=`makefilelist "${filelist}"`
	fi
	wc -l $filelist | sort -n | sed 's;total$;'"$1"' - &;'
}

makefilelist() {
	directory=$1
	find "${directory}" -type f -name '*.[chS]' |  filterfilelist | xargs
}

if [ $# -eq 0 ]; then
	lines .
else
	for arg in "$@"; do
		lines $arg
	done
fi
