#include <msp430.h> 

/* Constants */
#define LOCK_TIME 300						// Time in timer periods (32768Hz)
#define UNLOCK_TIME 500						// Time in timer periods (32768Hz)
#define INT_SENSOR 0x04;					// P1.2 is the Interior Sensor
#define EXT_SENSOR 0x02;					// P1.1 is the Exterior Sensor
#define BAT_MONITOR 0x01;					// P1.0 is the Battery Voltage Monitor

enum state{
	LOCKED, UNLOCKED, LATCHED;
};

/* Globals */
unsigned long int timer = 0;				// Timer, to be incremented every second?  or subdivision thereof
unsigned long int lockTime = 0;				// Semi-Constant, for computing if lock requirement is met
unsigned long int unlockTime = 0;			// Semi-Constant, for computing if unlock requirement is met
char mask = 0x00;							// Bitmask.  Not finished yet, may change
state currentState = LATCHED;				// ENUM to hold state for state machine

/*
 * main.c
 */
int main(void) {
	WDTCTL = WDTPW | WDTHOLD;				// Stop watchdog timer

	/* Configure Input Pins */
	P1SEL |= 0x01;							// Set P1.0 to A0
	P1SEL &= 0xF9;							// Ensure I/O function set for P1.1 and P1.2
	P1SEL2 &= 0xF9;							// Ensure I/O function set for P1.1 and P1.2
	P1DIR &= 0xF9;							// Set P1.1 and P1.2 as inputs
	P1REN &= 0xF9;							// Enable pull up/down resistor
	P1OUT |= 0x06;							// Set pull-up

	/* Configure ADC10 */
	ConfigureAdc();

	/* Configure Timer */
	TACTL = TASSEL_2 + MC_2 + ID_3 + TAIE;	// Source: SMCLK/4, Mode: UP/DOWN-MODE, Interrupts Enabled
	TACCRO = 32768;							// Counts up to 32768, then back down
	//CCR0 = 3 seconds						// Capture/Compare register, but timer keeps counting up.

	/* Configure Interrupts */
	P1IES |=0x06;							// Set High->Low transition
	P1IE |= 0x06;							// Enable interrupts for P1.1 and P1.2
	P1IFG &= 0xF9;							// Clear Pending Interrupts
	TACCTL0 = CCIE;							// CCR0 interrupt enabled (interrupt at peak and trough)

	/* Low Power Mode and Global Settings */
	_BIS_SR(GIE);							// Global Interrupt Enable
	//TODO enter low power mode here

	while(1){
		//Loop forever, we do  everything with interrupts!
	}

	return 0;
}

void ConfigureAdc(void){
	ADC10CTL1 = INCH_0 + ADC10DIV_3;         // Channel 0, ADC10CLK/3, Single-Channel/SingleConversion (Default)
	ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE + ENC;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable, enable conversion
	ADC10AE0 |= BIT0;                         // ADC input enable P1.0
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	P1IFG &= 0xF9;				// Clear Interrupt Flag
	/* Process Inputs */
	if(!(P1IFG & 0x06)){
		/* start timers */
		if((P1IN & 0x02) && currentState == LATCHED){
			lockTime = timer;	// If interior held, start lock timer
		}
		if((P1IN & 0x04) && currentState == LOCKED){
			unlockTime = timer;	// If exterior held, start unlock timer
		}
	}else{
		/* check timers */
		if((P1IN & 0x02)){
			if(lockTime && timer - lockTime >= LOCK_TIME){
				currentState = 1;		// Set the lock toggle
			}else{
				//TODO unlatch
			}
			lockTime = 0;
		}
		if(!currentState && (P1IN & 0x04)){
			//TODO unlatch
		}else if((P1IN & 0x04) && currentState == LOCKED){
			if(unlockTime && timer - unlockTime >= UNLOCK_TIME){
				//TODO unlatch
				currentState = 0;		// Reset the lock toggle
			}else{
				//Door remains locked
			}
			unlockTime = 0;
		}
	}
	P1IES ^= 0x06;				// Toggle rising/falling edge
}

// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void){
	timer++;
	/* Reset the timer and check battery at the same time. */
	if(timer > 32768){
		timer = 0;
		ADC10CTL0 |= ADC10SC;
	}
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void){
	//TODO Compare results from ADC10MEM < 0x00; (replace 0x00 with actual value before implementation)
}

//TODO latch/unlatch functions
void unlatch(state power){
	if(power == LOCKED){
		/* Pull Latch as far back as possible */
	}else{
		/* Don't engage catch */
	}
}

void latch(){
	/* Undo whatever output is done in unlatch */
}
