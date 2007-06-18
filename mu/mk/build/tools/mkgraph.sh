#! /bin/sh

bsdmake clean
env KERNEL_CFLAGS=-dr NO_ASSERT=t bsdmake
egypt --include-external --omit cpu_barrier,malloc,free,pool_allocate,pool_free,mutex_lock,mutex_unlock,mp_whoami,kcprintf,panic,spinlock_lock,spinlock_unlock *expand > graph/graph.dot
/usr/local/graphviz-2.12/bin/dot -Tpdf graph/graph.dot > graph/graph.pdf
rm *expand
bsdmake clean
