PROJECT=BootSelectWB
CC=m68k-amigaos-gcc
CFLAGS+=-nostartfiles -nostdlib -noixemul -mcpu=68000 -Wall -Wno-multichar -Wno-pointer-sign -Wno-attributes  -Wno-unused-value -s -Os -fomit-frame-pointer
LDFLAGS=-lgcc -lc

.PHONY:	clean all
OBJ = main.o

SRCS = $(OBJ:%.o=%.c)

$(PROJECT): $(SRCS)
	${CC} -o $@ $(CFLAGS) $(SRCS) endskip.S $(LDFLAGS)

clean:
	-rm -f $(PROJECT)