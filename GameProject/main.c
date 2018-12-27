/*
* GameProject.c
*
* Created: 2018-12-14 PM 11:00:09
* Author : Jinsan KIM
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "_glcd.h"
#include "_main.h"
#include "_buzzer.h"

#define F_CPU 14745600UL

unsigned int timerCnt=0;//count 1sec from timer
unsigned char gameCnt=5;//time limit (5sec)
unsigned char score=0;
unsigned char phase=0;
unsigned char timeflag=0;//manage time
unsigned char len=20;//lenght of box
unsigned char boxX1,boxX2,boxY1,boxY2;//location of box
unsigned char curX=31;//location of cursor
unsigned char curY=63;
unsigned char speed;//speed of cursor

unsigned int Data_ADC3=0; // move up or down value of joystick
unsigned int Data_ADC4=0; // move right or left value of joystick
#define ADC_VREF_TYPE 0x00 // Used voltage reference  vref
#define ADC_AVCC_TYPE 0x40 // Used voltage reference avcc
#define ADC_RES_TYPE 0x80 // Used voltage reference res
#define ADC_2_56_TYPE 0xc0 // Used voltage reference 2.56

#define    LDo        395
#define    LRe        354
#define    LMi        316
#define    LFa        298
#define    LSo        266
#define    LLa        237
#define    LSi        212

#define    BDo        200
#define    BRe        178
#define    BMi        159
#define    BFa        150
#define    BSo        134
#define    BLa        112
#define    BSi        107

#define    HDo        101
#define    HRe        90
#define    HMi        80
#define    HFa        76
#define    HSo        68
#define    HLa        61
#define    HSi        54

#define    hDo        51
#define    RB        50
#define    LB        65
#define    SB        30
#define    BB        35

#define     Buzzer_Port     PORTG
#define     Buzzer_Pin      4
#define     MelOn    SETBIT(Buzzer_Port, Buzzer_Pin)
#define     MelOff   CLEARBIT(Buzzer_Port, Buzzer_Pin)


unsigned int Read_Adc_Data(unsigned char adc_input);


void Port_init(void){
	PORTA=0x00; DDRA=0xff;
	PORTB=0xff; DDRB=0xff;
	PORTC=0x00; DDRC=0xf0;
	PORTD=0xff; DDRD=0x00;
	PORTE=0x00; DDRE=0xff;
	PORTF=0x00; DDRF=0x00;
}

void interrupt_init(void){
	EICRA = 0x02;//use interrupt 0
	EIMSK = 0x01;//allow interrupt 0
}


void creat_box(){
	srand(TCNT0);//provide random value of box's location
	boxX1 = (rand()%(42-len)) + 10;//min 10 max 32 maxLen=20
	boxX2 = boxX1+len;
	boxY1 = (rand()%(126-len));
	boxY2 = boxY1+len;
}

void cursur(){//adc value from joystick
	Data_ADC3 = Read_Adc_Data(3) / 14; //read 3 port
	Data_ADC4 = Read_Adc_Data(4) / 8;
	
	if(phase==5){//in phase 3, make cursor speed slowly
		speed=5;
	}else{
		speed=7;
	}
	if(Data_ADC3<36){//move up
		curX-=speed;
		}else if(Data_ADC3>36){//move down
		curX+=speed;
	}
	
	if(Data_ADC4>63){//move left side
		curY-=speed;
		}else if(Data_ADC4<63){//move right side
		curY+=speed;
	}
	
	GLCD_Circle_black(curX,curY,2);
}

void screen_start(void){
	lcd_clear();
	lcd_string(1,0, "====================");
	lcd_string(2,0, "*      G A M E     *");
	lcd_string(3,0, "*                  *");
	lcd_string(4,0, "*        BOX       *");
	lcd_string(5,0, "*      SHOTTER     *");
	lcd_string(6,0, "====================");
}

void screen_game(char cnt,char sco,char pha){
	if(pha==3){
		pha=2;
	}else if(pha==5){
		pha=3;
	}
	lcd_clear();
	GLCD_Rectangle(9,0,53,127);
	cursur();
	GLCD_Rectangle(boxX1,boxY1,boxX2,boxY2);
	lcd_string(0,0,"PLAY");
	lcd_string(0,13,"STAGE");
	lcd_xy(0,18);
	GLCD_2DigitDecimal(pha);
	lcd_string(7,0, "Time");
	lcd_xy(7,4);
	GLCD_2DigitDecimal(cnt);
	lcd_string(7,13, "Score");
	lcd_xy(7,18);
	GLCD_2DigitDecimal(sco);
}

void screen_over(int score){
	lcd_clear();
	SError();
	lcd_string(1,0, "====================");
	lcd_string(2,0, "*   GAME   OVER!   *");
	lcd_string(3,0, "*                  *");
	lcd_string(4,0, "*                  *");
	lcd_string(5,0, "* YOUR SCORE :  ");
	lcd_xy(5,15);
	GLCD_2DigitDecimal(score);
	lcd_string(5,19, "*");
	lcd_string(6,0, "====================");
}

void screen_win(){
	TCCR0=0x00;
	lcd_clear();
	lcd_string(1,0, "====================");
	lcd_string(2,0, "*   WELL   DONE!   *");
	lcd_string(3,0, "*    YOU   WIN!    *");
	lcd_string(4,0, "*                  *");
	lcd_string(5,0, "* YOUR SCORE :  ");
	lcd_xy(5,15);
	GLCD_2DigitDecimal(score);
	lcd_string(5,19, "*");
	lcd_string(6,0, "====================");
}

void screen_nextstage(){
	lcd_clear();
	lcd_string(1,0, "====================");
	lcd_string(2,0, "*      CLEAR!      *");
	lcd_string(3,0, "*                  *");
	lcd_string(4,0, "*    READY FOR     *");
	lcd_string(5,0, "*   NEXT  STAGE!   *");
	lcd_string(6,0, "====================");
	len=20;
	
}
bool isIn(){
	if(boxX1<curX && curX<boxX2 && boxY1<curY && curY<boxY2){
		return true;
		}else{
		return false;
	}
}
SIGNAL(INT0_vect){
	if(isIn()){
		S_Good();//buzzer
		creat_box();
		if(len>10){
			len-=3;
		}else{
			phase++;
			if(phase>5){//if win at the 3rd phase
				screen_win();
			}
		}
		timerCnt=0;//initialize timer
		score++;
		
		if(phase==1){//phase 1's time limit=5sec
			gameCnt=5;
		}else{
			gameCnt=3;//in phase 2 and 3, time limit change to 3sec 
		}
		
	}else{ //if cursor's location is out of bound from box
		SError();//buzzer
		TCCR0=0x00;//stop timer
		screen_over(score); 	
}

ISR(TIMER0_OVF_vect){
	if(phase==0 || phase==2 || phase==4){
		timerCnt++;
		if(!phase){
			screen_start();
		}else{
			screen_nextstage();
		}
		if(timerCnt==112){//show start screen for 2sec
			phase++;
			timerCnt=0;
			creat_box();
			MelOn;
		}
	}else{
		timerCnt++;
		if(timerCnt==14){//every 0.25 sec, init display. cause fluently show move of cursor
			if(gameCnt>0){//1phase. time limit : 5 sec
				screen_game(gameCnt,score,phase);
				timeflag++;
				if(timeflag%4 == 0){
					gameCnt--;
				}
			}else if(gameCnt==0){//if do not click on time
				TCCR0=0x00;//stop timer
				SError();
				screen_over(score);
			}
			timerCnt=0;
		}
	}
}

void ADC_init(void){
	ADCSRA = 0x00; // init adc
	ADMUX = 0x00; // select adc input
	ACSR = 0x80;
	ADCSRA = 0xc3;
}

void Init_Timer0(void){
	TCCR0=0x07;  //normal mode & 1024 division
	TCNT0=0x00;
	SREG=0x80;
	TIMSK=0x01;  //TOIE0=1; bit that allows timer overflow
}

unsigned int Read_Adc_Data(unsigned char adc_input){	//read adc val
	unsigned int adc = 0;
	ADCSRA = 0xc3; // set conversion time
	ADMUX = adc_input | ADC_AVCC_TYPE; //adc setting
	ADCSRA |= 0x40; // wait until ad convert finished
	while((ADCSRA & 0x10) != 0x10);
	adc += ADCL + (ADCH*256);
	ADCSRA=0x00;
	return adc;
}
void init_devices(void){
	cli();
	Port_init();
	ADC_init();
	lcd_init();
	Init_Timer0();
	interrupt_init();
	sei();
}

int main(void)
{
	init_devices();
	
	while (1)
	{
	}
}


