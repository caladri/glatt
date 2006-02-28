#! /bin/sh

if [ -z "$1" ]; then
	DIR=.
else
	DIR=$1
fi

find $DIR -type f -name '*.[chS]' | grep -v build | grep -v framebuffer_font | xargs wc -l | sort -n
