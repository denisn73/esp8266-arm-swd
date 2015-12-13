#include <stdint.h>

extern "C" void _start()
{
	// Set up GPIOs
	*(volatile uint32_t*)0x40048008=0;
	*(volatile uint32_t*)0x400ff014=0x00300800;

	// Blink LEDs
	while (1) {

		// Green
		*(volatile uint32_t*)0x400ff000=0x00200800;

		for (unsigned i = 0x123456; i; i--) asm __volatile__ ("");

		// Pink
		*(volatile uint32_t*)0x400ff000=0x00100000;

		for (unsigned i = 0x123456; i; i--) asm __volatile__ ("");
	}
}
