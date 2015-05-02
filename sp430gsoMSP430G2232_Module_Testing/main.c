#include <msp430.h>
#include "P1Interrupt.h"

//Testing code

#define SERVO BIT6							// P1.6 (TA0.1) is the Servo

void configureAdc();

long int timer = 0;

void pwm(int pulseWidth);

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;				// Stop watchdog timer

	/* Configure Output Pins */
	P1DIR |= SERVO;             			// P1.6 to output (Package pin 14)
	P1SEL |= SERVO;             			// P1.6 to TA0.1
	P1DIR |= BIT0;							// Set P1.0 to output direction
	P1SEL &= ~(BIT3 + BIT2 + BIT1);							// Button on P1.3
	P1DIR &= ~(BIT3 + BIT2 + BIT1);
	P1REN |= BIT3 + BIT2 + BIT1;
	P1OUT |= BIT3 + BIT2 + BIT1;
	/* Demonstrate code is running */
	P1OUT |= 0x01;							// Toggle P1.0
	int i;
	for(i = 0; i < 4000; i--){
		//wait
	}
	P1OUT &= ~BIT0;							// Toggle P1.0

	/* Configure Interrupts */
	P1IES |= BIT3;							// Set High->Low transition
	P1IE |= BIT3;							// Enable interrupts for P1.1 and P1.2
	P1IFG &= ~BIT3;							// Clear Pending Interrupts

	configureAdc();

	/* Configure Timer */
	TACTL = TASSEL_2 + MC_1 + ID_0 + TAIE;	// Source: SMCLK/1, Mode: UP-MODE, Interrupts Enabled
	TACCR0 = 73-1;							// ~450Hz
	TACCTL0 = CCIE;							// CCR0 interrupt enabled (hopefully not CCR1)

	//CCR0 = 13534;							// Capture/Compare register, but timer keeps counting up.
	_BIS_SR(GIE);
	//_BIS_SR(LPM0_bits);

	while(1){
		//wait?
	}

	return 0;
}

void configureAdc(){
	ADC10CTL1 = INCH_1 + ADC10DIV_3;         // Channel 0, ADC10CLK/3, Single-Channel/SingleConversion (Default)
	ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable
	ADC10AE0 |= BIT1;                         // ADC input enable P1.1
}


// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void){
	timer++;
	/* Reset the timer and check battery at the same time. */
	if(timer > 450){
		timer = 0;
		P1OUT ^= 0x01;
//		ADC10CTL0 |= ADC10SC;
	}
}


// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void){
	//TODO Compare results from ADC10MEM < 0x00; (replace 0x00 with actual value before implementation)
	//P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	P1IFG &= ~BIT3;				// Clear Interrupt Flag
//	if(P1IES & BIT3){
//		pwm(43);
//	}else{
//		pwm(15);
//	}
//	ADC10CTL0 |= ADC10SC + ENC;
	P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR
	/* Process Inputs */
//	if(!(P1IFG & 0x06)){
//		/* start timers */
//		if((P1IN & 0x02) && currentState == LATCHED){
//			lockTime = timer;	// If interior held, start lock timer
//		}
//		if((P1IN & 0x04) && currentState == LOCKED){
//			unlockTime = timer;	// If exterior held, start unlock timer
//		}
//	}else{
//		/* check timers */
//		if((P1IN & 0x02)){
//			if(lockTime && timer - lockTime >= LOCK_TIME){
//				currentState = 1;		// Set the lock toggle
//			}else{
//				//TODO unlatch
//			}
//			lockTime = 0;
//		}
//		if(!currentState && (P1IN & 0x04)){
//			//TODO unlatch
//		}else if((P1IN & 0x04) && currentState == LOCKED){
//			if(unlockTime && timer - unlockTime >= UNLOCK_TIME){
//				//TODO unlatch
//				currentState = 0;		// Reset the lock toggle
//			}else{
//				//Door remains locked
//			}
//			unlockTime = 0;
//		}
//	}
	P1IES ^= BIT3;				// Toggle rising/falling edge
}

void pwm(int pulseWidth){
	/* Function to generate PWM on an output pin */
	P1DIR |= SERVO;             		// P1.6 to output (Package pin 14)
	P1SEL |= SERVO;             		// P1.6 to TA0.1
	CCR0 = 73-1;						// PWM Period (needs to be ~450Hz)
	CCTL1 = OUTMOD_7;					// CCR1 reset/set
	if(pulseWidth < CCR0){				// Check that our pulses are valid width
		CCR1 = pulseWidth;				// CCR1 PWM duty cycle
	}else{
		CCR1 = CCR0/2;					// Default 50% duty cycle
	}
	TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
}
