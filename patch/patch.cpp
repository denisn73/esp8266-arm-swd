#include <stdint.h>

extern "C" void _start()
{
	// Set up GPIOs
	*(uint32_t*)0x40048008=0;
	*(uint32_t*)0x400ff014=0x00300800;

	// Blink LEDs
	while (1) {

		*(uint32_t*)0x400ff000=0x00100800;
		for (unsigned i = 2048; i--;) asm __volatile__ ("");

		*(uint32_t*)0x400ff000=0x00200000;
		for (unsigned i = 4096; i--;) asm __volatile__ ("");

	}
}
