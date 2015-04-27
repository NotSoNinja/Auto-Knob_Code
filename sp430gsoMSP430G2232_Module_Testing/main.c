#include <msp430.h>
#include "P1Interrupt.h"

void configureAdc();

int timer = 0;

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer
	configureP1Interrupts();
	configureAdc();
	/* Configure Timer */
	TACTL = TASSEL_2 + MC_2 + ID_3 + TAIE;	// Source: SMCLK/4, Mode: UP/DOWN-MODE, Interrupts Enabled
	TACCR0 = 0xFF;							// Counts up to 32768, then back down

	//CCR0 = 13534;						// Capture/Compare register, but timer keeps counting up.
	_BIS_SR(GIE);


	return 0;
}

void configureAdc(){
	ADC10CTL1 = INCH_1 + ADC10DIV_3;         // Channel 0, ADC10CLK/3, Single-Channel/SingleConversion (Default)
	ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable
	ADC10AE0 |= BIT1;                         // ADC input enable P1.0
}


// Timer A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void){
	timer++;
	/* Reset the timer and check battery at the same time. */
	if(timer > 32768){
		timer = 0;
		P1OUT ^= 0x01;
//		ADC10CTL0 |= ADC10SC;
	}
}


// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void){
	//TODO Compare results from ADC10MEM < 0x00; (replace 0x00 with actual value before implementation)
	P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	P1IFG &= ~BIT3;				// Clear Interrupt Flag
	ADC10CTL0 |= ADC10SC + ENC;	//not working
//	P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR
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
//	P1IES ^= BIT3;				// Toggle rising/falling edge
}
