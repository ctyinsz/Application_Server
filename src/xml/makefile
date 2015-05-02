#include $(INCLMAKE)
#include ${HOME}/src/etc/makefile.${GAPSPLATFORM}
MYLDFLAG= -brlt
TMP=${HOME}/app/tmp
CCFLAG=-D__ORACLE__
SHLIBDIR=${HOME}/app/myproject/shlib
LIBDIR=${HOME}/app/myproject/lib
RELEASE_VERSION=2.0.0
CC=gcc ${OS_CCFLAG} -m32
VPATH=${TMP}:.libs:.:

.SUFFIXES:
.SUFFIXES:.c .lo
.c.lo:
	libtool --mode=compile --tag=CC ${CC} -g -O -o ${TMP}/$*.lo  -c $<

all:libsxml.la 
libsxml.la:xmle.lo xml.lo xmlnode.lo trace.lo
	libtool --mode=link --tag=CC ${CC}  ${MYLDFLAG} -g  -O -o ${TMP}/$(@:.so=.la) $? -rpath ${LIBDIR}  -release ${RELEASE_VERSION} -export-dynamic -no-undefined -L${LIBDIR}

install:
	libtool --mode=install install -c ${TMP}/*.la ${LIBDIR}
	libtool --mode=finish  ${LIBDIR}

clean:
	@rm -f lib*.a *~ *core ${TMP}/*.lo ${TMP}/*.o ${TMP}/*.la ${TMP}/.libs/*
	@rm -rf ./.libs