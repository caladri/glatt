#include <core/types.h>
#include <core/string.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>

	/* Coprocessor 0 product ID fields.  */

#define	CP0_PRID_OPTIONS(p)		(((p) >> 24) & 0xff)
#define	CP0_PRID_COMPANY(p)		(((p) >> 16) & 0xff)
#define	CP0_PRID_TYPE(p)		(((p) >>  8) & 0xff)
#define	CP0_PRID_REVISION(p)		(((p) >>  0) & 0xff)

	/* Coprocessor 0 companies.  */

#define	CP0_PRID_COMPANY_ANCIENT	(0x00)
#define	CP0_PRID_COMPANY_MIPS		(0x01)

	/* Coprocessor 0 types.  */

	/* CP0_PRID_COMPANY_ANCIENT */
#define	CP0_PRID_TYPE_R4000		(0x04)

	/* CP0_PRID_COMPANY_MIPS */
#define	CP0_PRID_TYPE_5KC		(0x81)

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

	cpu.cpu_ntlbs = 0;

	prid = cpu_read_prid();

	switch (CP0_PRID_COMPANY(prid)) {
	case CP0_PRID_COMPANY_ANCIENT:
		cpu.cpu_company = "MIPS";
		switch (CP0_PRID_TYPE(prid)) {
		case CP0_PRID_TYPE_R4000:
			cpu.cpu_ntlbs = 48;
			switch (CP0_PRID_REVISION(prid)) {
			case CP0_PRID_REVISION_R4000A:
				cpu.cpu_type = "R4000A";
				break;
			case CP0_PRID_REVISION_R4000B:
				cpu.cpu_type = "R4000B";
				break;
			case CP0_PRID_REVISION_R4400A:
				cpu.cpu_type = "R4400A";
				break;
			case CP0_PRID_REVISION_R4400B:
				cpu.cpu_type = "R4400B";
				break;
			case CP0_PRID_REVISION_R4400C:
				cpu.cpu_type = "R4400C";
				break;
			default:
				cpu.cpu_type = "R4??";
				break;
			}
			break;
		default:
			cpu.cpu_type = NULL;
			break;
		}
		break;
	case CP0_PRID_COMPANY_MIPS:
		cpu.cpu_company = "MIPS";
		switch (CP0_PRID_TYPE(prid)) {
		case CP0_PRID_TYPE_5KC:
			cpu.cpu_type = "5Kc";
			cpu.cpu_ntlbs = 48;
			break;
		default:
			cpu.cpu_type = NULL;
			break;
		}
		break;
	default:
		cpu.cpu_company = NULL;
		cpu.cpu_type = NULL;
		break;
	}
	cpu.cpu_revision_major = CP0_PRID_REVISION_MAJOR(prid);
	cpu.cpu_revision_minor = CP0_PRID_REVISION_MINOR(prid);

	return (cpu);
}
