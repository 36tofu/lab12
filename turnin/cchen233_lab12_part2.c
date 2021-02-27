
/*	Author: lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: PORTB = tmpBT1;Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void transmit_data(unsigned char data) {
    int i;
    for (i = 0; i < 8 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
   	 PORTC = 0x08;
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;  
    }
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    PORTC |= 0x04;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
}


unsigned long _avr_timer_M = 1; //start count from here, down to 0. Dft 1ms
unsigned long _avr_timer_cntcurr = 0; //Current internal count of 1ms ticks

void TimerOn(){
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit 3 = 0: CTC mode (clear timer on compare)
	//AVR output compare register OCR1A
	OCR1A = 125; // Timer interrupt will be generated when TCNT1 == OCR1A
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; //bit1: OCIE1A -- enables compare match interrupt
	//Init avr counter
	TCNT1 = 0;

	_avr_timer_cntcurr = _avr_timer_M;
	//TimerISR will be called every _avr_timer_cntcurr ms
	
	//Enable global interrupts 
	SREG |= 0x80; //0x80: 1000000

}

void TimerOff(){
	TCCR1B = 0x00; //bit3bit1bit0 = 000: timer off
}


ISR(TIMER1_COMPA_vect){
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
			TimerISR();
			_avr_timer_cntcurr = _avr_timer_M;
			}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}




typedef struct task {
  int state; // Current state of the task
  unsigned long period; // Rate at which the task should tick
  unsigned long elapsedTime; // Time since task's previous tick
  int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[1];

const unsigned char tasksNum = 1;


const unsigned long tasksPeriodGCD = 1;
const unsigned long periodKP = 50;
const unsigned long periodSQ = 50;
const unsigned long periodIS = 100;
const unsigned long periodSample = 200;
const unsigned long demoSample = 100;



/*
const unsigned long periodThreeLEDs = 300;
unsigned long periodSpeaker = 2;
const unsigned long periodCombined = 1;
const unsigned long periodSample = 200;
*/

enum KP_States { KP_SMStart, KP_wait, KP_inc, KP_dec, KP_reset };
int TickFct_KP(int state);

enum Demo_States {wait, bp};
int Demo_Tick(int state);

/*
enum SQ_States { SQ_SMStart, SQ_init, SQ_begin, SQ_wait, SQ_check, SQ_match };
int TickFct_detectSQ(int state);


enum IS_States { IS_SMStart, IS_unlock, IS_lock };
int TickFct_IS(int state);


enum OnOff_States { OnOff_SMStart, s_Off, s_On};
int TickFct_OnOff(int state);

enum SP_States { SP_SMStart, SP_s0, SP_s1};
int TickFct_Speaker(int state);

enum FRQ_States { FRQ_SMStart, FRQ_s0, FRQ_inc, FRQ_dec};
int TickFct_FRQ(int state);

*/

void TimerISR() {
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += tasksPeriodGCD;
  }
}


unsigned char tmpBUL;
unsigned char t2unlock; //0 = locked    1 = unlocked
unsigned char keyPressed;


int main() {
 
  DDRA = 0x00; PORTA = 0x0F;
  DDRD = 0xFF; PORTD = 0x00;
  DDRC = 0xFF; PORTC = 0x00;
  unsigned char i=0;
  tasks[i].state = wait;
  tasks[i].period = demoSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &Demo_Tick;
  /*
  ++i;
  tasks[i].state = SQ_SMStart;
  tasks[i].period = periodSQ;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_detectSQ;
  ++i;
  tasks[i].state = IS_SMStart;
  tasks[i].period = periodIS;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_IS;
  ++i
  tasks[i].state = OnOff_SMStart;
  tasks[i].period = periodSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_OnOff;
  ++i;
  //TimerSet(tasksPeriodGCD);
  */

  TimerOn();

  
  
  while(1) {
  }
  return 0;
}

int Demo_Tick(int state) {

	unsigned  char tmpA = ~PINA & 0x0F;
	// Local Variables
	static unsigned char pattern = 0x80;	// LED pattern - 0: LED off; 1: LED on
	static unsigned char row = 0x1F;  	// Row(s) displaying pattern. 
							// 0: display pattern on row
							// 1: do NOT display pattern on row
	// Transitions
	switch (state) {
		case wait:
			if(tmpA == 4){
				if(pattern > 0x01) pattern = pattern >> 1;
				state = bp;
			}
			else if(tmpA == 8){
				if(pattern < 0x80) pattern = pattern << 1;
				state = bp;
			}
			else 
			state = wait;
			break;
		case bp:
			if(tmpA == 0)
				state = wait;
			else state = bp;
		default:	
			state = wait;
			break;
	}	
	// Actions
	switch (state) {
		case wait:	
			break;
		case bp:
			break;
		default:
		break;
	}
	PORTC = pattern;	// Pattern to display
	PORTD = ~row;		// Row(s) displaying pattern	
	return state;
}

