.PHONY: all clean code-cov feez-test

export ROOT ?= $(shell pwd)
export SRC ?= $(ROOT)/src
export TESTS ?= $(ROOT)/t
export CINCLUDE ?= $(ROOT)/include
export MK ?= $(ROOT)/mk
export CINCLUDEFILES := $(wildcard $(INCLUDE)/*.h)
export BUILD ?= $(ROOT)/build

export CFLAGS = -O0 -ggdb -Wall -Wextra -Werror -Wno-sign-compare

# Targets:
export COOKIE := $(BUILD)/libcookie.a
export CHECKER := $(BUILD)/checker

# Sources:
export COOKIESRC := $(SRC)/cookie.c
export CHECKERSRC := $(SRC)/checker.c

$(shell mkdir -p $(BUILD))
ifdef dump
	export DUMP := $(shell realpath $(dump))
$(shell mkdir -p $(DUMP))
endif

# - - - - - - - - - - - - - - - - - - - -
all: $(CHECKER)
clean:
	@rm -rfv $(BUILD)
	@find -name '*.o' -exec rm -fv {} \+
	@find -name '*.gcno' -exec rm -fv {} \+
	@find -name '*.gcda' -exec rm -fv {} \+
	@find -name '*.gcov' -exec rm -fv {} \+
	@find -name gmon.out -exec rm -fv {} \+

code-cov:
	@$(MAKE) --no-print-directory -C $(TESTS) code-cov

# - - - - - - - - - - - - - - - - - - - -
include $(MK)/compile.mk
include $(MK)/cookie.mk

$(CHECKER): $(CHECKERSRC:%.c=%.o) $(COOKIE)
	gcc $(CFLAGS) -o $@ $^

