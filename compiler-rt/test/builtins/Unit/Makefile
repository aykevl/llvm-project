
TEST ?= fixsfdi

CFLAGS = -mmcu=atmega1284p -Os -std=c11 -ffunction-sections -fno-inline
LDFLAGS = -I../../../lib/builtins -mmcu=atmega1284p -Wl,-u,vfprintf -lprintf_flt
#LDFLAGS = -I../../../lib/builtins -mmcu=atmega1284p -nostdlib /usr/lib/avr/lib/avr51/crtatmega1284p.o

gcc:
	@gcc -I../../../lib/builtins -O2 ../../../lib/builtins/$(TEST).c ../../../lib/builtins/fixunssfsi.c $(TEST)_test.c -o host-test
	@./host-test && echo ok || echo exited

clang:
	@../../../../../llvm-build.master/bin/clang --target=avr-unknown-unknown -nostdlibinc -mdouble=64 -I/usr/lib/avr/include -I../../../lib/builtins $(CFLAGS) -c -o test.o -DTESTCASE='"$(TEST)_test.c"' avr-runner.c
	@avr-gcc $(LDFLAGS) -o avr-runner.elf test.o ~/.cache/tinygo/compiler-rt-avr-atmel-none.a
	simavr -m atmega1284p avr-runner.elf

avr-gcc:
	@avr-gcc -I../../../lib/builtins -c -o test.o -DTESTCASE='"$(TEST)_test.c"' $(CFLAGS) avr-runner.c
	@avr-gcc $(LDFLAGS) -o avr-runner.elf test.o ~/.cache/tinygo/compiler-rt-avr-atmel-none.a
	simavr -m atmega1284p avr-runner.elf
