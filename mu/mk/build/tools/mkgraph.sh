#! /bin/sh

env KERNEL_CFLAGS=-dr bsdmake
egypt *expand > graph/graph.dot
/usr/local/graphviz-2.12/bin/dot -Tpdf graph/graph.dot > graph/graph.pdf
rm *expand
