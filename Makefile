all : bonk

CC	:= ${CROSS_COMPILE}gcc
LIBS	:= -lv4l2 -lv4lconvert
CFLAGS	+= -Wall -Wextra

bonk : bonk.o video.o network.o
	${CC} ${LDFLAGS} -o $@ $^ ${LIBS}

%.o : %.c
	${CC} -c ${CFLAGS} -o $@ $^

clean :
	rm -f bonk bonk.o video.o network.o
