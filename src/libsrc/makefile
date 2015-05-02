APPHOME=${HOME}/app/gas
INCLUDE=-I${APPHOME}/src/include
MYLDFLAG= -brlt
TMP=${APPHOME}/tmp
CCFLAG=-D__ORACLE__
SHLIBDIR=${APPHOME}/shlib
LIBDIR=${APPHOME}/lib
RELEASE_VERSION=2.0.0
CC=gcc ${OS_CCFLAG} -m32
VPATH=${TMP}:.libs:.:
LINKLIB=-lmxml -pthread -lsxml -ldl

.SUFFIXES:
.SUFFIXES:.c .lo
.c.lo:
	libtool --mode=compile --tag=CC ${CC} -g -O -o ${TMP}/$*.lo $(INCLUDE) -c $<

all:libpub.la  libbase.la
libpub.la:libpub.lo
	libtool --mode=link --tag=CC ${CC}  ${MYLDFLAG} -g  -O -o ${TMP}/$(@:.so=.la) $? -rpath ${LIBDIR}  -release ${RELEASE_VERSION} -export-dynamic -no-undefined  ${INCLUDE} -L${LIBDIR} ${LINKLIB}
libbase.la:libbase.lo
	libtool --mode=link --tag=CC ${CC}  ${MYLDFLAG} -g  -O -o ${TMP}/$(@:.so=.la) $? -rpath ${LIBDIR}  -release ${RELEASE_VERSION} -export-dynamic -no-undefined  ${INCLUDE} -L${LIBDIR} ${LINKLIB}


install:
	libtool --mode=install install -c ${TMP}/libpub.la ${LIBDIR}
	libtool --mode=install install -c ${TMP}/libbase.la ${LIBDIR}
	libtool --mode=finish  ${LIBDIR}

clean:
	@rm -f lib*.a *~ *core ${TMP}/*.lo ${TMP}/*.o ${TMP}/*.la ${TMP}/.libs/*
	@rm -rf ./.libs