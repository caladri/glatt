GLATT_SRC?=	${.CURDIR}
TOOLCHAIN_ROOT?=${.CURDIR}/../toolchain-root
PATH:=		${PATH}:${TOOLCHAIN_ROOT}/bin

all::
	@echo '>>> Top-level Glatt build system.'

TARGETS+=toolchain
toolchain-help:
	@echo 'Builds a complete set of compilers and build tools.'
	@echo 'usage: '${MAKE}' toolchain'
toolchain:
	@mkdir -p ${TOOLCHAIN_ROOT}
	${GO_MAKE:S,DST,aux/toolchain,:S,TGT,install,} PREFIX=${TOOLCHAIN_ROOT}
	${GO_MAKE:S,DST,aux/toolchain,:S,TGT,clean,}

.include "${GLATT_SRC}/aux/build/root.mk"
