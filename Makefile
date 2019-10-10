CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -fPIC\
         -fvisibility=hidden -fno-builtin
LDFLAGS = -shared
TARGET_LIB = libmalloc.so
OBJS = src/malloc.o
CALL_LIB = libtracemalloc.so
CALL_OBJS = test/call_trace.o

.PHONY: all ${TARGET_LIB} $(CALL_LIB) trace clean

${TARGET_LIB}: ${OBJS}
	${CC} ${LDFLAGS} -o $@ $^

all: ${TARGET_LIB}

trace: $(CALL_LIB)

CFLAGS = -g -fPIC -ldl
$(CALL_LIB): $(CALL_OBJS)
	$(CC) -g ${CFLAGS} $(LDFLAGS) -fPIC -o $@ $^

clean:
	${RM} ${TARGET_LIB} $(CALL_LIB) ${OBJS} $(CALL_OBJS)
