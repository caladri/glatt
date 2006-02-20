#! /bin/sh

find . -type f -name '*.[chS]' | grep -v build | grep -v framebuffer_font | xargs wc -l | sort -n
