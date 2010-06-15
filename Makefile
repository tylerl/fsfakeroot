CFLAGS+=-O2 -fPIC
#CFLAGS+=-g -fPIC
LFLAGS+=-ldl

fsfakeroot.so: fsfakeroot.c  fsfr_base.c  fsfr.h  fsfr_internal.c
	${CC} ${CFLAGS} ${LFLAGS} -shared -o fsfakeroot.so fsfakeroot.c  fsfr_base.c  fsfr.h  fsfr_internal.c

clean:
	rm fsfakeroot.so
