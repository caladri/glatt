###
# Defines build targets for anywhere the user may be invoking Make by hand to
# select between a number of targets.
###

.if !defined(GLATT_SRC)
.error "Must define GLATT_SRC!"
.endif

.include "${GLATT_SRC}/aux/build/vars.mk"

.if !empty(TARGETS)
all::
	@echo '>>> Available targets:'

.for _target in ${TARGETS}
all::
	@echo '>>> '${_target}
.if target(${_target}-help)
all:: 
	@echo '>>> ('${_target}'-help for '${_target}'-specific help.)'
.endif
.endfor
.else
all::
	@echo '>>> No available targets.'
.endif

