#! /bin/sh

bsdmake clean
env KERNEL_CFLAGS=-dr NO_ASSERT=t bsdmake
egypt --include-external --omit cpu_barrier,malloc,free,pool_allocate,pool_free,mutex_lock,mutex_unlock,mp_whoami,kcprintf,panic,spinlock_lock,spinlock_unlock,morder_lock,morder_unlock *expand > graph/graph.dot
dot -Tpng graph/graph.dot > graph/graph.png
rm *expand
bsdmake clean
