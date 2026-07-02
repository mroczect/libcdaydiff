CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
PREFIX = /usr/local
INCDIR = src/include
SRCDIR = src
OBJDIR = obj
LIBNAME = libcdaydiff.a

SOURCES = $(SRCDIR)/core.c $(SRCDIR)/parser.c $(SRCDIR)/diff.c $(SRCDIR)/manip.c \
          $(SRCDIR)/fmt.c $(SRCDIR)/tz.c $(SRCDIR)/epoch.c $(SRCDIR)/error.c \
          $(SRCDIR)/info.c $(SRCDIR)/comparison.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

all: $(LIBNAME)

$(LIBNAME): $(OBJECTS)
	ar rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/libcdaydiff.h $(SRCDIR)/internal/libcdaydiff_internal.h
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -I$(SRCDIR)/internal -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(LIBNAME)

install: $(LIBNAME)
	cp $(INCDIR)/libcdaydiff.h $(PREFIX)/include/
	cp $(LIBNAME) $(PREFIX)/lib/

.PHONY: all clean install