#! /bin/sh

lines() {
	find $1 -type f -name '*.[chS]' | grep -v build | grep -v framebuffer_font | xargs wc -l | sort -n | sed 's/total$/'"$1"' - &/'
}

if [ $# -eq 0 ]; then
	lines .
else
	for arg in "$@"; do
		lines $arg
	done
fi
