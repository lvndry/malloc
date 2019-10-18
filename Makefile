CC = gcc
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS = -Wall -Wextra -Werror -std=c99 -fPIC -fno-builtin
LDFLAGS = -shared
VPATH = src

TARGET_LIB = libmalloc.so
OBJS = malloc.o
CALL_LIB = libtracemalloc.so
CALL_OBJS = call_trace.o
TEST_SRC=tests/tests.c

all: $(TARGET_LIB)

$(TARGET_LIB): CFLAGS += -pedantic -fvisibility=hidden
$(TARGET_LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g
debug: clean $(TARGET_LIB)

compile_check:
	$(CC) $(CFLAGS) $(TEST_SRC) -o tests

check: compile_check
	$(./tests)
	
trace: $(CALL_LIB)

$(CALL_LIB): CPPFLAGS = -D_GNU_SOURCE
$(CALL_LIB): CFLAGS += -g
$(CALL_LIB): LDFLAGS += -ldl
$(CALL_LIB): $(CALL_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(TARGET_LIB) $(CALL_LIB) $(OBJS) $(CALL_OBJS)

.PHONY: all $(TARGET_LIB) $(CALL_LIB) trace clean
