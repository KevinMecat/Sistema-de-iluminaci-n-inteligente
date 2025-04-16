#include <24FJ256GA106.h>         // Modify for your chip

#use delay(crystal=20mhz)   // Modify for your clock and freq
                            //   for example: internal=8mhz

void main(void) {
    while(TRUE) {           // Produces a 1khz square wave on pin B0
        output_high(PIN_B0);
        delay_us(500);
        output_low(PIN_B0);
        delay_us(500);
    }
}
