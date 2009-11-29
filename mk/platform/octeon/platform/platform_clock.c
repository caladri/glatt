#include <core/types.h>
#include <platform/clock.h>

/*
 * Calibrate R4K clock subsystem -- lie.
 */
unsigned
platform_clock_calibrate(unsigned hz)
{
	return (0);
}
