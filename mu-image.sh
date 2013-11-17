#! /bin/sh

sudo rm -rf img
mkdir img
sudo make mu-install DESTDIR=$PWD/img

rm -f mu.img
makefs -b 50% -f 50% -B big -o version=2 mu.img $PWD/img
