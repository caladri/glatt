#ifndef	_CORE_CLOCK_H_
#define	_CORE_CLOCK_H_

typedef	uint64_t	clock_tick_t;

clock_tick_t clock(void);
void clock_ticks(clock_tick_t);

#endif /* !_CORE_CLOCK_H_ */
