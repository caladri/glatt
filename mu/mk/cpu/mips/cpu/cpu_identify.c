#include <core/types.h>
#include <core/string.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <io/device/console/console.h>

	/* Coprocessor 0 product ID fields.  */

#define	CP0_PRID_OPTIONS(p)		(((p) >> 24) & 0xff)
#define	CP0_PRID_COMPANY(p)		(((p) >> 16) & 0xff)
#define	CP0_PRID_TYPE(p)		(((p) >>  8) & 0xff)
#define	CP0_PRID_REVISION(p)		(((p) >>  0) & 0xff)

	/* Coprocessor 0 companies.  */

#define	CP0_PRID_COMPANY_ANCIENT	(0x00)

	/* Coprocessor 0 types.  */

#define	CP0_PRID_TYPE_R4000		(0x04)

	/* Coprocessor 0 revision major/minor.  */

#define	CP0_PRID_REVISION_MAJOR(p)	((CP0_PRID_REVISION((p)) >> 4) & 0xf)
#define	CP0_PRID_REVISION_MINOR(p)	((CP0_PRID_REVISION((p)) >> 0) & 0xf)

	/* Coprocessor 0 revisions.  */

#define	CP0_PRID_REVISION_R4000A	(0x00)
#define	CP0_PRID_REVISION_R4000B	(0x30)
#define	CP0_PRID_REVISION_R4400A	(0x40)
#define	CP0_PRID_REVISION_R4400B	(0x50)
#define	CP0_PRID_REVISION_R4400C	(0x60)

struct cpuinfo
cpu_identify(void)
{
	struct cpuinfo cpu;
	uint32_t prid;
	const char *company, *type;

	cpu.cpu_ntlbs = 0;

	prid = cpu_read_prid();

	switch (CP0_PRID_COMPANY(prid)) {
	case CP0_PRID_COMPANY_ANCIENT:
		company = "MIPS";
		switch (CP0_PRID_TYPE(prid)) {
		case CP0_PRID_TYPE_R4000:
			cpu.cpu_ntlbs = 48;
			switch (CP0_PRID_REVISION(prid)) {
			case CP0_PRID_REVISION_R4000A:
				type = "R4000A";
				break;
			case CP0_PRID_REVISION_R4000B:
				type = "R4000B";
				break;
			case CP0_PRID_REVISION_R4400A:
				type = "R4400A";
				break;
			case CP0_PRID_REVISION_R4400B:
				type = "R4400B";
				break;
			case CP0_PRID_REVISION_R4400C:
				type = "R4400C";
				break;
			default:
				type = "R4??";
				break;
			}
			break;
		default:
			type = NULL;
			break;
		}
		break;
	default:
		company = NULL;
		type = NULL;
		break;
	}

	kcprintf("cpu%u: ", mp_whoami());

	if (company == NULL)
		kcprintf("MIPS %x ", (unsigned)CP0_PRID_COMPANY(prid));
	else
		kcprintf("%s ", company);

	if (type == NULL)
		kcprintf("%x", (unsigned)CP0_PRID_TYPE(prid));
	else
		kcprintf("%s", type);

	if (CP0_PRID_REVISION_MAJOR(prid) != 0 ||
	    CP0_PRID_REVISION_MINOR(prid) != 0) {
		kcprintf(" revision %x.%x",
			 (unsigned)CP0_PRID_REVISION_MAJOR(prid),
			 (unsigned)CP0_PRID_REVISION_MINOR(prid));
	}
	kcprintf("\n");

	return (cpu);
}
