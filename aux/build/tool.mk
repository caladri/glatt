###
# Defines build targets for build tools.
###

.if !defined(GLATT_SRC)
.error "Must define GLATT_SRC!"
.endif

.include "${GLATT_SRC}/aux/build/vars.mk"

.if !defined(PROGRAM)
.error "Must define PROGRAM!"
.endif

.if !defined(SOURCES)
SOURCES=${PROGRAM}.c
.endif

all: ${PROGRAM}

OBJECTS=${SOURCES:R:S/$/.o/g}

${PROGRAM}: ${OBJECTS}
	@echo 'Linking '${PROGRAM}
	@${CC} ${CPPFLAGS} ${LDFLAGS} ${OBJECTS} -o $@

.c.o:
	@echo 'Compiling '$<' for '${PROGRAM}
	@${CC} ${CPPFLAGS} ${CFLAGS} -c -o $@ $<

install: ${PROGRAM}
	@echo 'Installing '${PREFIX}/bin/${PROGRAM}
	@-mkdir -p ${PREFIX}/bin
	@-install -m 755 ${PROGRAM} ${PREFIX}/bin/${PROGRAM}

clean:
	@echo 'Cleaning for '${PROGRAM}
	@-rm ${OBJECTS}
	@-rm ${PROGRAM}
