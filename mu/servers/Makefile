##
# Figure out what components to build.
##
SUBDIRS+=	mu-shell
SUBDIRS+=	net-test
SUBDIRS+=	test-client
SUBDIRS+=	test-server

.if !defined(GLATT_SRC) && exists(${.CURDIR}/../../config.mk)
GLATT_SRC=	${.CURDIR}/../..
.include "${GLATT_SRC}/config.mk"
.endif
.include "${GLATT_SRC}/aux/build/recurse.mk"
