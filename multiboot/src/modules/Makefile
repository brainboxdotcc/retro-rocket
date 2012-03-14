SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.ko,$(SRCS))

all: startmsg $(OBJS)
	@echo "*** COMPLETED COMPILING MODULES ***"

startmsg:
	@echo "*** COMPILING MODULES ***"

clean:
	rm -f *.ko *.o

%.ko: %.c
	gcc ${CFLAGS} -c $<
	ld -Ttext 0x600 --oformat binary -o $@ ../../asm/modload.o $(patsubst %.ko,%.o,$@)

install:
	install *.ko ../../os/modules/
