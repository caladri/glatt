# Load the kernel at 1MB physical.
KERNEL_TEXT=	0xa800000000100000

KERNEL_MACHINE=	malta
.if defined(LITTLE_ENDIAN)
KERNEL_SUBMACHINE=	malta
.else
KERNEL_SUBMACHINE=	maltabe
.endif
