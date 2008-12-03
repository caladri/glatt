.if !defined(GLATT_SRC)
.error "Must define GLATT_SRC!"
.endif

.include "${GLATT_SRC}/aux/build/vars.mk"

.if empty(SUBDIRS)
.error "Recursive build must define SUBIDRS."
.endif

.for _dir in ${SUBDIRS}
${.TARGETS}::
	@${GO_MAKE:S,DST,${.CURDIR}/${_dir},:S,TGT,${.TARGET},}
.endfor
