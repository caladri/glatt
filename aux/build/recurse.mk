.if !defined(GLATT_SRC)
.error "Must define GLATT_SRC!"
.endif

.include "${GLATT_SRC}/aux/build/vars.mk"

.if empty(SUBDIRS)
.error "Recursive build must define SUBIDRS."
.endif

.for _dir in ${SUBDIRS}
_path:=${.CURDIR}/${_dir}
.if !exists(${_path})
.error "Sub-directory must exist: ${_dir}"
.endif
${.TARGETS}::
	${GO_MAKE:S,DST,${_path},:S,TGT,${.TARGET},}
.endfor
