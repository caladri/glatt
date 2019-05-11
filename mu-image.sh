#! /bin/sh

sudo rm -rf img
mkdir img
if type bsdmake > /dev/null 2>&1; then
	sudo bsdmake mu-install DESTDIR=$PWD/img
else
	sudo make mu-install DESTDIR=$PWD/img
fi

rm -f mu.img
if type makefs > /dev/null 2>&1; then
	makefs -b 50% -f 50% -B big -o version=2 mu.img $PWD/img
else
	tar -C $PWD/img -cf mu.img .
fi
