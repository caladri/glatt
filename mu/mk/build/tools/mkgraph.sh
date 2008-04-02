#! /bin/sh

bsdmake clean
env KERNEL_CFLAGS=-dr NO_ASSERT=t bsdmake
egypt --include-external --omit morder_lock,morder_unlock *expand > graph/graph.dot
dot -Tpng graph/graph.dot > graph/graph.png
rm *expand
bsdmake clean
