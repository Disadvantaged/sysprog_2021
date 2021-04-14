CC	=  cc
CFLAGS	=  -Iinclude/
CFLAGS	+= -W -Wall -Wextra -Werror -Wfloat-equal
CFLAGS	+= -Wundef -Wpointer-arith -Wcast-align -Wshadow
CFLAGS	+= -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return
CFLAGS	+= -Wswitch-enum -Wunreachable-code -Winit-self
CFLAGS	+= -Wno-unused-parameter -pedantic -O3
CFLAGS  += -fsanitize=address -fno-omit-frame-pointer -g
LDFLAGS	=  -lpthread -fsanitize=address -g

SRC         = main.c \
			  shell.c \
			  parser.c \
			  signal_handlers.c \
			  support.c
SOURCES		= $(addprefix src/, $(SRC))
OBJS		= $(patsubst src/%.c, obj/%.o, $(SOURCES))
EXECUTABLE	= Cshell

RM = rm -f

MKDIR = @mkdir -p $(@D)

all: test

build: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

obj/%.o: src/%.c
	$(MKDIR)
	@$(CC) $(CFLAGS) -c -o $@ $<

test: build
	python checker.py -e ./$(EXECUTABLE)

clean:
	$(RM) -rf $(EXECUTABLE) $(OBJS)

.PHONY: clean
