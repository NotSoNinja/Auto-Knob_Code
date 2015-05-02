#include <msp430.h> 

//PWM_test

#define LOCK_TIME 693							// Time in timer periods (450Hz) not actually, is 231Hz
#define UNLOCK_TIME 1155						// Time in timer periods (450Hz)
#define LED 0x08								// P2.3 is the (blue) LED
#define SERVO 0x40								// P1.6 (TA0.1) is the Servo

typedef enum{
	LOCKED, UNLOCKED, LATCHED, LOW_POWER
} state;

void configureADC(void);

char mask = 0;								// BIT0 is for simulating 2 buttons, BIT2 is for locking, BIT1 for unlocking, BIT3 is for re-latching
long int timer = 0;
long int lockTime = 0;
long int unlockTime = 0;
long int latchTime = 0;
state currentState = UNLOCKED;

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    /* Configure Output Pins */
    	P1DIR |= BIT6;             			// P1.6 to output (Package pin 14)
    	P1SEL |= BIT6;             			// P1.6 to TA0.1
    	P1DIR |= BIT0;							// Set P1.0 to output direction
    	P1SEL &= ~(BIT3 + BIT2 + BIT1);							// Button on P1.3
    	P1DIR &= ~(BIT3 + BIT2 + BIT1);
    	P1REN |= (BIT3 + BIT2 + BIT1);
    	P1OUT |= (BIT3 + BIT2 + BIT1);
    	/* Configure Interrupts */
    	P1IES |= (BIT3 + BIT2 + BIT1);							// Set High->Low transition
    	P1IE |= (BIT3 + BIT2 + BIT1);							// Enable interrupts for P1.1 and P1.2
    	P1IFG &= ~(BIT3 + BIT2 + BIT1);							// Clear Pending Interrupts
    	P2SEL &= ~LED;								// P2.3 is Digital I/O
    	P2DIR |= LED;								// P2.3 is an output
    	P2OUT &= ~LED;

    	/* Configure ADC */
    	configureADC();

    	/* Configure Timer */
		TACTL = TASSEL_2 + MC_1;	// Source: SMCLK/1, Mode: UP-MODE, Interrupts Enabled
		CCR0 = 4113;                // CCR1 PWM duty cycle (428) -- 231-ish Hz
		CCTL0 = CCIE;          		// CCR0 interrupts
		CCTL1 = OUTMOD_7;					// CCR1 reset/set
		CCR1 = 1300;

    	/* Demonstrate code is running */
    	P2OUT |= LED;							// Toggle P1.0
    	int i;
    	for(i = 0; i < 4000; i--){
    		//wait
    	}
    	P2OUT &= ~LED;							// Toggle P1.0

    	_BIS_SR(LPM0_bits + GIE);        		// Enter LPM0

