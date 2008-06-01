#ifndef	_CORE_COPYRIGHT_H_
#define	_CORE_COPYRIGHT_H_

#ifndef CONFIGSTR
#error "New builds must have CONFIGSTR set."
#endif

#define	MK_NAME								\
	"Glatt MU/MK (" STRING(CONFIGSTR) ")"

#define	COPYRIGHT							\
	"Copyright (c) 2005-2008 The Positry.  All rights reserved."

#endif /* !_CORE_COPYRIGHT_H_ */
