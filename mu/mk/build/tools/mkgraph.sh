#! /bin/sh

bsdmake clean
env KERNEL_CFLAGS=-dr bsdmake
egypt --include-external *expand > graph/graph.dot
/usr/local/graphviz-2.12/bin/dot -Tpdf graph/graph.dot > graph/graph.pdf
rm *expand
bsdmake clean
