###
# Defines build targets for vendor-provided software that uses its own build
# system.
###

.if !defined(GLATT_SRC)
.error "Must define GLATT_SRC!"
.endif

.include "${GLATT_SRC}/aux/build/vars.mk"

all::
	@echo '>>> Building vendor software...'

.for _target in configure build install clean
pre-${_target}:
${_target}: pre-${_target}
post-${_target}: ${_target}
.PHONY: pre-${_target} ${_target} post-${_target}
.endfor

build: post-configure

install: post-build

all:: build
	@echo '>>> Finished building vendor software.'
