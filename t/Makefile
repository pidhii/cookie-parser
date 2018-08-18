.PHONY: $(BUILD)/code-cov $(COOKIESRC)

CFLAGS-GCOV := -fprofile-arcs -ftest-coverage -pg
CFLAGS += $(CFLAGS-GCOV)

include $(MK)/compile.mk
include $(MK)/cookie.mk

CODECOVSRC := code-cov.c

export COLUMNS
code-cov: $(CODECOVSRC:%.c=%.o) $(COOKIE)
	@rm -fv $(COOKIESRC:%=%.gcov)
	gcc $(CFLAGS-GCOV) -o $(BUILD)/$@ $^
	@./code-cov.pl
