.PHONY: all test run test-srv

all: checker

test: _test_ok _test_nok
	@if grep -q 'status: err' test-ok.log; then echo "OK-test failed"; else echo "OK-test succeed"; fi
	@if grep -q 'status: ok' test-nok.log; then echo "NOK-test failed"; else echo "NOK-test succeed"; fi

_test_ok: checker
	@for _ in {1..500}; do ./tester.pl --ok ; done > test-ok.log

_test_nok: checker
	@for _ in {1..200}; do ./tester.pl --nok ; done > test-nok.log

run: script
	@spawn-fcgi -p 4445 -n $<

test-srv: _launch $(foreach i,$(shell seq -s ' ' 1 20), _spawn$(i))
	wait

# - - - - - - - - - - - - - - - - - - - -
checker: main.c cookie.h
	gcc -Og -ggdb -Wall -Wextra -Werror -Wno-sign-compare main.c -o $@

script: script.c cookie.h
	gcc $< -lfcgi -o $@

# - - - - - - - - - - - - - - - - - - - -
_launch: script
	@spawn-fcgi -p 4445 -n $< &

_spawn%:
	@for _ in {1..1000}; do ./spawner.pl --len=$$((RANDOM % 1000)); done

