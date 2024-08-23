CC = $(CROSS_COMPILE)gcc
CFLAGS =  -Wall -Werror

SOURCES = aesdsocket.c
OBJS = $(SOURCES:.c=.o)
TARGET = $(OBJS:.o=)

%.o : %.c
	$(CC) $(CFLAGS) -g -c -o $@ $<
	@echo --created object file $@--

.PHONY: all
all: $(TARGET)

.PHONY: build
build: $(TARGET)

$(TARGET):$(OBJS)
	$(CC) $(CFLAGS) -g -o $@ $^

.PHONY: clean
clean:
	@echo --cleaning--
	rm -f *.o $(TARGET)
