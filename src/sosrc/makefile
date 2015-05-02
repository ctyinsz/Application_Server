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
LINKLIB=-lmxml -pthread -lsxml

.SUFFIXES:
.SUFFIXES:.c .lo
.c.lo:
	libtool --mode=compile --tag=CC ${CC} -g -O -o ${TMP}/$*.lo $(INCLUDE) -c $<

all:pub.la 
pub.la:pub.lo
	libtool --mode=link --tag=CC ${CC}  ${MYLDFLAG} -g  -O -o ${TMP}/$(@:.so=.la) $? -rpath ${SHLIBDIR}  -export-dynamic -no-undefined -module -avoid-version ${INCLUDE} -L${LIBDIR} ${LINKLIB}

install:
	libtool --mode=install install -c ${TMP}/pub.la ${SHLIBDIR}
	libtool --mode=finish  ${SHLIBDIR}

clean:
	@rm -f lib*.a *~ *core ${TMP}/*.lo ${TMP}/*.o ${TMP}/*.la ${TMP}/.libs/*
	@rm -rf ./.libs