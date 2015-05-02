#include <msp430.h> 


void pwm(int pulseWidth);
void squashPWM();
char mask = 0;
long int timer = 0;

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    /* Configure Output Pins */
    	P1DIR |= BIT6;             			// P1.6 to output (Package pin 14)
    	P1SEL |= BIT6;             			// P1.6 to TA0.1
    	P1DIR |= BIT0;							// Set P1.0 to output direction
    	P1SEL &= ~BIT3;							// Button on P1.3
    	P1DIR &= ~BIT3;
    	P1REN |= BIT3;
    	P1OUT |= BIT3;
    	/* Configure Interrupts */
    	P1IES |= BIT3;							// Set High->Low transition
    	P1IE |= BIT3;							// Enable interrupts for P1.1 and P1.2
    	P1IFG &= ~BIT3;							// Clear Pending Interrupts

    	/* Configure Timer */
//		TACCR0 = 2113;
//		TACTL = TASSEL_2 + MC_1;	// Source: SMCLK/1, Mode: UP-MODE, Interrupts Enabled
//		CCR0 = 4113;                // CCR1 PWM duty cycle (428)
//		CCTL0 = CCIE;          		// CCR0 interrupts
//		CCTL1 = OUTMOD_7;					// CCR1 reset/set
//		CCR1 = 1300;

		_BIS_SR(GIE);        // Enter LPM0

    	/* Demonstrate code is running */
    	P1OUT |= 0x01;							// Toggle P1.0
    	int i;
    	for(i = 0; i < 4000; i--){
    		//wait
    	}
    	P1OUT &= ~BIT0;							// Toggle P1.0

    	while(1){
//    		if(P1IN & BIT3){
//    			pwm(1238);
//    		}else{
//    			pwm(428);
//    		}
    	}

	return 0;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){
	P1IFG &= ~BIT3;				// Clear Interrupt Flag
	P1DIR &= ~BIT6;             		// P1.6 to output (Package pin 14)
	P1SEL &= ~BIT6;             		// P1.6 to TA0.1
//	CCTL0 &= ~CCIE;
	if(P1IES & BIT3){
		if(mask){
			mask = 0;
			runTimerA0();
		}else{
			mask = 1;
			P1OUT |= BIT0;
		}
	}else{
		P1OUT &= ~BIT0;
		stopTimerA0();
		P1DIR |= BIT6;             		// P1.6 to output (Package pin 14)
		P1SEL |= BIT6;             		// P1.6 to TA0.1
//			CCTL0 = ~CCIE;
		CCR0 = 4113;						// PWM Period (needs to be ~450Hz)  2113
		CCTL1 = OUTMOD_7;					// CCR1 reset/set
		CCR1 = 428;
		TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
	}

//	ADC10CTL0 |= ADC10SC + ENC;
//	P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-OR
	P1IES ^= BIT3;				// Toggle rising/falling edge
//	CCTL0 |= CCIE;
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

void pwm(int pulseWidth){
	/* Function to generate PWM on an output pin */
	P1DIR |= BIT6;             		// P1.6 to output (Package pin 14)
	P1SEL |= BIT6;             		// P1.6 to TA0.1
	CCR0 = 4113;						// PWM Period (needs to be ~450Hz)  2113
	CCTL1 = OUTMOD_7;					// CCR1 reset/set
	if(pulseWidth < CCR0){				// Check that our pulses are valid width
		CCR1 = pulseWidth;				// CCR1 PWM duty cycle
	}else{
		CCR1 = CCR0/2;					// Default 50% duty cycle
	}									// 438, 1300
	TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
}

void runTimerA0(){
//	TA2CTL = TASSEL_1 + MC_1 + ID_0;
//	TA2CCR0 = 163; // 327+1 = 328 ACLK tics = ~1/100 seconds
//	TA2CCTL0 = CCIE; // TA2CCR0 interrupt enabled
	timer = 0;
	TA0CTL = TASSEL_2 + MC_1 + TAIE;
	TA0CCR0 = 4113;
	TA0CCTL0 = CCIE;
}

void stopTimerA0(){
	TA0CTL = MC_0; // stop timer
	TA0CCTL0 &= ~CCIE; // TA2CCR0 interrupt disabled
}
