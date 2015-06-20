APPHOME=${HOME}/app/gas
INCLUDE=-I${APPHOME}/src/include
BINDIR=${APPHOME}/bin
LIBDIR=${APPHOME}/lib
TMP=${APPHOME}/tmp
CC=gcc -m32
CCFLAG =-g -Wall -O3 -march=i486
VPATH=${TMP}:
vpath %*.o ${TMP}
LINKLIB=-lmxml -pthread -lsxml -lpub -lbase -lrt

.SUFFIXES:
.SUFFIXES: .c .o
.c.o:
	${CC}  ${DEBUG} ${CCFLAG} ${INCLUDE} -c $< -o ${TMP}/$*.o 

all:mpp_sds
#mpp_sds:mpp_sds.o
#	${CC}  -g $? -o ${BINDIR}/$@  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} -L${LIBDIR} ${LINKLIB} 
#mpp_sds2:dataproc.o thread_producer.o thread_consumer.o mpp_sds2.o 
#	${CC}  -g $? -o  ${BINDIR}/$@  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} -L${LIBDIR} ${LINKLIB}
#mpp_sds3:dataproc.o mpp_sds3.o 
#	${CC}  -g $? -o  ${BINDIR}/$@  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} -L${LIBDIR} ${LINKLIB}
mpp_sds:MemoryPool.o datastruct.o log.o map.o threadpool.o connection.o server_core.o mpp_sds.o
	${CC}  $^ -o  ${BINDIR}/$@  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} -L${LIBDIR} ${LINKLIB}
#mpp_lisds:mpp_lisds.o
#	${CC}  -g $? -o  ${BINDIR}/$@  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} -L${LIBDIR} ${LINKLIB} 
#test:test.o
#	${CC}  -g ./test.c -o  ${BINDIR}/test  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} -L${LIBDIR} ${LINKLIB} $(INCLUDE)
clean:
	rm -f ${TMP}/*.o
	rm -f *.o
	rm -f ${BINDIR}/mpp_sds
	rm -f ${BINDIR}/mpp_sds2
	rm -f ${BINDIR}/mpp_lisds
	rm -f ${BINDIR}/mpp_sds3
	rm -f ${BINDIR}/test