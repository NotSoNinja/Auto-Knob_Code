#include <msp430.h>
#include "P1Interrupt.h"

void configureP1Interrupts(){
	P1DIR |= 0x01;							// Set P1.0 to output direction
//	P1SEL &= 0xF9;							// Ensure I/O function set for P1.1 and P1.2
//	P1SEL2 &= 0xF9;							// Ensure I/O function set for P1.1 and P1.2
//	P1DIR &= 0xF9;							// Set P1.1 and P1.2 as inputs
//	P1REN &= 0xF9;							// Enable pull up/down resistor
//	P1OUT |= 0x06;							// Set pull-up
	P1SEL &= ~BIT3;							// Button on P1.3
	P1DIR &= ~BIT3;
	P1REN |= BIT3;
	P1OUT |= BIT3;
	/* Demonstrate code is running */
	P1OUT |= 0x01;							// Toggle P1.0 using exclusive-OR
	int i;
	for(i = 0; i < 1000; i--){
		//wait
	}
	P1OUT &= ~BIT0;							// Toggle P1.0 using exclusive-OR
	/* Configure Interrupts */
	P1IES |= BIT3;							// Set High->Low transition
	P1IE |= BIT3;							// Enable interrupts for P1.1 and P1.2
	P1IFG &= ~BIT3;							// Clear Pending Interrupts
}

