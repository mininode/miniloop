# Copyright (c) 2020 Alex Caudill <alex.caudill@pm.me>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

CC ?= gcc
LD ?= ld
AR ?= ar
RM ?= rm
CFLAGS ?= -O2 -std=gnu99

SRCDIR := $(realpath .)
OBJDIR ?= $(SRCDIR)/obj

# Do not:
# o  use make's built-in rules and variables
#    (this increases performance and avoids hard-to-debug behaviour);
# o  print "Entering directory ...";
MAKEFLAGS += -rR --no-print-directory

# Avoid funny character set dependencies
unexport LC_ALL
LC_COLLATE=C
LC_NUMERIC=C
export LC_COLLATE LC_NUMERIC

# Avoid interference with shell env settings
unexport GREP_OPTIONS

define generateRule
$2 += $(patsubst %.c, %.o, $(subst $(SRCDIR), $(OBJDIR), ${1}))
$3 += $(patsubst %.c, %.d, $(subst $(SRCDIR), $(OBJDIR), ${1}))
$(patsubst %.c, %.o, $(subst $(SRCDIR), $(OBJDIR), ${1})): $(1) | objdir
	$$(CC) -c -fPIC $$(CFLAGS) $$^ -o $$@ -MT $$@ -MMD -MP -MF$(patsubst %.c, %.d, $(subst $(SRCDIR), $(OBJDIR), ${1})) 
endef

all: $(OBJDIR)/libminiloop.a

objdir:
	mkdir -p $(OBJDIR)/src

CORE_CFLAGS = $(CFLAGS)                           \
  					-Wall                                 \
						-I$(SRCDIR)/src                       \
						-DDUK_OPT_AUGMENT_ERRORS              \
						-D_POSIX_C_SOURCE=200809L             \
						-D_GNU_SOURCE                         \
						-D_XOPEN_SOURCE=700

SRCS = $(SRCDIR)/src/event.c    \
			 $(SRCDIR)/src/io.c       \
			 $(SRCDIR)/src/signal.c   \
			 $(SRCDIR)/src/timer.c    \
			 $(SRCDIR)/src/miniloop.c
OBJS =
DEPS =

$(foreach file,$(SRCS),$(eval $(call generateRule,$(file),OBJS,DEPS)))

-include $(DEPS)

$(OBJDIR)/libminiloop.a: $(OBJS)
	ar crs $@ $^

clean:
	rm -rf $(OBJDIR)/*

