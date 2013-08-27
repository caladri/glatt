#! /bin/sh

rm -rf img
mkdir img
sudo make mu-install DESTDIR=$PWD/img

rm -f mu.img
makefs -B big -o version=2 mu.img $PWD/img
