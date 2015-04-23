#include <msp430.h> 

/* Constants */
#define LOCK_TIME 300
#define UNLOCK_TIME 500

/* Globals */
unsigned long int timer = 0;		// Timer, to be incremented every second?  or subdivision thereof
unsigned long int lockTime = 0;		// Semi-Constant, for computing if lock requirement is met
unsigned long int unlockTime = 0;	// Semi-Constant, for computing if unlock requirement is met
char mask = 0x00;
char locked = 0;

/*
 * main.c
 */
int main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	/* Notes:	Clock Freq =  Default = 32768Hz
	 *			P1.0 = A0
	 *			P1.1, P1.2 = Digital Inputs
	 *			P1.1: Inside
	 *			P1.2: Outside
	 */
	/* Configure Input Pins */
	P1SEL |= 0x01;				// Set P1.0 to A0
	P1SEL &= 0xF9;				// Ensure I/O function set for P1.1 and P1.2
	P1SEL2 &= 0xF9;				// Ensure I/O function set for P1.1 and P1.2
	P1DIR &= 0xF9;				// Set P1.1 and P1.2 as inputs
	P1REN &= 0xF9;				// Enable pull up/down resistor
	P1OUT |= 0x06;				// Set pull-up
	/* Configure ADC10 */
	ConfigureAdc();
	/* Configure Interrupts */
	P1IES |=0x06;				// Set High->Low transition
	P1IE |= 0x06;				// Enable interrupts for P1.1 and P1.2
	P1IFG &= 0xF9;				// Clear Pending Interrupts

	//TODO enter low power mode here

	return 0;
}

void ConfigureAdc(void)
{
	ADC10CTL1 = INCH_0 + ADC10DIV_3;         // Channel 0, ADC10CLK/3, Single-Channel/SingleConversion (Default)
	ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable
	ADC10AE0 |= BIT0;                         // ADC input enable P1.0
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	P1IFG &= 0xF9;				// Clear Interrupt Flag
	/* Process Inputs */
	if(!(P1IFG & 0x06)){
		/* start timers */
		if((P1IN & 0x02) && !locked){
			lockTime = timer;	// If interior held, start lock timer
		}
		if((P1IN & 0x04) && locked){
			unlockTime = timer;	// If exterior held, start unlock timer
		}
	}else{
		/* check timers */
		if((P1IN & 0x02)){
			if(lockTime && timer - lockTime >= LOCK_TIME){
				locked = 1;		// Set the lock toggle
			}else{
				//TODO unlatch
			}
			lockTime = 0;
		}
		if(!locked && (P1IN & 0x04)){
			//TODO unlatch
		}else if((P1IN & 0x04) && locked){
			if(unlockTime && timer - unlockTime >= UNLOCK_TIME){
				//TODO unlatch
				locked = 0;		// Reset the lock toggle
			}else{
				//Door remains locked
			}
			unlockTime = 0;
		}
	}
	P1IES ^= 0x06;				// Toggle rising/falling edge
}

//TODO Long Timer ISR

/* Reset the timer and check battery at the same time. */
