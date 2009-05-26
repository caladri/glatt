#include <cpu/asm.h>

/*
 * Open Firmware note containing ancillary information.
 */

	.section ".note"

/* struct \ Note section header.  */
	/* /L	field	ns.namesz	*/
	.long	0x0008
	/* /L	field	ns.descrsz	*/
	.long	0x0014
	/* /L	field	ns.type		*/
	.long	0x1275
	/* 0	field	ns.name		*/
	.asciz	"PowerPC"
	.balign	4

/* struct \ Note section description.  */
	/* /L	field	ns.real-mode	*/
	.long	0
	/* /L	field	ns.real-base	*/
	.long	-1
	/* /L	field	ns.real-size	*/
	.long	-1
	/* /L	field	ns.virt-base	*/
	.long	-1
	/* /L	field	ns.virt-size	*/
	.long	-1