//    	while(1){
//    		if(P1IN & BIT3){
//    			pwm(1238);
//    		}else{
//    			pwm(428);
//    		}
//    	}

	return 0;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void){

	if(P1IFG & P1IES & BIT2){				// Interior, therefore locking
		P1DIR &= ~BIT6;             		// P1.6 to output (Package pin 14)
		P1SEL &= ~BIT6;             		// P1.6 to TA0.1
		if(currentState != LOCKED){
			lockTime = timer;
			mask |= BIT2;
		}else{
			currentState = UNLOCKED;					// Allow for unlocking when released
			P2OUT &= ~LED;
		}
	}else if(P1IFG & P1IES & BIT1){							// Exterior, therefore unlocking
		P1DIR &= ~BIT6;             		// P1.6 to output (Package pin 14)
		P1SEL &= ~BIT6;             		// P1.6 to TA0.1
		if(currentState == LOCKED){
			mask |= BIT1;
			unlockTime = timer;
		}
	}else if(P1IFG & P1IES & BIT3){
		if(mask & BIT0){
			mask &= ~BIT0;
			P1DIR |= BIT6;             		// P1.6 to output (Package pin 14)
			P1SEL |= BIT6;             		// P1.6 to TA0.1
			CCR0 = 4113;						// PWM Period (needs to be ~450Hz)  2113
			CCTL1 = OUTMOD_7;					// CCR1 reset/set
			CCR1 = 1300;
			TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
			if(currentState != LOCKED){
				lockTime = timer;
				mask |= BIT2;
			}
		}else{
			mask |= BIT0;
			P1DIR |= BIT6;             		// P1.6 to output (Package pin 14)
			P1SEL |= BIT6;             		// P1.6 to TA0.1
			CCR0 = 4113;						// PWM Period (needs to be ~450Hz)  2113
			CCTL1 = OUTMOD_7;					// CCR1 reset/set
			CCR1 = 428;
			TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
			if(currentState == LOCKED){
				mask |= BIT1;
				unlockTime = timer;
			}
		}
		//lockTime = 1;
	}else{
		P1DIR &= ~BIT6;             		// P1.6 to output (Package pin 14)
		P1SEL &= ~BIT6;             		// P1.6 to TA0.1
		//P1OUT &= BIT0;
//		currentState = UNLOCKED;
		lockTime = 0;
		unlockTime = 0;
		mask &= ~BIT2;
		mask &= ~BIT1;
		if(currentState != LOW_POWER && currentState != LOCKED){
			P1DIR |= BIT6;             		// P1.6 to output (Package pin 14)
			P1SEL |= BIT6;             		// P1.6 to TA0.1
			CCR0 = 4113;						// PWM Period (needs to be ~450Hz)  2113
			CCTL1 = OUTMOD_7;					// CCR1 reset/set
			CCR1 = 428;
			TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
			currentState = UNLOCKED;
			mask |= BIT3;						// Request a reset
			latchTime = timer;					// Provide timeframe for the reset
			P2OUT &= ~LED;
		}
	}
	P1IES ^= (BIT3 + BIT2 + BIT1);				// Toggle rising/falling edge
	P1IFG &= ~(BIT3 + BIT2 + BIT1);				// Clear Interrupt Flag
}

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void){
	timer++;
	/* Reset the timer and check battery at the same time. */
//	if(lockTime){
//		lockTime++;
//	}
	if(timer > 138600){	//10 minutes-ish
		if(lockTime){
			lockTime -= timer;
		}
		if(unlockTime){
			unlockTime -= timer;
		}
		if(latchTime){
			latchTime -= timer;
		}
		timer = 0;
		ADC10CTL0 |= ADC10SC + ENC;				// Start an ADC conversion to check battery voltage
	}
	if(currentState != LOCKED && (mask & BIT2) && (timer - lockTime > LOCK_TIME)){
		P2OUT |= LED;
		lockTime = 0;
		mask &= ~BIT2;
		currentState = LOCKED;
	}
	if(currentState == LOCKED && (mask & BIT1) && (timer - unlockTime > UNLOCK_TIME)){
		P2OUT &= ~LED;
		unlockTime = 0;
		mask &= ~BIT1;
		currentState = UNLOCKED;
	}else if(currentState == LOCKED && (mask & BIT1) && (timer - unlockTime < UNLOCK_TIME) && !(timer % 100)){
		P2OUT ^= LED;
	}
	if(mask & BIT3){
		if(timer - latchTime > LOCK_TIME){
			currentState = LATCHED;
			P1DIR |= BIT6;             			// P1.6 to output (Package pin 14)
			P1SEL |= BIT6;             			// P1.6 to TA0.1
			CCR0 = 4113;						// PWM Period (needs to be ~450Hz)  2113
			CCTL1 = OUTMOD_7;					// CCR1 reset/set
			CCR1 = 1300;
			TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
		}
		if(timer - latchTime > UNLOCK_TIME){
			//Door is considered latched
			latchTime = 0;
			//Turn off pwm
			P1DIR &= ~BIT6;             		// P1.6 to output (Package pin 14)
			P1SEL &= ~BIT6;             		// P1.6 to TA0.1
			mask &= ~BIT3;
		}
	}
//
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void){
	ADC10CTL0 &= ~ADC10IFG;						// Clear Interrupt Flag
	/* Check if low battery voltage */
	if(ADC10MEM < 0x1A6){						// 0x01A6 = 422 (~5V)
		P1DIR |= BIT6;             			// P1.6 to output (Package pin 14)
		P1SEL |= BIT6;             			// P1.6 to TA0.1
		CCR0 = 4113;						// PWM Period (needs to be ~450Hz)  2113
		CCTL1 = OUTMOD_7;					// CCR1 reset/set
		CCR1 = 428;
		TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
		currentState = LOW_POWER;						// If low battery, unlatch permanently
	}
}

void configureADC(void){
	ADC10CTL1 = INCH_0 + ADC10DIV_3;         	// Channel 0, ADC10CLK/3, Single-Channel/SingleConversion (Default)
	ADC10CTL0 = SREF_0 + ADC10SHT_3 + ADC10ON + ADC10IE;  // Vcc & Vss as reference, Sample and hold for 64 Clock cycles, ADC on, ADC interrupt enable, enable conversion
	ADC10AE0 |= BIT0;					     	// ADC input enable P1.0
}
