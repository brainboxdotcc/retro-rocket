SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.ko,$(SRCS))

all: $(OBJS)

clean:
	rm -f *.ko *.o

%.ko: %.c
	gcc ${CFLAGS} -c $<
	ld -Ttext 0x600 --oformat binary -o $@ $(patsubst %.ko,%.o,$@)

install:
	install *.ko ../../os/modules/
