SHLIBDIR=-L${HOME}/lib
BINDIR=.
CC=gcc
INCLUDE=-I./
VPATH=./

LINKLIB=-pthread

all:sem_mt sem_demo sem_p sem_c

sem_demo:semthread2.c
	${CC}  -g ./semthread2.c -o  sem_demo  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
sem_mt:sem_mt.c
	${CC}  -g ./sem_mt.c -o  sem_mt  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
sem_p:sem_producer.c
	${CC}  -g ./sem_producer.c -o  sem_p  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
sem_c:sem_consumer.c
	${CC}  -g ./sem_consumer.c -o  sem_c  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
clean:
	rm ${BINDIR}/sem_demo
	rm ${BINDIR}/sem_mt
	rm ${BINDIR}/sem_p
	rm ${BINDIR}/sem_c
