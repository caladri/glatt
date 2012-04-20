#! /bin/sh

rm -f mu.img
truncate -s 128m mu.img
mdunit=`sudo mdconfig -a -t vnode -f mu.img -n`
dev=/dev/md${mdunit}
sudo newfs -O 2 ${dev}
rm -rf img
mkdir img
sudo mount ${dev} $PWD/img
sudo mkdir -p $PWD/img/mu/etc
sudo mkdir -p $PWD/img/mu/servers
sudo mkdir -p $PWD/img/mu/test
sudo make mu-install DESTDIR=$PWD/img
sudo umount $PWD/img
sudo mdconfig -d -u ${mdunit}
bswapfs -y mu.img
