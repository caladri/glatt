##
# Figure out what components to build.
##
SUBDIRS+=	lib
SUBDIRS+=	bin
SUBDIRS+=	sbin
SUBDIRS+=	servers
SUBDIRS+=	test

SUBDIRS+=	etc

.if !defined(GLATT_SRC) && exists(${.CURDIR}/../config.mk)
GLATT_SRC=	${.CURDIR}/..
.include "${GLATT_SRC}/config.mk"
.endif
.include "${GLATT_SRC}/aux/build/recurse.mk"
