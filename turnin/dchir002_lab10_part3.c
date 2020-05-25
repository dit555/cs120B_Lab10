/*	Author: Dumitru Chiriac lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

volatile unsigned char TimerFlag = 0;

unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

void TimerOn(){
        TCCR1B = 0x0B;
        OCR1A = 125;
        TIMSK1 = 0x02;
        TCNT1 = 0;

        _avr_timer_cntcurr = _avr_timer_M;
        SREG |= 0x80;

}

void TimerISR() {
        TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect){
        _avr_timer_cntcurr--;
        if(_avr_timer_cntcurr == 0) {
                TimerISR();
                _avr_timer_cntcurr = _avr_timer_M;
        }
}

void TimerSet(unsigned long M){
        _avr_timer_M = M;
        _avr_timer_cntcurr = _avr_timer_M;
}

typedef struct task {
	int state;
	unsigned long period;
	unsigned long elapsedTime;
	int (*TickFct)(int);
} task;

task tasks[4];

const unsigned char tasksNum = 4;
const unsigned long pG = 2;
const unsigned long p1 = 300;
const unsigned long p2 = 1000;
const unsigned long p3 = 2;
const unsigned long p4 = 2;

//variables for CSM
unsigned char out1 = 0;
unsigned char out2 = 0;
unsigned char out3 = 0;

enum _3L_States { _3L_Start, _3L0, _3L1, _3L2};
int Tick_3L(int state);

enum BL_State {BL_Start, BL_On, BL_Off};
int Tick_BL(int state);

enum CL_State {CL_Start, CL_Comb };
int Tick_CL(int state);

enum SP_State {SP_Start, Wait_B, SP_On, SP_Off};
int Tick_SP(int state);


void RUN() { //changed name from timerISR to avoid conflictiong things
        unsigned char i;
        for (i = 0; i < tasksNum; i++){
                if (tasks[i].elapsedTime >= tasks[i].period){
			while (!TimerFlag);
                        tasks[i].state = tasks[i].TickFct(tasks[i].state);
                        tasks[i].elapsedTime = 0;
			TimerFlag = 0;
                }
                tasks[i].elapsedTime += pG;
        }
}

int main(void) {
	unsigned char i = 0;
	//task 1
	tasks[i].state = _3L_Start;
	tasks[i].period = p1;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct = &Tick_3L;
	i++;

	//task 2
	tasks[i].state = BL_Start;
        tasks[i].period = p2;
        tasks[i].elapsedTime = tasks[i].period;
        tasks[i].TickFct = &Tick_BL;
        i++;

	//task 3
	tasks[i].state = SP_Start;
        tasks[i].period = p4;
        tasks[i].elapsedTime = tasks[i].period;
        tasks[i].TickFct = &Tick_SP;
        i++;


	//task 4
	tasks[i].state = CL_Start;
        tasks[i].period = p3;
        tasks[i].elapsedTime = tasks[i].period;
        tasks[i].TickFct = &Tick_CL;

	TimerSet(pG);
	TimerOn();

    	DDRB = 0xFF; PORTB = 0x00;
	DDRA = 0x00; PORTA = 0xFF;
    	while (1){
		RUN();
	}
    	return 1;
}

int Tick_3L(int state){
	switch(state){
		case _3L_Start: state = _3L0; break;
		case _3L0: state = _3L1; break;
		case _3L1: state = _3L2; break;
		case _3L2: state = _3L0; break;
		default: state = _3L_Start; break;
	}	

	switch(state){
		case _3L0: out1 = 0x01; break;
		case _3L1: out1 = 0x02; break;
		case _3L2: out1 = 0x04; break;
		default: break;
	}

	return state;
}
int Tick_BL(int state){
	switch(state){
		case BL_Start: state = BL_Off; break;
		case BL_On: state = BL_Off; break;
		case BL_Off: state = BL_On; break;
		default: state = BL_Start; break;
	}

	switch(state){
		case BL_On: out2 = 0x08; break;
		case BL_Off: out2 = 0x00; break;
		default: break;
	}

	return state;
}

int Tick_SP(int state){
	
	unsigned char temp = ~PINA;
        switch(state){
                case SP_Start: state = SP_Off; break;
		case SP_On: state = (temp == 0x04) ? SP_Off : Wait_B; break;
		case SP_Off: state = (temp == 0x04) ? SP_On : Wait_B; break;
		case Wait_B: state = (temp == 0x04) ? SP_On : Wait_B; break;
                default: state = SP_Start; break;
        }

        switch(state){
                case SP_On: out3 = 0x10; break;
                case SP_Off: out3 = 0x00; break;
		case Wait_B: out3 = 0x00; break;
                default: break;
        }


        return state;
}

int Tick_CL(int state){
	switch(state){
		case CL_Start: state = CL_Comb; break;
		case CL_Comb: state = CL_Comb; break;
		default: state = CL_Start; break;
	}
	
	switch(state){
		case CL_Comb: PORTB = out1 | out2 | out3; break;
		default: break;
	}
	return state;
}
