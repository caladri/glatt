#ifndef	_CORE_COPYRIGHT_H_
#define	_CORE_COPYRIGHT_H_

#ifndef CONFIGSTR
#error "New builds must have CONFIGSTR set."
#endif

#define	MK_NAME								\
	"Glatt MU/MK"

#define	MK_CONFIG							\
	(STRING(CONFIGSTR))

#define	MK_COPYRIGHT							\
	"Copyright (c) 2005-2011 Juli Mallett.  All rights reserved."

#define	MK_VERSION							\
	"Kailua"

#endif /* !_CORE_COPYRIGHT_H_ */
