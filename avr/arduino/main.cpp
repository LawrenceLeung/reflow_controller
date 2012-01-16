#include <WProgram.h>

int main(void) __attribute__((noreturn));
int main(void)
{
	_init_Teensyduino_internal_();

	setup();
    
	for (;;)
		loop();
}

