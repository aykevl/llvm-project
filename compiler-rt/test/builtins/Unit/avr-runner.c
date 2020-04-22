// REQUIRES: skip
// RUN: fail
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

static int uart_putchar(char c, FILE *stream) {
	UDR0 = c;
	return 0;
}

void uart_printstring(char *msg) {
	char c;
	while ((c = *msg++)) {
		uart_putchar(c, NULL);
	}
}

__attribute__((weak))
void abort(void) {
	while (1) {
		sleep_cpu();
	}
}

__attribute__((weak))
void exit(int code) {
	while (1) {
		sleep_cpu();
	}
}


static unsigned xs = 1;
__attribute__((weak))
int rand(void)
{
    xs ^= xs << 7;
    xs ^= xs >> 9;
    xs ^= xs << 8;
    return xs;
}

__attribute__((weak))
int signbit(double x) {
    return __builtin_signbit(x);
}

#if __clang__
#define printf(format, ...) uart_printstring(format)
#define fprintf(stream, format, ...) uart_printstring(format)
#else
static FILE uart_stdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
#endif

// The main of the test file.
// Declared here so it can be called below from main.
int test_main(void);

int main() {
	// Replace the default (empty) stdout with a custom stdout that prints to
	// the UART.
#if !__clang__
	stdout = &uart_stdout;
#endif

	int result = test_main();
	if (result) {
		printf("exited with code %d\n", result);
	} else {
		printf("ok\n");
	}

	// Exit simavr by sleeping the CPU with interrupts disabled.
	sleep_cpu();
}

// Make sure the main function in the test case is renamed to test_main.
#define main(...) test_main()

// Include the test case (a .c file) here. It's main function is renamed.
#include TESTCASE
