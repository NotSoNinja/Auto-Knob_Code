#include <msp430.h> 

/* Constants */
#define LOCK_TIME 1346						// Time in timer periods (450Hz)
#define UNLOCK_TIME 2244					// Time in timer periods (450Hz)
#define INT_SENSOR 0x04						// P1.2 is the Interior Sensor
#define EXT_SENSOR 0x02						// P1.1 is the Exterior Sensor
#define SERVO 0x40							// P1.6 (TA0.1) is the Servo
#define LED 0x08							// P2.3 is the (blue) LED
#define BAT_MONITOR 0x01					// P1.0 is the Battery Voltage Monitor

enum state{
	LOCKED, UNLOCKED, LATCHED, LOW_POWER;
};

/* Globals */
unsigned long int timer = 0;				// Timer, to be incremented every second?  or subdivision thereof
long int lockTime = 0;						// Semi-Constant, for computing if lock requirement is met
long int unlockTime = 0;					// Semi-Constant, for computing if unlock requirement is met
state currentState;							// ENUM to hold state for state machine

/* Function Stubs for the Betterment of Humanity */
void ConfigureAdc(void);
void unlatch(state power);
void latch();
void pwm(int pulseWidth);

/*
 * main.c
 */
int main(void) {
	WDTCTL = WDTPW | WDTHOLD;				// Stop watchdog timer

	/* Configure Input Pins */
	P1SEL |= 0x01;							// Set P1.0 to A0
	P1SEL &= 0xF9;							// Ensure I/O function set for P1.1 and P1.2
	P1DIR &= 0xF9;							// Set P1.1 and P1.2 as inputs
	P1REN |= 0x06;							// Enable pull up/down resistor
	P1OUT |= 0x06;							// Set pull-up

	/* Configure Output Pins */
	P1DIR |= SERVO;             				// P1.6 to output (Package pin 14)
	P1SEL |= SERVO;             				// P1.6 to TA0.1
	P2SEL &= ~LED;								// P2.3 is Digital I/O
	P2DIR |= LED;								// P2.3 is an output

	/* Configure ADC10 */
	ConfigureAdc();

	/* Configure Timer */
	TACTL = TASSEL_2 + MC_1 + ID_0 + TAIE;	// Source: SMCLK/1, Mode: UP-MODE, Interrupts Enabled
	TACCRO = 73-1;							// ~450Hz

	/* Configure Interrupts */
	P1IES |=0x06;							// Set High->Low transition
	P1IE |= 0x06;							// Enable interrupts for P1.1 and P1.2
	P1IFG &= 0xF9;							// Clear Pending Interrupts
	TACCTL0 = CCIE;							// CCR0 interrupt enabled (hopefully not CCR1)

	/* Set servo to latched position */
	latch();

	/* Low Power Mode and Global Settings */
	_BIS_SR(GIE);							// Global Interrupt Enable
//	_BIS_SR(LPM0_bits);						// Enter Low Power Mode
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
		if((P1IN & 0x02) && (currentState == LATCHED)){
			lockTime = timer;	// If interior held, start lock timer
		}
		if((P1IN & 0x04) && (currentState == LOCKED)){
			unlockTime = timer;	// If exterior held, start unlock timer
			P2OUT |= LED;		// Turn on the LED
		}
	}else{
		/* check timers */
		if((P1IN & 0x02)){
			if(lockTime && (timer - lockTime >= LOCK_TIME)){
				currentState = LOCKED;		// Set the lock toggle
			}else{
				unlatch(UNLOCKED);
			}
			lockTime = 0;
		}
		if((currentState == LATCHED) && (P1IN & 0x04)){
			unlatch(UNLOCKED);
		}else if((P1IN & 0x04) && currentState == LOCKED){
			if(unlockTime && (timer - unlockTime >= UNLOCK_TIME)){
				unlatch(UNLOCKED);
				currentState = 0;		// Reset the lock toggle
			}else{
										//Door remains locked
			}
			P2OUT &= ~LED;				// Turn the LED off
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
	if(timer > 54000){
		/* Account for running timers */
		if(lockTime){
			locktime -= timer;
		}
		if(unlockTime){
			unlockTime -= timer;
		}
		timer = 0;
		ADC10CTL0 |= ADC10SC;
	}
}

// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void){
	ADC10CTL0 &= ~ADC10IFG;				// Clear Interrupt Flag
	/* Check if low battery voltage */
	if(ADC10MEM < 0x00){
		unlatch(LOW_POWER);				// If low battery, unlatch permanently
	}
}

void unlatch(state power){
	state = power;
	/* Pull Latch as far back as possible */
	//15 32768Hz Periods
	//repeated at 450Hz (73 periods)
	pwm(15);
}

void latch(){
	state = LATCHED;
	/* Undo whatever output is done in unlatch */
	//43 32768Hz Periods
	pwm(43);
}

void pwm(int pulseWidth){
	/* Function to generate PWM on an output pin */
	CCR0 = 73-1;						// PWM Period (needs to be ~450Hz)
	CCTL1 = OUTMOD_7;					// CCR1 reset/set
	if(pulseWidth < CCR0){				// Check that our pulses are valid width
		CCR1 = pulseWidth;				// CCR1 PWM duty cycle
	}else{
		CCR1 = CCR0/2;					// Default 50% duty cycle
	}
	TACTL = TASSEL_2 + MC_1;			// SMCLK, up mode (should be 32768 Hz)
}
