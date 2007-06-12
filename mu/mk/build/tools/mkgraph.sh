#! /bin/sh

env KERNEL_CFLAGS=-dr bsdmake
egypt *expand > graph.dot
/usr/local/graphviz-2.12/bin/dot -Tpdf graph.dot > graph.pdf
