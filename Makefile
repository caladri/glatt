GLATT_SRC?=	${.CURDIR}

all::
	@echo '>>> Top-level Glatt build system.'

TARGETS+=toolchain
toolchain-help:
	@echo 'Builds a complete set of compilers and build tools.'
	@echo 'usage: '${MAKE}' toolchain'
toolchain:
	${GO_MAKE:S,DST,aux/toolchain,:S,TGT,all,}

.include "${GLATT_SRC}/aux/build/root.mk"
