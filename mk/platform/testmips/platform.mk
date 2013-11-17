# Load the kernel at 1MB physical.
.if defined(MIPS_ABI) && ${MIPS_ABI} == "o64"
KERNEL_TEXT=	0x80100000
.else
KERNEL_TEXT=	0xa800000000100000
.endif

# Set the machine type for GXemul.
KERNEL_MACHINE=	testmips
